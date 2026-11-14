# 04 — Combat integration and lightweight progression

**Status: TARGET behavior plus OBSERVED integration points.** This document does not instruct a combat rewrite. Its purpose is to ensure the equipment simplification does not break the existing combat identity.

## 1. Preserve these systems

Relevant exported assets:

```text
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/BP_PlayerBase
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/AC_CharacterStatsComponent
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/AC_PlayerAttunementComponent
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/AC_PlayerShadowStrikeComponent
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/Components/AC_ProgressionComponent
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Core/DataAssets/DA_CombatStyle
/Game/CodeX_Restruct/Frameworks/ItemSystemFramework/Equipment/AC_EquipmentStatsComponent
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Animations/
/Game/CodeX_Restruct/Frameworks/PlayerFramework/Actions/
```

Keep movement, attack chains, guard/parry, dodge, hit and death reactions, lock-on, weapon collision, stamina usage, Shadow Strike and runtime resource handling. Targeted new contracts must integrate with these systems rather than reconstructing them.

## 2. Weapon assignment is not CombatStyle state

A player's **owned item** (`ItemID`) and equipped **slot assignment** are distinct from a **combat style** and an **active runtime weapon actor**.

Proposed resolution pipeline:

```text
Equipped Weapon ItemID
  -> FInventoryItemDefinition.WeaponData
  -> DA_WeaponDefinition / associated weapon-kit data
       -> WeaponFamilyTag
       -> CombatStyle / Animset selection
       -> physical weapon actor(s) + sockets
  -> BP_PlayerBase applies kit transition
  -> AC_EquipmentStatsComponent resolves active offensive statistics
  -> AC_PlayerAttunementComponent reapplies global effect/visual
  -> HUD + preview notified
```

`DA_CombatStyle` currently has `StyleTag`, `BaseAnimsetName`, `GuardAnimsetName`, `PrimaryWeapon`/`SecondaryWeapon` actor references, and a rich set of attack/defense montage fields. These concepts should not be deleted. The new *equipment* Primary/Secondary labels are player loadout positions, **not a mandate to rename the existing actor variables**.

### Supported weapon families

CodeX's first supported families are intended to include `SwordShield`, `Katana` and `Twinblades` as complete play styles, with a safe Unarmed fallback. The precise Gameplay Tags should be validated against the project's registered tags and style assets. If the existing `Weapon.Family.Sword` denotes SwordShield, keep the tag and document the meaning rather than renaming it for aesthetics. The November 14 export uses `Weapon.Family.Twinblade` (**singular**) in `DA_Weapon_Twinblades`, and `Weapon.Family.Katana` for Katana. Confirm those exact registered tags and `DA_CombatStyle` mapping in the live editor; do not rename automatically for pluralization.

**No unsupported combinations:** no shield-only kit; no Katana+shield; no Twinblades+shield; pure one-handed Sword without its required guard set is not enabled until the requisite normal/guard locomotion exists. These are intentional content rules, not UI error states.

### Swapping and defense state

Changing active weapon during guard, equip/unequip montage, attack, stagger, potion, death or cinematic input locks must not leave a half-applied CombatStyle or a stuck block state. The Player action framework must own transition authorization. `BP_PlayerBase` currently exports `CancelBlockForEquipmentChange`, `CE_EndBlocking` and `RefreshCombatStyleLocomotion`; inspect and preserve their intended cleanup behavior.

On a successful committed switch:

1. Resolve destination item's weapon kit and correct family;
2. Check action/transition validity;
3. Cleanly exit incompatible guard/action state if required;
4. Commit active slot and physical actor/Animset presentation;
5. Reapply the *global* Attunement and recalculate active-weapon stats;
6. Notify UI/HUD and preview.

Avoid visual Montage Notifies as sole authority for permanent combat-state transitions. Source-of-truth state should be explicit so interruptions are deterministic.

## 3. Attunement integration

The export's `AC_PlayerAttunementComponent` has `ActiveAttunementData`, `ActiveAttunementType`, `SetActiveAttunement` and `ReapplyAttunementToCurrentWeapons`. Existing `DA_Attunement` assets describe VFX/icon/name/description information. These are reusable.

**New rule:** `AC_EquipmentComponent` owns the chosen Attunement ItemID/slot. The Attunement component applies it to active weapon runtime instances and exposes effective status to combat. If no valid one is assigned, use an internal Neutral/no-effect behavior rather than a mysterious retained previous element.

**Initial three choices:**

| Attunement | Desired gameplay signature | Initial migration minimum |
|---|---|---|
| Fire | Sustained burn/pressure and corresponding blade treatment | Selection, stable global state, simple preview/VFX compatibility |
| Light | Guard pressure/disruption or later defensive utility | New identity/data distinct from old Electric; selectable placeholder allowed |
| Bleed | Status buildup and a meaningful threshold payoff | New identity/data; no unsupported promises of full buildup mechanics |

