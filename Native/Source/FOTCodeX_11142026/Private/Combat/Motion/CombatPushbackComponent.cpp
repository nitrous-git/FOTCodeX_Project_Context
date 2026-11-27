#include "Combat/Motion/CombatPushbackComponent.h"
#include "Combat/CombatActionRestrictionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContextualAnimSceneActorComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatPushback, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarCombatPushbackDebug(TEXT("codex.CombatPushback.Debug"), 0,
		TEXT("Log combat pushback requests and total requested displacement (before collision)."));

	struct FMeleePushbackContact
	{
		const AActor* Receiver;
		const AActor* Attacker;
		FVector Direction;
		FCombatPushbackSettings Settings;
		bool bConsumed = false;
	};

	// ApplyDamage broadcasts synchronously on the game thread. Nested damage calls get their own scope.
	FMeleePushbackContact* CurrentMeleeContact = nullptr;

	bool IsMonotonicBezier(double Start, double Control1, double Control2, double End)
	{
		// The derivative of a cubic Bezier is a quadratic. Check both ends and its interior minimum.
		const double FirstSlope = Control1 - Start;
		const double MiddleSlope = Control2 - Control1;
		const double LastSlope = End - Control2;
		if (!FMath::IsFinite(FirstSlope) || !FMath::IsFinite(MiddleSlope) || !FMath::IsFinite(LastSlope)
			|| FirstSlope < 0.0 || LastSlope < 0.0)
		{
			return false;
		}

		const double Quadratic = FirstSlope - 2.0 * MiddleSlope + LastSlope;
		const double Minimum = Quadratic > 0.0 ? (FirstSlope - MiddleSlope) / Quadratic : 0.0;
		return Minimum <= 0.0 || Minimum >= 1.0
			|| FirstSlope + Minimum * (2.0 * (MiddleSlope - FirstSlope) + Minimum * Quadratic) >= 0.0;
	}

	FVector2D MakeCurveHandle(float Tangent, float Weight, bool bWeighted, double SegmentTime)
	{
		if (!bWeighted)
		{
			return FVector2D(SegmentTime / 3.0, SegmentTime * Tangent / 3.0);
		}

		return FVector2D(1.0, Tangent).GetSafeNormal() * Weight;
	}
}

FCombatPushbackSettings UCombatPushbackLibrary::ResolvePushbackSettings(ECombatPushbackOverrideMode Mode,
	const FCombatPushbackSettings& InheritedSettings, float Distance, float Duration, UCurveFloat* DisplacementCurve)
{
	switch (Mode)
	{
	case ECombatPushbackOverrideMode::Inherit:
		return InheritedSettings;
	case ECombatPushbackOverrideMode::Override:
		return { true, Distance, Duration, DisplacementCurve };
	default:
		return {};
	}
}

float UCombatPushbackLibrary::ApplyDamageWithPushback(AActor* DamagedActor, float BaseDamage, AController* EventInstigator,
	AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, AActor* MeleeWindowTarget,
	FVector Direction, const FCombatPushbackSettings& Settings)
{
	check(IsInGameThread());
	FMeleePushbackContact Contact{ DamagedActor, DamageCauser, Direction, Settings };
	TGuardValue<FMeleePushbackContact*> Scope(CurrentMeleeContact,
		DamagedActor && DamagedActor == MeleeWindowTarget && BaseDamage > 0.0f && Settings.bEnabled ? &Contact : nullptr);
	return UGameplayStatics::ApplyDamage(DamagedActor, BaseDamage, EventInstigator, DamageCauser, DamageTypeClass);
}

void UCombatPushbackLibrary::CaptureAcceptedPushback(UCombatPushbackComponent* Pushback, AActor* DamageCauser)
{
	if (!IsValid(Pushback))
	{
		return;
	}

	Pushback->StopActivePushback(TEXT("replaced by accepted reaction"));
	Pushback->PendingSettings = {};
	Pushback->PendingDirection = FVector::ZeroVector;

	if (FMeleePushbackContact* Contact = CurrentMeleeContact;
		Contact && !Contact->bConsumed && Contact->Receiver == Pushback->GetOwner() && Contact->Attacker == DamageCauser)
	{
		Contact->bConsumed = true;
		Pushback->PendingSettings = Contact->Settings;
		Pushback->PendingDirection = Contact->Direction;
	}
}

