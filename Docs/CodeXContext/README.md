# CodeX context — document router

This repository holds **design decisions and searchable exported Blueprint snapshots** for a separate live Unreal Engine project. Start with `AGENTS.md`, then open **only relevant** documents. The running Editor/MCP is authoritative for current `.uasset` state; exports may predate local changes.

| Topic or task | Start here |
|---|---|
| Item data, inventory collections, equipment slots and UI semantics | `InventoryAndUI.md` |
| Next task: create Game Menu/Inventory/Equipment UMG foundations | `GameMenuFoundationSpec.md` (also consult `InventoryAndUI.md`) |
| Player CombatStyle, weapon actor refresh and Attunement | `PlayerCombat.md` |
| Regular enemies, patrol/detection and group attackers | `EnemyFramework.md` |
| Boss awakening, attacks, state stack/Director, UI | `BossFramework.md` |
| Cross-framework dependency and path orientation | `Architecture.md` |
| Current export metrics vs locally completed work | `CurrentState.md` |
| Accepted decisions and deferred work | `DecisionLog.md` |
| Blueprint JSON lookup and known parser warnings | `ExportGuide.md` |
| Changes/compile/save/permissions | `DevelopmentRules.md` |
| Live MCP connector/editor procedure | `MCPWorkflow.md` |

## Navigation examples

- Inventory: `CodeX_Restruct/Frameworks/ItemSystemFramework/Inventory/AC_InventoryComponent/{asset.json,graphs/...}`.
- Equipment: `CodeX_Restruct/Frameworks/ItemSystemFramework/Equipment/AC_EquipmentComponent/{asset.json,graphs/...}`.
- Player: `CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase/{asset.json,graphs/...}`.
- Proposed Game Menu widget location: `/Game/CodeX_Restruct/Game/UI/GameMenu/`; most widgets are **planned** and not in the latest exported asset tree.

## Explicit status boundary

The October 19, 2026 published export (`export_manifest.json`) captured the component and folder reset, but the user has performed further final cleanup **locally after the export**. Do not treat stale `EquipItemFromSlot` or focus-state graphs from the published JSON as instructions to reimplement them. Verify in live Unreal when needed.

Do not modify exported JSON, metadata or raw T3D to simulate project edits. Edit through supported Unreal Editor/MCP actions, compile/save, then rerun the exporter when the user wants a refreshed snapshot.
