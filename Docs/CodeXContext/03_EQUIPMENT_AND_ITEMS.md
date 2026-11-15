# 03 — Equipment, inventory and item contracts

**Fragment of the CodeX (FOTCodeX) — Revised CodeX model**  
**Revision:** 2026-11-15  
**Authority:** approved CodeX direction; source baseline is the November 15, 2026, Unreal 5.8.4 context export (`18f5dddef026cae964b3cdc0574fd3f1385b1d90`).  
**Status convention:** **IMPLEMENTED** = present in that export; **TARGET** = approved desired model; **R1/R2/R3/R4** = planned implementation step, not yet completed.

## 1. Design principles

CodeX is a curated combat-focused action-adventure Soulslike, **not** a traditional eight-attribute, four-piece armor RPG. The existing working equipment, weapon action, inventory and combat infrastructure is valuable, but legacy concepts should be **removed after their direct referencers are migrated**, not preserved through long-lived duplicate implementations.

- One Outfit, primarily aesthetic, with **one modest defense value** and at most **one small passive bonus**.
- Two assigned weapon kits, **Primary** and **Secondary**, with one active selection. Weapon styles come from existing CombatStyle/Animset assets.
- One passive Talisman and one global **Fire / Light / Bleed** Attunement, separate from each other and from the wheel.
- Two Medicine assignments, HP and EN, retaining the working potion action and quantities.
- Champion Artifacts are a collection/lore record, not equipment.
- No visible hand slots, armor-part slots, STR/DEX scaling, manual attribute requirements or old four-part resistance spreadsheet.
- Player-facing item detail should present a **few mechanically meaningful properties**, not an RPG accounting sheet. Sample weapons, numbers, names and effects in UI mockups are art-direction references, not automatically canonical item data.

### Source-of-truth boundaries

```text
DT_InventoryItemData + typed definitions     identity / static item data
                   |
                   v
AC_InventoryComponent                       owned ItemIDs / category storage / counts
                   |
                   v
AC_EquipmentComponent                       one map: Equipment.Slot.* -> ItemID
                   |                       active Weapon / active Medicine
         +---------+---------------+
         |                         |
         v                         v
BP_PlayerBase / actions          AC_EquipmentStatsComponent
runtime weapon actors,           normalized combat/defense snapshot
CombatStyle, outfit visuals      + AC_CharacterStatsComponent resources
         |                         |
         +------------+------------+
                      v
              HUD / GameMenu widgets
          (read state and submit requests)
```

No widget, Preview actor or PlayerBase visual component independently owns another loadout or separate Outfit selection.

## 2. Canonical equipment assignments

**TARGET authoritative storage:** existing `AC_EquipmentComponent.EquippedItemsBySlot : Map<GameplayTag, ItemID>`.

| Gameplay Tag | Content | Player-facing place | Status |
|---|---|---|---|
| `Equipment.Slot.Weapon.Primary` | Offensive weapon kit | Equipment | IMPLEMENTED (C1A–C1C) |
| `Equipment.Slot.Weapon.Secondary` | Offensive weapon kit | Equipment | IMPLEMENTED (C1A–C1C) |
| `Equipment.Slot.Outfit` | Complete Outfit ItemID | Character | IMPLEMENTED baseline in C2B; definition is being simplified in R1 |
| `Equipment.Slot.Talisman` | One genuine passive Talisman | Equipment | TARGET |
| `Equipment.Slot.Attunement` | One global Fire, Light or Bleed | Equipment | TARGET |
| `Equipment.Slot.Medicine.HP` | HP recovery item | Equipment | TARGET |
| `Equipment.Slot.Medicine.EN` | EN recovery item | Equipment | TARGET |

**Seven slots** total; **six rows** on Equipment because Outfit is selected on Character. Do not introduce additional right/left-hand or Head/Chest/Arms/Legs equipment assignments. Use exact slot validation, not a permissive group tag alone.

### Weapon state and integrity (already functioning)

