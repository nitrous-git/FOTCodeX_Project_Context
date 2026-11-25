#include "WeaponSlashTrail.h"
#include "WeaponTrailWindow.h"

#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/TypeData/ParticleModuleTypeDataAnimTrail.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeaponVisuals, Log, All);

bool FWeaponTrailEffect::HasEffect() const
{
    if (TrailType == E_WeaponTrailType::Niagara)
    {
        return IsValid(NiagaraSystem);
    }
    if (TrailType == E_WeaponTrailType::Cascade && IsValid(CascadeSystem))
    {
        for (const UParticleEmitter* Emitter : CascadeSystem->Emitters)
        {
            if (Emitter && Emitter->LODLevels.IsValidIndex(0))
            {
                const UParticleLODLevel* LOD = Emitter->LODLevels[0];
                if (LOD && LOD->bEnabled && IsValid(LOD->TypeDataModule) && LOD->TypeDataModule->IsA<UParticleModuleTypeDataAnimTrail>())
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void UWeaponVisualLibrary::SetBladeAuras(const TArray<UNiagaraComponent*>& Components, UNiagaraSystem* System)
{
    for (UNiagaraComponent* Component : Components)
    {
        if (!IsValid(Component))
        {
            continue;
        }
        if (Component->GetAsset() != System)
        {
            Component->DeactivateImmediate();
            Component->SetAsset(System);
        }
        if (System && !Component->IsActive())
        {
            Component->Activate(true);
        }
        else if (!System && Component->IsActive())
        {
            Component->DeactivateImmediate();
        }
    }
}

void UWeaponVisualLibrary::EndTrailsOnAttunementChange(AActor* Weapon, const UObject* Previous, const UObject* Next)
{
    if (IsValid(Weapon) && Previous != Next)
    {
        TInlineComponentArray<UWeaponTrailWindow*> Windows(Weapon);
        for (UWeaponTrailWindow* Window : Windows)
        {
            Window->DestroyComponent();
        }
    }
}

void UWeaponTrailWindow::Initialize(USkeletalMeshComponent* Mesh)
{
    SourceMesh = Mesh;
    SetupAttachment(Mesh);
    RegisterComponentWithWorld(Mesh->GetWorld());
    Mesh->OnAnimInitialized.AddDynamic(this, &UWeaponTrailWindow::OnAnimationReinitialized);
#if WITH_EDITOR
    UnregisteredHandle = Mesh->RegisterOnSkeletalMeshUnregistered(
        USkeletalMeshComponent::FOnSkeletalMeshUnregisteredDelegate::CreateWeakLambda(this,
            [this](const USkeletalMeshComponent*) { DestroyComponent(); }));
#endif
}

void UWeaponTrailWindow::OnAnimationReinitialized()
{
    DestroyComponent();
}

void UWeaponTrailWindow::OnUnregister()
{
    if (USkeletalMeshComponent* Mesh = SourceMesh.Get())
    {
        Mesh->OnAnimInitialized.RemoveDynamic(this, &UWeaponTrailWindow::OnAnimationReinitialized);
#if WITH_EDITOR
        Mesh->UnregisterOnSkeletalMeshUnregistered(UnregisteredHandle);
#endif
    }
    for (UFXSystemComponent* Effect : Effects)
    {
        if (IsValid(Effect))
        {
            if (UParticleSystemComponent* Cascade = Cast<UParticleSystemComponent>(Effect))
            {
                Cascade->EndTrails();
            }
            else
            {
                Effect->Deactivate();
            }
        }
    }
    Effects.Reset();
    Super::OnUnregister();
}

UWeaponSlashTrail::UWeaponSlashTrail()
{
    NotifyStateBehaviorFlags |= static_cast<uint8>(EAnimNotifyStateBehaviorFlags::NoMergeOnConcurrentPlay);
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = true;
#endif
}

static UFXSystemComponent* SpawnBladeTrail(USkeletalMeshComponent* Mesh, const FWeaponTrailEffect& Effect,
    FName Start, FName End, float Multiplier)
{
    const bool bCascade = Effect.TrailType == E_WeaponTrailType::Cascade;
    if (!Mesh->DoesSocketExist(Start) || (bCascade && !Mesh->DoesSocketExist(End)))
    {
        UE_LOG(LogWeaponVisuals, Warning, TEXT("Skipped blade on %s. Check trail sockets %s%s."),
            *GetNameSafe(Mesh), *Start.ToString(), bCascade ? *FString::Printf(TEXT(" / %s"), *End.ToString()) : TEXT(""));
        return nullptr;
    }
    if (bCascade)
    {
        UParticleSystemComponent* FX = NewObject<UParticleSystemComponent>(Mesh, NAME_None, RF_Transient);
        FX->bAutoActivate = false;
        FX->bAutoDestroy = true;
        FX->SetupAttachment(Mesh); // Native AnimTrail samples both sockets on its attachment parent.
        FX->SetTemplate(Effect.CascadeSystem);
        FX->RegisterComponentWithWorld(Mesh->GetWorld());
        FX->BeginTrails(Start, End, ETrailWidthMode_FromCentre, Effect.CascadeWidthScale * Multiplier);
        return FX;
    }

    UNiagaraComponent* FX = NewObject<UNiagaraComponent>(Mesh, NAME_None, RF_Transient);
    FX->bAutoActivate = false;
    FX->SetAutoDestroy(true);
    FX->SetupAttachment(Mesh, Start);
    FX->SetAsset(Effect.NiagaraSystem);
    if (!Effect.NiagaraWidthParameter.IsNone())
    {
        const FNiagaraVariable Width(FNiagaraTypeDefinition::GetFloatDef(), Effect.NiagaraWidthParameter);
        if (Effect.NiagaraSystem->GetExposedParameters().FindParameterOffset(Width))
        {
            FX->SetVariableFloat(Effect.NiagaraWidthParameter, Effect.NiagaraBaseWidth * Multiplier);
        }
        else
        {
            UE_LOG(LogWeaponVisuals, Warning, TEXT("%s has no exposed float %s. Using its authored width."),
                *GetNameSafe(Effect.NiagaraSystem), *Effect.NiagaraWidthParameter.ToString());
        }
    }
    FX->RegisterComponentWithWorld(Mesh->GetWorld());
    FX->Activate(true);
    return FX;
}

void UWeaponSlashTrail::BeginWindow(USkeletalMeshComponent* Mesh, int32 Id, bool bBranching) const
{
    if (!IsValid(Mesh) || !Mesh->GetWorld() || Mesh->GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }
    AActor* Weapon = nullptr;
    FWeaponTrailEffect Active, Neutral;
    const FWeaponTrailEffect* Effect = &EditorPreviewEffect;
    if (Mesh->GetWorld()->IsGameWorld())
    {
        ResolveTrailContext(Mesh, Weapon, Active, Neutral);
        if (!IsValid(Weapon))
        {
            return;
        }
        Effect = bUseRuntimeEffectOverride ? &RuntimeEffectOverride : &Active;
        if (Effect->TrailType != E_WeaponTrailType::None && !Effect->HasEffect())
        {
            Effect = &Neutral;
        }
    }
    else if (Mesh->GetWorld()->WorldType != EWorldType::EditorPreview)
    {
        return;
    }
    if (!Effect->HasEffect())
    {
        return;
    }
    EndWindow(Mesh, Id, bBranching);
    // Gameplay windows belong to the weapon, so destroying or replacing it also closes the window.
    UObject* Outer = Weapon ? static_cast<UObject*>(Weapon) : static_cast<UObject*>(Mesh);
    UWeaponTrailWindow* Window = NewObject<UWeaponTrailWindow>(Outer, NAME_None, RF_Transient);
    Window->Notify = this;
    Window->ActivationId = Id;
    Window->bBranchingPoint = bBranching;
    Window->Initialize(Mesh);
    auto AddBlade = [&](FName Start, FName End)
    {
        if (UFXSystemComponent* FX = SpawnBladeTrail(Mesh, *Effect, Start, End, FMath::Max(0.f, WidthMultiplier)))
        {
            Window->Effects.Add(FX);
        }
    };
    if (BladeSelection != E_WeaponTrailBlade::Secondary)
    {
        AddBlade(PrimaryStartSocket, PrimaryEndSocket);
    }
    if (BladeSelection != E_WeaponTrailBlade::Primary)
    {
        AddBlade(SecondaryStartSocket, SecondaryEndSocket);
    }
    if (Window->Effects.IsEmpty())
    {
        Window->DestroyComponent();
    }
}

void UWeaponSlashTrail::EndWindow(USkeletalMeshComponent* Mesh, int32 Id, bool bBranching) const
{
    if (!IsValid(Mesh))
    {
        return;
    }
    for (USceneComponent* Child : Mesh->GetAttachChildren())
    {
        UWeaponTrailWindow* Window = Cast<UWeaponTrailWindow>(Child);
        if (Window && Window->Notify.Get() == this && Window->ActivationId == Id && Window->bBranchingPoint == bBranching)
        {
            Window->DestroyComponent();
            return;
        }
    }
}

void UWeaponSlashTrail::NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase*, float, const FAnimNotifyEventReference& Event)
{
    BeginWindow(Mesh, Event.GetNotifyInstanceID(), false);
}

void UWeaponSlashTrail::NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase*, const FAnimNotifyEventReference& Event)
{
    EndWindow(Mesh, Event.GetNotifyInstanceID(), false);
}

void UWeaponSlashTrail::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
    BeginWindow(Payload.SkelMeshComponent, Payload.MontageInstanceID, true);
}

void UWeaponSlashTrail::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload)
{
    EndWindow(Payload.SkelMeshComponent, Payload.MontageInstanceID, true);
}
