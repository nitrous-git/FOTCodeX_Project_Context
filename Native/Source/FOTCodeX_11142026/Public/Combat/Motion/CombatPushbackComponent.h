#pragma once

#include "Components/ActorComponent.h"
#include "Combat/Motion/CombatPushbackTypes.h"
#include "CombatPushbackComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UCharacterMovementComponent;
struct FAnimMontageInstance;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class FOTCODEX_11142026_API UCombatPushbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatPushbackComponent();

	/** Scales the requested distance. Zero grants full resistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pushback", meta = (ClampMin = "0"))
	float PushbackMultiplier = 1.0f;

	/** Start or replace pushback on the character's current root-motion reaction montage. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Pushback")
	bool RequestPushback(FVector Direction, const FCombatPushbackSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Combat|Pushback")
	void CancelPushback();

	UFUNCTION(BlueprintPure, Category = "Combat|Pushback")
	bool IsPushbackActive() const { return ActiveDuration > 0.0f && DeferredStopReason == nullptr; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class UCombatPushbackLibrary;
	friend class FCombatPushbackCurveTest;
	friend class FCombatPushbackLifecycleTest;
	friend class FCombatPushbackMovementTest;

	static bool IsUsableCurve(const UCurveFloat* Curve);
	static float EvaluateFraction(float Alpha, const UCurveFloat* Curve);

	bool InitializeDisplacement(FVector Direction, const FCombatPushbackSettings& Settings);
	FVector AdvanceDisplacement(float MontageSeconds);
	void StopActivePushback(const TCHAR* Reason);
	const FAnimMontageInstance* GetTrackedReaction() const;
	const TCHAR* GetMovementRejection(const ACharacter* Character, const UCharacterMovementComponent* Movement) const;
	FTransform AddWorldSpacePushback(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	UFUNCTION()
	void OnReactionBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	FCombatPushbackSettings PendingSettings;

	FVector PendingDirection = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> ActiveCurve;

	TWeakObjectPtr<UCharacterMovementComponent> BoundMovement;
	TWeakObjectPtr<UAnimInstance> ReactionAnimInstance;
	TWeakObjectPtr<UAnimMontage> ReactionMontage;
	const TCHAR* DeferredStopReason = nullptr;
	int32 ReactionInstanceId = INDEX_NONE;
	float LastMontagePosition = 0.0f;
	FVector PushbackDirection = FVector::ZeroVector;
	float ActiveDuration = 0.0f;
	float EffectiveDistance = 0.0f;
	float Elapsed = 0.0f;
	float RequestedDistance = 0.0f;
};
