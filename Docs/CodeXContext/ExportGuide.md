# Blueprint export guide — UE 5.8.3

## Root files and their meaning

| File | What it contains |
|---|---|
| `export_manifest.json` | Engine version, exporter version, time, coverage counts, errors and warnings |
| `discovery_manifest.json` | Scanned asset inventory, classifications, filtering and selected export candidates |
| `discovery_candidates.txt` | Human-readable discovery candidates |
| `dependency_graph.json` | Directed asset dependency structure derived at export time |
| `CodeX_Restruct/**/asset.json` | Individual asset identity, classification, dependencies and selected metadata |
| `CodeX_Restruct/**/blueprint_graphs.json` | Collated Blueprint graph export data |
| `CodeX_Restruct/**/graphs/*.json` | Individual graph nodes, pins, properties and connections |
| `CodeX_Restruct/**/_raw/` | Raw export representation for analysis; not live UE assets |

## Example asset lookup

`/Game/CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase` maps to:

```text
CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase/
  asset.json
  blueprint_graphs.json
  graphs/EventGraph.json
  graphs/InitCombatStyle.json
  ...
  _raw/
```

Inspect `asset.json` → `identity.package_name`, `classification`, `architecture.dependencies` and `architecture.referencers`. Read only the graph files needed to answer the question. A `graphs/*.json` entry has top-level `schema_version`, `graph`, `stats`, `nodes`, `connections`. For example, `graph.kind` distinguishes delegates/functions where exported. `nodes` can contain GUIDs and pin references; search connections instead of guessing based on node display positions.

## Coverage and known limitations

`export_manifest.json` reports 293 exported candidates, 0 fatal errors, and 11 graph-parsing warning assets in the UE 5.8.3 run. The warning set includes `BP_EnemyBase`, `ABP_PlayerMaster`, `ABP_Enemy`, boss Animation Blueprints, projectiles, and traversal animation assets. Example warning: 'Parsed pin count ... does not match raw T3D marker count ...'. This is a **representation warning**, not evidence that the live Blueprint is broken.

The exporter intentionally excludes some content by path/class; 'not found' in the export does not prove 'does not exist' in the live project. UMG designer layout, Sequencer bindings, materials, skeletal animation, runtime values and dynamically constructed references may require live editor inspection.

## Source reconciliation procedure

1. Note manifest engine version, timestamp and Git commit.
2. Resolve the Unreal package path from the export `identity` object, not by inventing path segments.
3. Trace relevant export dependencies and graphs.
4. Query the current live asset via UE MCP.
5. If they differ, use live state for actual implementation, note differences, and refresh the export when appropriate.

Do not edit `graphs/*.json` expecting changes in UE. An exported Blueprint graph is a documentation artifact, not an editable Unreal package.
