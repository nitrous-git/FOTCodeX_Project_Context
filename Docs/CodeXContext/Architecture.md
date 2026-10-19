# CodeX — Architecture map

**Updated October 19, 2026.** The live Unreal Editor is authoritative; this document records established ownership and the accepted Item System / Game Menu redesign.

## Project overview

Blueprint-first UE 5.8.3 third-person action-adventure / soulslike. Existing player combat, regular-enemy AI/coordination, authored boss encounters and traversal should be reused rather than replaced by imported frameworks. The user authorized a deliberate Inventory/Equipment/UI reset; that authorization does **not** imply refactoring unrelated subsystems.

## Primary owners and directories

Paths are relative to exported `CodeX_Restruct/`; in Unreal prefix with `/Game/CodeX_Restruct/`.

| Domain | Location | Authoritative responsibility |
|---|---|---|
| Player | `Frameworks/PlayerFramework/` | `BP_PlayerBase`, CombatStyle, Attunement, locomotion/`ABP_PlayerMaster`, player equipment-actor presentation |
| Items / Inventory / Equipment | `Frameworks/ItemSystemFramework/` | Item definitions/data, owned inventory arrays, equipped-slot assignments, world pickup actors |
| Game Menu UI | `Game/UI/GameMenu/` | Sibling Inventory/Equipment/Status/System pages; local selection and navigation state; **no authoritative items** |
| Permanent gameplay HUD | `Game/UI/Widget/WBP_PlayerGUI/` | Always-on gameplay HUD; not a Game Menu page |
| Physical weapon actors | `Game/Items/Weapons/` | Real weapon subclasses and their effects/collision; resolved via equipment data |
| Regular enemies | `Frameworks/EnemyFramework/` | Base enemy identity, awareness, WorldBehavior, GroupMelee/Ranged combat styles and BT/BB |
| Bosses | `Frameworks/BossFramework/` | Placed activation lifecycle, boss state and Director/Metrics, special attack behavior, health UI |
| Group / encounters | `Shared/Managers/` | Ordinary-enemy coordination / encounter spawning (not general boss attack scheduling) |
| Controllers/modes | `Core/` | Input/menu lifecycle, possession, world-level core contracts |
| Other frameworks | `Frameworks/TeleporterFramework/`, `TraversalDeviceFramework/`, `TraversalHomingFramework/` | Existing traversal capabilities |

The former `InventorySystemFramework/InventorySystemRefactor` path was retired in the October 19 exported reset. **Do not recreate it** or split the accepted Item System into extra top-level Frameworks.

## Canonical ownership contracts

```text
AC_InventoryComponent        # owns inventory collections and quantities
  GetItemDefinition, AddItem, RemoveItemFromSlot, HasItem, ...
                |
                | owned-item/definition query
                v
AC_EquipmentComponent        # owns EquippedItemsBySlot: GameplayTag -> ItemID
  TryEquipItemToSlot, UnequipItemFromSlot, GetEquippedItemAtSlot,
  IsItemEquipped, ResolveEquippedWeaponClass, OnEquipmentSlotChanged
                |
                +--> BP_PlayerBase  # acts on equipment event; physical weapon spawn
                +--> WBP_Equipment  # displays assignment state and allows editing

WBP_GameMenu                  # parent page navigation only
  +-- WBP_Inventory           # reads stored items; browse/actions
  +-- WBP_Equipment           # interactive loadout, internal slot/choice switcher
  +-- WBP_Status              # stats
  +-- WBP_System              # system menu
```

For full slot contracts, paths and UI semantics consult **`InventoryAndUI.md`**. For the MCP foundation build consult **`GameMenuFoundationSpec.md`**.

## Player and other-system boundaries

- `BP_PlayerBase.SetCombatStyleWeapon` currently resolves right/left class through the new Equipment Component and continues spawning/attaching physical actor instances using existing player logic. Preserve weapon sockets/transforms, `PrimaryWeapon`/`SecondaryWeapon`, and Attunement reapplication.
- CombatStyle defines combat move/animation behavior and is distinct from equipped **physical ItemID**. Drawing/sheathing is also separate from loadout editing.
- Regular enemies: `AC_EnemyAwareness` handles discovery, `AC_EnemyWorldBehavior` handles roaming/patrol/return, and dedicated combat-style components handle combat. `BP_EnemyAIManager` coordinates regular attacker slots.
- Bosses: state logic and `BP_BossDirectorComponent`/`BP_MetricsComponent` own encounter orchestration. Placed dormant bosses may activate through awareness/area/fog triggers; do not assume they use regular group attacker scheduling.
- Player and Game Menu may share a capture actor for visual preview; it must not create an independent authoritative equipped-items model.

## Export and MCP navigation

Selected entries:

```text
CodeX_Restruct/
  Frameworks/ItemSystemFramework/Inventory/AC_InventoryComponent/
  Frameworks/ItemSystemFramework/Equipment/AC_EquipmentComponent/
  Frameworks/ItemSystemFramework/Items/FInventoryItemDefinition/
  Frameworks/ItemSystemFramework/Items/DT_InventoryItemData/
  Frameworks/ItemSystemFramework/Items/Definitions/Weapons/DA_WeaponDefinition/
  Frameworks/PlayerFramework/Core/BP_PlayerBase/
  Frameworks/PlayerFramework/Core/InventoryPreview/BP_PlayerBase_InventoryPreview/
  Frameworks/EnemyFramework/Core/BP_EnemyBase/
  Frameworks/BossFramework/Core/BP_BaseBoss/
  Core/Controllers/BP_CPC_PlayerController/
```

The `Game/UI/GameMenu` page widget assets are **planned**, not represented in the latest GitHub export. Some cleanup occurred locally after that export; use live MCP before making deletion or reconstruction decisions.

## Cross-system safeguards

- Inventory item removal vs equipped assignments is a cross-component integrity rule; do not silently leave stale equipment after the last owned instance disappears.
- High-impact changes to Player, Enemy, Boss base classes, shared structs/interfaces, gameplay tags, or Data Table enumerators need targeted live referencer/dependency checks.
- Do not force old Inventory selection semantics into new widgets, or import SLF gameplay systems as replacements.
- Operate on `.uasset` through Unreal Editor/MCP, not by filesystem edits or exported JSON reconstruction.
