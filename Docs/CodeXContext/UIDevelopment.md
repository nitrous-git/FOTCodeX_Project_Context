# CodeX — Lightweight UI Development Guide

**Stable operating guide · October 19, 2026 · UE 5.8.3 Blueprint/UMG**

**Purpose:** Give Unreal MCP enough design and architecture context to **build** useful UI with its own judgment, without expensive repeat audits, rigid pixel-by-pixel specifications or rewriting documentation each iteration. A current user prompt/screenshot and the live target widgets determine the actual feature scope. Read this guide **once per working session** and reuse it during follow-up widget tasks.

## 1. The established foundation — already created

All widgets below are CodeX-owned assets under:
`/Game/CodeX_Restruct/Game/UI/GameMenu/`

```text
WBP_GameMenu                         # root four-page menu
WBP_GameMenuButton                   # reusable navigation item
EGameMenuPage                        # Inventory, Equipment, Status, System
Inventory/
  WBP_Inventory                      # owned-item browser
  WBP_InventorySlot
  WBP_InventoryAction
  WBP_InventoryCategoryTab
Equipment/
  WBP_Equipment                      # equipped loadout editor
  WBP_EquipmentSlot
  WBP_EquipmentSelectionPanel
  WBP_EquipmentChoiceEntry
  Data/FEquipmentSlotUIData
  Data/DT_EquipmentSlots_UI
Status/WBP_Status
System/WBP_System
Shared/WBP_ItemDescriptionPanel
Shared/CharacterPreview/WBP_CharacterPreviewPanel  # retained, preserve
```

**B0 complete:** the 13 Widget Blueprints, enum, struct and Data Table were created and validated. The widgets began as *empty Designer shells*; later construction may already have occurred, so inspect live before making changes. Preserve typed foundation variables and event dispatchers—don't replace assets, reset Designer trees or re-create the B0 foundation.

**B1.1 complete:** live SLF Game Menu/button hierarchy, basic styles, dependencies and interaction flow were audited. This audit is available if a targeted property is missing; **do not rerun its full inspection for every UI task**.

## 2. The UI's meaning (non-negotiable)

| Screen | Function | Gameplay authority |
|---|---|---|
| `WBP_GameMenu` | Navigate four sibling pages: Inventory, Equipment, Status, System | Neither item inventory nor equipment state |
| `WBP_Inventory` | Browse all *owned* items by category, display description, contextual inventory actions | Query `AC_InventoryComponent` |
| `WBP_Equipment` | View and edit equipped loadout positions; clicking a slot opens an **internal** eligible-item picker | Query/call `AC_EquipmentComponent` and Inventory-owned item queries |
| `WBP_Status` | Show player attributes and derived stats | Player/stat systems (future work) |
| `WBP_System` | Game/system settings | Relevant settings systems (future work) |

**Do not** make inventory item clicks directly equip items. **Do not** use `WBP_Inventory` as the Equipment picker. Equipment owns a local `SelectedEquipmentSlot : GameplayTag` and switches between a slot overview and its own `WBP_EquipmentSelectionPanel`.

## 3. Backend and reusable contracts

- `AC_InventoryComponent` stores inventory item collections/quantities and exposes item definitions and ownership queries.
- `AC_EquipmentComponent` alone owns `EquippedItemsBySlot : Map<GameplayTag, ItemID>`, validates `TryEquipItemToSlot`, handles `UnequipItemFromSlot` and broadcasts `OnEquipmentSlotChanged`.
- Player logic handles spawning/attaching physical weapon actors, CombatStyle animations, Attunement and combat state. **UMG never spawns weapon actors or maintains its own authoritative inventory map.**
- Initial enabled equipment tags: `Equipment.Slot.RightHand1` and `Equipment.Slot.LeftHand1`. `DT_EquipmentSlots_UI` contains those two rows; `SlotGroup` may be unset until the optional UI group tag is registered. Keep slot UI generic for additional hand slots, four armor pieces, talismans, tools and ammunition later—don't implement them without a task.
- Existing B0 button contract: `WBP_GameMenuButton.Page : EGameMenuPage`, `OnPageRequested(Page)`; parent: `WBP_GameMenu.ActivePage : EGameMenuPage`. Preserve and use these rather than SLF navigation interfaces.
- Reusable widgets should communicate via narrow initialization APIs and event dispatchers. Avoid Tick-based polling for UI refresh; prefer explicit events and state updates.

