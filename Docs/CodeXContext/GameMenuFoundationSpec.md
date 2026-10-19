# CodeX — Game Menu UI Foundation: MCP Build Specification

**Status:** Approved **design contract** for the next Unreal MCP assignment; widget creation **not yet confirmed** in the 2026-10-19 GitHub export. Refer to the live Unreal project before edits. Read `InventoryAndUI.md` for full system semantics.

## Task objective

Create **clean, reusable, compile-safe UMG foundation assets** under `/Game/CodeX_Restruct/Game/UI/GameMenu/` with consistent naming, correct page separation and typed event contracts. This is a **base creation task**, not the final implemented inventory/equipment menu. The previous inventory HUD/widgets and direct-equip UI were intentionally removed: do not duplicate, restore or derive from them.

The user approved live MCP creation/modification of in-scope assets. Discover what UMG Designer and Blueprint tools can actually do; compile/save and report supported validation. For unsupported operations provide a concise exact blocker and manual steps; do **not** claim creation from a JSON export.

## Architectural rules

- Root `WBP_GameMenu` switches sibling pages Inventory, Equipment, Status and System. **No crafting** in initial build.
- `WBP_Inventory` is for browsing items owned by `AC_InventoryComponent` (inventory slot grid, category tabs, item detail/actions). Clicking an owned item must **not automatically equip it**.
- `WBP_Equipment` displays equipment position slots and a **second internal choice view**. Clicking `WBP_EquipmentSlot` switches **within Equipment** to the compatible-item selection subview, not to `WBP_Inventory`.
- All equipped assignments stay in `AC_EquipmentComponent.EquippedItemsBySlot`. All owned items stay in `AC_InventoryComponent`. Widgets have only transient presentation state.
- `WBP_EquipmentSlot` is generic; the initial displayed slots are `Equipment.Slot.RightHand1`, `Equipment.Slot.LeftHand1`. UI design must admit additional hands, four armor pieces, talismans and quick tools **without creating them all now**.
- Existing Player Framework, `WBP_PlayerGUI`, CombatStyle, Attunement/ABP, and preview actor/material/rotation are outside the edit scope. Reuse existing `Shared/CharacterPreview/WBP_CharacterPreviewPanel` rather than creating a `_v2` or duplicate actor.
- Use **one** `ItemSystemFramework`, no new top-level Inventory/Equipment/GameMenu frameworks.

## Required asset list and locations

```text
/Game/CodeX_Restruct/Game/UI/GameMenu/
  WBP_GameMenu                 # parent page navigation
  WBP_GameMenuButton           # generic nav item
  EGameMenuPage                # Inventory, Equipment, Status, System
  Inventory/
    WBP_Inventory
    WBP_InventorySlot
    WBP_InventoryAction
    WBP_InventoryCategoryTab   # only if necessary for clean category navigation
  Equipment/
    WBP_Equipment
    WBP_EquipmentSlot
    WBP_EquipmentSelectionPanel
    WBP_EquipmentChoiceEntry
    Data/
      FEquipmentSlotUIData     # if UDS creation is supported
      DT_EquipmentSlots_UI     # optional initial 2-row metadata (supported tools only)
  Status/WBP_Status
  System/WBP_System
  Shared/
    WBP_ItemDescriptionPanel
    CharacterPreview/
      WBP_CharacterPreviewPanel  # already exists, PRESERVE
```

Do **not** make placeholder assets for every category (e.g., separate armor-slot widgets or 10 quick-tool widgets). Reuse a generic slot and generic candidate entry.

## Expected Designer hierarchy

Exact panel names should be stable to simplify later guidance; use appropriate Slot wrappers, padding and responsive sizing instead of absolute Canvas offsets for every entry.

```text
WBP_GameMenu
  Root overlay / border
    HorizontalBox_Main
      VerticalBox_Navigation
        GameMenuButton_Inventory
        GameMenuButton_Equipment
        GameMenuButton_Status
        GameMenuButton_System
      WidgetSwitcher_Pages
        [0] WBP_Inventory
        [1] WBP_Equipment
        [2] WBP_Status
        [3] WBP_System

WBP_Inventory
  Root
    Category selector / tabs
    ScrollBox / uniform or wrap item grid container
    Selected item information area (WBP_ItemDescriptionPanel)
    Contextual action area (future WBP_InventoryAction)

WBP_Equipment
  Root
    WidgetSwitcher_Equipment
      [0] EquipmentSlotsView
        grouped weapon, armor, talisman and quick-item containers
        [initially instantiate only RightHand1 and LeftHand1]
      [1] WBP_EquipmentSelectionPanel
        ScrollBox_CompatibleItems
        Button_Unequip
        Button_Back
    Equipment item description area (reuse WBP_ItemDescriptionPanel)

WBP_EquipmentSlot
  Button_Slot
    Overlay with Image_EmptyIcon, Image_ItemIcon, optional Text_SlotLabel

WBP_EquipmentChoiceEntry
  Button_Choice
    Item icon, name, optional count/current-equipped indicator

WBP_Status and WBP_System
  Minimal named roots with neutral 'Coming later' placeholder content
```

