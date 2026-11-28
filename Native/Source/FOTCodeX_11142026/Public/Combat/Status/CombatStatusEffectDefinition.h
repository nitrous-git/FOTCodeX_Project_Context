#pragma once

#include "Engine/DataAsset.h"
#include "Combat/CombatActionRestrictionComponent.h"
#include "Combat/Status/CombatStatusEffectTypes.h"
#include "CombatStatusEffectDefinition.generated.h"

UCLASS(BlueprintType)
class FOTCODEX_11142026_API UCombatStatusEffectDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Stable status identifier. Use the corresponding CodeX Attunement name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(GetOptions="GetStatusTypes"))
	FName StatusType;

	/** Buildup from an accepted hit before the weapon affinity multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(ClampMin="0"))
	float BaseBuildupPerHit = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(ClampMin="0.01"))
	float BuildupThreshold = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(ClampMin="0", Units="s"))
	float DecayDelay = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(ClampMin="0"))
	float DecayPerSecond = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Buildup", meta=(ClampMin="0", Units="s"))
	float ImmunityDuration = 2.0f;

	/** Disabled effects cannot gain buildup or activate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	bool bGameplayEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	ECombatStatusExecution Execution = ECombatStatusExecution::Duration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect",
		meta=(ClampMin="0.01", Units="s", EditCondition="Execution != ECombatStatusExecution::InstantBurst", EditConditionHides))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect",
		meta=(ClampMin="0.01", Units="s", EditCondition="Execution == ECombatStatusExecution::PeriodicDamage", EditConditionHides))
	float DamageInterval = 1.0f;

	/** Percentage points of maximum health per tick. Enter 2 for 2%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect",
		meta=(ClampMin="0", ClampMax="100", EditCondition="Execution == ECombatStatusExecution::PeriodicDamage", EditConditionHides))
	float PeriodicMaxHealthPercent = 0.0f;

	/** Percentage points of maximum health. Enter 30 for 30%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect",
		meta=(ClampMin="0", ClampMax="100", EditCondition="Execution == ECombatStatusExecution::InstantBurst", EditConditionHides))
	float BurstMaxHealthPercent = 0.0f;

	/** Optional voluntary action limits for the active effect. Hit reactions and contextual animations retain control of motion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect",
		meta=(EditCondition="Execution != ECombatStatusExecution::InstantBurst", EditConditionHides))
	FCombatActionRestrictions ActionRestrictions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FCombatStatusVFX ApplicationVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Execution != ECombatStatusExecution::InstantBurst", EditConditionHides))
	FCombatStatusVFX PersistentVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FCombatStatusVFX ExpirationVFX;

	static TConstArrayView<FName> GetKnownStatusTypes();

private:
	UFUNCTION()
	TArray<FString> GetStatusTypes() const;
};
