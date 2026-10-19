# CodeX — Game Menu foundation, B0: asset contracts only

**Decision:** Approved B0 scope, 2026-10-19. **Replaces the earlier `GameMenuFoundationSpec.md`**, which requested Designer hierarchies, WidgetSwitchers and navigation wiring too early.

## B0 objective and stopping point

Create the **correct Unreal UMG asset shells, package structure and typed data/contracts** for CodeX's Soulslike-inspired Game Menu. **Do not build the visual UI or working navigation.** The user will import relevant SLF UI textures/materials, inspect SLF `W_` widgets and screenshots, and build the visual screens with the assistant one by one in later phases.

The user authorizes creating, compiling and saving the in-scope new UE assets through supported Unreal MCP/editor APIs. Existing working assets are not to be deleted, renamed, reparented, edited or cloned unless a strictly required in-scope dependency is identified and the user agrees. Live editor state is authoritative; repository exports are snapshots.

## Architectural separation

- `AC_InventoryComponent`: owns collected items and storage. `WBP_Inventory`: separate **owned-items browser**, *not* the equipment selection page.
- `AC_EquipmentComponent`: owns `EquippedItemsBySlot` (Gameplay Tag -> ItemID), validates equip/unequip and emits equipment-change events. `WBP_Equipment`: a separate **loadout editor**; its eventual selection subview is inside Equipment, not Inventory.
- `WBP_GameMenu`: eventually hosts sibling Inventory, Equipment, Status and System pages; no Crafting page in this phase.
- Widget shells own **only transient UI state**. Do not duplicate the inventory arrays or equipped-item Map.
- Preserve player combat, CombatStyle, Attunement, locomotion, `ABP_PlayerMaster`, `WBP_PlayerGUI`, `BP_PlayerBase_InventoryPreview` and all existing preview materials, render targets and rotation/capture behavior.
- SLF is a design/UX reference only; do not import or directly depend on SLF Blueprints, UI assets or plugins as part of B0.

## Canonical UE asset paths

Create exactly the missing items below under `/Game/CodeX_Restruct/Game/UI/GameMenu`. Check for existing assets to avoid duplicates.

```text
/Game/CodeX_Restruct/Game/UI/GameMenu/
  WBP_GameMenu                      (new Widget Blueprint)
  WBP_GameMenuButton                (new Widget Blueprint)
  EGameMenuPage                     (new Blueprint enum)
  Inventory/
    WBP_Inventory                   (new Widget Blueprint)
    WBP_InventorySlot               (new Widget Blueprint)
    WBP_InventoryAction             (new Widget Blueprint)
    WBP_InventoryCategoryTab        (new Widget Blueprint)
  Equipment/
    WBP_Equipment                   (new Widget Blueprint)
    WBP_EquipmentSlot               (new Widget Blueprint)
    WBP_EquipmentSelectionPanel     (new Widget Blueprint)
    WBP_EquipmentChoiceEntry        (new Widget Blueprint)
    Data/
      FEquipmentSlotUIData          (new Blueprint struct)
      DT_EquipmentSlots_UI          (new Data Table using FEquipmentSlotUIData)
  Status/
    WBP_Status                      (new Widget Blueprint)
  System/
    WBP_System                      (new Widget Blueprint)
  Shared/
    WBP_ItemDescriptionPanel        (new Widget Blueprint)
    CharacterPreview/
      WBP_CharacterPreviewPanel     (EXISTING, preserve; do not duplicate)
```

Do not create separate Armor, Talisman, Ammunition or Tool-specific Widget Blueprints. `WBP_EquipmentSlot` and `WBP_EquipmentChoiceEntry` are generic reusable types. The existing `/Game/CodeX_Restruct/Frameworks/ItemSystemFramework` is the only Item System framework and stays untouched in B0.

## Designer boundaries

**No Designer implementation**: no panels/grid layout, WidgetSwitchers, `Button`/`Image`/`TextBlock` placement, slots, anchors, sizing, padding, styles, animation, navigation buttons, preview placement, textures or materials. A minimal empty root widget/container is allowed **only if required by Unreal to create and compile the asset**. No UI construction, screenshots, visual theme or placeholder `Coming Soon` text in B0.

Do not create controller bindings (`OpenGameMenu`), runtime `CreateWidget` calls, menu input mode changes, page switcher graphs, gameplay equip calls, inventory query loops, inventory UI filtering or event subscriptions. No PIE functionality is expected from empty shells.

## Data definitions

### `EGameMenuPage` (Blueprint enum)

Create precisely: `Inventory`, `Equipment`, `Status`, `System`. This is **UI-only navigation state**, not `EItemCategory`.

### `FEquipmentSlotUIData` (Blueprint struct)