- Only owned, offensive and supported `Weapon` items may occupy Primary/Secondary.
- The same ItemID cannot be assigned to both slots.
- UI selection does not switch the active weapon; the active selection is separate.
- Cycling skips empty slots. Removing an active weapon selects the other equipped valid kit, or Unarmed if none remains.
- Input checks action safety before a normal cycle. Forced item loss/equipment changes reconcile without permanent visual/state desynchronization.
- Active kit changes update the physical weapon actor(s), CombatStyle, Animset, stats, Attunement reapplication and HUD via explicit events.
- Sword-style kit may spawn an **internal shield companion**; Katana and Twinblade do not. This is a kit implementation detail, not an equippable shield-only loadout.
- Physical `PrimaryWeapon`/`SecondaryWeapon` actor variables in `BP_PlayerBase` do **not** mean Primary/Secondary *equipment slots*.

**IMPLEMENTED:** canonical C1A–C1C APIs (`GetActiveWeaponSlot`, `GetActiveWeaponItem`, `TrySetActiveWeaponSlot`, `CycleActiveWeapon`, `OnActiveWeaponChanged`), PlayerBase integration and two-slot UI. Preserve these tested contracts throughout later work.

## 3. Outfit — one self-contained item (R1 TARGET)

### 3.1 What changes

The November 15 export has `DA_OutfitDefinition` containing `HeadDefinition`, `ChestDefinition`, `ArmsDefinition` and `LegsDefinition`, each pointing at `DA_ArmorDefinition` with separate meshes, `AdditionalMeshes`, weights, six damage negations, four resistances and primary-attribute bonuses. That is **temporary legacy structure** and is explicitly **not** the target.

The Outfit definition is one complete visual composition and one compact stat source. No per-body-part definition, no Head/Chest/Arms/Legs equipment slot, no aggregate armor calculation. The mesh assets may be individual helmet/chest/gloves/pants/shoes components internally, but they are just **entries in one list**, not independent equipment items or stat sources.

### 3.2 Final `DA_OutfitDefinition` schema

| Field | Type | Meaning |
|---|---|---|
| `OutfitMeshes` | `Array<SkeletalMesh>` | Ordered meshes constituting the complete look; no body-part classification |
| `DefensePercent` | `float` | Single modest incoming-damage reduction, expressed as percent (e.g. `5` = 5%) |
| `PassiveBonusType` | `ECodeXPassiveType` | Optional one-effect identifier; default `None` |
| `PassiveBonusPercent` | `float` | Magnitude of the optional effect; default `0` |

**Do not duplicate** the ItemID, localized ItemName, ItemDescription or ItemIcon in the Outfit DataAsset: they already exist in `FInventoryItemDefinition` / `DT_InventoryItemData`.

Suggested restricted `ECodeXPassiveType`: `None`, `MaxHPPercent`, `MaxENPercent`, `StaminaRegenPercent`. This is a tiny effect vocabulary, **not** a general-purpose stat-buff scripting framework. An Outfit supplies **at most one** bonus; its actual application is centralized with other passive sources in **R2**, not duplicated in PlayerBase. R1 may leave the development Outfit at `None` / `0` rather than implementing a second modifier engine.

`DefensePercent` is a **display percentage**, not a fraction. The gameplay resolver converts it to a clamped `[0,1]` fraction once at the boundary. A nominal 5% Outfit defense means `5 / 100 = 0.05` incoming damage reduction; it is **not** five defense points and it is not added to four legacy armor contributions. High-value balance caps and exact passive strengths will be decided separately; keep values small now.

**Not required in R1:** item-specific animations, individual part weights, per-type elemental resistances, material override systems, armor upgrade grades, outfit-comparison math, or a Headwear toggle. Add those only when a concrete design/use case justifies them.

### 3.3 Physical presentation

