#include "Combat/Status/CombatStatusEffectComponent.h"

#include "Combat/CombatActionRestrictionComponent.h"
#include "Combat/Motion/CombatPushbackComponent.h"
#include "Combat/Status/CombatStatusEffectDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContextualAnimSceneActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatStatusEffects, Log, All);

namespace
{
	constexpr double DecayUpdateInterval = 0.2;
	constexpr double TimerTolerance = 1.e-5;

#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<int32> CVarStatusDebug(TEXT("codex.StatusEffects.Debug"), 0,
		TEXT("Show status buildup and log status events for affected enemies."));
	TAutoConsoleVariable<FString> CVarStatusAffinityType(TEXT("codex.StatusEffects.DebugAffinityType"), TEXT("None"),
		TEXT("Override the weapon affinity while status debug is enabled. Use None, Fire, Bleed, or Lightning."));
	TAutoConsoleVariable<float> CVarStatusAffinityMultiplier(TEXT("codex.StatusEffects.DebugAffinityMultiplier"), 1.35f,
		TEXT("Buildup multiplier for the debug affinity type."));

	FAutoConsoleVariableSink StatusDebugSink(FConsoleCommandDelegate::CreateLambda([]
	{
		static int32 LastValue = 0;
		const int32 Value = CVarStatusDebug.GetValueOnGameThread();
		if (Value == LastValue)
		{
			return;
		}
		LastValue = Value;
		for (TObjectIterator<UCombatStatusEffectComponent> It; It; ++It)
		{
			if (It->HasBegunPlay())
			{
				It->UpdateDebugTimer();
			}
		}
	}));
#endif

	bool IsDebugEnabled()
	{
#if !UE_BUILD_SHIPPING
		return CVarStatusDebug.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	bool CanReceiveStatus(const FCombatStatusTargetState& Target)
	{
		return !Target.bProtected && FMath::IsFinite(Target.Health) && Target.Health > 0.0f
			&& FMath::IsFinite(Target.MaxHealth) && Target.MaxHealth > 0.0f;
	}

	void DestroyPersistentVFX(FCombatStatusRuntime& State)
	{
		if (IsValid(State.PersistentVFX))
		{
			State.PersistentVFX->DestroyComponent();
		}
		State.PersistentVFX = nullptr;
	}

	void ReleaseRestriction(FCombatStatusRuntime& State)
	{
		UCombatActionRestrictionComponent* Owner = State.RestrictionOwner.Get();
		State.RestrictionOwner.Reset();
		const FGuid Handle = State.RestrictionHandle;
		State.RestrictionHandle.Invalidate();
		if (Owner && Handle.IsValid())
		{
			Owner->ReleaseRestriction(Handle);
		}
	}
}

UCombatStatusEffectComponent::UCombatStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateDebugTimer();
}

void UCombatStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorld()->GetTimerManager().ClearTimer(DebugTimer);
	ResetEffects(false);
	Super::EndPlay(EndPlayReason);
}

FCombatStatusTargetState UCombatStatusEffectComponent::ReadTarget() const
{
	AActor* Owner = GetOwner();
	if (bOwnerDead || bEndingPlay || !IsValid(Owner) || Owner->IsActorBeingDestroyed() || !Owner->Implements<UCombatStatusTarget>())
	{
		return {};
	}

	FCombatStatusTargetState Target = ICombatStatusTarget::Execute_GetCombatStatusTargetState(Owner);
	const UContextualAnimSceneActorComponent* Scene = Owner->FindComponentByClass<UContextualAnimSceneActorComponent>();
	Target.bProtected |= !Owner->CanBeDamaged() || (Scene && Scene->IsInActiveScene());
	return Target;
}

double UCombatStatusEffectComponent::Now() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

TArray<FString> UCombatStatusEffectComponent::GetStatusTypes() const
{
	TArray<FString> Options;
	for (const FName Type : UCombatStatusEffectDefinition::GetKnownStatusTypes())
	{
		Options.Add(Type.ToString());
	}
	return Options;
}

float UCombatStatusEffectComponent::GetThreshold(const UCombatStatusEffectDefinition& Definition) const
{
	const FCombatStatusResistance* Resistance = Resistances.Find(Definition.StatusType);
	const float Threshold = Resistance && Resistance->ThresholdOverride > 0.0f
		? Resistance->ThresholdOverride : Definition.BuildupThreshold;
	return FMath::Max(0.01f, Threshold);
}

