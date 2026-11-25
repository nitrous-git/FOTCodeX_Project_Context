#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponSlashTrail.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem;

UENUM(BlueprintType)
enum class E_WeaponTrailType : uint8 { InheritNeutral, None, Niagara, Cascade };

UENUM(BlueprintType)
enum class E_WeaponTrailBlade : uint8 { Primary, Secondary, Both };

USTRUCT(BlueprintType)
struct WEAPONVISUALS_API FWeaponTrailEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail")
    E_WeaponTrailType TrailType = E_WeaponTrailType::InheritNeutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail", meta=(EditCondition="TrailType == E_WeaponTrailType::Niagara", EditConditionHides))
    TObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail", meta=(EditCondition="TrailType == E_WeaponTrailType::Cascade", EditConditionHides))
    TObjectPtr<UParticleSystem> CascadeSystem;

    /** Exposed float bound to ribbon width. None preserves the authored width. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail", meta=(EditCondition="TrailType == E_WeaponTrailType::Niagara", EditConditionHides))
    FName NiagaraWidthParameter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail", meta=(ClampMin="0", EditCondition="TrailType == E_WeaponTrailType::Niagara", EditConditionHides))
    float NiagaraBaseWidth = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trail", meta=(ClampMin="0", EditCondition="TrailType == E_WeaponTrailType::Cascade", EditConditionHides))
    float CascadeWidthScale = 1.f;

    bool HasEffect() const;
};

UCLASS(Abstract, Blueprintable)
class WEAPONVISUALS_API UWeaponSlashTrail : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UWeaponSlashTrail();
    UPROPERTY(EditAnywhere, Category="Trail Source")
    bool bUseRuntimeEffectOverride = false;

    UPROPERTY(EditAnywhere, Category="Trail Source", meta=(EditCondition="bUseRuntimeEffectOverride", EditConditionHides))
    FWeaponTrailEffect RuntimeEffectOverride;

    UPROPERTY(EditAnywhere, Category="Trail Source")
    FWeaponTrailEffect EditorPreviewEffect;

    UPROPERTY(EditAnywhere, Category="Blades")
    E_WeaponTrailBlade BladeSelection = E_WeaponTrailBlade::Primary;

    UPROPERTY(EditAnywhere, Category="Blades", meta=(EditCondition="BladeSelection != E_WeaponTrailBlade::Secondary", EditConditionHides))
    FName PrimaryStartSocket = TEXT("FX_WeaponBase");
    UPROPERTY(EditAnywhere, Category="Blades", meta=(EditCondition="BladeSelection != E_WeaponTrailBlade::Secondary", EditConditionHides))
    FName PrimaryEndSocket = TEXT("FX_WeaponTop");
    UPROPERTY(EditAnywhere, Category="Blades", meta=(EditCondition="BladeSelection != E_WeaponTrailBlade::Primary", EditConditionHides))
    FName SecondaryStartSocket = TEXT("FX_WeaponBase02");
    UPROPERTY(EditAnywhere, Category="Blades", meta=(EditCondition="BladeSelection != E_WeaponTrailBlade::Primary", EditConditionHides))
    FName SecondaryEndSocket = TEXT("FX_WeaponTop02");

    UPROPERTY(EditAnywhere, Category="Trail Dimensions", meta=(ClampMin="0"))
    float WidthMultiplier = 1.f;

    /** Gameplay only. Return the active offensive weapon and the authority's current and neutral configurations. */
    UFUNCTION(BlueprintImplementableEvent, Category="Trail Source")
    void ResolveTrailContext(USkeletalMeshComponent* Mesh, AActor*& Weapon, FWeaponTrailEffect& Active, FWeaponTrailEffect& Neutral) const;

    virtual void NotifyBegin(USkeletalMeshComponent*, UAnimSequenceBase*, float, const FAnimNotifyEventReference&) override;
    virtual void NotifyEnd(USkeletalMeshComponent*, UAnimSequenceBase*, const FAnimNotifyEventReference&) override;
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload&) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload&) override;
private:
    void BeginWindow(USkeletalMeshComponent* Mesh, int32 Id, bool bBranching) const;
    void EndWindow(USkeletalMeshComponent* Mesh, int32 Id, bool bBranching) const;
};

UCLASS()
class WEAPONVISUALS_API UWeaponVisualLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Weapon Visuals")
    static void SetBladeAuras(const TArray<UNiagaraComponent*>& Components, UNiagaraSystem* System);

    UFUNCTION(BlueprintCallable, Category="Weapon Visuals")
    static void EndTrailsOnAttunementChange(AActor* Weapon, const UObject* Previous, const UObject* Next);
};
