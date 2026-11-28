#include "Combat/CombatActionRestrictionComponent.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LatentActionManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/PathFollowingComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatRestrictions, Log, All);

namespace
{
	const FName CombatInterruptedKey(TEXT("bCombatInterrupted"));

	bool IsDebugEnabled()
	{
#if !UE_BUILD_SHIPPING
		const IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("codex.StatusEffects.Debug"));
		return Debug && Debug->GetInt() != 0;
#else
		return false;
#endif
	}

	const UCombatActionRestrictionComponent* FindRestrictions(const AActor* Actor)
	{
		return IsValid(Actor) ? Actor->FindComponentByClass<UCombatActionRestrictionComponent>() : nullptr;
	}
}

UCombatActionRestrictionComponent::UCombatActionRestrictionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatActionRestrictionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleControllerChanged);
	}
}

void UCombatActionRestrictionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
	}
	ShutDown();
	Super::EndPlay(Reason);
}

bool UCombatActionRestrictionComponent::CanAcquireRestriction(const FCombatActionRestrictions& Restrictions) const
{
	const AActor* Owner = GetOwner();
	return !bShutDown && !Restrictions.IsEmpty() && IsRegistered() && IsValid(Owner)
		&& Owner->HasAuthority() && (Owner->HasActorBegunPlay() || Owner->IsActorBeginningPlay()) && !Owner->IsActorBeingDestroyed()
		&& (!Restrictions.bRestrictMovement || Cast<ACharacter>(Owner))
		&& (!Restrictions.bRestrictCombat || (Owner->Implements<UCombatActionRestrictionTarget>()
			&& (Cast<ICombatActionRestrictionTarget>(Owner) || Owner->GetClass()->IsFunctionImplementedInScript(
				GET_FUNCTION_NAME_CHECKED(ICombatActionRestrictionTarget, OnCombatRestrictionChanged)))));
}

FGuid UCombatActionRestrictionComponent::AcquireRestriction(FCombatActionRestrictions Restrictions)
{
	const FGuid Handle = FGuid::NewGuid();
	return AcquireRestrictionWithHandle(Handle, Restrictions) ? Handle : FGuid();
}

bool UCombatActionRestrictionComponent::AcquireRestrictionWithHandle(FGuid Handle, const FCombatActionRestrictions& Restrictions)
{
	if (!Handle.IsValid() || Sources.Contains(Handle) || !CanAcquireRestriction(Restrictions))
	{
		return false;
	}

	Sources.Add(Handle, Restrictions);
	UpdateAggregate();
	return true;
}

bool UCombatActionRestrictionComponent::ReleaseRestriction(FGuid Handle)
{
	if (Sources.Remove(Handle) == 0)
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || (!Owner->HasActorBegunPlay() && !Owner->IsActorBeginningPlay()))
	{
		ShutDown();
		return true;
	}

	UpdateAggregate();
	return true;
}

void UCombatActionRestrictionComponent::UpdateAggregate()
{
	Aggregate = {};
	for (const auto& Entry : Sources)
	{
		Aggregate.bRestrictMovement |= Entry.Value.bRestrictMovement;
		Aggregate.bRestrictCombat |= Entry.Value.bRestrictCombat;
	}

	if (bPublishing || bShutDown)
	{
		return;
	}

	TGuardValue<bool> Publishing(bPublishing, true);
	while (!bShutDown && !(Published == Aggregate))
	{
		const FCombatActionRestrictions Previous = Published;
		const FCombatActionRestrictions Current = Aggregate;
		Published = Current;
		if (Previous.bRestrictMovement != Current.bRestrictMovement)
		{
			if (Current.bRestrictMovement)
			{
				ApplyMovementRestriction();
			}
			else
			{
				RemoveMovementRestriction();
			}
		}

		if (bShutDown)
		{
			break;
		}
		AActor* Owner = GetOwner();
		if (Previous.bRestrictCombat != Current.bRestrictCombat && Current.bRestrictCombat == Aggregate.bRestrictCombat)
		{
			ICombatActionRestrictionTarget::Execute_OnCombatRestrictionChanged(Owner, Current.bRestrictCombat);
			if (!bShutDown)
			{
				SynchronizeBlackboard(Cast<APawn>(Owner));
			}
		}

		if (!bShutDown && IsValid(Owner) && Owner->Implements<UCombatActionRestrictionTarget>()
			&& Previous.bRestrictMovement != Current.bRestrictMovement && Current.bRestrictMovement == Aggregate.bRestrictMovement)
		{
			ICombatActionRestrictionTarget::Execute_OnMovementRestrictionChanged(Owner, Current.bRestrictMovement);
		}

		if (!bShutDown && Current == Aggregate)
		{
			UE_CLOG(IsDebugEnabled(), LogCombatRestrictions, Log, TEXT("%s: movement restricted=%d, combat interrupted=%d (%d sources)"),
				*GetNameSafe(Owner), Current.bRestrictMovement, Current.bRestrictCombat, Sources.Num());
			OnRestrictionChanged.Broadcast(Current.bRestrictMovement, Current.bRestrictCombat);
		}
	}
}

