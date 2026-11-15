# 04 — Combat statistics, Attunement and lightweight progression

**Fragment of the CodeX — Revised compact combat model**  
**Revision:** 2026-11-15  
**Basis:** November 15, 2026, UE 5.8.4 context export, commit `18f5dddef026cae964b3cdc0574fd3f1385b1d90`.  
**Status:** **IMPLEMENTED** facts are source-observed; **TARGET** contracts below are approved for R1–R4 and are not all live yet.

## 1. Combat-first stat philosophy

CodeX's combat identity is conveyed through its weapon-family actions, hit/guard impact, **one active weapon**, global **Fire/Light/Bleed Attunement**, unique Talismans and small Outfit bonuses—not eight allocatable RPG attributes, modular armor resistances and weapon-scaling grades.

**Three layers, three responsibilities:**

| Layer | Owns | Player-facing representation |
|---|---|---|
| Runtime resources | HP, EN (implemented as FP internally), stamina, costs/regen, action state | HUD bars; perhaps a small later summary |
| Resolved combat stats | Active weapon damage/impact, Outfit defense, passive modifiers, Attunement effects | Short selected-item details in Equipment |
| Future progression | Selected abilities / discrete upgrades (Attunement and Shadow Strike) | Skill Tree later; no manual STR/DEX allocation |

All numbers displayed by the UI must originate in actual gameplay definitions or the same resolved snapshot combat uses. Screenshot numbers, UI sample lore, Trait names, damage bonuses and compatibility icons are illustrative until authored in live data.

## 2. Existing systems to preserve vs. obsolete systems to retire

### Keep working

- `BP_PlayerBase`: weapon-kit actor presentation, collision/hit traces, combat state, block/guard, dodge, death, equip/sheath, action locks and event integration.
- `DA_CombatStyle`, CombatStyle instances and locomotion/Animsets: SwordShield/SNS, Katana, Twinblade, Unarmed fallback.
- `AC_EquipmentComponent`: canonical equipped ItemIDs/active weapon/assignment dispatchers, tested C1A–C1C behavior.
- `AC_InventoryComponent`: item ownership/quantities and category storage, extended in C2A.
- `AC_CharacterStatsComponent`: CurrentHP/MaxHP, CurrentFP/MaxFP (the player-facing name is EN), CurrentStamina/MaxStamina, cost/regen and health-damage APIs.
- `AC_PlayerAttunementComponent`: existing Attunement runtime presentation and weapon VFX application, later fed by canonical `Equipment.Slot.Attunement`.
- `AC_PlayerShadowStrikeComponent`, player action framework, Enemy/Boss/Interaction systems, potion montage/notify logic.
- `AC_EquipmentStatsComponent` as the **single combat-stat resolution owner**, with a simplified implementation, rather than a parallel newly named resolver.

### Retire as the relevant migration becomes complete

- R1: four separate armor equippable items, Head/Chest/Arms/Legs armor stats and modifiers, `DA_ArmorDefinition` as a live item-definition class, four armor-slot visuals and four-part fallback pathways.
- R2: old resistances and six-channel **defense** accounting, legacy armor-stat widgets and unused load/requirements fields once their direct consumers are migrated.
- R3: weapon primary attribute requirements (`FPrimaryAttributes`-based), graded STR/DEX/etc scaling rules, multipliers derived from leveling and unused six-channel **offense** fields.
- R4: unreachable LevelUp/Status UI and dead remaining structs/enum members after direct reference checks; do not maintain compatibility wrappers for features CodeX no longer exposes.

**Migration policy:** a short-lived transport field used by a *working* consumer is acceptable while changing its producer, but **do not keep two independent gameplay implementations**. Destructive removal is approved for retired CodeX armor paths after their referencers have been migrated and the editor can compile.

## 3. The target core combat-stat set

### 3.1 Resource stats

- `MaxHP` / `CurrentHP` — health.
- `MaxEN` / `CurrentEN` in UI, backed by current `MaxFP` / `CurrentFP` until an intentional API rename.
- `MaxStamina` / `CurrentStamina`.
- Regeneration and action costs remain internal, or displayed where they are a specific implemented passive effect.

Resource values remain authoritative in `AC_CharacterStatsComponent`, not duplicated as fields in `DA_WeaponDefinition` or `DA_OutfitDefinition`.

### 3.2 Weapon stats

