#pragma once

#include "Animation/AnimInstance.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "CombatActionRestrictionComponent.generated.h"

class AController;
class APawn;
class UBlackboardComponent;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FCombatActionRestrictions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Restrictions")
	bool bRestrictMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Restrictions")
	bool bRestrictCombat = false;

	bool IsEmpty() const { return !bRestrictMovement && !bRestrictCombat; }
	bool operator==(const FCombatActionRestrictions& Other) const = default;
};

UINTERFACE(BlueprintType)
class FOTCODEX_11142026_API UCombatActionRestrictionTarget : public UInterface
{
	GENERATED_BODY()
};

class FOTCODEX_11142026_API ICombatActionRestrictionTarget
{
	GENERATED_BODY()

public:
	/** The owner cancels its current offensive action and releases any group attack slot here. */
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Restrictions")
	void OnCombatRestrictionChanged(bool bRestricted);

	/** The owner can reevaluate passive locomotion when the movement lock ends. */
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Restrictions")
	void OnMovementRestrictionChanged(bool bRestricted);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatActionRestrictionsChanged, bool, bMovementRestricted, bool, bCombatRestricted);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class FOTCODEX_11142026_API UCombatActionRestrictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatActionRestrictionComponent();

	/** Returns an invalid handle if the owner cannot enforce the requested restriction. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Restrictions")
	FGuid AcquireRestriction(FCombatActionRestrictions Restrictions);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Restrictions")
	bool ReleaseRestriction(FGuid Handle);

	UFUNCTION(BlueprintPure, Category="Combat|Restrictions")
	bool HasMovementRestriction() const { return Aggregate.bRestrictMovement; }

	UFUNCTION(BlueprintPure, Category="Combat|Restrictions")
	bool HasCombatRestriction() const { return Aggregate.bRestrictCombat; }

	UFUNCTION(BlueprintPure, Category="Combat|Restrictions", meta=(DefaultToSelf="Actor"))
	static bool IsActorMovementRestricted(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Combat|Restrictions", meta=(DefaultToSelf="Actor"))
	static bool IsActorCombatRestricted(const AActor* Actor);

	/** Guards queued offensive notifies on restricted enemies and notifies from an aborted montage. */
	UFUNCTION(BlueprintPure, Category="Combat|Restrictions")
	static bool CanRunOffensiveNotify(const USkeletalMeshComponent* Mesh, const UAnimSequenceBase* Animation);

	UFUNCTION(BlueprintCallable, Category="Combat|Restrictions")
	static void CancelComponentLatentActions(UActorComponent* Component);

	UFUNCTION(BlueprintCallable, Category="Combat|Restrictions")
	static void StopOffensiveMontage(ACharacter* Character, UAnimMontage* Montage, float BlendOutTime = 0.15f);

	static void SynchronizeBlackboard(APawn* Pawn, UBlackboardComponent* Blackboard = nullptr);

	UFUNCTION(BlueprintCallable, Category="Combat|Restrictions", meta=(DefaultToSelf="Enemy"))
	static void LogAttackerEligibility(const AActor* Enemy, bool bEligible);

	UFUNCTION(BlueprintCallable, Category="Combat|Restrictions")
	static void LogAttackerSlotReleased(const AActor* Enemy);

	UPROPERTY(BlueprintAssignable, Category="Combat|Restrictions")
	FOnCombatActionRestrictionsChanged OnRestrictionChanged;

	bool CanAcquireRestriction(const FCombatActionRestrictions& Restrictions) const;
	bool AcquireRestrictionWithHandle(FGuid Handle, const FCombatActionRestrictions& Restrictions);
	void ShutDown();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void UpdateAggregate();
	void ApplyMovementRestriction();
	void RemoveMovementRestriction();
	void SetRestrictedController(AController* Controller);

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	TMap<FGuid, FCombatActionRestrictions> Sources;
	FCombatActionRestrictions Aggregate;
	FCombatActionRestrictions Published;
	TWeakObjectPtr<AController> RestrictedController;
	TWeakObjectPtr<UAnimInstance> RestrictedAnimation;
	TEnumAsByte<ERootMotionMode::Type> PreviousRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
	bool bPublishing = false;
	bool bShutDown = false;
};
