# CodeX — Current state and export provenance

**Updated:** October 19, 2026. This page is a historical checkpoint, not a guarantee of live editor state. Do not preload it for every task; consult it for migration context, tested-vs-planned status, or export provenance.

## Latest inspected repository snapshot

| Property | Value |
|---|---|
| Repository | `nitrous-git/FOTCodeX_Project_Context` (`main`) |
| GitHub commit inspected | `89cb1449543df9d25465d194e6b1c7e575158cc6` — `Inventory - Equipment Refactor Reset` |
| Export generated | `2026-10-19T04:37:07.790582+00:00` |
| Live project engine reported / exported | Unreal Engine `5.8.3-58210709+++UE5+Release-5.8` |
| Exporter / schema | `0.4.0` / `1` |
| Content root | `/Game/CodeX_Restruct` |
| Asset scan | `6,058` |
| Export candidates / exported | `295` / `295` |
| Full-graph / structural | `212` / `83` |
| Authored / generated graphs | `712` / `819` |
| Variables / nodes / pins / links | `1,376` / `16,655` / `54,874` / `17,052` |
| Dependency edges | `917` |
| Metadata errors / graph errors | `0` / `0` |
| Graph parse warning assets | `11` (known raw T3D vs parsed pin-count discrepancies, see `export_manifest.json` / `ExportGuide.md`) |

These metrics describe the export's snapshot, not current in-editor compilation or gameplay validation.

## Item System / Game Menu — latest milestone

**Architecture explicitly approved October 18–19:** a single `/Frameworks/ItemSystemFramework` containing `Inventory`, `Equipment`, `Items` and `Pickups`, with menu widgets under `/Game/UI/GameMenu` rather than more framework roots. SLF-style semantic separation is adopted; SLF code is not imported as the new system.

### Verified in the published export

- `AC_InventoryComponent` in `Frameworks/ItemSystemFramework/Inventory/` owns inventory arrays, item records/queries and `HasItem`.
- `AC_EquipmentComponent` in `Frameworks/ItemSystemFramework/Equipment/` owns `EquippedItemsBySlot : Map<GameplayTag, Name>`, equip/unequip/lookup/resolve functions and `OnEquipmentSlotChanged(EquipmentSlot, PreviousItemID, NewItemID)`.
- `BP_PlayerBase.SetCombatStyleWeapon` uses the Equipment Component's `ResolveEquippedWeaponClass` for the two first hand slots; physical weapon spawn/attachment remains in the player.
- Existing definition assets: `DA_WeaponDefinition`, Knight Sword, Iron Sword, Knight Shield; item table/struct and pickup actors have been moved into the consolidated framework.
- Previous `InventorySystemFramework/InventorySystemRefactor` path and old inventory Widget Blueprints do **not** appear in the exported asset tree. Retained preview widget is under `Game/UI/GameMenu/Shared/CharacterPreview/`.
- `BP_CPC_PlayerController` no longer exports the former `OpenInventory` / `CloseInventory` / `ToggleInventory` functions or old HUD variables.

### Subsequent local work — not yet published in this export

The user explicitly reports completing **additional final Blueprint cleanup locally** after the above export. In the *exported* Inventory Component, the old `EquipItemFromSlot`, `FocusSlot`, `FocusedCategory`, `FocusedItemID`, `FocusedSlotIndex`, `OwnerPlayer`, `OnEquipmentChanged`, and `OnFocusedItemChanged` were still present. Their continued presence in GitHub **must not be mistaken for the current live state**. A refreshed export or focused MCP query can confirm when necessary; do not recreate them.

The user also reports having finished A1–A4: component split, player integration, old-menu disconnection, retained-asset relocation and UI cleanup. Earlier W3 PIE tests confirmed independent sword swapping before the split. A full **post-cleanup** PIE regression and any changes made after the export must be reported as locally verified only if an actual test is performed; don't fabricate a test pass.

### Next implementation milestone (not yet built)

MCP agent creates the **base Game Menu framework UI**:

- Root `WBP_GameMenu`, navigation `WBP_GameMenuButton` and page-switch contract.
- Separate `WBP_Inventory` and `WBP_Equipment` sibling pages.
- Equipment's internal two-view switcher, reusable `WBP_EquipmentSlot`, `WBP_EquipmentSelectionPanel`, `WBP_EquipmentChoiceEntry`.
- Shared item description and existing preview asset reuse; Status/System placeholder pages.
- Only minimal typed variables / event dispatchers and compile-safe Designer hierarchy. The first milestone is **foundation**, not full inventory actions, final polish, or arbitrary player combat changes.

See `InventoryAndUI.md` for authoritative design and `GameMenuFoundationSpec.md` for the next agent scope.

## Other framework continuity

- **Player:** keep `BP_PlayerBase`, CombatStyle, Attunement, locomotion, `ABP_PlayerMaster`, weapon actors, and the existing gameplay HUD. Weapon physical classes are decoupled from choosing a general CombatStyle in the first-hand slice; other combinations remain future work.
- **Enemies:** ordinary enemy modular awareness, WorldBehavior, melee/ranged styles and group coordination remain in their existing framework. No change requested in this migration.
- **Bosses:** keep `BP_BaseBoss`, boss-specific state/Director/Metrics, awareness/area activation and UI; do not route bosses through ordinary enemy attacker tokens by default.
- **MCP:** live inspection was previously validated on `BP_Pickup_HPPotion_C_1`. Graph editing, UMG Designer manipulation, compiling/saving and PIE must each be confirmed via the *actual* connected tools during the next task.

## Source-of-truth order

1. Running Unreal Editor through Unreal MCP for current assets, compile status and actual functionality.
2. Explicit current user decisions / local work report (do not falsely claim independent verification).
3. Current architecture documents for agreed design.
4. GitHub export as a read-only point-in-time reference.

Do not patch `graphs/*.json`, T3D or exported manifests to simulate `.uasset` modifications. User pushes a refreshed export after local work.
