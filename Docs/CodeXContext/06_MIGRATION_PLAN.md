# 06 — Migration plan, dependency order and checkpoints

**Status: PLANNED, not yet executed.** This is the technical conversion order and a review framework, **not an MCP implementation prompt**. The model is intentionally separated from the later agent task so it can be reviewed and revised first.

## Success definition

At migration completion, CodeX runs from its existing UE project with:

- One complete Outfit selection, two switchable weapon kits, one Talisman, one global Fire/Light/Bleed Attunement and two typed Medicine assignments.
- One weapon cycle and one Medicine selection on HUD; no separate hand slots, five-tool wheel, or equip-able independent shield.
- New five-page GameMenu and sober visual style; all six equipment rows and Character outfit page function.
- Existing attacks/guard/weapon families, Shadow Strike, enemy/boss encounters, HP/EN/stamina, potion montage, HUD, controller and rest/death flows survive.
- Manual stat allocation and time-of-day rest options are no longer accessible in CodeX.
- All changed Blueprints compiled/saved; the user manually verifies PIE for each accepted stage.

**Do not measure success by deleting the most assets.** Keep internal classes/functions when they are still useful and rename only when the downstream risk is justified.

## C0 — Baseline and local risk audit

**Scope:** read-only, no gameplay edits.

1. Confirm the live editor UE version, actual package/asset paths and current branch/project identity.
2. Inspect only affected live owners: `AC_EquipmentComponent`, `AC_InventoryComponent`, `AC_EquipmentStatsComponent`, `BP_PlayerBase`, `AC_PlayerAttunementComponent`, potion action, `WBP_ItemWheelGrid`, `WBP_GameMenu`, `WBP_Equipment`, `WBP_Inventory`, `WBP_RestMenu`, controller.
3. Inspect current registered Gameplay Tags, all `DT_EquipmentSlots_UI` rows and item-table categories, relevant default items/save profiles.
4. Check existing runtime preview method and whether preview actors duplicate PlayerBase equipment state.
5. Review direct `Find References` for old tags, current `ActiveRightHandSlot`, `ActiveToolSlot`, category enum, armor and menu enum.
6. Export/commit a baseline and retain a rollback branch/checkpoint before structural schema changes. Do not delete legacy assets yet.

**Deliverable at this stage:** a short confirmed delta against `02_EXPORT_AUDIT.md`, including any post-export differences. No architecture rewrite until high-fanout dependencies are understood.

## C1 — Introduce target equipment contracts while retaining old consumers

**Affected primary asset:** `AC_EquipmentComponent` plus target Gameplay Tags/compatible definitions.

**Desired changes:**

- Introduce canonical equipment tags for `Weapon.Primary`, `Weapon.Secondary`, `Outfit`, `Talisman`, `Attunement`, `Medicine.HP`, `Medicine.EN`.
- Add/normalize active weapon state and two-option Medicine selection. Preserve the existing Map, inventory ownership gate and slot-change dispatcher semantics.
- Expose stable queries: active weapon ItemID, assigned item by slot, legal slot category, cycle among occupied weapon slots, medicine selection state.
- Implement deterministic fallback when an active weapon is removed, and reject invalid duplicate/incompatible assignments.
- Preserve current right-hand/tool mechanisms as temporary adapters where required by unchanged consumers; they must not remain final authoritative state.
- Make original active-hand/tool dispatchers and new active-weapon/medicine events consistent during transition, then migrate subscribers.

**Checkpoint:** compilation and targeted internal state checks; no Player/UMG refactor claimed finished yet. Verify there is only **one** authoritative active weapon state after transition layers are removed.

**Avoid:** replacing the entire component, changing unrelated Inventory storage semantics, adding new save serialization without auditing it.

## C2 — Data and definition conversion

**Affected:** `EItemCategory`, `FInventoryItemDefinition`, `DA_WeaponDefinition`, `DA_ArmorDefinition` successors, `DT_InventoryItemData`, `DT_EquipmentSlots_UI`, new Outfit/Talisman definitions, Attunement definitions.

