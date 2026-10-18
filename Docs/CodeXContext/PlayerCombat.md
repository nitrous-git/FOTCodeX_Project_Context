# Player Framework — CombatStyle, Attunement and locomotion

## Existing assets

```text
CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase/
CodeX_Restruct/Frameworks/PlayerFramework/Animations/ABP/ABP_PlayerMaster/
CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_CombatStyle/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_Attunement/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_ActionMap/
CodeX_Restruct/Frameworks/PlayerFramework/Core/InventoryPreview/
```

## Nonnegotiable preservation boundary

The user intends to **preserve and polish the existing** player CombatStyle, Attunement, locomotion and `ABP_PlayerMaster` contracts. Third-party framework integration or convenience rewrites must not silently replace them. Before suggesting such an integration, describe precisely which events, data assets, animation transitions and equipment/presentation interfaces would be affected.

## Export-confirmed graph names

`BP_PlayerBase` includes `GetCombatStyle`, `SetCombatStyle`, `Set Combat Style from Enum`, `SetCombatStyleWeapon`, `SetLocomotionMode`, `SetLastMoveInput`, `InitPlayerUI`, `SetBossRef`, plus equip/unequip paths. `AC_PlayerAttunementComponent` and the CombatStyle/Attunement data-asset families are in the export. Do not infer exact network/replication behavior or every supported weapon slot from these names.

## Architectural intent

CombatStyle selection and data assets define player attack presentation/behavior; Attunement provides a distinct layer for elemental/style effects; locomotion and the animation blueprint handle movement and montage presentation. Inventory selection may trigger an existing style/attunement contract, but should not bypass it or duplicate authoritative state without a reason.

## Before editing

Inspect the data asset/struct definitions, player component call sites, `ABP_PlayerMaster` animation interfaces and the inventory UI's references. Note whether a proposed change affects player action maps, input binding, weapon meshes, animation montages, camera behavior, or boss UI. Compile and verify player locomotion/attack/attunement after any approved changes.
