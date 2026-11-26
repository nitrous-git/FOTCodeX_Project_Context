#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "CombatStatusEffectTypes.generated.h"

class UCombatStatusEffectDefinition;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ECombatStatusExecution : uint8
{
	Duration,
	PeriodicDamage,
	InstantBurst
};

UENUM(BlueprintType)
enum class ECombatStatusVFXAttachment : uint8
{
	World,
	Mesh,
	SkeletalMeshSampling
};

USTRUCT(BlueprintType)
struct FCombatStatusVFX
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UNiagaraSystem> System;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	ECombatStatusVFXAttachment Attachment = ECombatStatusVFXAttachment::World;

	/** The mesh origin is used when this is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Attachment != ECombatStatusVFXAttachment::World", EditConditionHides))
	FName Socket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FTransform RelativeTransform = FTransform::Identity;

	/** Optional exposed skeletal mesh data interface. Leave empty for a system that samples its attach parent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Attachment == ECombatStatusVFXAttachment::SkeletalMeshSampling", EditConditionHides))
	FName SkeletalMeshUserParameter;
};

USTRUCT(BlueprintType)
struct FCombatStatusResistance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resistance")
	bool bImmune = false;

	/** Zero uses the threshold on the incoming status definition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resistance", meta=(ClampMin="0"))
	float ThresholdOverride = 0.0f;
};

USTRUCT(BlueprintType)
struct FCombatWeaponStatus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status Effects")
	TObjectPtr<const UCombatStatusEffectDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status Effects")
	bool bAffinityMatches = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status Effects")
	float AffinityMultiplier = 1.0f;
};

/** Passed through the existing damage event so the receiver can skip weapon-specific reactions. */
UCLASS(NotBlueprintable)
class FOTCODEX_11142026_API UCombatStatusDamageType : public UDamageType
{
	GENERATED_BODY()
};