| Target property | Meaning | Notes |
|---|---|---|
| **Attack** | Effective direct damage / rating from the active weapon | `BaseDamage` + applicable normalized Attunement/direct modifiers |
| **Impact** | Real guard/stagger pressure, if implemented | Do not invent an Impact mechanic or fake values merely to match mockup |
| **Speed** | CombatStyle/Animset descriptor | Derived or qualitative; avoid duplicated speed authority |
| **Traits** | Explicit implemented abilities/effects | Show only implemented definitions |
| **Attunement compatibility** | Fire, Light, Bleed support | Start broadly compatible, add exceptions only for genuine content reasons |

The currently authored weapons have `BaseAttack` (`FEquipmentChannelValues`), `AttributeRequirements`, `ScalingRules`, `EquipLoadWeight`, family tag and actor class. **R3** replaces the obsolete formulas once current damage callers are migrated. R1 does not touch weapon instances or their active-runtime behavior.

**R3 TARGET definition:**

```text
DA_WeaponDefinition
  WeaponActorClass         Class<BP_WeaponBase>   // existing
  WeaponFamilyTag          GameplayTag            // existing
  AllowedEquipSlots        GameplayTagContainer   // existing
  bOffensiveWeapon         bool                   // existing
  BaseDamage               float >= 0            // new
  Impact                   float >= 0            // if a real impact resolver exists
  AttunementAffinity       float = 1.0            // new
```

`Speed` and `Traits` are presentation derived from real CombatStyle/actions, not a duplicate float/placeholder stat. No per-attribute requirements or S/A/B/C/D/E grades. Weight is removed unless a real design-approved movement/load effect requires it.

### 3.3 Outfit stats

**Only two gameplay concepts:**

```text
DA_OutfitDefinition
  OutfitMeshes             Array<SkeletalMesh>
  DefensePercent           float (display percent, e.g. 5.0 = 5%)
  PassiveBonusType         ECodeXPassiveType (None / compact resource choices)
  PassiveBonusPercent      float (0 when None)
```

No per-piece stats, six damage channels, four legacy resistances, armor requirement checks, stat-grant aggregation or outfit weight/encumbrance.

**R1** applies `DefensePercent` through the existing damage path once. The optional one passive can remain `None` in the prototype; **R2** centralizes its eventual idempotent effect with Talismans using resource/stat modifier-by-source logic.

### 3.4 Talisman stats

**TARGET C2C/R2:** one independently equipable Talisman with one clear passive modifier, optionally affecting Attunement. A Talisman is not an Attunement and cannot equip in its slot. One stable source key must apply and remove its effect exactly once, including after ownership loss or repeated stat refresh. The five existing legacy `Talisman_*` records that reference `DA_Attunement` are not yet valid passive content.

## 4. Exact R1 direct-damage defense contract

### Observed

In the exported `BP_PlayerBase.ApplyDirectDamage` graph, the player takes `EquipmentStats.DamageNegation.Physical`, clamps it to `[0,1]`, and applies damage as `(1 - fraction) * incoming`. No other damage-negation channel is read by **that graph**. Other damage sources still require normal regression checks and should not be assumed to have identical behavior without graph inspection.

### R1 target behavior

```text
OutfitDefenseFraction = Clamp(EquippedOutfit.DefensePercent / 100.0, 0.0, 1.0)
DamageTaken = Max(0, IncomingDamage) * (1.0 - OutfitDefenseFraction)
```

During R1, it is acceptable to write that single fraction into the existing `FEquipmentStatSnapshot.DamageNegation.Physical` **if direct damage already reads it**, rather than refactoring all damage consumers simultaneously. The other old channels must not be populated by four legacy armor definitions. The one Outfit is the **only** equipment-defense authority.

- No Outfit equipped → defense 0, no retained previous modifier.
- Equip/replace/unequip → recalculate once; no old armor values are added.
- `DefensePercent` uses a 0–100 *display* scale, converted once to 0–1 fraction. Never pass `5` directly to a fraction clamp and accidentally grant 100% defense.
- Damage resolution always reads the *current canonical Outfit*, not a cached old four-slot status.
- Guard/block damage and iframes remain distinct mechanics. Outfit defense should not disrupt them.
- Any unused armor resistance/attribute modifier application is removed in R1 rather than preserved as a second fallback.

