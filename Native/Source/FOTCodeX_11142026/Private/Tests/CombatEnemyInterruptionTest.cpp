#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BTTask_WaitForCombatRestriction.h"
#include "Combat/CombatActionRestrictionComponent.h"
#include "Combat/CombatEnemyAIController.h"
#include "Combat/Motion/CombatPushbackComponent.h"
#include "Combat/Status/CombatStatusEffectComponent.h"
#include "Combat/Status/CombatStatusEffectDefinition.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContextualAnimSceneActorComponent.h"
#include "ContextualAnimSceneAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
		if (Function)
		{
			FStructOnScope Parameters(Function);
			for (TFieldIterator<FBoolProperty> It(Function); It; ++It)
			{
				if (It->HasAllPropertyFlags(CPF_Parm | CPF_OutParm))
				{
					Object->ProcessEvent(Function, Parameters.GetStructMemory());
					return It->GetPropertyValue_InContainer(Parameters.GetStructMemory());
				}
			}
		}
		return false;
	}

	double ReadNumber(const UObject* Object, FName Name)
	{
		const FNumericProperty* Property = FindFProperty<FNumericProperty>(Object->GetClass(), Name);
		return Property ? Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Object)) : 0.0;
	}

	bool SetReactionMontages(ACharacter* Enemy, FName Name, UAnimMontage* Montage, int32 Count)
	{
		const FArrayProperty* Property = FindFProperty<FArrayProperty>(Enemy->GetClass(), Name);
		const FObjectPropertyBase* Inner = Property ? CastField<FObjectPropertyBase>(Property->Inner) : nullptr;
		if (!Inner || !Montage || !Montage->IsA(Inner->PropertyClass))
		{
			return false;
		}
		FScriptArrayHelper Array(Property, Property->ContainerPtrToValuePtr<void>(Enemy));
		Array.Resize(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Inner->SetObjectPropertyValue(Array.GetRawPtr(Index), Montage);
		}
		return true;
	}

	bool CallExecutionEvent(ACharacter* Enemy, const FName Name, ACharacter* ExecutingActor)
	{
		UFunction* Function = Enemy->FindFunction(Name);
		if (!Function)
		{
			return false;
		}
		FStructOnScope Parameters(Function);
		const auto* Player = FindFProperty<FObjectPropertyBase>(Function, TEXT("ExecutingPlayer"));
		if (Player && ExecutingActor->IsA(Player->PropertyClass))
		{
			Player->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), ExecutingActor);
		}
		Enemy->ProcessEvent(Function, Parameters.GetStructMemory());
		return true;
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

	bool IsActiveTask(const UBehaviorTreeComponent* Brain, const FName ClassName)
	{
		const UBTNode* Task = Brain ? Brain->GetActiveNode() : nullptr;
		return Task && Task->GetClass()->GetFName() == ClassName;
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
	UAnimInstance* MeleeAnimation = Melee->GetMesh()->GetAnimInstance();
	auto* Pushback = Melee->FindComponentByClass<UCombatPushbackComponent>();
	if (!TestTrue(TEXT("The melee archetype supplies animation and pushback components"), MeleeAnimation && Pushback))
	{
		return false;
	}
	const ERootMotionMode::Type RootMotionMode = MeleeAnimation->RootMotionMode;
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
	TestTrue(TEXT("The other enemy keeps its in-progress cast"), GetBoolProperty(RangedCombat, AttackInProgress));
	TestEqual(TEXT("Falling momentum remains"), Melee->GetCharacterMovement()->Velocity.Z, -85.0);
	TestEqual(TEXT("Lightning preserves the authored root motion mode"), MeleeAnimation->RootMotionMode.GetValue(), RootMotionMode);
	Melee->AddMovementInput(FVector::ForwardVector, 1.0f);
	TestTrue(TEXT("Restricted enemy cannot accumulate voluntary movement input"), Melee->ConsumeMovementInputVector().IsNearlyZero());
	TestTrue(TEXT("Restricted AI cannot issue a new move"), AI->MoveTo(FAIMoveRequest(Melee->GetActorLocation())).Code == EPathFollowingRequestResult::Failed);

	Tick(TestWorld);
	UBehaviorTreeComponent* Brain = Cast<UBehaviorTreeComponent>(AI->GetBrainComponent());
	const FName WaitTask = UBTTask_WaitForCombatRestriction::StaticClass()->GetFName();
	const FName DamageTask(TEXT("BTTask_Damage_C"));
	const FName ExecutionTask(TEXT("BTTask_Execution_C"));
	TestTrue(TEXT("Interruption branch aborts lower priority behavior"), IsActiveTask(Brain, WaitTask));
	TestTrue(TEXT("Lightning activates independently on the real ranged archetype"), RangedStatus->AddStatusBuildup(Definition, 100, Target, nullptr));
	TestFalse(TEXT("The ranged attack flag clears before projectile release"), GetBoolProperty(RangedCombat, AttackInProgress));
	TestFalse(TEXT("The canceled cast cannot emit its queued offensive notify"), UCombatActionRestrictionComponent::CanRunOffensiveNotify(Ranged->GetMesh(), RangedMontage));
	TestFalse(TEXT("Ranged enemy releases its own slot"), ArrayContains(Manager, RangedSlot, Ranged));
	if (UFunction* CastSpell = RangedCombat->FindFunction(TEXT("CE_CastSpell")))
	{
		RangedCombat->ProcessEvent(CastSpell, nullptr);
	}
	TestFalse(TEXT("The restricted ranged archetype cannot start another cast"), GetBoolProperty(RangedCombat, AttackInProgress));

	FCombatPushbackSettings PushbackSettings;
	PushbackSettings.bEnabled = true;
	PushbackSettings.Distance = 30.0f;
	// The sandbox resolves presentation during startup with a local player. Supply the authored
	// montage and a short recovery window in this standalone fixture without player startup.
	const FString ReactionPath = Root + TEXT("Animations/CombatStyle/Katana/Hurt/Frank_Damage_Strong_F1_Montage.Frank_Damage_Strong_F1_Montage");
	auto* Reaction = LoadObject<UAnimMontage>(nullptr, *ReactionPath);
	const auto* RecoveryTime = FindFProperty<FDoubleProperty>(MeleeClass, TEXT("DamageRecoveryDuration"));
	if (!TestTrue(TEXT("The standalone enemy has authored reaction assets and a recovery window"),
		Reaction && Reaction->HasRootMotion() && RecoveryTime
		&& SetReactionMontages(Melee, TEXT("ResolvedHitSmallMontages"), Reaction, 1)
		&& SetReactionMontages(Melee, TEXT("ResolvedHitLargeMontages"), Reaction, 2)))
	{
		return false;
	}
	RecoveryTime->SetPropertyValue_InContainer(Melee, 0.45);
	Melee->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const auto HitMelee = [&]()
	{
		const double Health = ReadNumber(Melee, TEXT("HealthCurrent"));
		const float Accepted = UCombatPushbackLibrary::ApplyDamageWithPushback(Melee, 5.0f, nullptr, Target, nullptr,
			Melee, FVector::ForwardVector, PushbackSettings);
		TestTrue(TEXT("The confirmed damage path accepts the hit and subtracts health"),
			Accepted > 0.0f && ReadNumber(Melee, TEXT("HealthCurrent")) < Health);
	};
	HitMelee();
	Tick(TestWorld, 1);
	TestTrue(TEXT("DamageState preempts the interruption wait"), IsActiveTask(Brain, DamageTask));
	TestTrue(TEXT("The hit during Lightning plays a root-motion reaction"), Melee->GetRootMotionAnimMontageInstance() != nullptr);
	TestTrue(TEXT("The hit during Lightning starts procedural pushback"), Pushback->IsPushbackActive());
	const FVector BeforeReaction = Melee->GetActorLocation();
	Tick(TestWorld, 2);
	TestTrue(TEXT("The damage reaction actually displaces the capsule"), FVector::DistSquared2D(BeforeReaction, Melee->GetActorLocation()) > 1.0);
	HitMelee();
	TestTrue(TEXT("The second confirmed hit refreshes authored pushback"), Pushback->IsPushbackActive());
	TestTrue(TEXT("Repeated damage preserves Lightning and blocks melee attacks"), Blackboard->GetValueAsBool(InterruptedKey)
		&& !CallBoolFunction(Melee, TEXT("CanAttackPlayer")));
	for (int32 Frame = 0; Frame < 35 && !IsActiveTask(Brain, WaitTask); ++Frame)
	{
		Tick(TestWorld, 1);
	}
	TestTrue(TEXT("A completed reaction returns to the interruption wait"), IsActiveTask(Brain, WaitTask));
	TestFalse(TEXT("Damage completion cannot restore group eligibility during Lightning"), CallBoolFunction(Melee, TEXT("CanBeGroupAttacker")));

	MeleeStatus->ClearStatusEffect(LightningType);
	Tick(TestWorld);
	TestFalse(TEXT("Recovery clears the Blackboard key"), Blackboard->GetValueAsBool(InterruptedKey));
	TestTrue(TEXT("Recovery leaves the interruption branch"), !Brain || !Brain->GetActiveNode() || !Brain->GetActiveNode()->IsA<UBTTask_WaitForCombatRestriction>());
	TestFalse(TEXT("Recovery does not give back the old slot"), ArrayContains(Manager, MeleeSlot, Melee));

	HitMelee();
	Tick(TestWorld, 1);
	HitMelee();
	TestTrue(TEXT("The triggering hit has already started its authored pushback"), Pushback->IsPushbackActive());
	TestTrue(TEXT("The triggering hit activates Lightning"), MeleeStatus->AddStatusBuildup(Definition, 100, Target, nullptr));
	TestTrue(TEXT("The triggering hit retains pushback after Lightning activates"), Pushback->IsPushbackActive());
	const FVector BeforeTriggerPushback = Melee->GetActorLocation();
	Tick(TestWorld, 2);
	TestTrue(TEXT("Accepted pushback still moves the target after activation"),
		FVector::DistSquared2D(BeforeTriggerPushback, Melee->GetActorLocation()) > 1.0);
	for (int32 Frame = 0; Frame < 35 && !IsActiveTask(Brain, WaitTask); ++Frame)
	{
		Tick(TestWorld, 1);
	}

	const FString ScenePath = TEXT("/Game/CodeX_Restruct/Shared/Animation/ContextualAnimations/PlayerInitiate/ExecutionLots/CAS_PlayerExecute_SNS_01.CAS_PlayerExecute_SNS_01");
	auto* SceneAsset = LoadObject<UContextualAnimSceneAsset>(nullptr, *ScenePath);
	auto* EnemyScene = Melee->FindComponentByClass<UContextualAnimSceneActorComponent>();
	ACharacter* ExecutionPartner = World->SpawnActor<ACharacter>(Melee->GetActorLocation() + FVector(-160, 0, 0), FRotator::ZeroRotator);
	auto* PartnerScene = NewObject<UContextualAnimSceneActorComponent>(ExecutionPartner);
	PartnerScene->RegisterComponent();
	if (!TestTrue(TEXT("The authored execution scene and enemy scene actor are present"), EnemyScene && SceneAsset && SceneAsset->HasValidData()))
	{
		return false;
	}
	// Use the authored roles directly so this standalone fixture does not depend on placement
	// inside the level's execution selection volumes.
	FContextualAnimSceneBindings Bindings(*SceneAsset);
	if (!TestTrue(TEXT("The authored execution scene binds an attacking actor and the real enemy"),
		Bindings.BindActorToRole(*ExecutionPartner, TEXT("Attacker"), 0, 0)
		&& Bindings.BindActorToRole(*Melee, TEXT("Defender"), 0, 0)))
	{
		return false;
	}
	UClass* PlayerClass = LoadClass<ACharacter>(nullptr,
		TEXT("/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase.BP_PlayerBase_C"));
	const ACharacter* PlayerDefaults = PlayerClass ? Cast<ACharacter>(PlayerClass->GetDefaultObject()) : nullptr;
	USkeletalMesh* PartnerMesh = PlayerDefaults ? PlayerDefaults->GetMesh()->GetSkeletalMeshAsset() : nullptr;
	if (!TestNotNull(TEXT("The paired attacker uses the authored player mesh"), PartnerMesh))
	{
		return false;
	}
	ExecutionPartner->GetMesh()->SetSkeletalMeshAsset(PartnerMesh);
	ExecutionPartner->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
	ExecutionPartner->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	ExecutionPartner->GetMesh()->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	ExecutionPartner->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	UClass* WarpClass = LoadClass<UActorComponent>(nullptr, TEXT("/Script/MotionWarping.MotionWarpingComponent"));
	if (!TestNotNull(TEXT("The existing motion warping component class"), WarpClass))
	{
		return false;
	}
	NewObject<UActorComponent>(ExecutionPartner, WarpClass)->RegisterComponent();

	const auto StartExecution = [&]()
	{
		SetBoolProperty(Melee, TEXT("bCanBeExecuted"), true);
		TestTrue(TEXT("The enemy's authored execution eligibility allows active Lightning"),
			CallBoolFunction(Melee, TEXT("BPI_CanBeExecuted")));
		TestTrue(TEXT("The enemy execution-start interface is called"),
			CallExecutionEvent(Melee, TEXT("BPI_OnExecutionStarted"), ExecutionPartner));
		return EnemyScene->StartContextualAnimScene(Bindings);
	};
	if (!TestTrue(TEXT("An eligible restricted enemy starts a paired execution"), StartExecution()))
	{
		return false;
	}
	Tick(TestWorld, 1);
	TestTrue(TEXT("Execution preempts the wait with Lightning still active"),
		IsActiveTask(Brain, ExecutionTask) && Blackboard->GetValueAsBool(InterruptedKey));
	TestTrue(TEXT("Both contextual participants play paired montages"), EnemyScene->IsInActiveScene() && PartnerScene->IsInActiveScene()
		&& Melee->GetCurrentMontage() && ExecutionPartner->GetCurrentMontage());
	TestTrue(TEXT("The existing motion warping hooks stay bound on both participants"),
		Melee->GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.IsBound()
		&& ExecutionPartner->GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.IsBound());
	EnemyScene->EarlyOutContextualAnimScene(true);
	Tick(TestWorld, 2);
	TestTrue(TEXT("Leaving a canceled scene returns to the wait while Lightning is active"),
		!EnemyScene->IsInActiveScene() && !GetBoolProperty(Melee, TEXT("bIsBeingExecuted"))
		&& IsActiveTask(Brain, WaitTask));

	MeleeStatus->ClearStatusEffect(LightningType);
	UCombatStatusEffectDefinition* ShortLightning = DuplicateObject(Definition, GetTransientPackage());
	ShortLightning->Duration = 0.3f;
	TestTrue(TEXT("Apply a short-duration fixture to exercise expiration inside the scene"),
		MeleeStatus->AddStatusBuildup(ShortLightning, 100, Target, nullptr));
	const FVector EnemyStartLocation = Melee->GetActorLocation();
	const FVector PartnerStartLocation = ExecutionPartner->GetActorLocation();
	if (!TestTrue(TEXT("The authored scene can restart after a canceled execution"), StartExecution()))
	{
		return false;
	}
	Tick(TestWorld, 1);
	const uint8 SceneMovementMode = Melee->GetCharacterMovement()->MovementMode;
	const FContextualAnimSceneBinding* Defender = EnemyScene->GetBindings().FindBindingByActor(Melee);
	const float DefenderStartTime = Defender ? Defender->GetAnimMontageTime() : -1.0f;
	Tick(TestWorld, 7);
	TestFalse(TEXT("Lightning naturally expires during execution"), Blackboard->GetValueAsBool(InterruptedKey));
	TestTrue(TEXT("Expiration preserves the paired scene and Execution priority"),
		IsActiveTask(Brain, ExecutionTask) && EnemyScene->IsInActiveScene() && PartnerScene->IsInActiveScene());
	TestEqual(TEXT("The scene retains its movement mode"), Melee->GetCharacterMovement()->MovementMode, SceneMovementMode);
	Defender = EnemyScene->GetBindings().FindBindingByActor(Melee);
	TestTrue(TEXT("The defender's paired animation advances across Lightning expiration"),
		Defender && DefenderStartTime >= 0.0f && Defender->GetAnimMontageTime() > DefenderStartTime);
	TestTrue(TEXT("The paired scene moves a participant horizontally through root motion"),
		FVector::DistSquared2D(EnemyStartLocation, Melee->GetActorLocation()) > 1.0
		|| FVector::DistSquared2D(PartnerStartLocation, ExecutionPartner->GetActorLocation()) > 1.0);
	EnemyScene->EarlyOutContextualAnimScene(true);

	MeleeStatus->ClearStatusEffect(LightningType);
	TestTrue(TEXT("Lightning can be applied again for the lethal hit scenario"), MeleeStatus->AddStatusBuildup(Definition, 100, Target, nullptr));
	Tick(TestWorld);
	TestTrue(TEXT("The enemy is waiting under Lightning before the lethal hit"), IsActiveTask(Brain, WaitTask));
	const FString DeathPath = Root + TEXT("Animations/CombatStyle/Katana/Hurt/AS_Hit_Death_Seq_Montage.AS_Hit_Death_Seq_Montage");
	auto* DeathMontage = LoadObject<UAnimMontage>(nullptr, *DeathPath);
	TestTrue(TEXT("The fixture has an authored death montage"), DeathMontage && SetObjectProperty(Melee, TEXT("ResolvedDeathMontage"), DeathMontage));
	UGameplayStatics::ApplyDamage(Melee, 100000.0f, nullptr, Target, nullptr);
	Tick(TestWorld, 1);
	TestTrue(TEXT("Death executes through the authored behavior tree during Lightning"), IsActiveTask(Brain, TEXT("BTTask_Dead_C")));
	TestTrue(TEXT("The normal death montage plays"), Melee->GetCurrentMontage() == DeathMontage && DeathMontage);
	TestFalse(TEXT("Death releases Lightning's restriction ownership"), UCombatActionRestrictionComponent::IsActorCombatRestricted(Melee));
	TestFalse(TEXT("Death cannot restore attacker eligibility"), CallBoolFunction(Melee, TEXT("CanBeGroupAttacker")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