void UCombatPushbackLibrary::StartAcceptedPushback(UCombatPushbackComponent* Pushback, UAnimMontage* ReactionMontage,
	float ReactionPlaybackLength)
{
	if (!IsValid(Pushback))
	{
		return;
	}

	const FCombatPushbackSettings Settings = Pushback->PendingSettings;
	const FVector Direction = Pushback->PendingDirection;
	Pushback->PendingSettings = {};
	Pushback->PendingDirection = FVector::ZeroVector;
	if (!Settings.bEnabled)
	{
		return;
	}
	const ACharacter* Character = Cast<ACharacter>(Pushback->GetOwner());
	const FAnimMontageInstance* Reaction = Character ? Character->GetRootMotionAnimMontageInstance() : nullptr;
	if (ReactionPlaybackLength > 0.0f && Reaction && Reaction->Montage == ReactionMontage)
	{
		Pushback->RequestPushback(Direction, Settings);
	}
	else if (CVarCombatPushbackDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogCombatPushback, Log, TEXT("%s: rejected, %s did not start as the root-motion reaction"),
			*GetNameSafe(Pushback->GetOwner()), *GetNameSafe(ReactionMontage));
	}
}

UCombatPushbackComponent::UCombatPushbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UCombatPushbackComponent::IsUsableCurve(const UCurveFloat* Curve)
{
	if (!IsValid(Curve))
	{
		return false;
	}

	const TArray<FRichCurveKey>& Keys = Curve->FloatCurve.GetConstRefOfKeys();
	if (Keys.Num() < 2 || Keys[0].Time != 0.0f || Keys[0].Value != 0.0f
		|| Keys.Last().Time != 1.0f || Keys.Last().Value != 1.0f)
	{
		return false;
	}

	for (int32 Index = 1; Index < Keys.Num(); ++Index)
	{
		const FRichCurveKey& Start = Keys[Index - 1];
		const FRichCurveKey& End = Keys[Index];
		if (!FMath::IsFinite(End.Time) || !FMath::IsFinite(End.Value)
			|| End.Time <= Start.Time || End.Value < Start.Value || End.Value > 1.0f)
		{
			return false;
		}

		if (Start.InterpMode == RCIM_Linear || (Start.InterpMode == RCIM_Constant && Start.Value == End.Value))
		{
			continue;
		}

		if (Start.InterpMode != RCIM_Cubic || !FMath::IsFinite(Start.LeaveTangent) || !FMath::IsFinite(End.ArriveTangent))
		{
			return false;
		}

		const bool bWeightedLeave = Start.TangentWeightMode == RCTWM_WeightedLeave || Start.TangentWeightMode == RCTWM_WeightedBoth;
		const bool bWeightedArrive = End.TangentWeightMode == RCTWM_WeightedArrive || End.TangentWeightMode == RCTWM_WeightedBoth;
		if ((bWeightedLeave && (!FMath::IsFinite(Start.LeaveTangentWeight) || Start.LeaveTangentWeight < 0.0f))
			|| (bWeightedArrive && (!FMath::IsFinite(End.ArriveTangentWeight) || End.ArriveTangentWeight < 0.0f)))
		{
			return false;
		}

		const double SegmentTime = End.Time - Start.Time;
		const FVector2D Leave = MakeCurveHandle(Start.LeaveTangent, Start.LeaveTangentWeight, bWeightedLeave, SegmentTime);
		const FVector2D Arrive = MakeCurveHandle(End.ArriveTangent, End.ArriveTangentWeight, bWeightedArrive, SegmentTime);
		if (!IsMonotonicBezier(0.0, Leave.X, SegmentTime - Arrive.X, SegmentTime)
			|| !IsMonotonicBezier(Start.Value, Start.Value + Leave.Y, End.Value - Arrive.Y, End.Value))
		{
			return false;
		}
	}

	return true;
}

float UCombatPushbackComponent::EvaluateFraction(float Alpha, const UCurveFloat* Curve)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (Curve && ClampedAlpha > 0.0f && ClampedAlpha < 1.0f)
	{
		return FMath::Clamp(Curve->GetFloatValue(ClampedAlpha), 0.0f, 1.0f);
	}

	const float Remaining = 1.0f - ClampedAlpha;
	return 1.0f - Remaining * Remaining * Remaining;
}

bool UCombatPushbackComponent::InitializeDisplacement(FVector Direction, const FCombatPushbackSettings& Settings)
{
	const float Distance = Settings.Distance * PushbackMultiplier;
	const FVector Horizontal = Direction.ContainsNaN() ? FVector::ZeroVector : Direction.GetSafeNormal2D();
	if (!Settings.bEnabled || !FMath::IsFinite(Settings.Distance) || Settings.Distance <= 0.0f
		|| !FMath::IsFinite(PushbackMultiplier) || PushbackMultiplier <= 0.0f || !FMath::IsFinite(Distance) || Distance <= 0.0f
		|| !FMath::IsFinite(Settings.Duration) || Settings.Duration < 0.001f
		|| Horizontal.IsNearlyZero() || Horizontal.ContainsNaN())
	{
		return false;
	}

	PushbackDirection = Horizontal;
	EffectiveDistance = Distance;
	ActiveDuration = Settings.Duration;
	ActiveCurve = IsUsableCurve(Settings.DisplacementCurve) ? Settings.DisplacementCurve.Get() : nullptr;
	return true;
}