All three are orthogonal to weapon-family selection. An Attunement change must not respawn/re-equip the entire inventory or alter the assigned weapon ItemIDs. The user cannot cycle Attunements in ItemWheel.

## 4. Talisman and Attunement modifier ordering

To avoid double application and unstable modifier stacking, use one predictable calculation path:

```text
Base resolved weapon properties
+ passive Talisman contributions
+ effective global Attunement contributions
+ later skill/milestone contributions
= final combat properties, clamped and exposed as a snapshot
```

Exact formulas are intentionally **not finalized**. For now, prefer the existing `AC_EquipmentStatsComponent` and `AC_CharacterStatsComponent` change/refresh contracts. A passive source must be unique and removable. Recalculation must be idempotent (refreshing twice cannot double a buff). UI reads the final snapshot rather than recomputing and disagreeing with combat damage.

## 5. Lightweight stats / no point allocation

### Preserve internally

`AC_CharacterStatsComponent` already owns CurrentHP, MaxHP, CurrentFP, MaxFP, CurrentStamina, MaxStamina, regen/action costs, effective/base attributes, modifiers and resource dispatchers. `AC_EquipmentStatsComponent` owns weapon rating, scaling rules, armor sources and resolved stat snapshots. **Keep these until their callers and gameplay relevance are audited.**

### Hide/retire from player-facing progression

- No manual STR/DEX/Int/Faith/etc distribution.
- No character-level purchase screen in CodeX resting.
- No four-piece armor weight/negation spreadsheet in the new main menu.
- No requirement to expose A–E scaling grades or attribute requirements in the first equipment UI.
- No player-driven stat allocation simply to justify existing scaling formulas.

### Long-term adjustment options (in order of minimal disturbance)

1. **First continuation:** keep base attributes at reasonable balance defaults, use current resolution functions, expose only HP, EN, stamina and essential weapon effects. Disable manual leveling workflow, not the stats component.
2. **Later:** add explicit discrete skill-tree modifiers using stable source IDs. They may increase Attunement effectiveness or Shadow Strike capability without editing the foundational attributes directly.
3. **If needed:** change weapon growth to upgrade rank/weapon mastery curves, while leaving the old scaling implementation available until obsolete callers are identified.

`AC_ProgressionComponent` currently owns Souls and `TryCommitLevelUp`. The currency may still be needed for future combat ability purchases or travel. Do not delete Souls, source pickups or progression APIs merely because manual Level Up disappears. A later economy audit should decide whether to rename currency and how to apply refunds/upgrade costs.

## 6. Damage/resources/UI contract

- HP / EN potions apply to live `AC_CharacterStatsComponent` resources through the working Player action (not inventory UI).
- Current stamina consumption for sprint/dodge/guard/attacks remains protected.
- ItemWheel always identifies the active weapon and active medicine; it does not independently choose a weapon actor or Attunement.
- The Equip screen may present Attack, Guard Break/Impact, speed, traits and Attunement, but no invented values. Every displayed number must come from an actual resolved definition/snapshot.
- Character page is an outfit selector rather than a general stats dashboard.
- World damage, boss HP UI, enemy HP bars and death flow are outside the initial migration except for required regressions.

## 7. Shadow Strike

`AC_PlayerShadowStrikeComponent` already implements targeting, aim, selected target, shadow travel, execution/chain and camera effects. Keep it intact. A future simple upgrade lane may affect **only** a few parameterized abilities: e.g. chain count, targeting quality, execution utility or cooldown. This is not a request to implement them now.

Any active-weapon change while Shadow Strike is aiming, traveling or executing must be explicitly rejected or deferred according to action-state safety; the first phase should favor rejection, because surprise weapon transitions during executions are high risk.

## 8. Resting menu target

Resting retains site discovery, fades, sitting pose, recovery, rest-related enemy/world reset behavior where available, and a reliable Leave action. The new top-level choices are:

```text
Skill Tree   [later / disabled until implemented]
Travel       [later / disabled until implemented]
Leave        [functional]
```

The existing `WBP_RestMenu` exposes time-selection state/events and the controller exports `RequestRestTime`, `ApplyRestTime`, etc. The previous LevelUp page also exists. Remove these **from the accessible CodeX flow** after inspecting callers; do not delete unrelated controller rest/death/animation code. Travel is not a promise of a functioning save-system/fast-travel implementation yet.

## 9. Test-sensitive integration checkpoints

- SwordShield attack, guarding, guard locomotion and stop-block cleanup;
- Katana and Twinblades correct Animsets/montages and transitions;
- switching active weapon with one/two/zero assignments;
- Attunement reapplication after weapon actor replacement;
- passive Talisman source application and removal;
- potion notify/interrupt semantics;
- HP, FP/EN and stamina preserved during loadout changes;
- Shadow Strike and cinematic/locked states reject unsafe weapon changes;
- attack rating/guard info reflects current active kit, not both simultaneously.

Validation specifics and ownership are in `07_VALIDATION_MATRIX.md`.
