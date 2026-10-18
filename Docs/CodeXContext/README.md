# CodeX context — document router

This repository is a **searchable snapshot and design reference** for the separate live Unreal Engine project. Codex should **not load every document for every task**. Start with the applicable `AGENTS.md`, then fetch only information needed for the user's goal.

| Work area / question | First reference to consult |
|---|---|
| Ordinary enemies, patrol, detection, attacker coordination | `EnemyFramework.md` |
| Boss attacks, awakening, intro, state stacks and health UI | `BossFramework.md` |
| Player animation, locomotion, CombatStyle, Attunement | `PlayerCombat.md` |
| Items, equipment, inventory and UI | `InventoryAndUI.md` |
| Change crosses several frameworks | `Architecture.md` |
| What was last exported, or what was tested historically? | `CurrentState.md` and optionally `DecisionLog.md` |
| Find an asset, function, pin or dependency in exported JSON | `ExportGuide.md` |
| Risky shared-asset modification | `DevelopmentRules.md` |
| MCP tools, connection and workspace troubleshooting | `MCPWorkflow.md` |

## Source hierarchy

- **Live Unreal Editor (MCP):** actual current asset state and tool-confirmed changes.
- **Documentation:** agreed design constraints and recorded decisions; may need updating.
- **Blueprint export:** `CodeX_Restruct/**/asset.json`, `graphs/*.json`, `dependency_graph.json` and manifests are **export-time** representations, not live `.uasset` files.

Example: `/Game/CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase` corresponds to `CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase/asset.json` and graph JSON files below the same folder.

Prefer specific metadata/graphs and live queries over opening all exports. The UE 5.8.3 baseline provenance is in `CurrentState.md`. After significant Unreal changes, the user can rerun the exporter and update the GitHub snapshot; do not edit export files to mimic an Unreal edit.
