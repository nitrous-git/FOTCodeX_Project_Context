#pragma once

#include "Combat/Status/CombatStatusEffectComponent.h"
#include "GameFramework/Actor.h"
#include "CombatStatusEffectTestActor.generated.h"

/** Damage receiver used by the native status automation tests. */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class ACombatStatusEffectTestActor : public AActor, public ICombatStatusTarget
{
	GENERATED_BODY()

public:
	ACombatStatusEffectTestActor();
	virtual FCombatStatusTargetState GetCombatStatusTargetState_Implementation() const override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* Causer) override;

	UPROPERTY()
	TObjectPtr<UCombatStatusEffectComponent> Status;

	UPROPERTY()
	TObjectPtr<AActor> LastStatusSource;

	float Health = 200.0f;
	float MaxHealth = 200.0f;
	int32 StatusDamageEvents = 0;
	bool bProtected = false;
	bool bRejectWeaponDamage = false;
};
