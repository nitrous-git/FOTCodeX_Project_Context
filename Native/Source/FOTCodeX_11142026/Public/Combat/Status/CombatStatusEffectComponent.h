#pragma once

#include "Components/ActorComponent.h"
#include "Combat/Motion/CombatPushbackTypes.h"
#include "Combat/Status/CombatStatusEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimerManager.h"
#include "UObject/Interface.h"
#include "CombatStatusEffectComponent.generated.h"

class UNiagaraComponent;

USTRUCT(BlueprintType)
struct FCombatStatusTargetState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Status Effects")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category="Status Effects")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category="Status Effects")
	bool bProtected = false;
};

UINTERFACE(BlueprintType)
class FOTCODEX_11142026_API UCombatStatusTarget : public UInterface
{
	GENERATED_BODY()
};

/** Reads the receiver's existing health and protection state. */
class FOTCODEX_11142026_API ICombatStatusTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Combat|Status Effects")
	FCombatStatusTargetState GetCombatStatusTargetState() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatStatusBuildupChanged, FName, StatusType, float, Buildup, float, Threshold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStatusApplied, FName, StatusType, AActor*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStatusExpired, FName, StatusType);

USTRUCT()
struct FCombatStatusRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UCombatStatusEffectDefinition> Definition;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> PersistentVFX;

	TWeakObjectPtr<AActor> Source;
	TWeakObjectPtr<AController> Instigator;
	FTimerHandle Timer;
	uint64 Generation = 0;
	float Buildup = 0.0f;
	double LastDecayTime = 0.0;
	double ActiveEnd = 0.0;
	double ImmunityEnd = 0.0;
	double ActivationTime = 0.0;
	int32 NextTick = 0;
	bool bActive = false;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class FOTCODEX_11142026_API UCombatStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatStatusEffectComponent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Status Effects")
	bool AddStatusBuildup(const UCombatStatusEffectDefinition* Definition, float Amount, AActor* Source, AController* Instigator);

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	float GetStatusBuildup(FName StatusType) const;

	/** Returns a fraction from 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	float GetStatusBuildupPercent(FName StatusType) const;

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	bool IsStatusActive(FName StatusType) const;

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	bool IsStatusImmune(FName StatusType) const;

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	float GetStatusRemainingTime(FName StatusType) const;

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	float GetStatusImmunityRemainingTime(FName StatusType) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Status Effects")
	void ClearStatusEffect(FName StatusType);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Status Effects")
	void ClearAllStatusEffects();

	/** Call when the owner's authoritative death sequence begins. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Status Effects")
	void ClearStatusEffectsOnDeath();

	UPROPERTY(BlueprintAssignable, Category="Combat|Status Effects")
	FOnCombatStatusBuildupChanged OnStatusBuildupChanged;

	UPROPERTY(BlueprintAssignable, Category="Combat|Status Effects")
	FOnCombatStatusApplied OnStatusApplied;

	UPROPERTY(BlueprintAssignable, Category="Combat|Status Effects")
	FOnCombatStatusExpired OnStatusExpired;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Status Effects|Resistance")
	bool bImmuneToAllStatuses = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Status Effects|Resistance",
		meta=(GetKeyOptions="GetStatusTypes"))
	TMap<FName, FCombatStatusResistance> Resistances;

	/** Definitions to display on targets before they receive any buildup. */
	UPROPERTY(EditDefaultsOnly, Category="Combat|Status Effects|Debug")
	TArray<TObjectPtr<const UCombatStatusEffectDefinition>> DebugDefinitions;

	FCombatStatusTargetState ReadTarget() const;
	void UpdateDebugTimer();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	TArray<FString> GetStatusTypes() const;

	double Now() const;
	float GetThreshold(const UCombatStatusEffectDefinition& Definition) const;
	bool IsIntrinsicallyImmune(FName StatusType) const;
	void StartEffect(FName Type, FCombatStatusRuntime& State, AActor* Source, AController* Instigator);
	void Advance(FName Type);
	void Schedule(FName Type, FCombatStatusRuntime& State, double Time);
	void Expire(FName Type, FCombatStatusRuntime& State);
	void DealStatusDamage(FName Type, const FCombatStatusRuntime& State, float Percent, bool bBurst);
	void ResetEffects(bool bNotify);
	void DrawDebugStatus();
	UNiagaraComponent* SpawnVFX(const FCombatStatusVFX& VFX, bool bPersistent) const;

	UPROPERTY(Transient)
	TMap<FName, FCombatStatusRuntime> Statuses;

	FTimerHandle DebugTimer;
	uint64 NextGeneration = 0;
	bool bOwnerDead = false;
	bool bEndingPlay = false;
};

UCLASS()
class FOTCODEX_11142026_API UCombatStatusEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Uses the existing weapon damage and pushback path, then checks the receiver's actual health change. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat|Status Effects")
	static float ApplyWeaponDamageWithStatus(AActor* DamagedActor, float BaseDamage, AController* EventInstigator,
		AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, AActor* MeleeWindowTarget,
		FVector Direction, const FCombatPushbackSettings& Settings, const FCombatWeaponStatus& WeaponStatus);

	UFUNCTION(BlueprintPure, Category="Combat|Status Effects")
	static bool IsStatusDamage(const UDamageType* DamageType);

	static float CalculateBuildup(float Base, bool bAffinityMatches, float Multiplier);
};
