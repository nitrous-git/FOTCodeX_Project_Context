# CodeX context — selective document routing

The live Unreal Editor/MCP is authoritative. GitHub Blueprint exports are dated snapshots. Read **AGENTS.md** first (once per session), then only the relevant guide and assets. Do not perform a general document audit for a routine change.

| Task | First guide |
|---|---|
| **Current Game Menu, Inventory, Equipment, Status/System UI construction** | **`UIDevelopment.md`** — the only routine UI guide |
| Deep Inventory / Equipment ownership, schema or gameplay change | `InventoryAndUI.md` |
| Player CombatStyle, physical weapon, Attunement, preview systems | `PlayerCombat.md` |
| Enemy AI behaviors | `EnemyFramework.md` |
| Boss design or behavior | `BossFramework.md` |
| Cross-framework dependencies or package boundaries | `Architecture.md` |
| Prior status, design decisions | `CurrentState.md`, `DecisionLog.md` |
| Tool permissions, exporting and connection issues | `DevelopmentRules.md`, `MCPWorkflow.md`, `ExportGuide.md` |

**Historical references (do not load by default):** `GameMenuFoundationSpec.md` (B0 completed; do not regenerate assets), earlier B1.1 SLF visual audit (completed), legacy Inventory HUD history.

UI target assets: `/Game/CodeX_Restruct/Game/UI/GameMenu/`.
Migrated visual library: `/Game/CodeX_Restruct/External/AssetContent/UI/SLF/`.
Source SLF reference widgets: `/Game/SoulslikeFramework/Widgets/` (never make these runtime dependencies).

For each new UI feature: inspect the specific live widget, use screenshot/design intent, edit, compile/save, report concisely. Do not rewrite these docs on every iteration.
