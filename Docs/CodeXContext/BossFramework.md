# Boss Framework — lifecycle and encounter architecture

## Reference assets

```text
CodeX_Restruct/Frameworks/BossFramework/Core/BP_BaseBoss/
CodeX_Restruct/Frameworks/BossFramework/Core/Components/BP_BossDirectorComponent/
CodeX_Restruct/Frameworks/BossFramework/Core/Components/BP_MetricsComponent/
CodeX_Restruct/Frameworks/BossFramework/Core/States/BP_BossStateBase/
CodeX_Restruct/Frameworks/BossFramework/Core/States/BPI_BossState/
CodeX_Restruct/Frameworks/BossFramework/Core/Triggers/BP_BossActivationTrigger/
CodeX_Restruct/Frameworks/BossFramework/UI/WBP_BossUI/
CodeX_Restruct/Frameworks/BossFramework/UI/WBP_BossOverheadUI/
CodeX_Restruct/Frameworks/BossFramework/Bosses/
```

## Established design contract

- Bosses are generally **placed in the authored map and dormant before activation**, rather than spawned by a general boss-spawning system.
- Activation can be initiated by authored area/fog-gate triggers or appropriate local awareness. Activation pathways should feed one coherent boss lifecycle and prevent duplicate activation.
- The boss lifecycle is separate from its attack/state orchestration. `BP_BaseBoss` owns shared lifecycle; boss-specific state graphs/montages/phase behavior remain authored per boss.
- Boss fights use a custom state-stack/director/metrics framework, **not** `BP_EnemyAIManager` regular attacker tokens or the normal `BT_BasicEnemy` scheduling architecture.
- Tier 2/elite encounters can use contextual overhead health presentation; Tier 3 major bosses can use main encounter HUD/cinematic presentation. These are configurable presentation concepts, not a reason to duplicate combat logic.
- Do not introduce boss respawning, reset/rearming, global pooling or a new generic boss architecture unless explicitly requested.

## Export-confirmed framework elements

`BP_BaseBoss` export includes graphs named:

- Lifecycle/presentation: `Initialize`, `InitializeCombat`, `InitializePassiveAwareness`, `StopPassiveAwareness`, `CreateHealthWidget`, `ShowHealthUI`, `HideHealthUI`, `OnBossDefeated`.
- State orchestration: `PushState`, `PopState`, `TickActiveState`, `RefillStack`, `RequestInjectionAfterThisState`, `RequestInjectionWInterrupt`, `PostPop_ApplyInjection`, `SetMode`.
- Intro-related: `GetSequenceForMode`, `OnBossIntroStarted`, `OnBossIntroFinished`, `OnBossIntroAborted`.
- Phase/combat: `CheckForPhase2Transition`, `InitializeFirstSequence`, `RegisterToDirector`.

The 5.8 export also includes `E_BossEncounterState`, `E_BossIntroMode`, `E_BossHealthBarMode`, `BP_BossActivationTrigger`, `BP_BossDirectorComponent` and `BP_MetricsComponent`.

**Important:** A graph or dispatcher existing does **not** prove that an intro Level Sequence, wake montage, skip path, interruption, player-control restoration or PIE behavior was completed. The user should approve runtime validation and any missing work in a later task.

## Historical design/test record

- Foundational lifecycle and placed dormant activation: reported tested.
- Stationary awareness / area-trigger activation: reported tested during Boss Framework upgrade.
- Overhead/world-space health-bar presentation: explored; polish/deferred decisions should be reconfirmed against the active UI implementation.
- Optional Tier 3 intro presentation and safe wake/skip transitions: a recent implementation topic, but **not assumed fully validated** by this static context.

## Safe change analysis

For any boss change, inspect `BP_BaseBoss`, the concrete boss derived asset, active state Blueprint, director/metrics if involved, and the exact UI/trigger/sequence references. Preserve existing Concubine/Oracle/Undead Prince authored behaviors. Verify dormant → activation → intro/wake (when configured) → CombatActive → defeat transition without blocking state progression or leaving player controls disabled. Check cleanup paths on interruptions and skip, if those features are in scope.
