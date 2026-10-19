# CodeX — Player Framework: CombatStyle, Equipment Actors and Attunement

**Updated October 19, 2026.** The Item System component split changed the source of physical weapon classes, not the owner of player combat animations.

## Important existing assets

```text
CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase/
CodeX_Restruct/Frameworks/PlayerFramework/Animations/ABP/ABP_PlayerMaster/
CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_CombatStyle/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_Attunement/
CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_ActionMap/
CodeX_Restruct/Frameworks/PlayerFramework/Core/InventoryPreview/
CodeX_Restruct/Frameworks/ItemSystemFramework/Equipment/AC_EquipmentComponent/
CodeX_Restruct/Frameworks/ItemSystemFramework/Inventory/AC_InventoryComponent/
CodeX_Restruct/Game/Items/Weapons/
```

## Three separate concepts

1. **Inventory ownership:** the player owns ItemID records in `AC_InventoryComponent`.
2. **Equipment assignment:** `AC_EquipmentComponent.EquippedItemsBySlot` associates specific ItemIDs with gameplay-tag positions (`RightHand1`, `LeftHand1` initially). A weapon definition supplies the concrete `BP_WeaponBase` actor class.
3. **CombatStyle / animation behavior:** existing `DA_CombatStyle`, `CombatStyleMode`, `ABP_PlayerMaster`, action maps, montages, blocking/parry/combos and locomotion remain player-owned. Equipping a different sword in a compatible slot should not automatically replace the animation system.

Equipping an item, drawing/sheathing it, using it for attacks and applying Attunement are related but **distinct state transitions**.

## Exported weapon application path

`BP_PlayerBase.SetCombatStyleWeapon` uses `AC_EquipmentComponent.ResolveEquippedWeaponClass` for `Equipment.Slot.RightHand1` and `.LeftHand1`, then follows existing `UnequipAllWeapon`, actor Spawn/Attach, socket/transform and `PrimaryWeapon`/`SecondaryWeapon` assignments. On equipment changes, the Player's listener refreshes the affected **weapon presentation** (current implementation may rebuild both hands).

Do not make UI spawn the weapon actor. Do not make `AC_EquipmentComponent` directly write meshes or UI. Preserve the existing weapon Actor subclasses, damage/collision and VFX.

`AC_PlayerAttunementComponent` is independent; after replacing weapon actors, ensure the existing Attunement reapplication path still works. Retain current `BP_PlayerBase_InventoryPreview` capture/rotation mechanisms; its eventual equipment sync should query the same authoritative assignment state.

## Verification and deferred work

- W3 user-reported PIE tests demonstrated Knight Sword/Iron Sword swaps and shield independence before the component migration. The exported A3 split is visible; post-A4 live PIE results are not independently established by the archive.
- Weapon-family compatibility is currently checked via `DA_WeaponDefinition.AllowedEquipSlots` (exact Gameplay Tag). A universal mapping of weapon families and combinations to combat styles is deferred.
- Do not remove `DA_CombatStyle` or player combat/animation fields solely because some older inventory-driven selection functions are retired.
- Before player changes inspect the relevant live Blueprint graphs and directly affected combat consumers. After edits compile and test movement, attacks, block/parry, weapon actor references and Attunement as relevant.

For item/menu semantics see `InventoryAndUI.md`.
