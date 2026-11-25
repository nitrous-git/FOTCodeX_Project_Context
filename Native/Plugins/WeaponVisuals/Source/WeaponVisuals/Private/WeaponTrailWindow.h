#pragma once

#include "Components/SceneComponent.h"
#include "WeaponTrailWindow.generated.h"

class UFXSystemComponent;
class UWeaponSlashTrail;

UCLASS(Transient)
class UWeaponTrailWindow : public USceneComponent
{
    GENERATED_BODY()
public:
    void Initialize(USkeletalMeshComponent* Mesh);
    TWeakObjectPtr<const UWeaponSlashTrail> Notify;
    int32 ActivationId = INDEX_NONE;
    bool bBranchingPoint = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UFXSystemComponent>> Effects;
protected:
    virtual void OnUnregister() override;
private:
    TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
    UFUNCTION()
    void OnAnimationReinitialized();
#if WITH_EDITOR
    FDelegateHandle UnregisteredHandle;
#endif
};
