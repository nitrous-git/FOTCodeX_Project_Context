#include "CombatStatusEffectTestActor.h"

#include "Combat/Status/CombatStatusEffectDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"

ACombatStatusEffectTestActor::ACombatStatusEffectTestActor()
{
	Status = CreateDefaultSubobject<UCombatStatusEffectComponent>(TEXT("CombatStatusEffects"));
	Restrictions = CreateDefaultSubobject<UCombatActionRestrictionComponent>(TEXT("CombatActionRestrictions"));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACombatStatusEffectTestActor::OnCombatRestrictionChanged_Implementation(bool bRestricted)
{
	if (OnCombatRestriction)
	{
		OnCombatRestriction(bRestricted);
	}
}

FCombatStatusTargetState ACombatStatusEffectTestActor::GetCombatStatusTargetState_Implementation() const
{
	FCombatStatusTargetState State;
	State.Health = Health;
	State.MaxHealth = MaxHealth;
	State.bProtected = bProtected;
	return State;
}

float ACombatStatusEffectTestActor::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* Causer)
{
	const bool bStatusDamage = DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->IsChildOf<UCombatStatusDamageType>();
	if (Health <= 0.0f || bProtected || (!bStatusDamage && bRejectWeaponDamage))
	{
		return 0.0f;
	}
	if (bStatusDamage)
	{
		++StatusDamageEvents;
		LastStatusSource = Causer;
	}
	Health = FMath::Max(0.0f, Health - Damage);
	if (Health == 0.0f)
	{
		Status->ClearStatusEffectsOnDeath();
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, Causer);
}

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName Fire(TEXT("Fire"));
	const FName Bleed(TEXT("Bleed"));
	const FName LightningType(TEXT("Lightning"));

	UCombatStatusEffectDefinition* MakeFire()
	{
		auto* Definition = NewObject<UCombatStatusEffectDefinition>();
		Definition->StatusType = Fire;
		Definition->Execution = ECombatStatusExecution::PeriodicDamage;
		Definition->Duration = 6.0f;
		Definition->PeriodicMaxHealthPercent = 2.0f;
		return Definition;
	}

	UCombatStatusEffectDefinition* MakeBleed()
	{
		auto* Definition = NewObject<UCombatStatusEffectDefinition>();
		Definition->StatusType = Bleed;
		Definition->Execution = ECombatStatusExecution::InstantBurst;
		Definition->BurstMaxHealthPercent = 30.0f;
		return Definition;
	}

	UCombatStatusEffectDefinition* MakeLightning()
	{
		auto* Definition = NewObject<UCombatStatusEffectDefinition>();
		Definition->StatusType = LightningType;
		Definition->BaseBuildupPerHit = 20.0f;
		Definition->Duration = 2.5f;
		Definition->ImmunityDuration = 5.0f;
		Definition->ActionRestrictions = {true, true};
		return Definition;
	}

	void AdvanceWorld(FTestWorldWrapper& World, float Seconds)
	{
		constexpr float Step = 0.05f;
		const int32 Steps = FMath::CeilToInt(Seconds / Step);
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			World.TickTestWorld(Step);
		}
	}

	float WeaponHit(AActor* Target, AActor* Attacker, const FCombatWeaponStatus& Status,
		TSubclassOf<UDamageType> DamageType = nullptr)
	{
		return UCombatStatusEffectLibrary::ApplyWeaponDamageWithStatus(
			Target, 1.0f, nullptr, Attacker, DamageType, Target, FVector::ForwardVector, {}, Status);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatStatusAffinityTest, "CodeX.Combat.StatusEffects.Affinity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatStatusAffinityTest::RunTest(const FString& Parameters)
{
	const FCombatWeaponStatus Default;
	TestEqual(TEXT("Default affinity"), Default.AffinityMultiplier, 1.0f);
	TestEqual(TEXT("Default affinity preserves buildup"), UCombatStatusEffectLibrary::CalculateBuildup(25.0f, true, Default.AffinityMultiplier), 25.0f);
	TestEqual(TEXT("Matching affinity"), UCombatStatusEffectLibrary::CalculateBuildup(25.0f, true, 1.35f), 33.75f);
	TestEqual(TEXT("Mismatched affinity"), UCombatStatusEffectLibrary::CalculateBuildup(25.0f, false, 1.35f), 25.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatStatusLifecycleTest, "CodeX.Combat.StatusEffects.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatStatusLifecycleTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestTrue(TEXT("Transient game world starts"), TestWorld.CreateTestWorld(EWorldType::Game) && TestWorld.BeginPlayInTestWorld()))
	{
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	auto* Receiver = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto* Other = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto* Boss = World->SpawnActor<ACombatStatusEffectTestActor>();
	Boss->Status->bImmuneToAllStatuses = true;
	AActor* Attacker = World->SpawnActor<AActor>();
	AActor* OtherSource = World->SpawnActor<AActor>();
	UCombatStatusEffectDefinition* FireDefinition = MakeFire();
	UCombatStatusEffectDefinition* BleedDefinition = MakeBleed();
	auto* Lightning = NewObject<UCombatStatusEffectDefinition>();
	Lightning->StatusType = LightningType;
	Lightning->bGameplayEnabled = false;
	auto* TimedEffect = NewObject<UCombatStatusEffectDefinition>();
	TimedEffect->StatusType = TEXT("TestTimedEffect");
	TimedEffect->Duration = 1.5f;
	UCombatStatusEffectComponent& Status = *Receiver->Status;
	const FCombatWeaponStatus FireWeapon{FireDefinition, false, 1.35f};

	Receiver->bRejectWeaponDamage = true;
	WeaponHit(Receiver, Attacker, FireWeapon);
	TestEqual(TEXT("Rejected weapon damage adds no buildup"), Status.GetStatusBuildup(Fire), 0.0f);
	Receiver->bRejectWeaponDamage = false;
	Receiver->bProtected = true;
	TestFalse(TEXT("Protected receiver rejects explicit buildup"), Status.AddStatusBuildup(FireDefinition, 100.0f, Attacker, nullptr));
	Receiver->bProtected = false;
	TestFalse(TEXT("Lightning is disabled"), Status.AddStatusBuildup(Lightning, 100.0f, Attacker, nullptr));
	TestTrue(TEXT("Boss-style immunity reports immune"), Boss->Status->IsStatusImmune(Fire));
	TestFalse(TEXT("Boss-style immunity rejects buildup"), Boss->Status->AddStatusBuildup(FireDefinition, 100.0f, Attacker, nullptr));

	WeaponHit(Receiver, Attacker, FireWeapon);
	WeaponHit(Other, OtherSource, FireWeapon);
	TestEqual(TEXT("First target gets its own buildup"), Status.GetStatusBuildup(Fire), 25.0f);
	TestEqual(TEXT("Second target gets its own buildup"), Other->Status->GetStatusBuildup(Fire), 25.0f);
	TestTrue(TEXT("Second status builds independently"), Status.AddStatusBuildup(BleedDefinition, 25.0f, Attacker, nullptr));
	TestEqual(TEXT("Bleed has its own threshold percentage"), Status.GetStatusBuildupPercent(Bleed), 0.25f);
	AdvanceWorld(TestWorld, 3.8f);
	TestEqual(TEXT("No decay before the delay"), Status.GetStatusBuildup(Fire), 25.0f);
	AdvanceWorld(TestWorld, 0.7f);
	TestEqual(TEXT("Decay consumes 15 per second"), Status.GetStatusBuildup(Fire), 17.5f, 0.02f);
	AdvanceWorld(TestWorld, 1.3f);
	TestEqual(TEXT("Decay stops at zero"), Status.GetStatusBuildup(Fire), 0.0f);

	const float BeforeStatus = Receiver->Health;
	TestTrue(TEXT("Threshold activates Fire even with no Niagara asset"), Status.AddStatusBuildup(FireDefinition, 120.0f, Attacker, nullptr));
	TestTrue(TEXT("Fire is active"), Status.IsStatusActive(Fire));
	TestEqual(TEXT("Activation discards excess"), Status.GetStatusBuildup(Fire), 0.0f);
	TestFalse(TEXT("Active Fire cannot refresh or change source"), Status.AddStatusBuildup(FireDefinition, 100.0f, OtherSource, nullptr));
	TestTrue(TEXT("Fire also activates on a second target"), Other->Status->AddStatusBuildup(FireDefinition, 100.0f, OtherSource, nullptr));
	TestTrue(TEXT("Bleed can burst alongside Fire"), Status.AddStatusBuildup(BleedDefinition, 100.0f, OtherSource, nullptr));
	TestEqual(TEXT("Bleed hits once for 30% of Max HP"), Receiver->Health, BeforeStatus - 60.0f, 0.01f);
	TestTrue(TEXT("Bleed immediately grants immunity"), Status.IsStatusImmune(Bleed));
	TestFalse(TEXT("Bleed cannot rebuild during immunity"), Status.AddStatusBuildup(BleedDefinition, 100.0f, OtherSource, nullptr));
	TestTrue(TEXT("Fire stays active after Bleed"), Status.IsStatusActive(Fire));
	TestTrue(TEXT("A second timed effect can activate while Fire is active"),
		Status.AddStatusBuildup(TimedEffect, 100.0f, OtherSource, nullptr));
	TestTrue(TEXT("Both timed effects are simultaneously active"),
		Status.IsStatusActive(Fire) && Status.IsStatusActive(TimedEffect->StatusType));
	AdvanceWorld(TestWorld, 1.6f);
	TestFalse(TEXT("The second effect expires on its own schedule"), Status.IsStatusActive(TimedEffect->StatusType));
	TestTrue(TEXT("The first effect keeps its original schedule"), Status.IsStatusActive(Fire));

	AdvanceWorld(TestWorld, 4.2f);
	TestEqual(TEXT("Five Fire ticks before expiration plus one Bleed burst"), Receiver->StatusDamageEvents, 6);
	AdvanceWorld(TestWorld, 0.3f);
	TestEqual(TEXT("Sixth Fire tick executes at the expiry boundary"), Receiver->StatusDamageEvents, 7);
	TestEqual(TEXT("Fire totals 12% of Max HP"), Receiver->Health, BeforeStatus - 84.0f, 0.01f);
	TestEqual(TEXT("Fire preserves its original source"), Receiver->LastStatusSource.Get(), Attacker);
	TestEqual(TEXT("Second target independently receives all six ticks"), Other->StatusDamageEvents, 6);
	TestFalse(TEXT("Fire expires"), Status.IsStatusActive(Fire));
	TestTrue(TEXT("Fire begins immunity"), Status.IsStatusImmune(Fire));
	TestFalse(TEXT("Fire cannot rebuild during immunity"), Status.AddStatusBuildup(FireDefinition, 100.0f, OtherSource, nullptr));
	AdvanceWorld(TestWorld, 2.1f);
	TestFalse(TEXT("Immunity expires"), Status.IsStatusImmune(Fire));

	WeaponHit(Receiver, Attacker, FireWeapon, UCombatStatusDamageType::StaticClass());
	TestEqual(TEXT("Status-tagged damage never generates buildup"), Status.GetStatusBuildup(Fire), 0.0f);
	Status.Resistances.FindOrAdd(Fire).ThresholdOverride = 150.0f;
	Status.AddStatusBuildup(FireDefinition, 100.0f, Attacker, nullptr);
	TestFalse(TEXT("Elite threshold can override the definition"), Status.IsStatusActive(Fire));
	Status.AddStatusBuildup(FireDefinition, 50.0f, Attacker, nullptr);
	TestTrue(TEXT("Overridden threshold activates Fire"), Status.IsStatusActive(Fire));
	Receiver->Health = 3.0f;
	AdvanceWorld(TestWorld, 1.1f);
	TestEqual(TEXT("Fire can kill through the damage path"), Receiver->Health, 0.0f);
	TestFalse(TEXT("Death clears active Fire"), Status.IsStatusActive(Fire));
	TestFalse(TEXT("Death rejects new buildup"), Status.AddStatusBuildup(BleedDefinition, 100.0f, Attacker, nullptr));

	Other->Health = 50.0f;
	Other->Status->AddStatusBuildup(BleedDefinition, 100.0f, Attacker, nullptr);
	TestEqual(TEXT("Bleed is lethal when its percentage exceeds remaining HP"), Other->Health, 0.0f);
	TestFalse(TEXT("Lethal Bleed leaves no immunity state"), Other->Status->IsStatusImmune(Bleed));

	auto* Destroyed = World->SpawnActor<ACombatStatusEffectTestActor>();
	UCombatStatusEffectComponent* DestroyedStatuses = Destroyed->Status;
	DestroyedStatuses->AddStatusBuildup(FireDefinition, 100.0f, Attacker, nullptr);
	Destroyed->Destroy();
	TestFalse(TEXT("Destruction abandons active Fire immediately"), DestroyedStatuses->IsStatusActive(Fire));
	AdvanceWorld(TestWorld, 6.1f);
	TestEqual(TEXT("Destroyed target receives no damage ticks"), Destroyed->StatusDamageEvents, 0);

	auto* Executing = World->SpawnActor<ACombatStatusEffectTestActor>();
	Executing->Status->AddStatusBuildup(FireDefinition, 100.0f, Attacker, nullptr);
	Executing->bProtected = true;
	AdvanceWorld(TestWorld, 1.1f);
	TestEqual(TEXT("A protected sequence skips the due periodic tick"), Executing->StatusDamageEvents, 0);
	Executing->bProtected = false;
	AdvanceWorld(TestWorld, 1.0f);
	TestEqual(TEXT("The next scheduled tick resumes after protection"), Executing->StatusDamageEvents, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLightningDurationTest, "CodeX.Combat.StatusEffects.LightningDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLightningDurationTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestTrue(TEXT("Transient game world starts"), TestWorld.CreateTestWorld(EWorldType::Game) && TestWorld.BeginPlayInTestWorld()))
	{
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	auto* Receiver = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto* Other = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto* Unsupported = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto* Attacker = World->SpawnActor<AActor>();
	const auto* Lightning = MakeLightning();
	const FCombatWeaponStatus Matching{Lightning, true, 1.35f};
	const FCombatWeaponStatus Mismatched{Lightning, false, 1.35f};
	auto& Status = *Receiver->Status;
	auto& Restrictions = *Receiver->Restrictions;

	Unsupported->Restrictions->DestroyComponent();
	TestFalse(TEXT("Unsupported receiver rejects even partial buildup"), Unsupported->Status->AddStatusBuildup(Lightning, 20, Attacker, nullptr));
	Receiver->bProtected = true;
	TestFalse(TEXT("Protected receiver rejects Lightning"), Status.AddStatusBuildup(Lightning, 100, Attacker, nullptr));
	Receiver->bProtected = false;
	Receiver->SetCanBeDamaged(false);
	TestFalse(TEXT("Invulnerable receiver rejects Lightning"), Status.AddStatusBuildup(Lightning, 100, Attacker, nullptr));
	Receiver->SetCanBeDamaged(true);

	for (int32 Hit = 0; Hit < 3; ++Hit)
	{
		WeaponHit(Receiver, Attacker, Matching);
		WeaponHit(Other, Attacker, Mismatched);
	}
	TestEqual(TEXT("Typed matching affinity adds 27 per hit"), Status.GetStatusBuildup(LightningType), 81.0f);
	TestEqual(TEXT("Mismatched affinity adds 20"), Other->Status->GetStatusBuildup(LightningType), 60.0f);
	TestFalse(TEXT("Below threshold does not interrupt"), Restrictions.HasCombatRestriction());
	WeaponHit(Receiver, Attacker, Matching);
	WeaponHit(Other, Attacker, Mismatched);
	TestTrue(TEXT("Fourth matching hit activates even without Niagara"), Status.IsStatusActive(LightningType));
	TestFalse(TEXT("Fourth mismatched hit remains below threshold"), Other->Status->IsStatusActive(LightningType));
	TestEqual(TEXT("Active duration starts at 2.5 seconds"), Status.GetStatusRemainingTime(LightningType), 2.5f);
	TestTrue(TEXT("Lightning restricts movement"), Restrictions.HasMovementRestriction());
	TestTrue(TEXT("Lightning restricts combat"), Restrictions.HasCombatRestriction());

	AdvanceWorld(TestWorld, 0.5f);
	WeaponHit(Other, Attacker, Mismatched);
	TestTrue(TEXT("A second enemy has an independent stun"), Other->Restrictions->HasCombatRestriction());
	TestFalse(TEXT("Lightning rejects active buildup"), Status.AddStatusBuildup(Lightning, 100, Attacker, nullptr));
	TestEqual(TEXT("Buildup is zero during stun"), Status.GetStatusBuildup(LightningType), 0.0f);
	TestEqual(TEXT("Repeated hits do not refresh duration"), Status.GetStatusRemainingTime(LightningType), 2.0f, 0.01f);
	AdvanceWorld(TestWorld, 1.9f);
	TestTrue(TEXT("Stun is still active before 2.5 seconds"), Restrictions.HasCombatRestriction());
	// Timers dispatch on the first world tick at or beyond the measured end time.
	AdvanceWorld(TestWorld, 0.15f);
	TestFalse(TEXT("Movement lock ends at expiration"), Restrictions.HasMovementRestriction());
	TestFalse(TEXT("Combat lock ends at expiration"), Restrictions.HasCombatRestriction());
	TestTrue(TEXT("Second enemy remains stunned"), Other->Restrictions->HasCombatRestriction());
	TestEqual(TEXT("Immunity starts at 5 seconds"), Status.GetStatusImmunityRemainingTime(LightningType), 5.0f, 0.06f);
	TestFalse(TEXT("Immunity rejects buildup"), Status.AddStatusBuildup(Lightning, 100, Attacker, nullptr));
	TestEqual(TEXT("Lightning never applies status damage"), Receiver->StatusDamageEvents, 0);
	AdvanceWorld(TestWorld, 4.9f);
	TestTrue(TEXT("Immunity remains active before five seconds"), Status.IsStatusImmune(LightningType));
	AdvanceWorld(TestWorld, 0.15f);
	TestFalse(TEXT("Immunity ends"), Status.IsStatusImmune(LightningType));
	TestTrue(TEXT("Buildup can resume"), Status.AddStatusBuildup(Lightning, 20, Attacker, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatRestrictionOwnershipTest, "CodeX.Combat.StatusEffects.RestrictionOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatRestrictionOwnershipTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestTrue(TEXT("Transient game world starts"), TestWorld.CreateTestWorld(EWorldType::Game) && TestWorld.BeginPlayInTestWorld()))
	{
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	auto* Receiver = World->SpawnActor<ACombatStatusEffectTestActor>();
	auto& Status = *Receiver->Status;
	auto& Restrictions = *Receiver->Restrictions;
	const auto* Lightning = MakeLightning();
	int32 Transitions = 0;
	Receiver->OnCombatRestriction = [&Transitions](bool) { ++Transitions; };
	const FGuid Movement = Restrictions.AcquireRestriction({true, false});
	const FGuid Combat = Restrictions.AcquireRestriction({false, true});
	TestTrue(TEXT("Independent sources get unique handles"), Movement.IsValid() && Combat.IsValid() && Movement != Combat);
	TestTrue(TEXT("A separate status can own both restrictions"), Status.AddStatusBuildup(Lightning, 100, nullptr, nullptr));
	TestTrue(TEXT("Releasing movement does not drop either aggregate flag"), Restrictions.ReleaseRestriction(Movement)
		&& Restrictions.HasMovementRestriction() && Restrictions.HasCombatRestriction());
	TestFalse(TEXT("A previously released handle is harmless"), Restrictions.ReleaseRestriction(Movement));
	Status.ClearStatusEffect(LightningType);
	TestFalse(TEXT("Explicit removal releases the status's movement lock"), Restrictions.HasMovementRestriction());
	TestTrue(TEXT("Explicit removal leaves the other combat source"), Restrictions.HasCombatRestriction());
	TestEqual(TEXT("Overlapping active sources cause one combat interruption"), Transitions, 1);
	TestTrue(TEXT("Last combat owner can release its own handle"), Restrictions.ReleaseRestriction(Combat));
	TestEqual(TEXT("Last release causes one combat recovery"), Transitions, 2);

	Receiver->OnCombatRestriction = [&Status](bool bRestricted)
	{
		if (bRestricted)
		{
			Status.ClearAllStatusEffects();
		}
	};
	Status.AddStatusBuildup(Lightning, 100, nullptr, nullptr);
	TestFalse(TEXT("Reentrant clear cancels the effect"), Status.IsStatusActive(LightningType));
	TestFalse(TEXT("Reentrant clear leaves no movement restriction"), Restrictions.HasMovementRestriction());
	TestFalse(TEXT("Reentrant clear leaves no combat restriction"), Restrictions.HasCombatRestriction());

	Receiver->OnCombatRestriction = [&Transitions](bool) { ++Transitions; };
	Status.AddStatusBuildup(Lightning, 100, nullptr, nullptr);
	Receiver->Health = 1.0f;
	Status.AddStatusBuildup(MakeFire(), 100, nullptr, nullptr);
	AdvanceWorld(TestWorld, 1.1f);
	TestEqual(TEXT("Fire can kill while the receiver is stunned"), Receiver->Health, 0.0f);
	TestFalse(TEXT("Death releases the action lock"), Restrictions.HasCombatRestriction() || Restrictions.HasMovementRestriction());
	TestFalse(TEXT("Death clears active Lightning"), Status.IsStatusActive(LightningType));
	TestFalse(TEXT("Dead owners cannot acquire new restrictions"), Restrictions.AcquireRestriction({true, true}).IsValid());
	TestEqual(TEXT("Death does not invoke the living recovery callback"), Transitions, 3);
	Receiver->OnCombatRestriction = {};

	auto* RemovedStatus = World->SpawnActor<ACombatStatusEffectTestActor>();
	RemovedStatus->Status->AddStatusBuildup(Lightning, 100, nullptr, nullptr);
	RemovedStatus->Status->DestroyComponent();
	TestFalse(TEXT("Destroying just the status component releases its ownership"), RemovedStatus->Restrictions->HasCombatRestriction());
	auto* Destroyed = World->SpawnActor<ACombatStatusEffectTestActor>();
	Destroyed->Status->AddStatusBuildup(Lightning, 100, nullptr, nullptr);
	Destroyed->Destroy();
	TestFalse(TEXT("Actor destruction clears Lightning"), Destroyed->Status->IsStatusActive(LightningType));
	TestFalse(TEXT("Actor destruction cleans restrictions"), Destroyed->Restrictions->HasCombatRestriction());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
