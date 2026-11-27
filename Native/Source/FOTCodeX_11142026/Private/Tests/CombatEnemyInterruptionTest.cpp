#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BTTask_WaitForCombatRestriction.h"
#include "Combat/CombatActionRestrictionComponent.h"
#include "Combat/CombatEnemyAIController.h"
#include "Combat/Status/CombatStatusEffectComponent.h"
#include "Combat/Status/CombatStatusEffectDefinition.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Navigation/PathFollowingComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName LightningType(TEXT("Lightning"));
	const FName InterruptedKey(TEXT("bCombatInterrupted"));
	const FName TargetKey(TEXT("Target"));
	const TCHAR* EnemyRoot = TEXT("/Game/CodeX_Restruct/Frameworks/EnemyFramework/");

	UObject* GetObjectProperty(const UObject* Object, const FName Name)
	{
		const FObjectPropertyBase* Property = Object ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
		return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}

	bool SetObjectProperty(UObject* Object, const FName Name, UObject* Value)
	{
		const FObjectPropertyBase* Property = Object ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
		if (!Property || (Value && !Value->IsA(Property->PropertyClass)))
		{
			return false;
		}
		Property->SetObjectPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool SetBoolProperty(UObject* Object, const FName Name, bool bValue)
	{
		const FBoolProperty* Property = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), Name) : nullptr;
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, bValue);
		return true;
	}

	bool GetBoolProperty(const UObject* Object, const FName Name)
	{
		const FBoolProperty* Property = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), Name) : nullptr;
		return Property && Property->GetPropertyValue_InContainer(Object);
	}

	bool CallBoolFunction(UObject* Object, const FName Name)
	{
		UFunction* Function = Object->FindFunction(Name);
		const FBoolProperty* Result = Function ? FindFProperty<FBoolProperty>(Function, TEXT("ReturnValue")) : nullptr;
		if (!Result)
		{
			return false;
		}
		FStructOnScope Parameters(Function);
		Object->ProcessEvent(Function, Parameters.GetStructMemory());
		return Result->GetPropertyValue_InContainer(Parameters.GetStructMemory());
	}

	bool SetObserverState(ACharacter* Enemy)
	{
		UFunction* Function = Enemy->FindFunction(TEXT("SetEnemyState"));
		const FByteProperty* State = Function ? FindFProperty<FByteProperty>(Function, TEXT("NewState")) : nullptr;
		if (!State || !State->Enum)
		{
			return false;
		}
		const int64 Observer = State->Enum->GetValueByNameString(TEXT("NewEnumerator1"));
		if (Observer == INDEX_NONE)
		{
			return false;
		}
		FStructOnScope Parameters(Function);
		State->SetPropertyValue_InContainer(Parameters.GetStructMemory(), static_cast<uint8>(Observer));
		Enemy->ProcessEvent(Function, Parameters.GetStructMemory());
		return true;
	}

	bool CallWithEnemy(AActor* Manager, const FName FunctionName, ACharacter* Enemy)
	{
		UFunction* Function = Manager->FindFunction(FunctionName);
		if (!Function)
		{
			return false;
		}
		FStructOnScope Parameters(Function);
		for (TFieldIterator<FObjectPropertyBase> It(Function); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm) && Enemy->IsA(It->PropertyClass))
			{
				It->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Enemy);
				Manager->ProcessEvent(Function, Parameters.GetStructMemory());
				return true;
			}
		}
		return false;
	}

	bool ArrayContains(const UObject* Object, const FName Name, const UObject* Value)
	{
		const FArrayProperty* Property = FindFProperty<FArrayProperty>(Object->GetClass(), Name);
		const FObjectPropertyBase* Inner = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
		if (!Inner)
		{
			return false;
		}
		FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = 0; Index < Array.Num(); ++Index)
		{
			if (Inner->GetObjectPropertyValue(Array.GetRawPtr(Index)) == Value)
			{
				return true;
			}
		}
		return false;
	}

	void Tick(FTestWorldWrapper& TestWorld, int32 Frames = 3)
	{
		for (int32 Index = 0; Index < Frames; ++Index)
		{
			TestWorld.TickTestWorld(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatEnemyInterruptionTest, "CodeX.Combat.StatusEffects.StandardEnemyIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatEnemyInterruptionTest::RunTest(const FString& Parameters)
{
	const FString Root(EnemyRoot);
	UClass* MeleeClass = LoadClass<ACharacter>(nullptr, *(Root + TEXT("Core/Archetype/MeleeVariant/BP_Enemy_GroupMelee_SKSword.BP_Enemy_GroupMelee_SKSword_C")));
	UClass* RangedClass = LoadClass<ACharacter>(nullptr, *(Root + TEXT("Core/Archetype/RangedVariant/BP_Enemy_Ranged_SKMage.BP_Enemy_Ranged_SKMage_C")));
	UClass* ControllerClass = LoadClass<ACombatEnemyAIController>(nullptr, *(Root + TEXT("AI/BehaviorTree/AIController_BasicEnemy.AIController_BasicEnemy_C")));
	UClass* ManagerClass = LoadClass<AActor>(nullptr, TEXT("/Game/CodeX_Restruct/Shared/Managers/BP_EnemyAIManager.BP_EnemyAIManager_C"));
	auto* Tree = LoadObject<UBehaviorTree>(nullptr, *(Root + TEXT("AI/BehaviorTree/BT_BasicEnemy.BT_BasicEnemy")));
	auto* Asset = LoadObject<UCombatStatusEffectDefinition>(nullptr,
		TEXT("/Game/CodeX_Restruct/Shared/CombatFeedback/Data/StatusEffects/DA_StatusEffect_Lightning.DA_StatusEffect_Lightning"));
	if (!TestTrue(TEXT("Load the authored enemy classes, controller, manager, tree, and Lightning"), MeleeClass && RangedClass && ControllerClass && ManagerClass && Tree && Asset))
	{
		return false;
	}

	FTestWorldWrapper TestWorld;
	if (!TestTrue(TEXT("Create a standalone automation world"), TestWorld.CreateTestWorld(EWorldType::Game)))
	{
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	if (!TestTrue(TEXT("Initialize AI and start the world"), (World->GetAISystem() || World->CreateAISystem()) && TestWorld.BeginPlayInTestWorld()))
	{
		return false;
	}
	AActor* Manager = World->SpawnActor<AActor>(ManagerClass);
	ACharacter* Melee = World->SpawnActor<ACharacter>(MeleeClass, FVector(0, 0, 400), FRotator::ZeroRotator);
	ACharacter* Ranged = World->SpawnActor<ACharacter>(RangedClass, FVector(300, 0, 400), FRotator::ZeroRotator);
	AActor* Target = World->SpawnActor<AActor>();
	auto* Definition = DuplicateObject(Asset, GetTransientPackage());
	Definition->bGameplayEnabled = true;
	Definition->ActionRestrictions = {true, true};
	if (!TestTrue(TEXT("Spawn actual melee and ranged archetypes"), Manager && Melee && Ranged && Target))
	{
		return false;
	}

	const FName CombatBehavior(TEXT("CombatBehaviorComponent"));
	const FName AttackInProgress(TEXT("bAttackInProgress"));
	UCombatStatusEffectComponent* MeleeStatus = Melee->FindComponentByClass<UCombatStatusEffectComponent>();
	UCombatStatusEffectComponent* RangedStatus = Ranged->FindComponentByClass<UCombatStatusEffectComponent>();
	UObject* MeleeCombat = GetObjectProperty(Melee, CombatBehavior);
	UObject* RangedCombat = GetObjectProperty(Ranged, CombatBehavior);
	if (!TestTrue(TEXT("Both archetypes register their actual combat component"), MeleeStatus && RangedStatus && MeleeCombat && RangedCombat))
	{
		return false;
	}

	ACombatEnemyAIController* AI = Cast<ACombatEnemyAIController>(Melee->GetController());
	if (!AI)
	{
		AI = World->SpawnActor<ACombatEnemyAIController>(ControllerClass);
		AI->Possess(Melee);
	}
	if (!TestTrue(TEXT("Start the authored behavior tree"), AI->RunBehaviorTree(Tree)))
	{
		return false;
	}
	UBlackboardComponent* Blackboard = AI->GetBlackboardComponent();
	Blackboard->SetValueAsObject(TargetKey, Target);
	Melee->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Melee->GetCharacterMovement()->Velocity = FVector(120, 0, -85);

	// The sandbox's placed manager supplies slot counts as instance settings.
	for (const FName Setting : {FName(TEXT("MaxMeleeAttackers")), FName(TEXT("MaxRangedAttackers"))})
	{
		const FIntProperty* Limit = FindFProperty<FIntProperty>(ManagerClass, Setting);
		if (!TestNotNull(TEXT("Manager has a configurable slot limit"), Limit))
		{
			return false;
		}
		Limit->SetPropertyValue_InContainer(Manager, 1);
	}
	for (ACharacter* Enemy : {Melee, Ranged})
	{
		SetObjectProperty(Enemy, TEXT("AIManagerRef"), Manager);
		SetBoolProperty(Enemy, TEXT("bIsDead"), false);
		TestTrue(TEXT("Set aggro for this fixture"), SetBoolProperty(Enemy, TEXT("bIsInAggroRange"), true));
		SetBoolProperty(Enemy, TEXT("bIsCombatReady"), true);
		TestTrue(TEXT("Put the newly spawned enemy in Observer"), SetObserverState(Enemy));
		TestTrue(TEXT("Unrestricted enemy is group eligible"), CallBoolFunction(Enemy, TEXT("CanBeGroupAttacker")));
		TestTrue(TEXT("Call the roster's register function"), CallWithEnemy(Manager, TEXT("RegisterEnemy"), Enemy));
		TestTrue(TEXT("Registered enemy is present in the roster"), ArrayContains(Manager, TEXT("Enemies"), Enemy));
		TestTrue(TEXT("Call the roster's assignment function"), CallWithEnemy(Manager, TEXT("SetAttacker"), Enemy));
	}
	const FName MeleeSlot(TEXT("ActiveMeleeAttackers"));
	const FName RangedSlot(TEXT("ActiveRangedAttackers"));
	TestTrue(TEXT("The melee enemy occupies its real group slot"), ArrayContains(Manager, MeleeSlot, Melee));
	TestTrue(TEXT("The ranged enemy occupies its real group slot"), ArrayContains(Manager, RangedSlot, Ranged));

	Melee->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	UAnimMontage* RangedMontage = Cast<UAnimMontage>(GetObjectProperty(GetObjectProperty(RangedCombat, TEXT("RangedProfile")), TEXT("CastMontage")));
	if (UFunction* CastSpell = RangedCombat->FindFunction(TEXT("CE_CastSpell")))
	{
		RangedCombat->ProcessEvent(CastSpell, nullptr);
	}
	TestTrue(TEXT("Ranged attack starts through the authored cast event"),
		RangedMontage && GetBoolProperty(RangedCombat, AttackInProgress));
	TestTrue(TEXT("Seed a melee attack in progress"), SetBoolProperty(MeleeCombat, AttackInProgress, true));

	TestTrue(TEXT("Lightning activates on the real melee archetype"), MeleeStatus->AddStatusBuildup(Definition, 100, Target, nullptr));
	TestTrue(TEXT("Combat lock synchronizes the active Blackboard"), Blackboard->GetValueAsBool(InterruptedKey));
	TestTrue(TEXT("Blackboard Target is preserved"), Blackboard->GetValueAsObject(TargetKey) == Target);
	TestFalse(TEXT("Melee attack flag is cleared"), GetBoolProperty(MeleeCombat, AttackInProgress));
	TestFalse(TEXT("Melee attacker slot is released"), ArrayContains(Manager, MeleeSlot, Melee));
	TestFalse(TEXT("Interrupted enemy is ineligible"), CallBoolFunction(Melee, TEXT("CanBeGroupAttacker")));
	TestTrue(TEXT("Interrupted enemy remains registered"), ArrayContains(Manager, TEXT("Enemies"), Melee));
	CallWithEnemy(Manager, TEXT("SetAttacker"), Melee);
	TestFalse(TEXT("Roster cannot reassign the interrupted enemy"), ArrayContains(Manager, MeleeSlot, Melee));
	TestFalse(TEXT("Ranged enemy is unaffected by the melee lock"), UCombatActionRestrictionComponent::IsActorCombatRestricted(Ranged));
	TestEqual(TEXT("Horizontal velocity is cleared while vertical momentum remains"),
		Melee->GetCharacterMovement()->Velocity, FVector(0, 0, -85));
	Melee->AddMovementInput(FVector::ForwardVector, 1.0f);
	TestTrue(TEXT("Restricted enemy cannot accumulate voluntary movement input"), Melee->ConsumeMovementInputVector().IsNearlyZero());
	TestTrue(TEXT("Restricted AI cannot issue a new move"), AI->MoveTo(FAIMoveRequest(Melee->GetActorLocation())).Code == EPathFollowingRequestResult::Failed);

	Tick(TestWorld);
	UBehaviorTreeComponent* Brain = Cast<UBehaviorTreeComponent>(AI->GetBrainComponent());
	TestTrue(TEXT("Interruption branch aborts lower priority behavior"), Brain && Brain->GetActiveNode() && Brain->GetActiveNode()->IsA<UBTTask_WaitForCombatRestriction>());
	TestTrue(TEXT("Lightning activates independently on the real ranged archetype"), RangedStatus->AddStatusBuildup(Definition, 100, Target, nullptr));
	TestFalse(TEXT("The ranged attack flag clears before projectile release"), GetBoolProperty(RangedCombat, AttackInProgress));
	TestFalse(TEXT("The canceled cast cannot emit its queued offensive notify"), UCombatActionRestrictionComponent::CanRunOffensiveNotify(Ranged->GetMesh(), RangedMontage));
	TestFalse(TEXT("Ranged enemy releases its own slot"), ArrayContains(Manager, RangedSlot, Ranged));

	MeleeStatus->ClearStatusEffect(LightningType);
	Tick(TestWorld);
	TestFalse(TEXT("Recovery clears the Blackboard key"), Blackboard->GetValueAsBool(InterruptedKey));
	TestTrue(TEXT("Recovery leaves the interruption branch"), !Brain || !Brain->GetActiveNode() || !Brain->GetActiveNode()->IsA<UBTTask_WaitForCombatRestriction>());
	TestFalse(TEXT("Recovery does not give back the old slot"), ArrayContains(Manager, MeleeSlot, Melee));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