**Desired changes:**

- Introduce one logical Outfit data definition; map selected legacy visual assets into test ensemble(s). Do not mass-delete source pieces.
- Distinguish real passive Talismans from existing `Talisman_Fire/Neutral/Frost/Poison/Electric` which point to `DA_Attunement` data.
- Ensure Fire/Light/Bleed Attunement entries are genuine selectable items/definitions; keep Neutral as internal fallback if useful; retire Frost/Poison/Electric from active CodeX roster without breaking reference chains.
- Reclassify champion fragment records from Tool into Fragment collection semantics. Preserve stable ItemIDs if feasible.
- Restrict Medicine HP/EN to their typed recovery items. Five legacy tool slots become two canonical slots.
- Convert `DT_EquipmentSlots_UI` to target schema: six visible Equipment rows, plus Outfit represented on Character. Slot `GridRow/GridColumn` values and icon assignments should reflect the new layout.
- Validate real `WeaponFamilyTag` and CombatStyle mappings; verify the newly exported Katana and `Weapon.Family.Twinblade` tags against the live style resolver; do not use the obsolete November 13 Sword family observation.
- When a truly new item category is needed, migrate **enum referencers and DataTable rows carefully**; ordinal-based UE enum defaults should not be blindly overwritten.

**Checkpoint:** DataTables load, definitions query correctly, no user-visible category contains the wrong semantic item, no missing tag resolution. Verify integrity of world pickup references and starting inventory.

**Avoid:** simply renaming `Tool` to Medicine while leaving fragments behind; simply renaming old Attunement Talismans into passive Talismans; deleting armor data before Outfit visual migration.

## C3 — Player, combat, stat and HUD integration

**Affected:** `BP_PlayerBase`, player action/component references, `AC_EquipmentStatsComponent`, `AC_PlayerAttunementComponent`, HUD/ItemWheel, controller input and potion action.

**Desired changes:**

1. Resolve active weapon kit from the two assigned ItemIDs rather than `RightHand1–3` and `LeftHand1` as player-facing state.
2. Map valid kit → proper `DA_CombatStyle`/Animset (including guard), physical weapon actors and any SwordShield supporting actor.
3. Preserve existing equip/unequip, attack, defense, draw/sheath and guard cleanup behavior; adapt event subscriptions.
4. Update `AC_EquipmentStatsComponent` so **active** weapon determines offensive stats; remove contribution from inactive weapon and four-piece armor equipment sources as CodeX transitions to Outfit.
5. Apply Outfit visuals by an atomic ensemble refresh; preserve compatible skeletal followers/preview.
6. Connect Equipment's selected Attunement to `AC_PlayerAttunementComponent` and reapply on every committed weapon switch.
7. Connect passive Talisman as a unique, non-stacking stat modifier.
8. Convert item-wheel weapon cycling to Primary/Secondary; Medicine wheel to two typed HP/EN selections. Stop showing armor/talisman/Attunement as cyclic quick items.
9. Preserve Potion input/montage/notify/consumption semantics; use owned quantities rather than slot-local quantities.
10. Refresh Equipment/Inventory/HUD from change events rather than duplicating equip logic.

**Checkpoint:** compile affected Blueprints; inspect graph paths for safe block/action transition, actor cleanup and event ownership. **Manual PIE by user** covers real combat and animation; see validation matrix.

**Avoid:** automatic broad refactor of Enemy/Boss behavior, abandoning Combo/Defense assets, using permanent display widgets to spawn gameplay actors.

## C4 — Five-tab GameMenu transformation

**Affected:** `EGameMenuPage`, `WBP_GameMenu`, menu button, `WBP_Equipment`, its picker/slot/entries, `WBP_Inventory`, `WBP_Status`, `WBP_System`, controller page routing, new Character/Artifacts pages.

**Desired changes:**

