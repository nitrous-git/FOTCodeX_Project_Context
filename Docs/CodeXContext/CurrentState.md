# Current state and export provenance

As of **2026-10-17**, the GitHub `main` export includes a UE **5.8.3** snapshot. This document is a historical baseline, **not** a mandatory pre-read for every development task. Use live MCP for the latest state.

## Verified from GitHub export

- Repository: `nitrous-git/FOTCodeX_Project_Context`.
- Baseline commit: `96fb78b3f02a78916bf01148623c6cb0f47b34c6` (`upgrade to 5.8 version`).
- `export_manifest.json` generated: `2026-10-17T18:06:46.931932+00:00`.
- Engine: `5.8.3-58210709+++UE5+Release-5.8`; exporter: `0.4.0`; root: `/Game/CodeX_Restruct`.
- Scanned **6,085** assets; exported **293** (**214** full-graph; **79** structural).
- **734** authored graphs, **863** generated graphs, **1,400** variables, **17,047** nodes, **55,944** pins, **17,438** connections.
- Structural exports: 1 behavior tree, 1 blackboard, 18 data assets, 1 data table, 16 input actions, 1 input mapping context, 31 enums, 10 structs.
- Dependency graph **911** edges; **0** fatal export errors, **11** graph-parsing-warning assets (see `ExportGuide.md`).

Do not extrapolate from this snapshot to unsaved or post-export editor changes.

## Proven MCP milestone

The user verified that local Codex correctly identified live selected actor **`BP_Pickup_HPPotion_C_1`**. Live read connectivity works; specific mutation, compilation and save abilities remain tool-dependent and should be verified during actual development tasks.

## Recorded development state — verify specifics as needed

- **Enemy framework:** modular group melee and ranged components; awareness, placed-enemy world behaviors, roam/patrol and disengagement hooks exist in the export. Earlier playtests reported basic WorldBehavior and patrol milestones working.
- **Group AI:** `BP_EnemyAIManager` exports `FillOpenAttackerSlots`, `ReconcileAttackerSlots`, `GetEnemyCombatCategory`, and rotation/roster functions. Current attacker capacities and fairness should be checked live if relevant.
- **Boss framework:** `BP_BaseBoss`, activation trigger, Director/Metrics, boss health UI and intro-related graphs are present. Boss intro skip/abort and cinematic runtime behavior are not proven merely by graph existence.
- **Player:** `BP_PlayerBase`, `ABP_PlayerMaster`, CombatStyle/Attunement data assets are present; respect established integration.
- **Inventory:** `AC_InventoryComponent` and existing widgets/item data structures are present. Third-party inventory migration has not been established by this baseline.

This document records context, **not a restriction on future work**. Codex should implement whatever development task the user requests within the rules in `AGENTS.md`.

Refresh these claims after the export is regenerated or a relevant subsystem changes substantially.