bool UCombatStatusEffectComponent::IsIntrinsicallyImmune(FName StatusType) const
{
	const FCombatStatusResistance* Resistance = Resistances.Find(StatusType);
	return bImmuneToAllStatuses || (Resistance && Resistance->bImmune);
}

float UCombatStatusEffectComponent::GetStatusBuildup(FName StatusType) const
{
	const FCombatStatusRuntime* State = Statuses.Find(StatusType);
	if (!State || !State->Definition)
	{
		return 0.0f;
	}
	const double Elapsed = FMath::Max(0.0, Now() - State->LastDecayTime);
	return FMath::Max(0.0f, State->Buildup - float(Elapsed) * FMath::Max(0.0f, State->Definition->DecayPerSecond));
}

float UCombatStatusEffectComponent::GetStatusBuildupPercent(FName StatusType) const
{
	const FCombatStatusRuntime* State = Statuses.Find(StatusType);
	return State && State->Definition
		? FMath::Clamp(GetStatusBuildup(StatusType) / GetThreshold(*State->Definition), 0.0f, 1.0f) : 0.0f;
}

bool UCombatStatusEffectComponent::IsStatusActive(FName StatusType) const
{
	const FCombatStatusRuntime* State = Statuses.Find(StatusType);
	return State && State->bActive;
}

bool UCombatStatusEffectComponent::IsStatusImmune(FName StatusType) const
{
	return IsIntrinsicallyImmune(StatusType) || GetStatusImmunityRemainingTime(StatusType) > 0.0f;
}

float UCombatStatusEffectComponent::GetStatusRemainingTime(FName StatusType) const
{
	const FCombatStatusRuntime* State = Statuses.Find(StatusType);
	return State && State->bActive ? float(FMath::Max(0.0, State->ActiveEnd - Now())) : 0.0f;
}

float UCombatStatusEffectComponent::GetStatusImmunityRemainingTime(FName StatusType) const
{
	const FCombatStatusRuntime* State = Statuses.Find(StatusType);
	return State ? float(FMath::Max(0.0, State->ImmunityEnd - Now())) : 0.0f;
}

bool UCombatStatusEffectComponent::AddStatusBuildup(const UCombatStatusEffectDefinition* Definition,
	float Amount, AActor* Source, AController* Instigator)
{
	if (bResettingEffects || !IsValid(Definition) || !Definition->bGameplayEnabled || Definition->StatusType.IsNone()
		|| !FMath::IsFinite(Amount) || Amount <= 0.0f || !GetWorld() || !GetOwner() || !GetOwner()->HasAuthority()
		|| !CanReceiveStatus(ReadTarget()))
	{
		return false;
	}

	UCombatActionRestrictionComponent* Restrictions = nullptr;
	if (!Definition->ActionRestrictions.IsEmpty())
	{
		Restrictions = GetOwner()->FindComponentByClass<UCombatActionRestrictionComponent>();
		if (Definition->Execution == ECombatStatusExecution::InstantBurst
			|| !Restrictions || !Restrictions->CanAcquireRestriction(Definition->ActionRestrictions))
		{
			UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s rejected (unsupported restrictions)"),
				*GetNameSafe(GetOwner()), *Definition->StatusType.ToString());
			return false;
		}
	}

	const FName Type = Definition->StatusType;
	if (IsStatusImmune(Type) || IsStatusActive(Type))
	{
		UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s rejected (%s)"),
			*GetNameSafe(GetOwner()), *Type.ToString(), IsStatusActive(Type) ? TEXT("active") : TEXT("immune"));
		return false;
	}

	const float PreviousBuildup = GetStatusBuildup(Type);
	FCombatStatusRuntime& State = Statuses.FindOrAdd(Type);
	State.Definition = Definition;
	State.Generation = ++NextGeneration;
	State.ImmunityEnd = 0.0;
	const float Threshold = GetThreshold(*Definition);
	State.Buildup = FMath::Min(Threshold, PreviousBuildup + Amount);
	State.LastDecayTime = Now() + FMath::Max(0.0f, Definition->DecayDelay);
	UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s buildup +%.2f (%.2f / %.2f) from %s"),
		*GetNameSafe(GetOwner()), *Type.ToString(), Amount, State.Buildup, Threshold, *GetNameSafe(Source));

	if (State.Buildup >= Threshold)
	{
		return StartEffect(Type, State, Source, Instigator, Restrictions);
	}
	else
	{
		if (Definition->DecayPerSecond > 0.0f)
		{
			Schedule(Type, State, State.LastDecayTime + FMath::Min(DecayUpdateInterval,
				double(State.Buildup) / Definition->DecayPerSecond));
		}
		OnStatusBuildupChanged.Broadcast(Type, State.Buildup, Threshold);
	}
	return true;
}