void UCombatActionRestrictionComponent::ShutDown()
{
	if (!bShutDown)
	{
		bShutDown = true;
		Sources.Reset();
		Aggregate = {};
		Published = {};
		RemoveMovementRestriction();
	}
}

void UCombatActionRestrictionComponent::ApplyMovementRestriction()
{
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	// Abort with Keep before resetting movement requests. StopActiveMovement can otherwise
	// abort an active path with its default velocity reset.
	SetRestrictedController(Character->GetController());
	Character->ConsumeMovementInputVector();
	Character->GetCharacterMovement()->StopActiveMovement();
}

void UCombatActionRestrictionComponent::RemoveMovementRestriction()
{
	SetRestrictedController(nullptr);
}

void UCombatActionRestrictionComponent::SetRestrictedController(AController* Controller)
{
	if (RestrictedController.Get() == Controller)
	{
		return;
	}
	if (AController* Previous = RestrictedController.Get())
	{
		Previous->SetIgnoreMoveInput(false);
	}
	RestrictedController = Controller;
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
		if (const AAIController* AI = Cast<AAIController>(Controller))
		{
			if (UPathFollowingComponent* Path = AI->GetPathFollowingComponent())
			{
				Path->AbortMove(*this, FPathFollowingResultFlags::ForcedScript,
					FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);
			}
		}
	}
}

void UCombatActionRestrictionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (HasMovementRestriction())
	{
		SetRestrictedController(NewController);
	}
	SynchronizeBlackboard(Pawn);
}

bool UCombatActionRestrictionComponent::IsActorMovementRestricted(const AActor* Actor)
{
	const UCombatActionRestrictionComponent* Restrictions = FindRestrictions(Actor);
	return Restrictions && Restrictions->HasMovementRestriction();
}

bool UCombatActionRestrictionComponent::IsActorCombatRestricted(const AActor* Actor)
{
	const UCombatActionRestrictionComponent* Restrictions = FindRestrictions(Actor);
	return Restrictions && Restrictions->HasCombatRestriction();
}

bool UCombatActionRestrictionComponent::CanRunOffensiveNotify(const USkeletalMeshComponent* Mesh, const UAnimSequenceBase* Animation)
{
	const UCombatActionRestrictionComponent* Restrictions = FindRestrictions(IsValid(Mesh) ? Mesh->GetOwner() : nullptr);
	if (!Restrictions)
	{
		return true;
	}
	if (Restrictions->bShutDown || Restrictions->HasCombatRestriction())
	{
		return false;
	}

	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const UAnimInstance* Instance = Mesh->GetAnimInstance();
	return !Montage || (Instance && Instance->Montage_IsPlaying(Montage));
}

void UCombatActionRestrictionComponent::CancelComponentLatentActions(UActorComponent* Component)
{
	if (IsValid(Component))
	{
		if (UWorld* World = Component->GetWorld())
		{
			World->GetLatentActionManager().RemoveActionsForObject(Component);
		}
	}
}

void UCombatActionRestrictionComponent::StopOffensiveMontage(ACharacter* Character, UAnimMontage* Montage, float BlendOutTime)
{
	if (IsValid(Character) && IsValid(Montage))
	{
		if (UAnimInstance* Animation = Character->GetMesh()->GetAnimInstance())
		{
			Animation->Montage_Stop(IsActorCombatRestricted(Character) ? 0.0f : BlendOutTime, Montage);
		}
	}
}

void UCombatActionRestrictionComponent::SynchronizeBlackboard(APawn* Pawn, UBlackboardComponent* Blackboard)
{
	if (!Blackboard && IsValid(Pawn))
	{
		AAIController* Controller = Cast<AAIController>(Pawn->GetController());
		Blackboard = Controller ? Controller->GetBlackboardComponent() : nullptr;
	}
	if (Blackboard)
	{
		const FBlackboard::FKey Key = Blackboard->GetKeyID(CombatInterruptedKey);
		if (Key != FBlackboard::InvalidKey && Blackboard->GetKeyType(Key) == UBlackboardKeyType_Bool::StaticClass())
		{
			Blackboard->SetValue<UBlackboardKeyType_Bool>(Key, IsActorCombatRestricted(Pawn));
		}
	}
}

void UCombatActionRestrictionComponent::LogAttackerEligibility(const AActor* Enemy, bool bEligible)
{
	UE_CLOG(IsDebugEnabled(), LogCombatRestrictions, Log, TEXT("%s: group attacker eligible=%d"),
		*GetNameSafe(Enemy), bEligible);
}

void UCombatActionRestrictionComponent::LogAttackerSlotReleased(const AActor* Enemy)
{
	UE_CLOG(IsDebugEnabled(), LogCombatRestrictions, Log, TEXT("%s: released group attacker slot"), *GetNameSafe(Enemy));
}
