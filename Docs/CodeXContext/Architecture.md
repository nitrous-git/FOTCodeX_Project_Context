# CodeX architecture map

## Project architecture

CodeX is a **Blueprint-first third-person action-adventure** organized around reusable player combat, ordinary enemy AI/coordination, authored boss encounters, traversal and world presentation. The implementation is modular, but it is **not** a mandate to replace working components with a new generic framework.

The live Unreal asset root is `/Game/CodeX_Restruct`. The exported snapshot has matching top-level folders:

| Area | Export folder | Primary responsibility |
|---|---|---|
| Player | `Frameworks/PlayerFramework/` | Player actor, combat style selection, attunement, actions, animation/locomotion |
| Regular enemies | `Frameworks/EnemyFramework/` | Enemy base, perception, noncombat world behavior, group melee/ranged styles, BT/blackboard |
| Bosses | `Frameworks/BossFramework/` | Placed bosses, activation lifecycle, state orchestration, boss UI, bespoke attacks |
| Inventory | `Frameworks/InventorySystemFramework/` | Item records, inventory component, inventory widgets and pickup contracts |
| Shared orchestration | `Shared/Managers/` | Group enemy manager, encounters spawner and related shared services |
| Game | `Game/Items/`, `Game/UI/` | Items/weapons and game-facing UI |
| Other existing frameworks | `Frameworks/TeleporterFramework/`, `TraversalDeviceFramework/`, `TraversalHomingFramework/` | Teleport and traversal capabilities; inspect before use |
| Core | `Core/` | Controllers and game modes |

Paths above are relative to exported `CodeX_Restruct/` unless noted otherwise.

## Established ownership boundaries

- **Player:** `BP_PlayerBase`, player components, CombatStyle/Attunement data assets and `ABP_PlayerMaster`. New features should integrate rather than bypass these contracts.
- **Ordinary enemies:** `BP_EnemyBase` retains common identity/lifecycle. `AC_EnemyAwareness` detects targets; `AC_EnemyWorldBehavior` handles non-combat behavior and disengagement; dedicated combat-style components own group melee/ranged behavior. AI Controller + Behavior Tree govern routine combat AI.
- **Group coordination:** `Shared/Managers/BP_EnemyAIManager` manages regular enemy participation and attacker slots. It is not the source of a boss's attack decisions.
- **Boss encounters:** `BP_BaseBoss` and boss-specific state Blueprints own the encounter lifecycle, stack/state logic, phase behaviors and presentation. `BP_BossDirectorComponent` and `BP_MetricsComponent` support the boss's combat decision loop. Boss activation is intentionally separate from regular enemy group AI.
- **Inventory/UI:** `AC_InventoryComponent` and inventory data/UI widgets are the existing implementation. UI changes must respect current data and Player framework contracts.

## Important entry points

```text
CodeX_Restruct/
  Frameworks/
    PlayerFramework/Core/BP_PlayerBase/
    PlayerFramework/Animations/ABP/ABP_PlayerMaster/
    EnemyFramework/Core/BP_EnemyBase/
    EnemyFramework/Core/Components/AC_EnemyAwareness/
    EnemyFramework/Core/Components/AC_EnemyWorldBehavior/
    EnemyFramework/Core/Components/AC_EnemyCombat_GroupMelee/
    EnemyFramework/Core/Components/AC_EnemyCombat_Ranged/
    EnemyFramework/AI/BehaviorTree/BT_BasicEnemy/
    BossFramework/Core/BP_BaseBoss/
    BossFramework/Core/Components/BP_BossDirectorComponent/
    BossFramework/Core/Components/BP_MetricsComponent/
    BossFramework/Core/Triggers/BP_BossActivationTrigger/
    InventorySystemFramework/InventorySystemRefactor/Inventory/AC_InventoryComponent/
  Shared/Managers/BP_EnemyAIManager/
  Shared/Managers/BP_EncountersSpawner/
```

Every directory shown is present in the October 17, 2026 export; this does not by itself prove a particular runtime test has passed.

## Cross-framework dependency warnings

- Bosses and regular enemies share some combat contracts and may reference each other (e.g. certain boss archetypes reference `BP_EnemyBase`). **Do not infer that all bosses use the same AI lifecycle** from such references.
- Changes to player data assets/animation can affect inventory, combat and presentation simultaneously.
- Changes to `BP_EnemyBase`, `BP_BaseBoss`, or shared interfaces have a large downstream impact. Inspect `architecture.referencers` and `dependency_graph.json` before editing.
- Editor asset operations, not filesystem renames, should maintain Unreal references.