bool UCombatStatusEffectComponent::StartEffect(FName Type, FCombatStatusRuntime& State, AActor* Source, AController* Instigator,
	UCombatActionRestrictionComponent* Restrictions)
{
	const UCombatStatusEffectDefinition& Definition = *State.Definition;
	const bool bBurst = Definition.Execution == ECombatStatusExecution::InstantBurst;
	State.Buildup = 0.0f;
	State.Source = Source;
	State.Instigator = Instigator;
	State.ActivationTime = Now();
	State.ActiveEnd = State.ActivationTime + (bBurst ? 0.0 : FMath::Max(0.01f, Definition.Duration));
	State.NextTick = Definition.Execution == ECombatStatusExecution::PeriodicDamage && Definition.DamageInterval > 0.0f ? 1 : 0;
	State.bActive = true;
	const uint64 Generation = State.Generation;

	FCombatStatusRuntime* Current = &State;
	if (Restrictions)
	{
		// Publish the ownership before acquisition can call back into status removal.
		const FGuid Handle = FGuid::NewGuid();
		State.RestrictionOwner = Restrictions;
		State.RestrictionHandle = Handle;
		if (!Restrictions->AcquireRestrictionWithHandle(Handle, Definition.ActionRestrictions))
		{
			GetWorld()->GetTimerManager().ClearTimer(State.Timer);
			Statuses.Remove(Type);
			return false;
		}
		Current = Statuses.Find(Type);
		if (!Current || Current->Generation != Generation)
		{
			return true;
		}
	}

	SpawnVFX(Definition.ApplicationVFX, false);
	if (!bBurst)
	{
		Current->PersistentVFX = SpawnVFX(Definition.PersistentVFX, true);
		const double FirstTick = Current->NextTick > 0 ? Current->ActivationTime + Definition.DamageInterval : Current->ActiveEnd;
		Schedule(Type, *Current, FMath::Min(FirstTick, Current->ActiveEnd));
	}

	UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s applied (%.2fs)"),
		*GetNameSafe(GetOwner()), *Type.ToString(), Current->ActiveEnd - Current->ActivationTime);
	OnStatusBuildupChanged.Broadcast(Type, 0.0f, GetThreshold(Definition));
	Current = Statuses.Find(Type);
	if (!Current || Current->Generation != Generation)
	{
		return true;
	}
	OnStatusApplied.Broadcast(Type, Source);
	Current = Statuses.Find(Type);
	if (bBurst && Current && Current->Generation == Generation)
	{
		DealStatusDamage(Type, *Current, Definition.BurstMaxHealthPercent, true);
		Current = Statuses.Find(Type);
		if (Current && Current->Generation == Generation)
		{
			Expire(Type, *Current);
		}
	}
	return true;
}

void UCombatStatusEffectComponent::Schedule(FName Type, FCombatStatusRuntime& State, double Time)
{
	const float Delay = float(FMath::Max(TimerTolerance, Time - Now()));
	GetWorld()->GetTimerManager().SetTimer(State.Timer,
		FTimerDelegate::CreateUObject(this, &UCombatStatusEffectComponent::Advance, Type), Delay, false);
}