- Each valid `OutfitMeshes` entry becomes a visual follower skeletal mesh component attached/aligned with `BP_PlayerBase.SK_BaseBody` and synchronized using the existing **Leader Pose** pattern.
- Maintain a single `OutfitFollowerComponents : Array<SkeletalMeshComponent>` (or equivalent) to track components created for the current Outfit. Clear/destroy those **once** before applying a replacement. Do not mix them with weapons or base-body components.
- Avoid an extra permanent component per possible body region; the list may contain 1, 5 or more meshes. Use the same base transform/leader pose conventions already proven by the existing armor followers; preserve skeleton compatibility and visual alignment.
- On equip, replace, unequip, inventory loss and initialization, refresh from **one** assigned Outfit ItemID. When none is equipped, show the base body/default appearance with no orphan followers.
- If a referenced SkeletalMesh is invalid/incompatible, report it and handle it safely; never crash or attach incompatible assets silently. The outfit definition is still one logical item.
- Outfit selection is cosmetic-first and should **not** affect weapon actors, CombatStyle, guard state or damage traces.
- The future Character page and preview capture must read the same assigned Outfit and render the same appearance. Do not implement the Character page in R1.

### 3.4 R1: direct legacy retirement, not dual-mode support

**Approved migration policy:** do not maintain a legacy Head/Chest/Arms/Legs fallback after R1. Work directly to the target model, migrate affected consumers, then remove obsolete code and assets when reference checks are clean.

- Populate `DA_Outfit_Prototype01.OutfitMeshes` from the **actual skeletal meshes** presently referenced by the four prototype armor definitions, including additional shoes/other followers. Do **not** keep references to those armor DataAssets.
- Rework `BP_PlayerBase.RefreshOutfitVisuals` around this array; remove old `RefreshAllArmorVisuals`, `RefreshArmorSlotFromEquipment`, `ApplyArmorPieceVisual`, `GetArmorComponentForSlot` and old `SK_Armor_Head/Chest/Arms/Legs` runtime paths once migrated. Retain `SK_BaseBody`, animation leader-pose behavior and unrelated weapon components.
- Remove the four legacy Head/Chest/Arms/Legs rows from Equipment slot metadata and adapt equipment widgets so the player cannot assign them. Outfit remains a Character-page item, with a **development-only** equip/unequip path during R1.
- Remove legacy armor contribution calls and modifier sources from `AC_EquipmentStatsComponent`. `DefensePercent` is sourced **once** from the canonical Outfit. The existing combat snapshot may remain as a narrow transport adapter until R2, but there must be **no alternative legacy armor stat implementation** left running.
- Remove or retire legacy `Armor_*` item table entries and starter grants after confirming no test content/save references need remapping. The four armor definition instances and `DA_ArmorDefinition` may then be deleted with Unreal asset tools when their live referencers are zero. No reason to keep them solely as Blueprint wrappers around visual meshes.
- Remove `FInventoryItemDefinition.ArmorData` and dead armor-specific UI/stat helpers if all real referencers are migrated. The shared `Armor` enum member/storage can be made inert or removed if serialized enum safety permits; do **not** spend a separate compatibility project maintaining its behavior. If a particular enum field must remain to avoid corrupting other category values, treat it as a dead compatibility identifier, not a live feature.

**Before destructive edits:** use a recoverable checkpoint, inspect *direct* referencers, migrate them, compile, then delete through Unreal Editor asset APIs with redirector fix-up. Never delete `.uasset` using filesystem operations. Do not delete skeletal mesh files from third-party packs; the Outfit uses those source meshes directly.

### 3.5 Ownership and validation

One Outfit item is owned in `AC_InventoryComponent.OutfitSlots`. The equipped slot is `Equipment.Slot.Outfit` in the existing Map. `TryEquipItemToSlot` validates the item is owned, has category `Outfit`, has a valid `OutfitData`, and contains at least one usable mesh. Duplicate equip is idempotent, replacement is atomic, and unequip/inventory loss clears appearance and its one defense contribution. Widgets do not cache an alternative outfit state.

**IMPLEMENTED snapshot:** `Outfit_Prototype01` is already a 20th row in `DT_InventoryItemData`; its existing ID should be retained. `DA_Outfit_Prototype01` is an editable development definition, not final cosmetic content. R1 replaces its internals in place.

## 4. Weapon definition — simplified CodeX combat model (R3 TARGET)

Weapons remain the game's main combat differentiators. They do **not** require STR/DEX eligibility tests, per-attribute grades or six separate base-damage channels.