**R2** replaces the transport with an explicit compact `DefenseFraction` in the new combat snapshot, while preserving existing direct-damage behavior. Do not rename/delete `FEquipmentStatSnapshot` in R1 if the current player/UI graph still depends on it.

## 5. Weapon damage and Attunement (R3 target)

### Active-weapon-only attack

The canonical selected `Equipment.Slot.Weapon.Primary` or `.Secondary` ItemID resolves a supported `DA_WeaponDefinition`. Only the *active* kit contributes weapon damage. Inactive assigned weapons are available for switching, not additive stat sources.

**IMPLEMENTED now:** C1B `AC_EquipmentStatsComponent` uses canonical active weapon resolution, but still evaluates legacy base-attack channels and primary-attribute scaling. This math is not the final CodeX design.

### Lightweight affinity formula

For an illustrative later direct damage bonus:

```text
ResolvedAttack = BaseDamage * (1 + GlobalAttunementDamageBonusFraction * WeaponAttunementAffinity)
```

- `GlobalAttunementDamageBonusFraction` is an internal **fraction** (e.g. `0.15`), not a displayed percent.
- `WeaponAttunementAffinity` is a normalized per-weapon coefficient (default `1.0`).
- If there is no Attunement, bonus fraction is zero.
- A Bleed buildup or Fire status effect is a **separate effect**, not necessarily a direct-damage multiplier. Do not claim all three Attunements increase direct damage identically.
- Final damage is also subject to actual combat action, hit, guard, enemy defense and other implemented mechanics; the formula is not a replacement for the attack pipeline.

These semantics are **TARGET for R3**. Do **not** implement or rebalance them during R1.

## 6. Player attributes and future progression

### Old system status

`FPrimaryAttributes` currently has eight members: `Vigor`, `Mind`, `Endurance`, `Strength`, `Dexterity`, `Intelligence`, `Faith`, `Arcane`. `AC_CharacterStatsComponent` still uses `BaseAttributes`, `EffectiveAttributes`, recalculated MaxHP/FP/Stamina, modifiers and gameplay-resource functions. `AC_ProgressionComponent` still owns Souls and `TryCommitLevelUp`; Status and LevelUp UI call some of these APIs.

These are **current backend dependencies, not a design requirement to keep player-allocated attributes**.

### Target design

- The player does not distribute attribute points. No STR/DEX requirement blocks a weapon kit.
- Base HP, EN and stamina are defined by curated balance settings and adjusted by a small number of discrete passive sources or later skill unlocks.
- Outfit grants one defense value and, optionally, one resource-oriented passive. Talisman is the principal passive specialization slot.
- The Skill Tree later has **Attunement** and **Shadow Strike** lanes. It is not a reincarnation of traditional attribute allocation.
- Resource regen, guard/parry/dodge costs, sprint exhaustion and death are retained even if their underlying formula source changes.
- Currency/Souls may be kept as a progression resource pending the later economy design; do not delete pickups and currency ownership simply to remove LevelUp UI.

**R2/R3 conversion guidance:** migrate real resource maxima and stat consumers to a small balance/config plus source-based modifiers, then delete old unused attribute-dependent functions/structs once the live graph reference map is clean. Do not create a second eight-attribute system in disguise. Do not overhaul resource logic in R1.

### Modifier source policy (R2)

An Outfit passive and Talisman passive must be applied **exactly once per equipped source** and removed when changed/unequipped/lost. Existing `AC_CharacterStatsComponent.SetModifierForSource` / `RemoveModifiersBySource` are useful infrastructure, but their old primary-attribute modifier format may require replacement with a narrower resource/passive format. Use a single calculation path; never double-apply through both Outfit and old armor callbacks.

## 7. Combat-kit and action safety

### Supported family mapping

| Current `WeaponFamilyTag` | Gameplay representation |
|---|---|
| `Weapon.Family.Sword` | SNS SwordShield kit with internal shield companion |
| `Weapon.Family.Katana` | Katana; no shield |
| `Weapon.Family.Twinblade` | Twinblade (singular tag spelling); no shield |
| Missing/unsupported | Unarmed safe fallback |

The weapon family controls physical actors and Animsets through current `BP_PlayerBase`/CombatStyle. It is independent of the new weapon *assignment* slot. R1 Outfit work must not alter these contracts, guard locomotion, hit traces or action authorization.

