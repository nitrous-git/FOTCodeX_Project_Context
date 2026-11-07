# H6 — Runtime resources, combat integration and soul rewards

Implemented in the live FOTCodeX_SLF_583 UE 5.8.3 project on 2026-11-06. Live assets remain authoritative.

## Changed assets

Created:
- `/Game/CodeX_Restruct/Frameworks/InteractionFramework/Rewards/BP_SoulPickup`
- `/Game/CodeX_Restruct/Developer/Tests/H6/BP_H6ResourceVerification`

Modified:
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/AC_CharacterStatsComponent`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/BP_PlayerActionBase`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/BP_PlayerActionComponent`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/LocomotionAction/BP_PlayerAction_Sprint`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/CombatAction/BP_PlayerAction_MeleeCombo`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/LocomotionAction/BP_PlayerAction_Dodge`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/LocomotionAction/BP_PlayerAction_Jump`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/LocomotionAction/BP_PlayerAction_Block`
- `/Game/CodeX_Restruct/Frameworks/EnemyFramework/Core/BP_EnemyBase`
- `/Game/CodeX_Restruct/Frameworks/BossFramework/Core/BP_BaseBoss`
- `/Game/CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyCombat_GroupMelee`
- `/Game/CodeX_Restruct/Frameworks/EnemyFramework/Core/Components/AC_EnemyCombat_Ranged`
- `/Game/CodeX_Restruct/Core/Controllers/BP_CPC_PlayerController`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_CharacterStatBalance`
- `/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_CharacterStatBalance_Default`

No HUD, inventory, equipment calculation, FP progression, CombatStyle, Attunement, animation Blueprint, or external locomotion component assets were replaced.

## Resource ownership and APIs

`AC_CharacterStatsComponent` retains CurrentHP, CurrentFP and CurrentStamina as the resource authority. Added ApplyHealthDamage, HealHealth, SetCurrentStamina, HasEnoughStamina, TryConsumeStamina, RestoreStamina, StartStaminaRegeneration and StopStaminaRegeneration. SetCurrentHP remains supported. Negative/invalid costs fail without deduction; resource updates clamp and broadcast OnStatsChanged.

The existing recalculation/clamp pipeline is retained. Equipment and level changes preserve current resources rather than refill them. Player death locks HP at zero; ordinary healing does not revive. FP data, snapshots, HUD and progression remain intact.

One component-owned ResourcePulse timer uses elapsed game time. It handles regeneration and moving sprint drain, guarding reduction, post-consumption delay, guard-break recovery and exhaustion recovery. No new Tick consumer was added. StopStaminaRegeneration pauses regeneration while retaining the single runtime resource timer.

## Tuning

Canonical tuning is on `DA_CharacterStatBalance_Default`, with editable fields defined by DA_CharacterStatBalance. The stats component caches these settings at BeginPlay.

| Setting | Default |
|---|---:|
| Regeneration | 20 points/second |
| Delay after consumption | 0.85 seconds |
| Moving sprint drain | 12 points/second |
| Exhausted sprint restart threshold | 20% maximum |
| Idle guarding regeneration multiplier | 0.35 |
| Light swing | 18 |
| Heavy cost reserved for existing shell | 30 |
| Dodge | 25 |
| Jump | 10 |
| Optional parry cost | 0 |
| Guard-break recovery lockout | 1.2 seconds, followed by regeneration delay |
| Runtime timer interval | 0.05 seconds |

## Combat and action integration

The BPI_CombatReceiver signatures were preserved. Regular enemies and bosses now own editable AttackDamage, AttackDamageOverrides, GuardStaminaMultiplier and BlockChipFraction. ResolveAttackDamage uses a montage/attack-name override when present and otherwise the attacker-specific default. Initial enemy damage is 22; boss damage is 35; guard cost multiplier is 1.25; chip fraction is zero. Configure child archetypes or placed instances for balance.

Regular enemy damage enters through CS_DoDamage and the existing hit interface. The attack context now identifies the current montage. Same-target duplicate callbacks in the same damage frame are rejected; montage starts also reset the hit set. Later distinct damage-notify frames remain valid for multi-swing montages. Boss WeaponBeginOverlap retains its existing attack-context and HitActors handling.

BP_PlayerBase resolves the attack resource profile before its original Parry/Block/Hit reaction branches. Hit applies the existing equipment snapshot's physical damage-negation fraction, then calls the stats damage API. Block charges guard stamina; zero/insufficient stamina cancels guarding and invokes the existing hit-reaction action plus guard-break lockout. Chip is applied only with a configured positive fraction. Parry applies no ordinary HP damage. Ignore changes no resources. Explicitly parryable attacks retain the original parry precedence; unblockable attacks bypass ordinary guarding.

BP_BaseProjectile's existing native ApplyDamage path is retained. Player AnyDamage recognizes that projectile source, resolves the outcome and derives damage from its existing Base Damage. Other generic damage uses the numeric Damage event input. Native AnyDamage from the regular-enemy/boss sources is suppressed because their resolved interface path owns their resource damage, preventing the two pathways from stacking.

TryActivate checks availability before canceling the current action. Light combo swings charge at montage playback, individually; dodges validate the selected montage before charging; jumps validate CanJump and charge in the action framework. BP_PlayerAction_Heavy is an existing empty, unmapped shell; no new heavy attack, input or montage was invented.

Sprint remains connected to SetLocomotionMode and CT_Cmpt_Locomotion.SetSprint. It drains only while moving, exits to jogging on exhaustion, resets the external sprint flag, and uses the existing normal movement speed. Locked-on/stunned/dead sprint cancellation is checked by the same controlled timer. Sprint input uses Started, so holding it does not repeatedly restart after exhaustion. Completed/Canceled now release only a current sprint action, preserving another action started afterward.

## Soul rewards and UI

BP_SoulPickup derives from BP_InteractableBase and uses the H4 interaction coordinator. Its 180 cm trigger registers/unregisters the interactable; overlap never grants souls. GetInteractionInfo supplies the E icon and “Collect Souls (amount)” text. ExecuteInteraction resolves the actual interacting actor's progression and interaction components, locks reentry, calls GrantSouls, then disables/unregisters/destroys the pickup only on success. Invalid collectors or rejected grants preserve it.

BP_EnemyBase emits at the terminal SetEnemyState commit; both confirmed boss death paths invoke an idempotent SpawnSoulReward. SoulReward defaults to 250 for regular enemies and 1000 for bosses and is configurable, including zero. A latch prevents duplicate rewards. Spawned pickups have independent actor ownership and survive enemy cleanup. Rewards do not enter inventory and are not deposited automatically.

Existing WBP_PlayerHUD HandleStatsChanged/RefreshResources -> WBP_Resources.SetResourceSnapshot and progression -> WBP_CurrencyContainer.HandleProgressionChanged remain the UI paths. Existing leveling/spending APIs remain authoritative.

## Niagara selection

The actual imported NS_Souls, NS_DeathCurrency and NS_ItemLootSimple assets were inspected and opened in Niagara preview editors. NS_Souls has a player-position target and is suited to traveling feedback. NS_DeathCurrency supplies a standalone world presentation with looping emitters and was assigned to the pickup's swappable Niagara component. A small sphere core remains visible independently of VFX. Final Niagara visibility, scale and style need the manual gameplay visual pass; the editor preview inspection was not a confirmed visual match.

## Verification and handoff

- All 17 created/modified Blueprint classes passed compilation with warnings treated as errors. The balance data instance was saved.
- The final Blueprint-driven PIE suite logged `H6 RESOURCE CHECKS=37 FAILURES=0`.
- Checks cover clamping, invalid costs, exact atomic deduction, regeneration rate/delay/guard reduction, stationary sprint, recalculation preservation, FP preservation, currency addition/spending, resolved Hit/Parry/Ignore/Block, guard break, actual pickup eligibility and exact GrantSouls transaction, invalid collector preservation, collection destruction, enemy hit deduplication, one reward on terminal death, duplicate death suppression, survival after enemy destruction, no automatic deposit, player death and dead-action/HP protection.
- The tests invoke gameplay APIs directly. They do not establish input timing, combo feel, moving sprint/lock-on animation behavior, live bar animation, range-exit prompt behavior, or manual Level Up/inventory-loot regression.
- PIE was stopped and the temporary verification actor removed. The reusable test Blueprint is saved; its integration suite intentionally changes the PIE player/enemy state and should only be run as a test.
- No test map save was performed. The editor map package remains dirty after temporary actor placement/removal; existing map edits were not discarded.
- Final manual PIE: moving/released/exhausted sprint and a fresh restart request; lock-on transitions; attack/combo/dodge/jump timing and buffering; ordinary hits and shield/parry/iframes; E prompt entry/exit and collection; currency HUD and Level Up spending; existing item loot; Niagara presentation.
- Element-specific damage packets, armor/guard balance beyond the existing physical negation channel, a dedicated guard-break animation, respawn/checkpoint restoration and lost-soul recovery remain outside H6.

