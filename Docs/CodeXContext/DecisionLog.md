# CodeX — Architecture decisions and deferred work

**Reconciled October 19, 2026.** Decisions below supersede the 2026-10-17 inventory scope, which stated that modular equipment slots were not approved. That earlier position is obsolete.

## Accepted standing decisions

| Domain | Decision | Status |
|---|---|---|
| Unreal implementation | Blueprint-first (Blueprint-only unless requested); small compile-safe increments | Standing |
| Development workflow | ChatGPT designs/reviews; local Codex agent modifies live Unreal via MCP; GitHub export documents snapshots | Standing |
| Source of truth | Live Editor > latest user-reported local changes > design docs > archived exports | Standing |
| Player systems | Preserve CombatStyle, Attunement, `ABP_PlayerMaster`, locomotion, existing weapon actor functionality | Standing |
| Item architecture | ONE `/Frameworks/ItemSystemFramework/`; internal Inventory, Equipment, Items/Definitions, Pickups | Approved 2026-10-18 |
| Component ownership | `AC_InventoryComponent` owns collected items; `AC_EquipmentComponent` alone owns GameplayTag→ItemID equipment Map | Migrated and present in 2026-10-19 export |
| Weapon actors | Weapon class resolved from an equipped item via `DA_WeaponDefinition`; CombatStyle not authoritative for which specific sword is owned/equipped | W1–W3 established; new component split in A1–A3 |
| Inventory UI | `WBP_Inventory` is **owned-item browsing**, categories, item description/actions, **not equipment slot selection** | Approved; widget rebuild pending |
| Equipment UI | `WBP_Equipment` edits loadout with per-slot click → internal compatible-item choice view | Approved; widget rebuild pending |
| Game Menu | `WBP_GameMenu` navigates sibling Inventory/Equipment/Status/System pages; no crafting in initial scope | Approved; widget rebuild pending |
| Framework location for UI | UI lives in `Game/UI/GameMenu/`, not separate `GameMenuFramework` or under Item System | Approved |
| Legacy cleanup | Remove old `WBP_PlayerInventoryHUD_manual`, slot containers/direct-equip UI and old folder hierarchy; do not maintain a permanent Legacy folder | A4 user-completed |
| SLF usage | Naming, screen structure and UX **reference only**; retain CodeX item/combat/backend implementations | Approved |
| Initial equipment functionality | `Equipment.Slot.RightHand1` and `Equipment.Slot.LeftHand1`, two sword definitions and one shield | Current vertical slice |
| Regular enemy | WorldBehavior / awareness separate from combat profile; multiple attacker categories are planned/partly implemented | Historical development |
| Boss lifecycle | Authored dormant placed bosses, activation triggers, bespoke state/Director orchestration rather than standard enemy token manager | Standing |
| Export tooling | Export JSON/T3D is read-only reference, never a substitute for editing Unreal `.uasset` | Standing |

## Immediate approved MCP scope

After local cleanup, create **foundation-only** Game Menu widget assets, Designer hierarchies, page/subview switchers, relevant variables/dispatchers, and optional slot UI metadata. Existing live Blueprints must be inspected first. **Do not** reactivate the old direct-equip UI, clone deprecated widgets, rewrite player combat, or implement the full item-selection/equipment mutation logic in the foundation pass. See `GameMenuFoundationSpec.md`.

## Deliberately deferred decisions / technical obligations

- When a player removes/sells/drops the final copy of an equipped item: reject action or atomically unequip before removal (must be specified and validated).
- Multiple separately upgraded copies of one weapon need item-instance IDs; ItemID-only assignments suffice for the present simple vertical slice.
- Add `Armor` / `Talisman` / `Tool` data categories and definitions in their own scoped migration; existing enum still has legacy `Equipment`, `CombatStyle`, `Attunement`, etc.
- Future hands, ammunition, four armor parts, talisman capacity, quick-tool positions, and compatibility rules: reserve UI extensibility, **do not assume all are currently enabled**.
- Weapon-family-to-CombatStyle auto-resolution for all equipped combinations; current sword/shield functionality does not settle every pairing.
- Game Menu input ownership, preview synchronization and inventory actions: implement in explicit later milestones after widgets exist.
- Boss respawn policy and cinematic skip/interruption safety, complex ranged LOS repositioning, and visual polish beyond accepted scope are not automatically authorized.

## Decision maintenance

When a new direction is approved, record date, ownership/path impact, previous contract, new contract, *implemented vs merely planned*, validation and limits. Do not interpret a stale exported graph as proof that later user-described local cleanup did not occur.
