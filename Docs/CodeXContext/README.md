# CodeX continuation — documentation index

**Document revision:** 2026-11-14  
**Basis:** `nitrous-git/FOTCodeX_Project_Context` branch `main` at `1581bbee8ad9f0fcb73090b32f53fa99300ac52a`; export generated `2026-11-13T06:32:12Z`.  
**Implementation status:** new model **not migrated yet**. Existing export details are observations; new interfaces/tags are targets, not declarations of existing assets.

## What to read

| File | Purpose | Read when |
|---|---|---|
| `01_GAME_DIRECTION.md` | Product philosophy, scope and player experience | Design choices, feature triage |
| `02_EXPORT_AUDIT.md` | Verified snapshot, existing Blueprint map, critical incompatibilities | Beginning or reviewing the migration |
| `03_EQUIPMENT_AND_ITEMS.md` | Authoritative desired equipment/inventory semantics | Item, slot, weapon/outfit/potion/talisman changes |
| `04_COMBAT_AND_PROGRESSION.md` | CombatStyle, Attunements, stats and progression contracts | Player, weapons, effects, resting |
| `05_UI_UX_SPECIFICATION.md` | Five-menu architecture and implementation-friendly visual language | UI/UMG work |
| `06_MIGRATION_PLAN.md` | Dependency-ordered adaptation phases with rollback boundaries | Planning and reviewing an agent implementation |
| `07_VALIDATION_MATRIX.md` | Static checks, regressions and manual PIE scenarios | Before accepting each migration phase |
| `08_DATA_MIGRATION_REGISTER.md` | Explicit old-to-target mappings and data remediation | DataTables, tags, enum and assets |
| `09_UNRESOLVED_DECISIONS.md` | Open choices, defaults and deferred features | When a needed behavior is unspecified |
| `10_PROTECTED_SYSTEMS.md` | Subsystem ownership and out-of-scope protections | Any broad refactor |

This is a **design and migration specification**, not an MCP task prompt. Implementation sessions should use the subset relevant to their work rather than reread all files.

## How to interpret assertions

- **OBSERVED** means an asset, variable, function, row or dependency was seen in the referenced November 13 export.
- **TARGET** means the intended CodeX architecture, not yet necessarily in the project.
- **VERIFY LIVE** identifies runtime behavior, values, tags, unsaved modifications or referencers requiring Unreal Editor validation before edits.
- **DEFERRED** means intentionally excluded from the first model conversion.

The exporter reports no metadata-export errors, but structural exports and graph dumps do not establish that Blueprints compile or that behaviors pass PIE. Pin-parser count warnings also exist; never equate the export with a graph edit surface.

## Core architectural invariants

1. `AC_InventoryComponent`: ownership, counts, item lookup and inventory-change events.
2. `AC_EquipmentComponent`: single authority for assigned equipment and active weapon/medicine selection.
3. `BP_PlayerBase` + player actions + `DA_CombatStyle`/AnimBP: spawned weapon actors, animation, actions and combat presentation.
4. `AC_EquipmentStatsComponent` + `AC_CharacterStatsComponent`: resolved gameplay stats/resources, not UI-derived math.
5. Widgets: browse/select/compare/display; never independently own equipment state or spawn combat weapons.
6. Controller: menu lifecycle, input, focus and transitions.
7. Existing bosses, regular AI, interaction and cinematic systems remain protected unless the task explicitly targets them.

## Migration rule

**No blanket rename or deletion first.** Introduce the target contract, migrate callers and data, validate, then retire old properties/rows/tags. Preserve stable ItemIDs where reasonable; perform semantic migration for miscategorized items. All Unreal changes require actual live editor tools.

## Export source links

- [Repository](https://github.com/nitrous-git/FOTCodeX_Project_Context)
- [Export manifest](https://github.com/nitrous-git/FOTCodeX_Project_Context/blob/main/export_manifest.json)
- [Dependency graph](https://github.com/nitrous-git/FOTCodeX_Project_Context/blob/main/dependency_graph.json)
- [Equipment component export](https://github.com/nitrous-git/FOTCodeX_Project_Context/tree/main/Game/CodeX_Restruct/Frameworks/ItemSystemFramework/Equipment/AC_EquipmentComponent)
- [Inventory component export](https://github.com/nitrous-git/FOTCodeX_Project_Context/tree/main/Game/CodeX_Restruct/Frameworks/ItemSystemFramework/Inventory/AC_InventoryComponent)