Switching between Primary/Secondary remains action-gated: no normal user weapon swap through death, cinematic, fog/rest/menu lock, attacks, potions, stagger, dodge, guarded state, Shadow Strike aim/travel/execution unless a supported safe transition is already implemented. Forced inventory loss reconciles deterministically.

### Runtime systems explicitly protected

- HP/FP(EN)/stamina spending, healing, regen and action costs.
- Existing potion montage notify/commit/interruption semantics.
- Guard start/stop, guard Animset cleanup, shield-only restriction, combo and hit animations.
- Shadow Strike targeting/travel/execution, camera, enemy/boss damage and death integration.
- Existing world interactions, resting/bonfire sequences, cinematic input lock and item pickups.

R1 is **not** a reason to rewrite these systems.

## 8. UI data contract

Based on the approved reference screenshots, a concise Equipment detail panel is appropriate:

| Selected item | Show | Do not show |
|---|---|---|
| Weapon | Attack, effective Impact if implemented, speed/moveset descriptor, implemented Traits, Attunement compatibility | STR/DEX requirements, S–E scaling grades, invented stats |
| Outfit | Name, preview, `DefensePercent` (if displayed), one implemented passive description | Body-part slots, six damage negations, multiple resistances |
| Talisman | Passive description and actual resolved effect | Attunement selection, fake multi-attribute columns |
| Attunement | Fire/Light/Bleed description and real effect/potency | Wheel position or all legacy Frost/Poison/Electric |
| Medicine | HP/EN identity, recovery and owned quantity | Unrelated fragment items |
| Artifact | Title, lore, discovered/locked | Equipment/weapon stat panels |

The Character page selects the complete Outfit and displays one character preview. The Equipment page has **six** assignment rows. Inventory/Artifacts/System remain separate. Mockup sword names and values (`Damage`, `Impact`, `Weight`, `Traits`) are **not** verified gameplay data.

During R1, remove obsolete armor presentation in the current Equipment widget while preserving its tested canonical weapon rows and Tool/potion UI. The final five-tab UI and stat visualization redesign is separately scheduled.

## 9. Development milestones and acceptance

| Stage | Implementation | Agent validation |
|---|---|---|
| **R1 — Outfit simplification** | Direct mesh array, one defense stat, no legacy four-piece equip/visual/stats paths; retire referenced armor assets after migration | Data, Blueprint compile, cleanup/referencer checks; **no PIE** |
| **R2 — Compact combat snapshot** | Replace old damage-negation/armor resistance and requirement transport with real small resolved stats + passive sources | Deterministic values, no duplicate stats, existing HP/EN/stamina APIs |
| **R3 — Simplified weapon** | BaseDamage, Impact when real, AttunementAffinity, migrate formulas and UI comparisons | Active-only damage, family fidelity, no scaling requirements |
| **R4 — Cleanup and UI** | Delete now-unused structs/functions, polish item panels, remove old Status/LevelUp UI as separately authorized | No dead references, consistent UI and docs |

### R1 must prove

1. `DA_OutfitDefinition` has no Head/Chest/Arms/Legs-definition properties; `OutfitMeshes[]` points directly to compatible SkeletalMeshes.
2. Old four-piece armor definitions and their runtime references have been migrated away, without retaining a second functional code path.
3. Outfit replacement/unequip/inventory loss destroys previous followers and correctly restores the base appearance; repeat refresh is idempotent.
4. One `DefensePercent` is converted to one damage-mitigation fraction and affects `ApplyDirectDamage` consistently; zero Outfit gives zero Outfit defense.
5. Legacy armor weight, negation aggregation, attribute grants and old UI slot sources no longer produce gameplay effects.
6. C1A–C1C weapon assignments, physical actors, combat, potion wheel and resource systems still compile and are available for manual PIE regression.
7. Destructive deletes occur **only after** live referencer checks and Unreal Editor asset operations. Compiled/saved assets are reported. The agent does not run PIE.

## 10. Notes on scope and source evidence

This revision intentionally replaces previous advice to keep long-lived legacy armor fallback paths and detailed Souls scaling. The **R1 Outfit cleanup is immediately authorized** once direct consumers are migrated; weapon/attribute cleanup waits for the planned R2/R3 refactor to avoid an unrelated combat breakage. The export is a read-only view of a working project, not runtime proof. For implementation, inspect the live Unreal graphs, compile/save and let the user conduct PIE.