The Game Menu preview (if placed in the foundation layout) should be an existing reusable panel, with **no new preview actor spawning logic or lighting changes** in this stage. Existing preview may remain unused until integration.

## Proposed minimal Blueprint contracts

Prefer event dispatchers and explicit initialization functions; implement only what is needed for the widget tree to compile.

| Widget | UI-only variable(s) | Event / contract |
|---|---|---|
| `WBP_GameMenu` | `ActivePage : EGameMenuPage` | `ShowPage(EGameMenuPage)` sets page switcher index; don't change controller input yet |
| `WBP_GameMenuButton` | `Page : EGameMenuPage` | `OnPageRequested(Page)` dispatcher |
| `WBP_Inventory` | `SelectedInventoryItemID : Name`, optional `SelectedCategory` | Selection event placeholder; no auto-equip |
| `WBP_InventorySlot` | `ItemID : Name` | `OnInventoryItemPressed(ItemID)` dispatcher |
| `WBP_Equipment` | `SelectedEquipmentSlot : GameplayTag` | `HandleEquipmentSlotPressed(Tag)` stores tag and opens subview; subview Back returns to slots |
| `WBP_EquipmentSlot` | `SlotTag : GameplayTag` | `OnEquipmentSlotPressed(SlotTag)` dispatcher |
| `WBP_EquipmentSelectionPanel` | `RequestedSlotTag : GameplayTag` | `OnItemChosen(ItemID)` / `OnUnequipRequested` / `OnSelectionCancelled` |
| `WBP_EquipmentChoiceEntry` | `ItemID : Name` | `OnChoicePressed(ItemID)` dispatcher |
| `WBP_ItemDescriptionPanel` | no authoritative selection | Basic `SetItemDetails` / display placeholder when no selection |

No Blueprint `TryEquipItemToSlot` calls, dynamic inventory filtering, no Data Table enumeration migration, no controller input changes, and no general gameplay interaction in this **foundation** pass. The future implementation connects the selector to `AC_EquipmentComponent` and Inventory ownership/compatibility queries.

### Optional two-row UI slot metadata

Only if editor tools support real structure/Data Table creation: `FEquipmentSlotUIData` fields `SlotTag` (GameplayTag), `SlotLabel` (Text), `SlotGroup` (GameplayTag), `EmptyIcon` (Texture2D reference), `GridRow` (Int), `GridColumn` (Int), `bEnabled` (Bool). `DT_EquipmentSlots_UI` initial entries are RightHand1 and LeftHand1. Slot UI metadata is not the authoritative loadout Map. Category filtering belongs in backend queries, **not** solely to `InventoryCategory` hardcoded inside the selector.

Do not create invalid/unusable empty Data Tables merely to satisfy a checklist; report unsupported operations explicitly.

## Visual specification (foundation only)

- Professional, restrained dark-fantasy / Soulslike visual language, consistent typography, subtle separators, dark translucent surfaces; leave final polish for later iteration.
- Keep page navigation obvious, separate owned inventory browsing from interactive loadout editing, and preserve room for character preview.
- Use reusable buttons/entries and content containers; don't copy SLF's implementation or create a huge single `WBP_GameMenu` EventGraph.
- Avoid expensive animated effects, tick-driven refresh, persistent preview captures, new render targets or experimental materials in the foundation pass.

## Build phases and acceptance

1. **Inspect live:** verify current paths/components and preserved preview asset, and discover supported editor widget APIs; no full unrelated project audit.
2. **Create:** all feasible in-scope folders, Widget Blueprints, enum and minimal Designer hierarchy. Reuse existing Shared preview.
3. **Wire only UI navigation:** page buttons drive `WidgetSwitcher_Pages`; equipment slot press switches Equipment's internal view; Back restores it. Default page = Inventory; default equipment subview = slots. Item equip action stays unwired.
4. **Compile/save:** all created Blueprints that editor tools can compile; report individual failures and unsupported UMG operations.
5. **Report:** exact created/modified asset paths, hierarchy achieved, compile/save results, and remaining manual items. No PIE or production integration claims unless actually performed.

**Stop point:** The user can open `WBP_GameMenu` Designer and see an Inventory/Equipment/Status/System page layout; clicking navigation in a relevant widget test (if supported) switches pages; Equipment has independent Slots and Selection views. There is **no duplicated authoritative state and no old Inventory HUD**. Later tasks implement inventory data population, item matching, actual equip/unequip, controller opening/closing, preview synchronization and full polish.