| Field | Type / semantics | Status |
|---|---|---|
| `WeaponActorClass` | Existing class reference | Keep |
| `WeaponFamilyTag` | Existing Gameplay Tag for CombatStyle/Animset | Keep |
| `AllowedEquipSlots` | Primary/Secondary allowlist | Keep |
| `bOffensiveWeapon` | Offensive kit validation | Keep |
| `BaseDamage` | `float`, nonnegative | R3 replaces multi-channel `BaseAttack` for ordinary weapons |
| `Impact` | `float`, nonnegative | R3 adds only when a real guard/stagger resolver is connected; until then disabled/zero |
| `AttunementAffinity` | `float`, default `1.0` | R3 global affinity coefficient used by future Attunement calculation |
| `Speed` | Derived from style/animation, not authoritative duplicate float | UI may show Fast/Normal/Slow if backed by real data |
| `Traits` | Optional references to implemented effects | UI only shows real implemented traits |

**R3 removes:** `AttributeRequirements`, `ScalingRules`, S–E scaling grades and per-attribute damage factors. `EquipLoadWeight` and the load ratio are candidates for removal in R3 if no actual movement/encumbrance mechanic uses them. Do **not** change these weapon fields in R1; they remain necessary to the currently working damage pipeline until R3 migrates it.

### Canonical weapon-family constraints

- `Weapon.Family.Sword` = current SwordShield runtime kit, internally accompanied by shield when required.
- `Weapon.Family.Katana` = Katana family, no shield.
- `Weapon.Family.Twinblade` = **singular current tag spelling**, no shield.
- The Shield source asset is not independently equipable in Primary/Secondary.
- Unresolvable weapon → safe Unarmed presentation.
- The ItemWheel contains one active weapon icon and switches between assigned Primary/Secondary through established action guards. Do not rework these tested systems during R1.

## 5. Passive Talisman (C2C TARGET)

One Talisman equipped in `Equipment.Slot.Talisman`, no wheel slot. It grants a small, clearly described passive or assists Attunement. Use one typed Talisman definition or a compact `ECodeXPassiveType` + magnitude if appropriate; do not build a Souls-like multi-attribute item formula. Buff application/removal occurs through a unique source ID, is deterministic and cannot stack on re-equip or repeated refresh.

**OBSERVED legacy issue:** the existing `Talisman_Neutral`, `Talisman_Fire`, `Talisman_Frost`, `Talisman_Poison` and `Talisman_Electric` rows reference `DA_Attunement` and are **not genuine passive Talismans**. C2C reclassifies them or retires them; don't rename old Attunement assets as passive buffs.

## 6. Global Attunement (C2C + R3 TARGET)

The sole `Equipment.Slot.Attunement` assignment selects **Fire**, **Light** or **Bleed** for either active weapon. Neutral/no-effect is a safe internal fallback. The existing `AC_PlayerAttunementComponent` remains responsible for applying visuals and effective Attunement state to player weapons, but is not a second owner of an independently selected item.

A later normalized combat resolver uses the selected Attunement plus the weapon's **one** affinity coefficient. Illustrative direct-damage formula:

```text
EffectiveDamage = BaseDamage * (1 + AttunementDamageBonus * AttunementAffinity)
```

Here `AttunementDamageBonus` is a **fraction** (e.g. `0.15`), unlike `Outfit.DefensePercent` (e.g. `5`). Actual Fire/Light/Bleed effects and Bleed buildup are separate gameplay mechanics. A special effect may be shown in the item detail panel **only when implemented**; mockup sample traits or numbers are not canonical content. No elemental compatibility spreadsheet or A–E scaling grades.

The November 15 export still has Neutral/Fire/Frost/Poison/Electric Attunement definitions. Light and Bleed require authored data later. Do **not** introduce them during R1.

## 7. Medicine HP / EN (C2D TARGET)

Exactly two typed assignments, `Equipment.Slot.Medicine.HP` and `.EN`. Inventory owns item stacks; equipment owns assignments; the existing potion montage/notify and resource-commit chain remains authoritative.

