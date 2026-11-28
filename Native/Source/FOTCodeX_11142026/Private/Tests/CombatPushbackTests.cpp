#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/Motion/CombatPushbackComponent.h"

#include "Combat/CombatActionRestrictionComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPushbackCurveTest, "CodeX.Combat.Pushback.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatPushbackCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Ease out starts at zero"), UCombatPushbackComponent::EvaluateFraction(0.0f, nullptr), 0.0f);
	TestEqual(TEXT("Ease out ends at one"), UCombatPushbackComponent::EvaluateFraction(1.0f, nullptr), 1.0f);
	TestEqual(TEXT("Cubic midpoint"), UCombatPushbackComponent::EvaluateFraction(0.5f, nullptr), 0.875f);
	TestEqual(TEXT("Alpha clamps"), UCombatPushbackComponent::EvaluateFraction(1.25f, nullptr), 1.0f);

	UCurveFloat* Curve = NewObject<UCurveFloat>();
	const FKeyHandle Start = Curve->FloatCurve.AddKey(0.0f, 0.0f);
	const FKeyHandle End = Curve->FloatCurve.AddKey(1.0f, 1.0f);
	TestTrue(TEXT("Linear cumulative curve is valid"), UCombatPushbackComponent::IsUsableCurve(Curve));
	TestEqual(TEXT("Custom curve is evaluated"), UCombatPushbackComponent::EvaluateFraction(0.5f, Curve), 0.5f);

	Curve->FloatCurve.GetKey(Start).InterpMode = RCIM_Cubic;
	TestTrue(TEXT("Smooth cumulative curve is valid"), UCombatPushbackComponent::IsUsableCurve(Curve));
	Curve->FloatCurve.GetKey(Start).LeaveTangent = 8.0f;
	TestFalse(TEXT("Cubic overshoot between keys is rejected"), UCombatPushbackComponent::IsUsableCurve(Curve));
	Curve->FloatCurve.GetKey(Start).InterpMode = RCIM_Linear;
	Curve->FloatCurve.GetKey(End).Value = 0.8f;
	TestFalse(TEXT("Wrong endpoint is rejected"), UCombatPushbackComponent::IsUsableCurve(Curve));
	Curve->FloatCurve.GetKey(End).Value = 1.0f;
	Curve->FloatCurve.AddKey(0.25f, 0.8f);
	Curve->FloatCurve.AddKey(0.75f, 0.6f);
	TestFalse(TEXT("Reversing curve is rejected"), UCombatPushbackComponent::IsUsableCurve(Curve));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPushbackLifecycleTest, "CodeX.Combat.Pushback.DisplacementAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatPushbackLifecycleTest::RunTest(const FString& Parameters)
{
	UCombatPushbackComponent* Component = NewObject<UCombatPushbackComponent>();
	FCombatPushbackSettings Settings;
	Settings.bEnabled = true;
	Settings.Distance = 30.0f;
	const FVector Direction(4.0, 0.0, 10.0);
	const FVector Expected(30.0, 0.0, 0.0);

	for (const int32 FrameRate : { 30, 60, 120 })
	{
		Component->CancelPushback();
		TestTrue(TEXT("Displacement initializes"), Component->InitializeDisplacement(Direction, Settings));
		FVector Total = FVector::ZeroVector;
		for (int32 Frame = 0; Frame < FrameRate; ++Frame)
		{
			Total += Component->AdvanceDisplacement(1.0f / FrameRate);
		}
		TestTrue(FString::Printf(TEXT("30 cm horizontal at %d FPS"), FrameRate), Total.Equals(Expected, 0.0001));
	}

	Component->CancelPushback();
	Component->InitializeDisplacement(Direction, Settings);
	FVector IrregularTotal = FVector::ZeroVector;
	for (const float Interval : { 0.001f, 0.016f, 0.045f, 0.0f, 0.005f, 0.083f, 0.020f })
	{
		IrregularTotal += Component->AdvanceDisplacement(Interval);
	}
	TestTrue(TEXT("Irregular intervals and final overshoot integrate to 30 cm"), IrregularTotal.Equals(Expected, 0.0001));
	TestTrue(TEXT("Completed displacement does not repeat"), Component->AdvanceDisplacement(0.1f).IsZero());

	Component->CancelPushback();
	Component->InitializeDisplacement(Direction, Settings);
	const FVector FirstDelta = Component->AdvanceDisplacement(0.01f);
	TestTrue(TEXT("First advanced pose contributes immediately"), FirstDelta.X > 0.0);
	TestTrue(TEXT("Paused reaction contributes zero"), Component->AdvanceDisplacement(0.0f).IsZero());
	TestTrue(TEXT("Resumed reaction completes the remaining displacement"),
		(FirstDelta + Component->AdvanceDisplacement(0.14f)).Equals(Expected, 0.0001));

	Component->StopActivePushback(TEXT("replaced"));
	Settings.Distance = 50.0f;
	TestTrue(TEXT("Replacement initializes independently"), Component->InitializeDisplacement(-Direction, Settings));
	TestTrue(TEXT("Replacement owns its distance and direction"),
		Component->AdvanceDisplacement(Settings.Duration).Equals(FVector(-50.0, 0.0, 0.0), 0.0001));
	Component->PendingSettings = Settings;
	Component->PendingDirection = Direction;
	Component->CancelPushback();
	TestFalse(TEXT("Cancellation clears active state"), Component->IsPushbackActive());
	TestFalse(TEXT("Cancellation clears pending hit"), Component->PendingSettings.bEnabled);
	TestTrue(TEXT("Cancellation discards outstanding translation"), Component->AdvanceDisplacement(1.0f).IsZero());

	Settings.Distance = 0.0f;
	TestFalse(TEXT("Zero distance is rejected"), Component->InitializeDisplacement(Direction, Settings));
	Settings.Distance = 30.0f;
	Settings.Duration = 0.0f;
	TestFalse(TEXT("Zero duration is rejected"), Component->InitializeDisplacement(Direction, Settings));
	Settings.Duration = -0.15f;
	TestFalse(TEXT("Negative duration is rejected"), Component->InitializeDisplacement(Direction, Settings));
	Settings.Duration = 0.15f;
	TestFalse(TEXT("Vertical direction is rejected"), Component->InitializeDisplacement(FVector::UpVector, Settings));
	TestFalse(TEXT("Coincident hit positions are rejected"), Component->InitializeDisplacement(FVector::ZeroVector, Settings));
	Component->PushbackMultiplier = 0.0f;
	TestFalse(TEXT("Full resistance is rejected"), Component->InitializeDisplacement(Direction, Settings));
	Component->PushbackMultiplier = 0.5f;
	TestTrue(TEXT("Partial resistance initializes"), Component->InitializeDisplacement(Direction, Settings));
	TestTrue(TEXT("Partial resistance halves displacement"),
		Component->AdvanceDisplacement(Settings.Duration).Equals(Expected / 2.0, 0.0001));
	Component->CancelPushback();

	UCurveFloat* InvalidCurve = NewObject<UCurveFloat>();
	InvalidCurve->FloatCurve.AddKey(0.0f, 0.5f);
	InvalidCurve->FloatCurve.AddKey(1.0f, 1.0f);
	Settings.DisplacementCurve = InvalidCurve;
	Component->PushbackMultiplier = 1.0f;
	Component->InitializeDisplacement(Direction, Settings);
	TestTrue(TEXT("An invalid custom curve uses cubic ease out"),
		Component->AdvanceDisplacement(Settings.Duration / 2.0f).Equals(Expected * 0.875, 0.0001));
	Component->CancelPushback();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatPushbackMovementTest, "CodeX.Combat.Pushback.CharacterMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatPushbackMovementTest::RunTest(const FString& Parameters)
{
	// Use the sword soldier's authored normal reaction in a transient game world.
	UAnimMontage* Reaction = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/CodeX_Restruct/Frameworks/EnemyFramework/Animations/CombatStyle/Katana/Hurt/Frank_Damage_Strong_F1_Montage.Frank_Damage_Strong_F1_Montage"));
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/CodeX_Restruct/Frameworks/EnemyFramework/Enemies/Enemy_UndeadSoldier/Character/Mesh/SK_Ancient_Undead_Warrior.SK_Ancient_Undead_Warrior"));
	UClass* MotionWarpingClass = LoadClass<UActorComponent>(nullptr, TEXT("/Script/MotionWarping.MotionWarpingComponent"));
	if (!TestTrue(TEXT("A root-motion reaction, mesh and Motion Warping class are available"),
		Reaction && Reaction->HasRootMotion() && Mesh && MotionWarpingClass))
	{
		return false;
	}

	FTestWorldWrapper TestWorld;
	if (!TestTrue(TEXT("Transient game world starts"), TestWorld.CreateTestWorld(EWorldType::Game) && TestWorld.BeginPlayInTestWorld()))
	{
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	TArray<ACharacter*> Characters;
	TArray<UCombatPushbackComponent*> Pushbacks;
	TArray<UCombatActionRestrictionComponent*> Restrictions;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ACharacter* Character = World->SpawnActor<ACharacter>(FVector(0.0, Index * 1000.0, 100.0), FRotator::ZeroRotator);
		Character->GetMesh()->SetSkeletalMeshAsset(Mesh);
		Character->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Character->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
		UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
		if (!TestNotNull(TEXT("Character animation instance"), Anim))
		{
			return false;
		}
		Anim->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
		Character->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		UActorComponent* MotionWarping = NewObject<UActorComponent>(Character, MotionWarpingClass);
		MotionWarping->RegisterComponent();
		TestTrue(TEXT("Motion Warping owns the pre-conversion hook"),
			Character->GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.IsBound());
		UCombatPushbackComponent* Pushback = NewObject<UCombatPushbackComponent>(Character);
		Pushback->RegisterComponent();
		UCombatActionRestrictionComponent* Restriction = NewObject<UCombatActionRestrictionComponent>(Character);
		Restriction->RegisterComponent();
		Characters.Add(Character);
		Pushbacks.Add(Pushback);
		Restrictions.Add(Restriction);
	}

	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Box->InitBoxExtent(FVector(20.0, 180.0, 180.0));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Wall->SetRootComponent(Box);
	const double WallGap = 5.0;
	Box->SetWorldLocation(FVector(Characters[3]->GetCapsuleComponent()->GetScaledCapsuleRadius() + 20.0 + WallGap, 3000.0, 100.0));
	Box->RegisterComponent();

	FCombatPushbackSettings Settings;
	Settings.bEnabled = true;
	Settings.Distance = 30.0f;
	AAIController* Controller = World->SpawnActor<AAIController>();
	Controller->Possess(Characters[1]);
	Characters[1]->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	const FGuid AlreadyRestricted = Restrictions[2]->AcquireRestriction({true, false});
	TestTrue(TEXT("Movement is restricted before the second reaction"), AlreadyRestricted.IsValid());
	for (int32 Index = 0; Index < Characters.Num(); ++Index)
	{
		if (!TestTrue(TEXT("Authored reaction plays"), Characters[Index]->PlayAnimMontage(Reaction) > 0.0f))
		{
			return false;
		}
		if (Index != 0 && !TestTrue(TEXT("Pushback request binds to the reaction"),
			Pushbacks[Index]->RequestPushback(FVector::ForwardVector, Settings)))
		{
			return false;
		}
	}
	const FGuid DuringReaction = Restrictions[1]->AcquireRestriction({true, false});
	TestTrue(TEXT("Activation leaves the accepted pushback bound"), DuringReaction.IsValid() && Pushbacks[1]->IsPushbackActive());
	TestEqual(TEXT("Activation preserves montage root motion extraction"),
		Characters[1]->GetMesh()->GetAnimInstance()->RootMotionMode.GetValue(), ERootMotionMode::RootMotionFromMontagesOnly);
	Characters[1]->AddMovementInput(FVector::RightVector, 1.0f);
	TestTrue(TEXT("Voluntary input is ignored during a live root-motion reaction"),
		Characters[1]->ConsumeMovementInputVector().IsNearlyZero());

	UAnimInstance* SlowAnimation = Characters[2]->GetMesh()->GetAnimInstance();
	SlowAnimation->Montage_SetPlayRate(Reaction, 0.01f);
	TestWorld.TickTestWorld(0.01f);
	TestTrue(TEXT("The first extracted pose advances pushback"), Pushbacks[1]->Elapsed > 0.0f);
	TestTrue(TEXT("Reduced montage rate reduces elapsed pushback time"),
		Pushbacks[2]->Elapsed > 0.0f && Pushbacks[2]->Elapsed < Pushbacks[1]->Elapsed * 0.03f);
	SlowAnimation->Montage_Pause(Reaction);
	const float PausedTime = Pushbacks[2]->Elapsed;

	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		TestWorld.TickTestWorld(0.01f);
	}

	const FVector Baseline = Characters[0]->GetActorLocation() - FVector(0.0, 0.0, 100.0);
	const FVector Pushed = Characters[1]->GetActorLocation() - FVector(0.0, 1000.0, 100.0);
	TestTrue(TEXT("The authored reaction moves the baseline"), !Baseline.IsNearlyZero());
	TestEqual(TEXT("Movement adds 30 cm horizontally to authored root motion"), Pushed - Baseline, FVector(30.0, 0.0, 0.0), 0.25);
	TestTrue(TEXT("Authored rotation is preserved"), Characters[1]->GetActorQuat().Equals(Characters[0]->GetActorQuat(), 0.001));
	TestTrue(TEXT("The wall limits movement"), Characters[3]->GetActorLocation().X <= WallGap + 0.5);
	TestFalse(TEXT("The wall does not extend pushback"), Pushbacks[3]->IsPushbackActive());
	TestFalse(TEXT("Unobstructed pushback completes"), Pushbacks[1]->IsPushbackActive());
	TestTrue(TEXT("Paused pushback consumes no montage time"), Pushbacks[2]->IsPushbackActive() && Pushbacks[2]->Elapsed == PausedTime);
	TestTrue(TEXT("Release preserves an ongoing reaction and pushback"),
		Restrictions[2]->ReleaseRestriction(AlreadyRestricted) && Pushbacks[2]->IsPushbackActive());
	TestTrue(TEXT("Completion leaves Motion Warping bound and releases PB1's hook"),
		Characters[1]->GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.IsBound()
		&& !Characters[1]->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.IsBound());

	SlowAnimation->Montage_Resume(Reaction);
	SlowAnimation->Montage_SetPlayRate(Reaction, 1.0f);
	TestWorld.TickTestWorld(0.01f);
	TestTrue(TEXT("Restoring the rate resumes displacement"), Pushbacks[2]->Elapsed > PausedTime);
	SlowAnimation->Montage_Stop(0.05f, Reaction);
	TestWorld.TickTestWorld(0.01f);
	TestFalse(TEXT("Montage interruption cancels remaining displacement"), Pushbacks[2]->IsPushbackActive());

	UCharacterMovementComponent* BaselineMovement = Characters[0]->GetCharacterMovement();
	BaselineMovement->ProcessRootMotionPostConvertToWorld.BindLambda(
		[](const FTransform& RootMotion, UCharacterMovementComponent*, float) { return RootMotion; });
	TestFalse(TEXT("A competing post-conversion delegate rejects pushback"),
		Pushbacks[0]->RequestPushback(FVector::ForwardVector, Settings));
	TestTrue(TEXT("The existing post-conversion owner stays bound"), BaselineMovement->ProcessRootMotionPostConvertToWorld.IsBound());
	BaselineMovement->ProcessRootMotionPostConvertToWorld.Unbind();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
