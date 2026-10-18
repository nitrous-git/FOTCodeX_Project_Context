# Decisions and unresolved questions

This log records **standing architectural decisions**, distinct from requested/implemented work. Historical decisions must be reconfirmed if later user direction supersedes them.

## Standing decisions

| Domain | Decision | Status |
|---|---|---|
| Implementation | Blueprint-first/Blueprint-only gameplay; inspect/compile safely in small steps | User preference / standing constraint |
| Codex workflow | Keep ChatGPT design review + local Codex live UE MCP + GitHub context exports complementary | Accepted workflow |
| Existing player systems | Preserve CombatStyle, Attunement, locomotion and `ABP_PlayerMaster` | Explicit user constraint |
| Regular enemies | Split generic enemy identity/world behavior from melee/ranged style components | Prior implemented architecture; assets exported |
| Enemy participation | Regular enemy group coordination may support multiple attacker slots/categories | Prior development direction; live values require verification |
| Bosses | Authored placed dormant; activate via local awareness or area/fog trigger | Agreed design and historical tests |
| Boss orchestration | Boss states/director/metrics separate from `BP_EnemyAIManager` token scheduling | Architectural boundary |
| Inventory | Keep the existing component/model as baseline until a scoped redesign is approved | Current setup scope; no migration authorized |
| Static export | Treat JSON and T3D as searchable evidence, not as mutable `.uasset` files | Tooling boundary |

## Deferred / not automatically authorized

- Importing or replacing systems with a third-party Soulslike Framework.
- Large cross-framework refactors or redesign of equipment slots.
- Boss reset/respawn policy, cinematic/wake/skip behaviors beyond what the live editor verifies.
- Extensive LOS repositioning/complex ranged AI, when not specifically needed.
- Polishing boss UI or adding new boss encounters solely to exercise MCP.

## Rules for new decisions

When the user approves a new design direction, note: date, affected assets, previous contract, new contract, tested implementation state, and known limitations. Keep `CurrentState.md` aligned with newly verified results, but do not overwrite this log with speculative roadmap items.