void UCombatStatusEffectComponent::Advance(FName Type)
{
	FCombatStatusRuntime* State = Statuses.Find(Type);
	if (!State)
	{
		return;
	}

	const double Time = Now();
	const UCombatStatusEffectDefinition& Definition = *State->Definition;
	if (State->bActive)
	{
		const uint64 Generation = State->Generation;
		// The last scheduled tick wins at the exact expiry boundary, including after a hitch.
		while (State->NextTick > 0)
		{
			const double TickTime = State->ActivationTime + double(State->NextTick) * Definition.DamageInterval;
			if (TickTime > FMath::Min(Time, State->ActiveEnd) + TimerTolerance)
			{
				break;
			}
			++State->NextTick;
			DealStatusDamage(Type, *State, Definition.PeriodicMaxHealthPercent, false);
			State = Statuses.Find(Type);
			if (!State || State->Generation != Generation)
			{
				return;
			}
		}
		if (Time + TimerTolerance >= State->ActiveEnd)
		{
			Expire(Type, *State);
		}
		else
		{
			const double NextTick = State->NextTick > 0
				? State->ActivationTime + double(State->NextTick) * Definition.DamageInterval : State->ActiveEnd;
			Schedule(Type, *State, FMath::Min(NextTick, State->ActiveEnd));
		}
		return;
	}

	if (State->Buildup > 0.0f)
	{
		State->Buildup = GetStatusBuildup(Type);
		State->LastDecayTime = FMath::Max(State->LastDecayTime, Time);
		const float Buildup = State->Buildup;
		const float Threshold = GetThreshold(Definition);
		if (Buildup > UE_KINDA_SMALL_NUMBER && Definition.DecayPerSecond > 0.0f)
		{
			Schedule(Type, *State, State->LastDecayTime + FMath::Min(DecayUpdateInterval,
				double(Buildup) / Definition.DecayPerSecond));
		}
		else if (Buildup <= UE_KINDA_SMALL_NUMBER)
		{
			Statuses.Remove(Type);
		}
		OnStatusBuildupChanged.Broadcast(Type, Buildup <= UE_KINDA_SMALL_NUMBER ? 0.0f : Buildup, Threshold);
	}
	else if (Time + TimerTolerance >= State->ImmunityEnd)
	{
		Statuses.Remove(Type);
	}
	else
	{
		Schedule(Type, *State, State->ImmunityEnd);
	}
}

void UCombatStatusEffectComponent::Expire(FName Type, FCombatStatusRuntime& State)
{
	const UCombatStatusEffectDefinition& Definition = *State.Definition;
	FCombatStatusRuntime Released;
	Released.RestrictionOwner = State.RestrictionOwner;
	Released.RestrictionHandle = State.RestrictionHandle;
	State.RestrictionOwner.Reset();
	State.RestrictionHandle.Invalidate();
	DestroyPersistentVFX(State);
	SpawnVFX(Definition.ExpirationVFX, false);
	State.bActive = false;
	State.Source.Reset();
	State.Instigator.Reset();
	if (Definition.ImmunityDuration > 0.0f)
	{
		State.ImmunityEnd = Now() + Definition.ImmunityDuration;
		Schedule(Type, State, State.ImmunityEnd);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(State.Timer);
		Statuses.Remove(Type);
	}
	UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s expired (%.2fs immunity)"),
		*GetNameSafe(GetOwner()), *Type.ToString(), Definition.ImmunityDuration);
	ReleaseRestriction(Released);
	if (!bEndingPlay)
	{
		OnStatusExpired.Broadcast(Type);
	}
}

void UCombatStatusEffectComponent::DealStatusDamage(FName Type, const FCombatStatusRuntime& State, float Percent, bool bBurst)
{
	const FCombatStatusTargetState Target = ReadTarget();
	if (!CanReceiveStatus(Target) || !FMath::IsFinite(Percent) || Percent <= 0.0f)
	{
		return;
	}
	const float Damage = Target.MaxHealth * (FMath::Min(100.0f, Percent) * 0.01f);
	UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: %s %s %.2f damage from %s"),
		*GetNameSafe(GetOwner()), *Type.ToString(), bBurst ? TEXT("burst") : TEXT("tick"), Damage, *GetNameSafe(State.Source.Get()));
	UGameplayStatics::ApplyDamage(GetOwner(), Damage, State.Instigator.Get(), State.Source.Get(), UCombatStatusDamageType::StaticClass());
}

void UCombatStatusEffectComponent::ClearStatusEffect(FName Type)
{
	FCombatStatusRuntime State;
	if (!Statuses.RemoveAndCopyValue(Type, State))
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(State.Timer);
	DestroyPersistentVFX(State);
	ReleaseRestriction(State);
	if (bEndingPlay)
	{
		return;
	}
	if (State.Buildup > 0.0f)
	{
		OnStatusBuildupChanged.Broadcast(Type, 0.0f, GetThreshold(*State.Definition));
	}
	if (State.bActive && !bEndingPlay)
	{
		OnStatusExpired.Broadcast(Type);
	}
}