## 4. Visual direction and asset library

**Reference:** original SLF widgets under `/Game/SoulslikeFramework/Widgets/` (not CodeX runtime dependencies). User screenshots and direct user feedback take priority over exact SLF clone fidelity.

**Already-prepared CodeX visual assets:**
`/Game/CodeX_Restruct/External/AssetContent/UI/SLF/`

The user migrated textures, materials and fonts and checked internal material/material-function dependencies. Live inspection is allowed when choosing an actual brush/font, but **do not recopy the full SLF library or overwrite migrated assets**. CodeX UI resources should reference the CodeX copies, not originals under `/Game/SoulslikeFramework/`. For missing resources, report precisely what's missing instead of inventing asset paths.

**Desired style:** sober dark-fantasy / Soulslike, clean typography, desaturated bronze/gold highlights, rich dark translucent surfaces, restrained gradients and tasteful fading panels. Use consistent button selected/hover states, spacing and scalable responsive UMG layout. A screenshot is the **primary visual target** where provided. The SLF menu uses Liberation Serif and fading-border materials; match visual intent without reproducing fragile SLF graph logic or extreme layout offsets.

Favor reusable `WBP_GameMenuButton`, `WBP_InventorySlot`, `WBP_EquipmentSlot` and choice-entry components. For Game Menu there are **four pages (no Crafting)**. Focus/hover highlighting must not inadvertently change the active page unless the user requests it. Prefer one native clickable button containing icon and label, with a separate selected-state visual.

Existing character preview rendering/camera/rotation is valuable. Preserve its captured transparency, aspect/lighting work and the permanent `WBP_PlayerGUI`; modify them only when a task explicitly requires it.

## 5. Working method — minimal context / maximum implementation

1. Read **this guide once** for the working session; apply user-provided screenshot, latest instructions and the *current* live Widget Blueprint state.
2. Inspect **only** the named widget(s), directly needed SLF reference/UI resource(s), and immediate dependency graphs. No project-wide Blueprint audit, document regeneration, export scan or broad asset inventory by default.
3. For a normal design/build request, make sensible Blueprint Designer/layout/styling choices **autonomously**. Use known CodeX variables/dispatchers; implement the requested interaction without asking for node-by-node approval.
4. Build one cohesive widget or closely related pair per task. Compile and save changes, then verify the widget tree, resource references and the requested visual/interaction behavior as tools allow. Don't state a visual match without actual viewport/preview verification.
5. Return a **brief** summary: changed assets, main UI behavior/visual decisions, compile/save result, and only genuine blockers or requested screenshots.
6. For follow-up corrections, edit the existing widget **incrementally**. Do not repeat the initial audit or reread documentation, unless the agent session is new or a specific contract has changed.

**Inspection safety:** MCP `read_graph_dsl` previously dirtied SLF reference packages while attempting a supposedly read-only graph query. Avoid it for read-only SLF inspections until fixed. Do not save unexpected unsaved changes to the original SLF sources.

**Escalate only for risk:** broad asset deletion/migration, shared item/category schema changes, combat/Attunement/player input rewrites, or new runtime dependencies. For such tasks, inspect the relevant deeper docs and callers first. Routine widget styling/Designer work is explicitly authorized when requested.

## 6. Current development sequence

- **Next:** build/review `WBP_GameMenuButton` and `WBP_GameMenu` using the user's SLF screenshot and the prepared CodeX assets. Agent can propose reasonable layout/design improvements while respecting the four-page and B0 event contracts.
- **Then:** construct `WBP_Inventory` item browse/actions, `WBP_Equipment` slot overview and internal selection panel, one screen at a time. Add backend interaction only in the corresponding scoped implementation task.
- **Later:** Status/System detail, controller open/close/input, keyboard/gamepad navigation, animation/polish and richer equipment categories. Don't build them simply because they are described here.

**Deep references (optional):** `InventoryAndUI.md` for detailed model semantics; `GameMenuFoundationSpec.md` for original B0 signatures; previous B1.1 audit for precise SLF dimensions/material samples; `PlayerCombat.md` for player weapon/Attunement integrations. Do not load all of them by default.
