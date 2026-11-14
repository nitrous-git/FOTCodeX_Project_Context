# 10 — Protected existing subsystems and integration boundaries

**Status:** Existing systems observed or previously established; this list is deliberately protective. Migration agents should leave them intact unless they are direct consumers of changed CodeX equipment/menu contracts.

## 1. Player combat and input

- `BP_PlayerBase` and the player action framework remain the authority for combat action execution and physical weapon actors.
- `DA_CombatStyle`, `DA_CombatStyle_*` instances, `ABP_PlayerMaster`/locomotion assets, guard/block/dodge/combos and action montages are not to be recreated.
- `AC_PlayerShadowStrikeComponent` remains authoritative for Shadow Strike targeting, travel, execution, chain and camera behavior.
- `AC_CharacterStatsComponent` remains authoritative for HP, FP/EN, stamina, costs, regen and damage/heal.
- `AC_PlayerAttunementComponent` remains the actual Attunement applicator; only its selection source changes to an Equipment slot.
- `AC_EquipmentStatsComponent` retains resolved-stat ownership, adjusted for one active weapon, Outfit/passive sources.

## 2. AI and bosses

`Frameworks/EnemyFramework` and `Frameworks/BossFramework` are not equipment/UI design dependencies to be rewritten. Preserve awareness, world behavior, attack scheduling/group coordination, boss dormant/activation lifecycle, Director/Metrics, state transitions, fog and cinematic behavior. Player damage/hit/interface regressions must be tested after changing player equipment; no broad AI refactor is justified by this migration.

## 3. Interaction, loot and pickups

`Frameworks/InteractionFramework`, item pickups and source Inventory drop/amount-action logic remain. Changing category semantics can influence available actions and pickup routing, so inspect direct references; don't duplicate the Interaction framework or replace all pickups as a convenience.

## 4. World, traversal and cinematics

Keep authored levels, traversal device/homing/teleporter, checkpoint/rest activation, cinematic input locking and fog traversal. The CodeX UI target does not authorize modifying castle geometry, boss placement, landscape or cinematics. Resting UI changes should preserve transitions and controller state restoration.

## 5. HUD and notifications

`Game/UI/HUD` contains permanent `WBP_PlayerHUD`, `WBP_ItemWheelGrid`, resource display, item notifications, interaction prompts and screen fades. Update only weapon/Medicine slot presentation and related event sources. Do not replace the permanent HUD with the GameMenu shell or redesign the entire gameplay HUD in this phase.

## 6. System and title menus

The existing `WBP_System` implements more than a visual shell, including preferences, audio, graphics, camera and confirmation paths. `Game/UI/MainMenu/WBP_MainMenu` has new journey/continue/settings/quit logic. Integrate System with five-tab navigation without destroying these backends. Title/Save & Quit behavior must be checked after menu routing changes.

## 7. Runtime and export responsibilities

- Live Unreal Editor is the source of truth for assets, not JSON/T3D exported copies.
- Context repo `Game/CodeX_Restruct/` may lag editor and omits non-Blueprint content.
- Do not rename/move/delete `.uasset` by ordinary file operations; use Unreal assets operations and redirector fixups.
- Stop doing project-wide repeated audits after direct callers are established; run incremental referencer checks as the schema changes.
- Do not run PIE in agent sessions. Compile/save and inspect structurally; user supplies PIE results.

## 8. Explicit change classification

| Change | Allowed by equipment/UI migration? |
|---|---|
| New target equipment slot Map keys | Yes, scoped |
| Adapt active weapon/Medicine switching | Yes, scoped |
| Adapt PlayerBase equip/preview/guard integration | Yes, only required paths |
| Add `DA_OutfitDefinition`, `DA_TalismanDefinition` | Yes, when data conversion stage is authorized |
| Change GameMenu enum and page layout | Yes, after backend compatibility stage |
| Re-skin existing WBP_System | Yes, preserve logic |
| Remove old Level Up/time UI entry points | Yes, rest simplification stage |
| Rewrite whole Player Action framework | No |
| Replace Enemy AI or BossFramework | No |
| Replace pickup/interaction/traversal framework | No |
| Build new Skill Tree/fast-travel gameplay | No, future tasks |
| Rebuild every UI material/font | No |
| Delete old assets before referencer migration | No |
| Change landscape/level design | No |

## 9. Minimum change report

An agent working later should report: scope/asset paths modified, what contract changed, compile/save evidence, referencer/data checks, what was not tested (PIE), and any unresolved integration. No need for extensive report prose when the docs already define the agreed design.