- Carefully migrate four-page enum / typed button dispatchers / `ActivePage` / switcher indices to five tabs in order Character, Equipment, Inventory, Artifacts, System.
- Create a Character page grounded in the existing preview pipeline, with one full-Outfit chooser.
- Rework Equipment to six slot rows, existing compatible-item picker, compact right-side category-relevant item details; safe active kit indicator.
- Keep Inventory as owned-item browser; simplify or reuse current category grid and contextual action.
- Add Artifacts viewer using Fragment-owned state, images/description/collection progress; no independent authority.
- Retain existing System settings functions and Save & Quit/confirmation flows, adjusting shared shell and style.
- Remove the Status **tab** from navigation; do not delete character stats logic. Remove obsolete standalone WBP_Status only after checking other references.
- Use sober black/charcoal background, one reusable header/control style, and existing player/weapon previews. Do not reproduce scenic mockup background.

**Checkpoint:** UMG compile/focus/page-index checks; no page switch incorrectly instantiates/overwrites gameplay state. Manual navigation/preview check by user.

## C5 — Rest/level flow simplification

**Affected:** `WBP_RestMenu`, old `LevelUp` widgets, `BP_CPC_PlayerController` rest/time functions and related UI dispatchers, progression entry points.

**Desired changes:**

- Replace menu actions with Skill Tree, Travel, Leave (future unavailable actions disabled or hidden until implemented).
- Remove playable time-of-day selection and manual level-up entry.
- Retain resting recovery, fades, sitting, interaction lifecycle, controller lock/unlock and leave behavior.
- Keep `AC_ProgressionComponent`/currency data intact until the future skill progression economy is decided.

**Checkpoint:** rest interactions no longer open old tabs; Leave always restores correct control/camera/position; death while resting remains safe.

## C6 — Reference cleanup and documentation refresh

**Only after passing C1–C5 checkpoints:**

- `Find References` to old `Equipment.Slot.RightHand*`, `LeftHand1`, `Head/Chest/Arms/Legs`, `Tool01..05`, `ActiveRightHandSlot`, `ActiveToolSlot`, old page enum members and abandoned data rows.
- Remove obsolete pathways, variable adapters and UMG panels **only when referencer checks show they are dead**.
- Decide whether to retain legacy assets as internal templates or remove them through Unreal's asset tools. Use redirector fixups where needed.
- Inspect actual package compilation and runtime state; refresh GitHub export **after** user-approved implementation is saved.
- Update docs' OBSERVED/IMPLEMENTED status. Do not let target design assertions masquerade as verified current behavior.

## Risk register

| Risk | Severity | Mitigation |
|---|---|---|
| Old tags referenced across Blueprint graphs | High | Add/bridge, migrate subscribers, only then retire |
| `EItemCategory` enum/default ordinal serialization | High | Map rows and referencers explicitly; staged compile |
| Wrong `Talisman_*` data semantics | High | Build separate passive definitions and Attunement records |
| Swap breaks guard Animset/blocked state | High | Action gate + centralized cleanup + manual PIE |
| Old shield model coupled to left-hand slots | High | Resolve SwordShield as single kit with internal support actors |
| Outfit mesh follower lifecycle | Medium/high | Reuse current follower approach, clear old parts atomically |
| EquipmentStats still sums four armor parts | Medium/high | Resolve one Outfit source, active weapon only |
| Rest/controller code has multiple overlapping locks | Medium | Scope UI changes, test rest/death/cinematic transitions |
| Old menu enum with serialized button indices | Medium/high | Controlled enum rewrite, navigation/focus regression |
| Save/profile references stale ItemIDs/tags | High if persistent | Audit serialized format, explicit remap or test-save reset |
| Mockup exceeds implementable visual budget | Medium | Use cheap gradient, reuse captures/materials and native UMG |
| Broad deletions break unrelated systems | High | Only delete after live references/compiles, checkpoint first |

## Phase acceptance policy

Each stage may be implemented in one or more small MCP changes. Agent reports *actual* changed assets, compilation, saves and static checks. The user tests PIE and either accepts or reports regressions. **Do not automatically advance** to the next stage when a blocking mismatch remains; isolate and resolve it. Avoid repetitive full-project analysis once direct dependency mapping is established.