- HP slot accepts HP recovery item only; EN accepts EN only, using a typed recovery identifier rather than label text.
- Keep current interrupt, quantity, rest/refill and input safety behavior until C2D migrates those routes.
- The Medicine wheel toggles between HP/EN, not a five-slot Tool array. Do not show Talisman, Attunement or Artifact on the wheel.
- On depletion, keep a consistent empty/unavailable state without granting free recovery. Exact refill/persistent assignment behavior is decided against live potion semantics.

**OBSERVED:** `Tool_HPPotion` and `Tool_ENPotion` still exist as Tool items. This is expected until C2D; R1 does not touch them.

## 8. Artifacts (C2D / later UI TARGET)

Champion Fragments are permanent collection/lore entries, **not** Medicine or equipment. Keep `Tool_FragmentChampion01/02/03` ItemIDs where possible while migrating category ownership to `Fragment` storage. Artifacts UI reads collected entries from Inventory rather than maintaining a second discovery list. No equip or direct combat-stat bonuses in this migration.

## 9. Inventory and UI responsibilities

Inventory category storage has been extended in C2A to support `Outfit`, `Medicine`, `Attunement` and `Fragment`, alongside the current Weapon/Armor/Talisman/Tool arrays. **Do not change existing category enumeration values casually**; values and switch-case pins are serialized in Blueprints and DataTables.

Final main tabs: **Character, Equipment, Inventory, Artifacts, System**. Character selects one Outfit; Equipment shows Primary/Secondary/Talisman/Attunement/Medicine HP/Medicine EN; Inventory browses owned items; Artifacts presents collection. System reuses its existing settings. These full UI changes are later work; R1 only removes obsolete *armor* presentation and provides development Outfit testing access.

The shared Equipment picker must use equipment-component validation, not a parallel slot policy. Replacing an item is atomic. Selection/focus does not imply activation. UI stats read resolved gameplay data; never independently sum armor parts or make up weapon Impact, Traits, compatibility or damage values.

## 10. Migration roadmap and non-negotiable R1 acceptance

| Phase | Scope | Rule |
|---|---|---|
| **R1** | Self-contained Outfit data + direct legacy armor retirement | **No four-definition references, no four-piece fallback, one DefensePercent source** |
| **R2** | Slim unified combat-stat snapshot and passive-modifier sources | One defense, one active weapon, resource modifiers; no duplicate armor math |
| **R3** | Weapon BaseDamage/Impact/Affinity + simplified Attunement combat calculation | Remove primary-attribute requirements and scaling grades after migrating callers |
| **R4** | Final cleanup / UI stat presentation / docs | Delete dead compatibility structures after live referencer checks |

**R1 acceptance gates:**

1. `DA_Outfit_Prototype01` references meshes directly via a single array; no `DA_ArmorDefinition` links.
2. One Outfit item and one `Equipment.Slot.Outfit` assignment are the only selection authority.
3. Outfit replace/unequip/inventory-loss clears old followers and applies the new set exactly once.
4. The old Head/Chest/Arms/Legs gameplay equip, visuals and armor-stat paths are gone—not left as live alternatives.
5. `DefensePercent` affects the existing direct-damage mitigation path **once** and is zero when no Outfit is equipped; percent unit conversion is explicit.
6. The old four armor row content/Equipment UI references and relevant Blueprint/data-only assets are removed or demonstrably dead after safe referencer cleanup. No permanent legacy fallback for test convenience.
7. Existing canonical weapons, guarding, potions, HUD, HP/EN/stamina and Shadow Strike keep functioning; the agent only does compilation/structural checks. **The user performs PIE.**

## 11. Integrity / safety

- Validate ItemID ownership and definition type before equip; reject invalid items without side effects.
- Prevent duplicate event subscriptions or repeated follower creation.
- Avoid stale buffs after unequip, inventory removal or reload.
- Make no edits to unrelated enemy, boss, traversal, cinematic or weapon-family systems for R1.
- User-approved destructive simplification applies to **deprecated Outfit/armor machinery**, not to unrelated working systems. Delete assets using Unreal Editor asset operations, resolve referencers and redirectors, then compile/save intentionally changed packages only.
- Snapshot/schema details can be observed in GitHub's export; all runtime/asset operations must be verified against live Unreal Editor assets.