void UCombatStatusEffectComponent::ResetEffects(bool bNotify)
{
	TGuardValue<bool> Resetting(bResettingEffects, true);
	TMap<FName, FCombatStatusRuntime> Removed = MoveTemp(Statuses);
	for (TPair<FName, FCombatStatusRuntime>& Pair : Removed)
	{
		GetWorld()->GetTimerManager().ClearTimer(Pair.Value.Timer);
		DestroyPersistentVFX(Pair.Value);
		ReleaseRestriction(Pair.Value);
	}
	if (bNotify)
	{
		for (const TPair<FName, FCombatStatusRuntime>& Pair : Removed)
		{
			if (bEndingPlay)
			{
				break;
			}
			if (Pair.Value.Buildup > 0.0f)
			{
				OnStatusBuildupChanged.Broadcast(Pair.Key, 0.0f, GetThreshold(*Pair.Value.Definition));
			}
			if (Pair.Value.bActive && !bEndingPlay)
			{
				OnStatusExpired.Broadcast(Pair.Key);
			}
		}
	}
}

void UCombatStatusEffectComponent::ClearAllStatusEffects()
{
	ResetEffects(true);
}

void UCombatStatusEffectComponent::ClearStatusEffectsOnDeath()
{
	if (!bOwnerDead)
	{
		bOwnerDead = true;
		UE_CLOG(IsDebugEnabled(), LogCombatStatusEffects, Log, TEXT("%s: status death cleanup"), *GetNameSafe(GetOwner()));
		if (UCombatActionRestrictionComponent* Restrictions = GetOwner()->FindComponentByClass<UCombatActionRestrictionComponent>())
		{
			Restrictions->ShutDown();
		}
		GetWorld()->GetTimerManager().ClearTimer(DebugTimer);
		ResetEffects(true);
	}
}

UNiagaraComponent* UCombatStatusEffectComponent::SpawnVFX(const FCombatStatusVFX& VFX, bool bPersistent) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(VFX.System) || !IsValid(Owner) || Owner->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	const FTransform& Relative = VFX.RelativeTransform;
	const ENCPoolMethod Pooling = bPersistent ? ENCPoolMethod::None : ENCPoolMethod::AutoRelease;
	UNiagaraComponent* Component = nullptr;
	if (VFX.Attachment == ECombatStatusVFXAttachment::World)
	{
		const FTransform WorldTransform = Relative * Owner->GetActorTransform();
		Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Owner, VFX.System, WorldTransform.GetLocation(),
			WorldTransform.Rotator(), WorldTransform.GetScale3D(), !bPersistent, false, Pooling, !bPersistent);
	}
	else
	{
		const ACharacter* Character = Cast<ACharacter>(Owner);
		USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();
		USceneComponent* Parent = IsValid(Mesh) ? Mesh : Owner->GetRootComponent();
		if (!IsValid(Parent) || (VFX.Attachment == ECombatStatusVFXAttachment::SkeletalMeshSampling && !IsValid(Mesh)))
		{
			return nullptr;
		}
		Component = UNiagaraFunctionLibrary::SpawnSystemAttached(VFX.System, Parent, VFX.Socket, Relative.GetLocation(),
			Relative.Rotator(), Relative.GetScale3D(), EAttachLocation::KeepRelativeOffset, !bPersistent, Pooling, false, !bPersistent);
		if (Component && VFX.Attachment == ECombatStatusVFXAttachment::SkeletalMeshSampling && !VFX.SkeletalMeshUserParameter.IsNone())
		{
			UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(
				Component, VFX.SkeletalMeshUserParameter.ToString(), Mesh);
		}
	}
	if (Component)
	{
		Component->Activate(true);
	}
	return Component;
}

void UCombatStatusEffectComponent::UpdateDebugTimer()
{
#if ENABLE_DRAW_DEBUG && !UE_BUILD_SHIPPING
	if (UWorld* World = GetWorld(); World && World->IsGameWorld() && !bOwnerDead && !bEndingPlay)
	{
		if (IsDebugEnabled() && GetOwner()->GetNetMode() != NM_DedicatedServer)
		{
			World->GetTimerManager().SetTimer(DebugTimer, this, &UCombatStatusEffectComponent::DrawDebugStatus, 0.25f, true);
		}
		else
		{
			World->GetTimerManager().ClearTimer(DebugTimer);
		}
	}
#endif
}

