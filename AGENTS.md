# Fragment of the CodeX — Agent Operating Guide

**Project:** Fragment of the CodeX (CodeX / FOTCodeX)  
**Status:** Approved continuation target; architecture migration **not yet implemented**.  
**Engine:** latest context export identifies Unreal Engine 5.8.4; verify the live editor version before work.  
**Implementation:** Blueprint-first with Unreal Editor/MCP.

## Read this first

This repository is the **CodeX** project context/export, not a source for any other game. The live Unreal assets are the only write authority. JSON, T3D and Markdown in this repository are read-only context and design specifications; editing exported text does not edit `.uasset` files.

1. Read `Docs/CodeXContext/README.md` for status and selective document routing.
2. For equipment/inventory work, read `03_EQUIPMENT_AND_ITEMS.md` and the applicable rows in `06_MIGRATION_PLAN.md`.
3. For combat/animation/stat integration, read `04_COMBAT_AND_PROGRESSION.md`.
4. For GameMenu/UMG work, read `05_UI_UX_SPECIFICATION.md`.
5. For risk, regressions, and validation, use `07_VALIDATION_MATRIX.md`.
6. Audit the few relevant **live** assets before changing them. Do not repeatedly load every document or dump entire graphs.

## Non-negotiable target

- CodeX is a curated **combat-first action-adventure Soulslike**, not a full attribute-allocation RPG.
- Single *Outfit*; two *Weapon* assignments (Primary/Secondary) with one active-weapon cycle; one passive *Talisman*; one global *Attunement* (Fire/Light/Bleed); two *Medicine* assignments (HP/EN).
- No left/right-hand equipment slots in the **player-facing** model; physical hand sockets remain a valid implementation detail.
- A single equipment component owns assignments, Inventory owns items and quantities, Player/CombatStyle owns animation/weapon presentation, and widgets own only presentation/selection state.
- Combat styles, weapon-family animation sets, Shadow Strike, combat, enemy/boss systems and existing controller/HUD behaviors should be preserved unless explicitly in scope.
- Main GameMenu tabs: **Character, Equipment, Inventory, Artifacts, System**. No Status tab and no manual attribute Level Up page.
- Resting: Skill Tree (later), Travel (later), Leave. Remove the old time-of-day and attribute-leveling UI from the CodeX flow.
- UI art direction: restrained, dark charcoal gradient; subtle warm-metal selection; thin rules and large preview areas. Avoid complex cinematic 3D background scenery or new special shaders as prerequisites.

## How to work

- **Plan or audit request:** read-only. This documentation drop is **not** permission to implement the migration.
- **Implementation request:** modify live Unreal via MCP in small, compile-safe increments. Reuse existing assets/dispatchers and adjust existing references, not parallel duplicate systems.
- Confirm actual tool capabilities. Use Unreal asset APIs for renaming/deleting/migrating `.uasset`; never use filesystem operations for it.
- For requested implementation: compile/save edited Blueprints, run targeted structural checks. **Do not run PIE verification**; the user performs manual PIE and reports results for iteration.
- Report changed assets, compile/save state, structural checks, failures and remaining migration obligations. Do not claim unperformed tests.
- Never silently classify a proposed design as currently implemented. The export is a snapshot, not proof of current runtime behavior.

## Important current-state caveats

The November 13 export still has `ActiveRightHandSlot`, five Tool slots, four armor pieces, a four-page `EGameMenuPage`, and ItemTable `Talisman_*` entries pointing to Attunement data. These are **migration sources**, not a target to perpetuate. Read `02_EXPORT_AUDIT.md` for evidence.

### Source of truth

1. Current live Unreal Editor asset and its actual properties/graphs.
2. Latest user-approved CodeX specification in these documents and current conversation.
3. November 13, 2026 export metadata and Blueprint graphs.
4. Historical documents, only when a particular legacy implementation needs examination.
