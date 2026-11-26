#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatPushbackTypes.generated.h"

class UAnimMontage;
class UCombatPushbackComponent;
class UCurveFloat;
class UDamageType;

USTRUCT(BlueprintType)
struct FOTCODEX_11142026_API FCombatPushbackSettings
{
	GENERATED_BODY()

	/** Allow accepted melee reactions to add procedural displacement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pushback")
	bool bEnabled = false;

	/** Total horizontal displacement before receiver resistance and collision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pushback", meta = (ClampMin = "0", Units = "cm"))
	float Distance = 20.0f;

	/** Time in the receiving reaction montage's timeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pushback", meta = (ClampMin = "0.001", Units = "s"))
	float Duration = 0.15f;

	/** Monotonic cumulative displacement from (0, 0) to (1, 1). Empty or invalid uses cubic ease out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pushback")
	TObjectPtr<UCurveFloat> DisplacementCurve = nullptr;
};

UENUM(BlueprintType)
enum class ECombatPushbackOverrideMode : uint8
{
	Inherit,
	Override,
	Disabled
};

/** Handoff between the existing melee damage call and the receiving actor's reaction. */
UCLASS()
class FOTCODEX_11142026_API UCombatPushbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Combat|Pushback")
	static FCombatPushbackSettings ResolvePushbackSettings(ECombatPushbackOverrideMode Mode,
		const FCombatPushbackSettings& InheritedSettings, float Distance, float Duration, UCurveFloat* DisplacementCurve);

	/** Apply the original damage and expose this contact's settings only during synchronous damage reception. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Pushback")
	static float ApplyDamageWithPushback(AActor* DamagedActor, float BaseDamage, AController* EventInstigator,
		AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, AActor* MeleeWindowTarget,
		FVector Direction, const FCombatPushbackSettings& Settings);

	/** Call on the receiver's accepted damage branch before entering or refreshing its damage state. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Pushback")
	static void CaptureAcceptedPushback(UCombatPushbackComponent* Pushback, AActor* DamageCauser);

	/** Call immediately after playing the accepted hit reaction. Consumes the receiver's captured contact. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Pushback")
	static void StartAcceptedPushback(UCombatPushbackComponent* Pushback, UAnimMontage* ReactionMontage, float ReactionPlaybackLength);
};
