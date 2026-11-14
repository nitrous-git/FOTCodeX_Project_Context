# 02 — Current export audit (before conversion)

**Evidence status: OBSERVED in GitHub Blueprint/context export, not independently PIE-tested.**  
**Audit source:** [`nitrous-git/FOTCodeX_Project_Context`](https://github.com/nitrous-git/FOTCodeX_Project_Context), `main`, commit `8950d6d294292fc95d4af302d5cca8387309126d`.  
**Export date:** `2026-11-14T07:31:28.750062Z`; **exporter:** `0.5.0`, schema `1`; **UE:** `5.8.4-58902052+++UE5+Release-5.8`.

## 1. Snapshot reliability

`export_manifest.json` reports:

| Measure | Reported value |
|---|---:|
| Assets scanned | 8,515 |
| Excluded by path | 4,974 |
| Ignored by class | 3,076 |
| Export candidates | 457 |
| Full graph exports | 322 |
| Structural exports | 135 |
| Metadata errors / warnings | 0 / 0 |
| Graph export errors | 0 |
| Graph parser warning assets | 27 |
| Authored graphs | 1,768 |
| Graph nodes | 37,995 |
| Dependency-graph edges | 1,417 |

The warning set includes pin-count parsing discrepancies in player/enemy AnimBPs and other high-node-count assets. This is **not** a compile failure or verified runtime issue. Exclusion filters omit certain paths and many non-Blueprint assets. For a live migration, check the editor asset graph and exact Gameplay Tags; export metadata is navigational evidence, never an edit target.

Pre-continuation architecture notes dated October are superseded by the November 14 continuation documentation. This audit section references the current November 14 export; live assets remain authoritative.

## 2. Existing system ownership / asset locations

All paths below begin with `/Game/CodeX_Restruct/` in Unreal and `Game/CodeX_Restruct/` in the context mirror.

| Domain | Asset or directory | What is observed |
|---|---|---|
| Owned items | `Frameworks/ItemSystemFramework/Inventory/AC_InventoryComponent` | Category slot arrays, quantities, `GetItemDefinition`, `HasItem`, Add/Remove, potion finding/usage, inventory actions/events |
| Equipment state | `Frameworks/ItemSystemFramework/Equipment/AC_EquipmentComponent` | `EquippedItemsBySlot` Map, equip/unequip, active right-hand slot, active tool slot, related dispatchers |
| Equipment stats | `Frameworks/ItemSystemFramework/Equipment/AC_EquipmentStatsComponent` | Requirement/scaling math, armor sources, calculated snapshot, change subscriptions |
| Item schema | `Frameworks/ItemSystemFramework/Items/` | `EItemCategory`, `FInventoryItemDefinition`, `DT_InventoryItemData` and definitions |
| Weapon data | `Items/Definitions/Weapons/DA_WeaponDefinition` | `AllowedEquipSlots`, `WeaponFamilyTag`, `WeaponActorClass`, `BaseAttack`, `AttributeRequirements`, `ScalingRules`, weight |
| Armor data | `Items/Definitions/Armor/DA_ArmorDefinition` | `ArmorSlotTag`, `ArmorMesh`, `AdditionalMeshes`, negation, resistances, attribute bonuses, weight |
| Player | `Frameworks/PlayerFramework/Core/BP_PlayerBase` | Active combat style and spawned weapon actors, armor mesh application, equipment refresh, guard cleanup |
| Player stats | `Core/Components/AC_CharacterStatsComponent` | CurrentHP/FP/Stamina, runtime regen, action costs, base/effective attributes |
| Progression | `Core/Components/AC_ProgressionComponent` | Souls count and manual level-up APIs including `TryCommitLevelUp` |
| Attunement | `Core/Components/AC_PlayerAttunementComponent` | `ActiveAttunementData`, `SetActiveAttunement`, reapplication to weapons |
| Shadow Strike | `Core/Components/AC_PlayerShadowStrikeComponent` | Targeting, aim/travel/execution, chain behavior, camera effects |
| Combat styles | `Core/DataAssets/DA_CombatStyle` | `BaseAnimsetName`, `GuardAnimsetName`, `StyleTag`, weapon actor/animation references, block montages |
| Permanent HUD | `Game/UI/HUD/` | `WBP_PlayerHUD`, resources, resource bars, ItemWheel widgets, interaction notifications |
| Game Menu | `Game/UI/GameMenu/` | Functional `WBP_GameMenu`, sibling pages, equipment picker, item info and inventory actions |
| System Menu | `Game/UI/GameMenu/System/WBP_System` | Game/graphics/audio/camera/preferences/exit-oriented functions and UI state |
| Rest Menu | `Game/UI/RestMenu/WBP_RestMenu` | Rest selection + time-selection state and event dispatchers |
| Main Menu | `Game/UI/MainMenu/WBP_MainMenu` | New journey, load/profile selection, Settings, Quit |
| Controller | `Core/Controllers/BP_CPC_PlayerController` | Menu opening, page routing, HUD binding, resting, potion inputs, death/cinematic input |

**Protected adjacent systems:** `EnemyFramework`, `BossFramework`, `CinematicFramework`, `InteractionFramework`, traversals and the current combat action system. These already exist and are **not** migration targets except for audited regressions caused by changing player/equipment data.

## 3. Specific current component contracts

### `AC_EquipmentComponent`

Observed state:

```text
EquippedItemsBySlot : Map<GameplayTag, ItemID>
ActiveRightHandSlot : GameplayTag
ActiveToolSlot      : GameplayTag
InventoryRef        : AC_InventoryComponent
```

Observed public/internal functions include:

```text
GetEquippedItemAtSlot   TryEquipItemToSlot   UnequipItemFromSlot
ResolveEquippedWeaponClass  IsItemEquipped
GetActiveRightHandSlot  GetActiveRightHandItem
IsRightHandSlot         IsValidRightHandAssignment
CanAssignRightHandItem  SetActiveRightHandSlot
FindNextOccupiedRightHandSlot  ReconcileActiveRightHandSlot
GetToolSlotsInOrder    GetActiveToolSlot    GetActiveToolItem
CanEquipToolItem       TryEquipToolToSlot
SetActiveToolSlot      FindNextOccupiedToolSlot
ReconcileActiveToolSlot HandleToolInventoryChanged CycleActiveToolSlot
```

Observed dispatchers: `OnEquipmentSlotChanged`, `OnActiveRightHandSlotChanged`, `OnActiveToolSlotChanged`. The new design needs to replace the *semantics* of active right-hand + five tools with active weapon + two medicines; preserve useful event behavior and callers until each migration stage finishes.

### `AC_InventoryComponent`

Observed arrays/counts: `WeaponSlots`, `ArmorSlots`, `TalismanSlots`, `ToolSlots`; corresponding slot counts; `InventoryDataTable`; `OnInventoryChanged`; potion selection state `SelectedPotionID`. Functions include `AddItem`, `RemoveItemFromSlot`, `HasItem`, category-index lookups, `UseConsumableFromSlot`, `FindPotionStack`, `CycleSelectedPotion` and amount-action validation. This is substantial functioning infrastructure; do not replace it with a new Inventory class merely to rename UI categories.

### `BP_PlayerBase`

Relevant observed functions:

```text
SetCombatStyle           SetCombatStyleWeapon   UnequipAllWeapon
RefreshEquippedWeapons   PreparePrimaryWeaponRefresh
PrepareSecondaryWeaponRefresh
CanChangeActiveRightHandWeapon RequestActiveRightHandSlot
HandleActiveRightHandSlotChanged
InitializeArmorFollowers GetArmorComponentForSlot
ApplyArmorPieceVisual    RefreshArmorSlotFromEquipment
RefreshAllArmorVisuals   ClearAdditionalArmorMeshes
CancelBlockForEquipmentChange CE_EndBlocking RefreshCombatStyleLocomotion
```

The current names `PrimaryWeapon` and `SecondaryWeapon` **already exist** in the player/`DA_CombatStyle` as combat/presentation references. Do not assume they are identical to the **new Primary/Secondary equipment slots** without inspecting graphs and variable types.

### `DA_CombatStyle`

Observed fields include `BaseAnimsetName`, `GuardAnimsetName`, `StyleTag`, `PrimaryWeapon`, `SecondaryWeapon`, `Defense_Start`, `Defense_Stop`, `Equip`, `Unequip`, `Death`, blocking speed, combo/lunge/parry/dodge/hit variants. Exported families include Common, Katana, SNS and Twinblades variants. These are valuable and should be preserved.

### Stats

`AC_EquipmentStatsComponent` currently calculates weapon rating and combines multi-armor negation/bonuses. `AC_CharacterStatsComponent` implements current HP, FP, stamina, regen and resource spending. `AC_ProgressionComponent` implements Souls/Level advancement. **Do not delete resource or resolved-stat logic when hiding manual level-up in the UI.**

## 4. Current item taxonomy is NOT the target taxonomy

`EItemCategory` currently has **seven** display members:

```text
Tool, Armor, Talisman, CombatStyle, Attunement, Fragment, Weapon
```

`FInventoryItemDefinition` exposes 14 fields including `ItemID`, `ItemCategory`, `ItemName`, `ItemDescription`, `ItemIcon`, stack/use/equip flags, `CombatStyleData`, `ArmorData`, `AttunementData`, `WeaponData`, `WorldPickupClass`.

`DT_InventoryItemData` contains **19 rows** by current category (the November 13 export previously listed 37; the reduction has already occurred in the newest snapshot):

| Category | Rows | Important issue |
|---|---:|---|
| `Armor` | 4 | Four retained individual Head/Chest/Arms/Legs test entries |
| `Weapon` | 5 | KnightSword, IronSword, KnightShield, Twinblades, **Katana** |
| `Talisman` | 5 | **Actually point to `DA_Attunement` definitions**: Neutral/Fire/Frost/Poison/Electric |
| `Tool` | 5 | Three Champion fragments **plus** HP and EN potions |
| `Attunement`, `Fragment`, `CombatStyle` | 0 in this table | Enum exists, but listed instance rows do not yet use these categories |

This is a central migration concern: **Talisman must become a genuine passive category, Attunement must become its own selectable class, Champion fragments must move to narrative collection semantics, and Medicine must not accidentally accept unrelated Tool items.** Preserve stable ItemIDs where possible or explicitly remap any existing save data when relevant.

## 5. Current equipment UI slot table is NOT the target

`DT_EquipmentSlots_UI` uses `FEquipmentSlotUIData` and has **13 enabled rows**:

```text
RightHand1, RightHand2, RightHand3, LeftHand1,
Head, Chest, Arms, Legs,
Tool01, Tool02, Tool03, Tool04, Tool05
```

Tags include `Equipment.Slot.RightHand1..3`, `.LeftHand1`, `.Head/.Chest/.Arms/.Legs`, `.Tool01..05`. Groups include `Equipment.Group.Weapons`, `.Armor`, `.Tools`.

The new model needs six visible Equipment rows (two weapons, Talisman, Attunement, two Medicines) and one **separate Character/Outfit selection** backed by the same equipment authority. Old tag references may exist across the UI, player, HUD and stats systems, and cannot be removed before dependents are migrated.

## 6. Other concrete mismatches

1. **GameMenu enum is 4-page:** exported `EGameMenuPage` = Inventory, Equipment, Status, System. Target is five pages: Character, Equipment, Inventory, Artifacts, System.
2. **Menu composition is not a shell:** `WBP_GameMenu` has page logic `RefreshPage`, `RequestFullScreenPage`, and controller routing. Reuse; modify safely.
3. **Twinblades family tag:** exported `DA_Weapon_Twinblades` now has `WeaponFamilyTag=Weapon.Family.Twinblade` (**singular**), while `DA_Weapon_Katana` uses `Weapon.Family.Katana`. Check the registered Gameplay Tags and the mapping to `DA_CombatStyle_Twinblades` before renaming; a singular/plural label mismatch is not itself proof of a runtime failure.
4. **Legacy Talisman semantics:** `Talisman_Fire` references `DA_Attunement_Fire`, etc. Do not rename the label and call it a passive buff.
5. **Legacy shield identity:** `Weapon_KnightShield` is a Weapon item and separate hand-slot content; CodeX wants Sword+Shield to be one resolved weapon kit where needed, not separately assignable shield-only equipment.
6. **Legacy armor composition:** `DA_ArmorDefinition` already supports `AdditionalMeshes`; reuse this insight for curated whole-Outfit visuals instead of automatically destroying usable mesh code.
7. **Rest:** current `WBP_RestMenu` and `BP_CPC_PlayerController` include time selection (`TimeSelection`, `RequestRestTime`, `ApplyRestTime`) and level-up UI. These need scope-limited replacement, not a controller rewrite.
8. **Potions:** current potion paths span Inventory, `BP_PlayerAction_UsePotion`, controller input, Equipment active tool, and HUD/ItemWheel. There may also be `SelectedPotionID`/`CycleSelectedPotion` inventory selection. Resolve which is authoritative before converting to two Medicine slots.
9. **System menu:** existing `WBP_System` is substantive; re-skin/nav-shell integration should not wipe settings/preferences logic.

## 7. Known dependency hotspots

From `asset.json` referencers:

- `AC_EquipmentComponent` is referenced by PlayerBase, EquipmentStats, Inventory, GameMenu, Equipment/Inventory UI, item wheel, potion action and controller.
- `AC_InventoryComponent` is referenced by PlayerBase, the equipment component, inventory and equipment UI, item wheel and HUD, pickups and potion use.
- `EItemCategory` is referenced by many shared components and widgets. Changing enum structure/ordinal values has broad serialized-data consequences.
- `DA_ArmorDefinition` has broad UI/component/player references; avoid deleting it before moving visuals and stat consumers.
- `BP_PlayerBase` has very high fanout to combat, bosses, enemies, controller and UI. Prefer small, isolated changes.

## 8. What still requires live verification

- Correct Gameplay Tag registration and `Weapon.Family.*` mapping in actual project settings.
- Node-level active-weapon switching and equip interruption rules (including block state).
- Current Blueprint compile status, actual runtime UI behavior and any post-export local changes.
- Full inventory/drop/pickup consumption behavior for the final equipped instance.
- Data-only assets and meshes that the exporter omitted.
- Current save/load persistence of item assignments, Souls and story Artifacts.
- Which preview widget and render target is currently active; exact slots/resources.
- Actual implementation of System options and title/main menu flows.

**Audit conclusion:** This is a targeted multi-system contract migration. Most runtime mechanics already exist. The main hazards are cross-component tags, semantic category data, active weapon/tool ownership, derived stat dependencies, menu enum serialization, and hidden PlayerBase hand assumptions.
