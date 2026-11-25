#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AnimPreviewInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PreviewScene.h"
#include "WeaponSlashTrail.h"
#include "WeaponTrailWindow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTrailPreviewTest, "CodeX.WeaponVisuals.MontagePreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTrailPreviewTest::RunTest(const FString&)
{
    auto* Source = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/CodeX_Restruct/Frameworks/PlayerFramework/Animations/CombatStyle/Katana/Combo/Anim_DK2_Combo_A2_RM_Montage.Anim_DK2_Combo_A2_RM_Montage"));
    auto* SkeletonMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/LocomotionSystem/Characters/Mannequin_UE4/Meshes/SK_Mannequin.SK_Mannequin"));
    auto* NotifyClass = LoadClass<UWeaponSlashTrail>(nullptr, TEXT("/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/AnimNotify/ANS_WeaponSlashTrail.ANS_WeaponSlashTrail_C"));
    auto* Cascade = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/CodeX_Restruct/External/AssetContent/VFX/NiagaraMagicalSlashes/Fx/SwordTrail/P_SlashTrail_00.P_SlashTrail_00"));
    auto* Niagara = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/CodeX_Restruct/External/AssetContent/VFX/SwordEffects/Particles/NS_SwordEffect1_Trails.NS_SwordEffect1_Trails"));
    if (!TestTrue(TEXT("Preview assets available"), Source && SkeletonMesh && NotifyClass && Cascade && Niagara))
    {
        return false;
    }

    FPreviewScene Scene(FPreviewScene::ConstructionValues().SetCreateDefaultLighting(false));
    auto* Mesh = NewObject<UDebugSkelMeshComponent>();
    Scene.AddComponent(Mesh, FTransform::Identity);
    Mesh->SetSkeletalMeshAsset(SkeletonMesh);
    auto* Montage = DuplicateObject<UAnimMontage>(Source, GetTransientPackage());
    Montage->Notifies.Reset();
    const float Length = Montage->GetPlayLength();
    for (bool bCascade : {true, false})
    {
        auto* Notify = NewObject<UWeaponSlashTrail>(Montage, NotifyClass);
        Notify->BladeSelection = bCascade ? E_WeaponTrailBlade::Both : E_WeaponTrailBlade::Primary;
        Notify->EditorPreviewEffect.TrailType = bCascade ? E_WeaponTrailType::Cascade : E_WeaponTrailType::Niagara;
        Notify->EditorPreviewEffect.CascadeSystem = Cascade;
        Notify->EditorPreviewEffect.NiagaraSystem = Niagara;
        FAnimNotifyEvent& Event = Montage->Notifies.AddDefaulted_GetRef();
        Event.NotifyStateClass = Notify;
        Event.Link(Montage, Length * (bCascade ? .1f : .2f));
        Event.EndLink.Link(Montage, Length * (bCascade ? .4f : .7f));
        Event.SetDuration(Length * (bCascade ? .3f : .5f));
    }
    Montage->RefreshCacheData();
    Mesh->EnablePreview(true, Montage);
    if (!TestNotNull(TEXT("Persona preview instance"), Mesh->PreviewInstance.Get()))
    {
        return false;
    }
    Mesh->PreviewInstance->MontagePreview_PreviewNormal();
    bool bOverlap = false, bSurvivor = false, bEnded = false;
    for (int32 Frame = 0; Frame < 80; ++Frame)
    {
        Mesh->TickAnimation(Length / 90.f, false);
        Mesh->RefreshBoneTransforms();
        Mesh->ConditionallyDispatchQueuedAnimEvents();
        int32 NiagaraCount = 0, CascadeCount = 0;
        for (USceneComponent* Child : Mesh->GetAttachChildren())
        {
            if (auto* Window = Cast<UWeaponTrailWindow>(Child))
            {
                for (UFXSystemComponent* Effect : Window->Effects)
                {
                    CascadeCount += Effect && Effect->IsA<UParticleSystemComponent>();
                    NiagaraCount += Effect && Effect->IsA<UNiagaraComponent>();
                }
            }
        }
        bOverlap |= CascadeCount == 2 && NiagaraCount == 1;
        bSurvivor |= bOverlap && CascadeCount == 0 && NiagaraCount == 1;
        bEnded |= bSurvivor && CascadeCount == 0 && NiagaraCount == 0;
    }
    TestTrue(TEXT("Both Cascade blades overlap a Niagara window in Montage preview"), bOverlap);
    TestTrue(TEXT("Ending Cascade preserves the independent Niagara activation"), bSurvivor);
    TestTrue(TEXT("The final notify end releases the remaining activation"), bEnded);
    Scene.RemoveComponent(Mesh);
    return bOverlap && bSurvivor && bEnded;
}
#endif