FVector UCombatPushbackComponent::AdvanceDisplacement(float MontageSeconds)
{
	if (!IsPushbackActive() || !FMath::IsFinite(MontageSeconds) || MontageSeconds <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	Elapsed = FMath::Min(Elapsed + MontageSeconds, ActiveDuration);
	const float Cumulative = EffectiveDistance * EvaluateFraction(Elapsed / ActiveDuration, ActiveCurve);
	const float Increment = FMath::Max(Cumulative - RequestedDistance, 0.0f);
	RequestedDistance += Increment;
	return PushbackDirection * Increment;
}

const TCHAR* UCombatPushbackComponent::GetMovementRejection(const ACharacter* Character,
	const UCharacterMovementComponent* Movement) const
{
	if (UCombatActionRestrictionComponent::IsActorMovementRestricted(Character))
	{
		return TEXT("movement is restricted");
	}

	if (!Character || !Movement || !Movement->HasValidData() || Character->GetLocalRole() == ROLE_SimulatedProxy
		|| !Movement->IsComponentTickEnabled() || Movement->MovementMode == MOVE_None)
	{
		return TEXT("no active character movement");
	}
	if (!Character->GetMesh() || Character->GetMesh()->IsAnySimulatingPhysics())
	{
		return TEXT("mesh unavailable or simulating physics");
	}
	const UContextualAnimSceneActorComponent* Scene = Character->FindComponentByClass<UContextualAnimSceneActorComponent>();
	return Scene && Scene->IsInActiveScene() ? TEXT("contextual animation active") : nullptr;
}

bool UCombatPushbackComponent::RequestPushback(FVector Direction, const FCombatPushbackSettings& Settings)
{
	PendingSettings = {};
	PendingDirection = FVector::ZeroVector;
	StopActivePushback(TEXT("replaced"));

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const TCHAR* Rejection = GetMovementRejection(Character, Movement);
	if (!Rejection && Movement->ProcessRootMotionPostConvertToWorld.IsBound())
	{
		Rejection = TEXT("post-conversion root motion delegate already bound");
	}
	const FAnimMontageInstance* Reaction = Rejection ? nullptr : Character->GetRootMotionAnimMontageInstance();
	if (!Rejection && (!Reaction || !Reaction->IsActive() || Reaction->IsRootMotionDisabled()))
	{
		Rejection = TEXT("no active root-motion reaction montage");
	}
	if (!Rejection && !InitializeDisplacement(Direction, Settings))
	{
		Rejection = TEXT("disabled or invalid distance, duration, direction, or resistance");
	}

	if (CVarCombatPushbackDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogCombatPushback, Log, TEXT("%s: %s, Distance=%.2f cm, Effective=%.2f cm, Duration=%.3f s, Direction=%s%s%s"),
			*GetNameSafe(GetOwner()), Rejection ? TEXT("rejected") : TEXT("accepted"), Settings.Distance,
			Settings.Distance * PushbackMultiplier, Settings.Duration, *Direction.GetSafeNormal2D().ToCompactString(),
			Rejection ? TEXT(", Reason=") : TEXT(""), Rejection ? Rejection : TEXT(""));
		if (!Rejection && Settings.DisplacementCurve && !ActiveCurve)
		{
			UE_LOG(LogCombatPushback, Log, TEXT("%s: invalid cumulative curve, using cubic ease out"), *GetNameSafe(GetOwner()));
		}
	}
	if (Rejection)
	{
		return false;
	}

	ReactionInstanceId = Reaction->GetInstanceID();
	ReactionMontage = Reaction->Montage;
	LastMontagePosition = Reaction->GetPosition();
	ReactionAnimInstance = Character->GetMesh()->GetAnimInstance();
	ReactionAnimInstance->OnMontageBlendingOut.AddUniqueDynamic(this, &UCombatPushbackComponent::OnReactionBlendingOut);

	BoundMovement = Movement;
	Movement->ProcessRootMotionPostConvertToWorld.BindUObject(this, &UCombatPushbackComponent::AddWorldSpacePushback);
	AddTickPrerequisiteComponent(Movement);
	SetComponentTickEnabled(true);
	return true;
}

const FAnimMontageInstance* UCombatPushbackComponent::GetTrackedReaction() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const FAnimMontageInstance* Reaction = Character ? Character->GetRootMotionAnimMontageInstance() : nullptr;
	return Reaction && Reaction->GetInstanceID() == ReactionInstanceId && Reaction->Montage == ReactionMontage.Get()
		&& Character->GetMesh() && Character->GetMesh()->GetAnimInstance() == ReactionAnimInstance.Get() ? Reaction : nullptr;
}

