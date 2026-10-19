# CodeX — Current state and snapshot provenance

**Updated:** 2026-10-19 (Game Menu B0 complete; B1.1 audit next). This file is a checkpoint, not evidence that every later local asset is included in GitHub. The live UE editor/MCP is authoritative.

## Latest inspected GitHub export

| Item | Value |
|---|---|
| Repository | `nitrous-git/FOTCodeX_Project_Context`, `main` |
| Export generated (UTC) | `2026-10-19T05:47:52.985949+00:00` |
| Exporter / schema | `0.4.0` / `1` |
| Engine | `5.8.3-58210709+++UE5+Release-5.8` |
| Root | `/Game/CodeX_Restruct` |
| Candidates / exported | `311` / `311` |
| Full-graph / structural | `225` / `86` |
| Metadata / graph errors | `0` / `0` |
| Graph warning assets | `11` parser pin-count discrepancies; see `export_manifest.json` |

These describe an export, not compilation or runtime results.

## Agreed architecture

The former `InventorySystemFramework/InventorySystemRefactor` structure has been retired. The consolidated `/Game/CodeX_Restruct/Frameworks/ItemSystemFramework/` houses Inventory, Equipment, Items and Pickups. `AC_InventoryComponent` owns collected items; `AC_EquipmentComponent` alone owns `EquippedItemsBySlot` and equipping operations. The Player retains physical weapon spawning and its CombatStyle/Attunement systems. The old Player Inventory HUD was removed and replaced with a new UI architecture at `/Game/CodeX_Restruct/Game/UI/GameMenu/`.

### B0 Game Menu foundation — COMPLETED

User supplied the live MCP completion report, and the updated GitHub Blueprint export contains the 16 new B0 assets:

```text
WBP_GameMenu; WBP_GameMenuButton; EGameMenuPage
Inventory/WBP_Inventory; WBP_InventorySlot; WBP_InventoryAction; WBP_InventoryCategoryTab
Equipment/WBP_Equipment; WBP_EquipmentSlot; WBP_EquipmentSelectionPanel; WBP_EquipmentChoiceEntry
Equipment/Data/FEquipmentSlotUIData; DT_EquipmentSlots_UI
Status/WBP_Status
System/WBP_System
Shared/WBP_ItemDescriptionPanel
```

- Live MCP reported 13 compiled Widget Blueprints, all 16 assets saved, and Designer trees with zero elements. No page navigation or gameplay binding was implemented. The GitHub export confirms the foundation assets/interfaces; this document does **not** claim an independent live recompile.
- `EGameMenuPage`: Inventory, Equipment, Status, System.
- `DT_EquipmentSlots_UI`: two rows with valid `Equipment.Slot.RightHand1` / `Equipment.Slot.LeftHand1` position tags and empty icon references. At B0, both `SlotGroup` fields were unset (no `Equipment.Group.Weapons` tag yet). Do not register it as part of B1.1.
- `WBP_GameMenu` and `WBP_GameMenuButton` exist as empty shells with the approved typed `ActivePage`, `Page` and `OnPageRequested(Page)` contracts. **Do not recreate.**

### SLF resource preparation — USER REPORTED COMPLETE

The user has copied/moved selected SLF visual resources (textures, materials and fonts) into `/Game/CodeX_Restruct/External/AssetContent/UI/SLF/` and checked that required materials/material functions depend only on CodeX-owned resources. The original reference content remains under `/Game/SoulslikeFramework/Widgets/`. The Blueprint exporter does not necessarily enumerate those material/texture/font assets; a live read-only MCP query should verify **exact** copied paths before they are used in CodeX widgets.

### B1.1 — NEXT, READ-ONLY

Inspect original SLF reference widget assets:

- `/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu`
- `/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu_Button`

Document widget trees, layout/style, dynamic selection states, button interactions/animations and exact visual dependencies; compare with already-copied CodeX resources. No copying, migration, Blueprint modifications, UI Designer build or compilation in B1.1. See `GameMenuReferenceAudit.md`.

### Later milestones, NOT authorized by this audit

- B1.2, if required: address specifically missing copied visual resources after reviewing audit.
- B2: build CodeX `WBP_GameMenuButton` first, then `WBP_GameMenu`, using independent CodeX visual asset references and only four pages (no Crafting).
- Later: actual page layouts, Input/Controller integration, equipment slot matching and inventory actions, preview synchronization and polish.

## Other framework boundaries

Enemy WorldBehavior/awareness and melee/ranged components, manager/group combat, authored boss activation and Director logic, traversal systems, Player locomotion/ABP, weapon Actors and permanent gameplay HUD remain outside this UI audit. Do not change them in B1.1.

## Provenance rule

Priority order: live UE Editor/MCP > latest explicit user-reported local changes > accepted architecture docs > dated GitHub export. Use the export for offline Blueprint graph reference and known past state; do not change export JSON to mimic `.uasset` edits. Changes to assets must be made through authorized live Unreal tools in a separate implementation phase.