| Field | UE type | Meaning |
|---|---|---|
| `SlotTag` | Gameplay Tag | Unique equipment-position identifier |
| `SlotLabel` | Text | Display label; *not* the ItemID |
| `SlotGroup` | Gameplay Tag | Optional future UI grouping, separate from map key |
| `EmptyIcon` | Texture2D Object Reference | May be None until art import |
| `GridRow` | Integer | Planned within-group UI row |
| `GridColumn` | Integer | Planned within-group UI column |
| `bEnabled` | Boolean | Slot available for display |

Do not add `InventoryCategory` to this UI struct solely for candidate compatibility filtering; the eventual Equipment/Inventory backend owns item eligibility. Do not add weapon classes, item ownership arrays, equipped ItemIDs or combat state to this struct.

### `DT_EquipmentSlots_UI` (Data Table)

If supported by MCP/editor tools, create the Data Table **using** `FEquipmentSlotUIData` and only **two initial rows**:

| Row name | SlotTag | SlotLabel | GridRow | GridColumn | bEnabled |
|---|---|---|---:|---:|---|
| `RightHand1` | `Equipment.Slot.RightHand1` | `Right Hand 1` | 0 | 0 | true |
| `LeftHand1` | `Equipment.Slot.LeftHand1` | `Left Hand 1` | 1 | 0 | true |

`EmptyIcon = None` for both. Use existing `Equipment.Group.Weapons` for `SlotGroup` **only if the tag already exists**; otherwise leave `SlotGroup` unset and report that the UI group tag can be registered later. Do not invent tag paths or globally modify Gameplay Tags in B0. Confirm both slot tags exist before creating rows. If either is unavailable, create the structure/table shell but report the missing tag rather than fabricate a valid-looking record. Data Table rows are **UI metadata**, not the authoritative `AC_EquipmentComponent.EquippedItemsBySlot`.

Do **not** create guessed future armor, talisman, ammo, additional hand or quick-item rows. Do not rename the current `EItemCategory` or legacy item-table records.

## Minimal widget Blueprint contracts

Create only the following typed variables and dispatchers where supported by the editor API. No event firing/binding and no implementation graphs are required.

| Widget | Variables | Dispatcher(s) |
|---|---|---|
| `WBP_GameMenu` | `ActivePage : EGameMenuPage` | none yet |
| `WBP_GameMenuButton` | `Page : EGameMenuPage` | `OnPageRequested(Page : EGameMenuPage)` |
| `WBP_Inventory` | `SelectedInventoryItemID : Name` | none yet |
| `WBP_InventorySlot` | `ItemID : Name` | `OnInventoryItemPressed(ItemID : Name)` |
| `WBP_Equipment` | `SelectedEquipmentSlot : GameplayTag` | none yet |
| `WBP_EquipmentSlot` | `SlotTag : GameplayTag` | `OnEquipmentSlotPressed(SlotTag : GameplayTag)` |
| `WBP_EquipmentSelectionPanel` | `RequestedSlotTag : GameplayTag` | `OnItemChosen(ItemID : Name)`, `OnUnequipRequested()`, `OnSelectionCancelled()` |
| `WBP_EquipmentChoiceEntry` | `ItemID : Name` | `OnChoicePressed(ItemID : Name)` |
| `WBP_InventoryAction`, `WBP_InventoryCategoryTab`, `WBP_ItemDescriptionPanel`, `WBP_Status`, `WBP_System` | no additional variables required in B0 | none required in B0 |

Keep these data contracts strongly typed. Do not add speculative actions, enums, functions or stub business logic. When additional contracts become necessary during the individual widget build, define them then.

## Execution and verification

1. Read `AGENTS.md`, `Docs/CodeXContext/InventoryAndUI.md` and **this updated document**; do not perform a full unrelated project audit.
2. Inspect the **live** `/Game/CodeX_Restruct/Game/UI/GameMenu` paths and determine which assets already exist. Review supported MCP creation, Blueprint variable/dispatcher, struct, enum and Data Table APIs.
3. Create only missing in-scope assets and the typed data/contracts above. No duplicates or SLF asset imports.
4. Compile and save each new widget/enum/struct where supported; verify Data Table schema and row contents through editor queries. If a required operation is unsupported, **do not simulate it by writing export JSON, T3D or `.uasset` files to disk**. Report exactly what was created and what must be done manually.
5. Return a concise inventory of created asset paths, data schema/rows, Blueprint compilation and save results, and any blockers.

**B0 acceptance:** all feasible asset shells exist in the agreed package locations, the enum/struct/table metadata is correct, core UI-only variables and dispatchers are typed, and no actual Game Menu visual layouts or gameplay navigation/equipping behavior has been built. **Stop here.**

## Subsequent phases (NOT authorized as part of B0)

- **B1**: bring SLF visual references/textures/materials into the project by user-controlled selection/import; inspect SLF `W_Inventory`, `W_Equipment`, `W_GameMenu` and screenshots.
- **B2+**: build the actual widget Designer hierarchies **one widget at a time**, review styling, add interactions and later integrate Inventory/Equipment components and controller input.
