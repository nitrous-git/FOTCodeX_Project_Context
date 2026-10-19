# CodeX — Codex project instructions

**UE 5.8.3 · Blueprint-first · Updated October 19, 2026**

This repository contains architecture notes and exported Blueprint snapshots for the **live Unreal Editor** project. The live assets accessed through Unreal MCP are authoritative; exported JSON/T3D may lag local changes and must never be edited as a substitute for `.uasset` operations.

## Operating mode and permissions

- For an explicit development request, perform routine in-scope live Unreal work: inspect, create, edit, compile, save and run targeted checks. Do **not** seek approval for every node, widget, compile or save.
- Planning/audit requests are read-only unless the user authorizes implementation. Ask before broad destructive actions or modifications outside the task.
- **UI work:** read **`Docs/CodeXContext/UIDevelopment.md` once per working session**, then inspect only the target live widgets and directly used assets. Reuse findings across follow-up tasks. **Do not automatically reread** `InventoryAndUI.md`, `GameMenuFoundationSpec.md`, old SLF audits, `CurrentState.md` or every Blueprint export. Open a deeper document only when a specific unanswered dependency or gameplay contract requires it.
- **Non-UI work:** use `Docs/CodeXContext/README.md` to select the relevant subsystem guide; inspect dependencies proportionally to the risk.
- Existing functionality is valuable: preserve CodeX Player CombatStyle/Attunement/ABP, Enemy AI, Boss Framework, traversal, and permanent gameplay HUD unless the task specifically targets them.
- Be resource-conscious: make a short focused plan, act on the smallest useful asset set, avoid repeated global searches/full graph dumps, and provide concise completion reports.

## Current Item / Game Menu architecture

- One `/Game/CodeX_Restruct/Frameworks/ItemSystemFramework/` contains Inventory, Equipment, Items and Pickups. Do not reintroduce `InventorySystemFramework/InventorySystemRefactor`.
- `AC_InventoryComponent` owns **collected items and quantities**; `AC_EquipmentComponent` owns **equipped slot assignments** and equip/unequip operations. Widgets own neither storage collection.
- `/Game/CodeX_Restruct/Game/UI/GameMenu/` has CodeX-native `WBP_GameMenu`, `WBP_GameMenuButton` and sibling pages `WBP_Inventory`, `WBP_Equipment`, `WBP_Status`, `WBP_System`. Exactly **four** top-level pages—no Crafting.
- Inventory is the item browser. Equipment is the loadout editor with a separate internal compatible-item selection view. `SelectedEquipmentSlot` is UI state in `WBP_Equipment`.
- The B0 widget shells, enum, typed dispatchers and UI slot metadata already exist. **Reuse; do not recreate or clone legacy Inventory UI.**
- Source SLF widgets under `/Game/SoulslikeFramework/` are **references only**. Only CodeX-owned UI visual assets from `/Game/CodeX_Restruct/External/AssetContent/UI/SLF/` may be used in reconstructed CodeX widgets; do not copy SLF navigation/controller/gameplay Blueprint dependencies.

## Tools and validation

- Use actual Unreal MCP/editor asset APIs; do not move/rename `.uasset` using filesystem operations. Inspect tool capabilities before relying on an unsupported editor action.
- Compile/save changed Blueprints and perform focused visual/interaction checks when supported. Do not claim PIE, screenshot verification or clean packages without evidence.
- Report only: assets changed, functionality/design implemented, compile/save status, notable blockers. Avoid long routine audits and documentation rewrites unless requested.

## Further reading on demand

`UIDevelopment.md` = compact UI guide; `InventoryAndUI.md` = deeper domain contracts; `DevelopmentRules.md` = broader permissions; `PlayerCombat.md`, `EnemyFramework.md`, `BossFramework.md` = subsystem-specific details. The historical B0 and B1.1 audit documents are **optional references, not mandatory startup reading**.