FTransform UCombatPushbackComponent::AddWorldSpacePushback(const FTransform& WorldRootMotion,
	UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	if (!IsPushbackActive() || DeltaSeconds <= 0.0f)
	{
		return WorldRootMotion;
	}

	const TCHAR* Rejection = GetMovementRejection(Cast<ACharacter>(GetOwner()), Movement);
	const FAnimMontageInstance* Reaction = GetTrackedReaction();
	if (Rejection || !Reaction || Reaction->IsRootMotionDisabled())
	{
		DeferredStopReason = Rejection ? Rejection : TEXT("reaction ended, replaced, or lost root motion");
		return WorldRootMotion;
	}

	const float Position = Reaction->GetPosition();
	if (Position == LastMontagePosition)
	{
		// Paused montage, duplicate conversion, or request made after this pose was extracted.
		return WorldRootMotion;
	}

	const float MontageSeconds = Position - LastMontagePosition;
	if (MontageSeconds < 0.0f || !FMath::IsNearlyEqual(Reaction->GetPreviousPosition(), LastMontagePosition, 0.0001f)
		|| !FMath::IsNearlyEqual(Reaction->GetDeltaMoved(), MontageSeconds, 0.0001f))
	{
		DeferredStopReason = TEXT("reaction montage playback jumped");
		return WorldRootMotion;
	}

	LastMontagePosition = Position;
	FTransform Combined = WorldRootMotion;
	Combined.AddToTranslation(AdvanceDisplacement(MontageSeconds));
	if (Elapsed >= ActiveDuration)
	{
		// Leave the executing single-cast delegate intact until movement returns.
		DeferredStopReason = TEXT("completed");
	}
	return Combined;
}

void UCombatPushbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DeferredStopReason)
	{
		StopActivePushback(DeferredStopReason);
		return;
	}
	const UCharacterMovementComponent* Movement = BoundMovement.Get();
	const TCHAR* Rejection = GetMovementRejection(Cast<ACharacter>(GetOwner()), Movement);
	const FAnimMontageInstance* Reaction = GetTrackedReaction();
	if (Rejection || !Reaction || Reaction->IsRootMotionDisabled()
		|| !Movement->ProcessRootMotionPostConvertToWorld.IsBoundToObject(this))
	{
		StopActivePushback(Rejection ? Rejection : TEXT("reaction or root-motion hook stopped"));
	}
	else if (Reaction->GetPosition() != LastMontagePosition)
	{
		StopActivePushback(TEXT("reaction advanced without usable root motion"));
	}
}

void UCombatPushbackComponent::OnReactionBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	const FAnimMontageInstance* Reaction = GetTrackedReaction();
	if (bInterrupted && Montage == ReactionMontage.Get() && (!Reaction || Reaction->IsStopped()))
	{
		StopActivePushback(TEXT("reaction interrupted"));
	}
}

void UCombatPushbackComponent::StopActivePushback(const TCHAR* Reason)
{
	if (ActiveDuration > 0.0f && CVarCombatPushbackDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogCombatPushback, Log, TEXT("%s: %s, requested %.2f / %.2f cm before collision"),
			*GetNameSafe(GetOwner()), Reason, RequestedDistance, EffectiveDistance);
	}

	if (UCharacterMovementComponent* Movement = BoundMovement.Get())
	{
		if (Movement->ProcessRootMotionPostConvertToWorld.IsBoundToObject(this))
		{
			Movement->ProcessRootMotionPostConvertToWorld.Unbind();
		}
		RemoveTickPrerequisiteComponent(Movement);
	}
	if (UAnimInstance* AnimInstance = ReactionAnimInstance.Get())
	{
		AnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &UCombatPushbackComponent::OnReactionBlendingOut);
	}

	SetComponentTickEnabled(false);
	BoundMovement.Reset();
	ReactionAnimInstance.Reset();
	ReactionMontage.Reset();
	DeferredStopReason = nullptr;
	ReactionInstanceId = INDEX_NONE;
	LastMontagePosition = 0.0f;
	PushbackDirection = FVector::ZeroVector;
	ActiveCurve = nullptr;
	ActiveDuration = 0.0f;
	EffectiveDistance = 0.0f;
	Elapsed = 0.0f;
	RequestedDistance = 0.0f;
}

void UCombatPushbackComponent::CancelPushback()
{
	PendingSettings = {};
	PendingDirection = FVector::ZeroVector;
	StopActivePushback(TEXT("canceled"));
}

void UCombatPushbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelPushback();
	Super::EndPlay(EndPlayReason);
}

void UCombatPushbackComponent::OnUnregister()
{
	CancelPushback();
	Super::OnUnregister();
}