void UCombatStatusEffectComponent::DrawDebugStatus()
{
#if ENABLE_DRAW_DEBUG && !UE_BUILD_SHIPPING
	FString Text;
	if (const UCombatActionRestrictionComponent* Restrictions = GetOwner()->FindComponentByClass<UCombatActionRestrictionComponent>())
	{
		Text += FString::Printf(TEXT("Movement restricted: %s | Combat interrupted: %s\n"),
			Restrictions->HasMovementRestriction() ? TEXT("yes") : TEXT("no"),
			Restrictions->HasCombatRestriction() ? TEXT("yes") : TEXT("no"));
	}
	for (const UCombatStatusEffectDefinition* Definition : DebugDefinitions)
	{
		if (!IsValid(Definition))
		{
			continue;
		}
		const FName Type = Definition->StatusType;
		const FString TypeName = Type.ToString();
		if (!Definition->bGameplayEnabled || IsIntrinsicallyImmune(Type))
		{
			Text += FString::Printf(TEXT("%s %s\n"), *TypeName, Definition->bGameplayEnabled ? TEXT("immune") : TEXT("disabled"));
			continue;
		}
		Text += FString::Printf(TEXT("%s %.1f/%.0f"), *TypeName, GetStatusBuildup(Type), GetThreshold(*Definition));
		if (IsStatusActive(Type))
		{
			Text += FString::Printf(TEXT(" active %.1fs"), GetStatusRemainingTime(Type));
		}
		else if (IsStatusImmune(Type))
		{
			Text += FString::Printf(TEXT(" immune %.1fs"), GetStatusImmunityRemainingTime(Type));
		}
		Text += TEXT("\n");
	}
	if (!Text.IsEmpty())
	{
		const FVector Offset(0.0, 0.0, GetOwner()->GetSimpleCollisionHalfHeight() + 35.0);
		DrawDebugString(GetWorld(), Offset, Text, GetOwner(), FColor::White, 0.25f, true);
	}
#endif
}

float UCombatStatusEffectLibrary::ApplyWeaponDamageWithStatus(AActor* DamagedActor, float BaseDamage, AController* EventInstigator,
	AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, AActor* MeleeWindowTarget, FVector Direction,
	const FCombatPushbackSettings& Settings, const FCombatWeaponStatus& WeaponStatus)
{
	const UCombatStatusEffectDefinition* Definition = WeaponStatus.Definition;
	UCombatStatusEffectComponent* Receiver = nullptr;
	FCombatStatusTargetState Before;
	if (IsValid(DamagedActor) && DamagedActor->HasAuthority() && IsValid(Definition) && Definition->bGameplayEnabled
		&& BaseDamage > 0.0f && (!DamageTypeClass || !DamageTypeClass->IsChildOf<UCombatStatusDamageType>()))
	{
		Receiver = DamagedActor->FindComponentByClass<UCombatStatusEffectComponent>();
		Before = Receiver ? Receiver->ReadTarget() : FCombatStatusTargetState{};
	}

	const float Applied = UCombatPushbackLibrary::ApplyDamageWithPushback(DamagedActor, BaseDamage, EventInstigator,
		DamageCauser, DamageTypeClass, MeleeWindowTarget, Direction, Settings);
	if (Applied <= 0.0f || !CanReceiveStatus(Before) || !IsValid(Receiver))
	{
		return Applied;
	}

	const FCombatStatusTargetState After = Receiver->ReadTarget();
	if (CanReceiveStatus(After) && After.Health < Before.Health)
	{
		bool bMatches = WeaponStatus.bAffinityMatches;
		float Multiplier = WeaponStatus.AffinityMultiplier;
#if !UE_BUILD_SHIPPING
		if (IsDebugEnabled())
		{
			const FName OverrideType(*CVarStatusAffinityType.GetValueOnGameThread());
			if (!OverrideType.IsNone())
			{
				bMatches = OverrideType == Definition->StatusType;
				Multiplier = CVarStatusAffinityMultiplier.GetValueOnGameThread();
			}
		}
#endif
		Receiver->AddStatusBuildup(Definition, CalculateBuildup(Definition->BaseBuildupPerHit, bMatches, Multiplier),
			DamageCauser, EventInstigator);
	}
	return Applied;
}

bool UCombatStatusEffectLibrary::IsStatusDamage(const UDamageType* DamageType)
{
	return IsValid(DamageType) && DamageType->IsA<UCombatStatusDamageType>();
}

float UCombatStatusEffectLibrary::CalculateBuildup(float Base, bool bAffinityMatches, float Multiplier)
{
	const float Scale = bAffinityMatches && FMath::IsFinite(Multiplier) ? FMath::Max(0.0f, Multiplier) : 1.0f;
	return FMath::IsFinite(Base) ? FMath::Max(0.0f, Base) * Scale : 0.0f;
}
