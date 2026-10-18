# Regular Enemy Framework and WorldBehavior

## Reference paths

```text
CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyAwareness/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyWorldBehavior/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyCombat_GroupMelee/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyCombat_Ranged/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Blueprints/BPI_EnemyCombat/
CodeX_Restruct/Frameworks/EnemyFramework/Core/Blueprints/BP_EnemyPatrolPath/
CodeX_Restruct/Frameworks/EnemyFramework/AI/BehaviorTree/{AIController_BasicEnemy,BB_BasicEnemy,BT_BasicEnemy}/
CodeX_Restruct/Shared/Managers/BP_EnemyAIManager/
CodeX_Restruct/Shared/Managers/BP_EncountersSpawner/
```

## Architectural ownership

`BP_EnemyBase` is the shared enemy actor and entry point; it owns common combat identity and coordinates components rather than embedding every style's attack logic. `BPI_EnemyCombat` is the combat-style interface; `AC_EnemyCombat_GroupMelee` and `AC_EnemyCombat_Ranged` contain style-specific positioning and actions. `ABP_Enemy` supports animation. Regular enemy decision-making uses the `BT_BasicEnemy` / blackboard / AI controller path.

`AC_EnemyAwareness` detects targets and hands off to the enemy entry behavior. This is intentionally distinct from group-attacker scheduling. Detection/vision versus aggro/coordination ranges must not be conflated.

`AC_EnemyWorldBehavior` owns behavior **outside active combat**: stationary/resting, random roaming, spline patrol and the safe transition home following disengagement. Placed enemies and encounter-spawned enemies have different initialization/appearance requirements; don't force placed actors through a spawn-dissolve montage.

## Export-confirmed graphs

- `BP_EnemyBase`: `InitCombatStyle`, `ApplyCombatPresentation`, `ApplyWeaponLoadout`, `InitializePassiveAwareness`, `RegisterCombatBehavior`, `ComputeStrafeDirection`, `SetEnemyState`.
- `AC_EnemyWorldBehavior`: `StartWorldBehavior`, `StopWorldBehavior`, `AdvancePatrolPoint`, `ApplyWorldBehaviorLocomotion`, `EvaluateDisengage`, `OnDisengageRequested`, `OnWorldReturnCompleted`, `StartDisengageMonitoring`, `StopDisengageMonitoring`.
- `AC_EnemyCombat_GroupMelee`: `ApplyGroupMeleeProfile`, `ComputeStrafeDirection`.
- `AC_EnemyCombat_Ranged`: `ApplyRangedProfile`, `CanPerformCombatUpdate`, `ComputeStrafeDirection`, `GetDistanceToPlayer`, `HasLineOfSightTo`.
- `BP_EnemyAIManager`: `GetEnemyCombatCategory`, `FillOpenAttackerSlots`, `ReconcileAttackerSlots`, `GetEligibleEnemies`, `SetAttacker`, `ClearAttacker`, `TryRotateAttacker`, `RegisterEnemy`, `UnregisterEnemy`, `EvaluateRoster`.

These names were read from the UE 5.8 export folder. Inspect node wiring and current values before stating exact behavior.

## Prior development decisions

- Use minimal, incremental phases; do not initiate another broad EnemyBase refactor.
- Normal enemies may be placed in the world and start stationary or roam/patrol, as distinct from encounter-spawned enemies.
- Group melee and staff/ranged styles remain modular and can share an animation foundation where appropriate.
- Ranged behavior aims for straightforward Souls-like approach/limited strafing and leash return; elaborate avoidance/LOS reposition was deliberately de-prioritized in earlier work.
- The AI Manager is intended to support several attacker slots, with melee/ranged category distinctions and rotation fairness. This is a regular-enemy coordination concern, never a boss-token mechanism.
- On disengage/return, restore the intended noncombat world behavior and movement presentation without leaving combat state/observer/attacker references stale.

## Before edits

Trace `BP_EnemyBase` → combat-style component → AI Manager → BT task/decorator and `AC_EnemyWorldBehavior` transitions for the exact scenario. Check pawn movement speed versus animation speed, home/leash references, state reset and unregister behavior. Use live MCP for the actual Blueprint state, and perform an in-editor compilation and PIE test when edits are authorized.
