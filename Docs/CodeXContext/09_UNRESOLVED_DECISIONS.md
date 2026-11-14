# 09 — Open decisions and safe defaults

**Purpose:** prevent accidental over-design while preserving necessary decisions for later iterations. These are not blockers to preparing docs. User-approved requirements take precedence; defaults below guide the first conversion unless changed deliberately.

## A. Settled design decisions

| Topic | Decision |
|---|---|
| Game identity | Original combat-focused CodeX action-adventure Soulslike |
| Weapon slots | Exactly two visible assignments, Primary and Secondary |
| Hand handling | Internal only; no user-facing Right/Left equipment slots |
| Weapon selection | One active-weapon cycle, two assigned choices maximum |
| Outfit | Exactly one full outfit; small curated set, not four-piece equipment |
| Talisman | One passive item; no quick wheel |
| Attunement | One global Fire/Light/Bleed selection; no quick wheel |
| Medicine | Exactly two slots, HP and EN, using established potion actions |
| Artifacts | Champion CodeX fragments as collection/lore screen |
| Menu tabs | Character / Equipment / Inventory / Artifacts / System |
| Stats | No manually distributed attributes or classic Level Up page |
| Rest | Skill Tree (later), Travel (later), Leave; no time selection |
| UI art direction | Polished but minimal dark background; cheap to reproduce in UMG |
| Implementation priority | Existing framework adaptation first, visual rework second, later features last |
| PIE ownership | User tests PIE; implementation agent compiles and does structural checks |

## B. Recommended initial defaults, still adjustable

| Topic | Default for first migration | Why |
|---|---|---|
| Weapon duplicate assignments | Reject same ItemID in both slots | Existing ItemID-only ownership, no instance-level weapons |
| No active weapon | Unarmed safe fallback | Deterministic equipment state |
| Valid weapon switch during locked action | Reject now, do not queue | Reduces montage/combat transition risks |
| Shield | Internal companion of supported SwordShield kit | Avoids incompatible combinations and extra assignment UI |
| Attunement compatibility | All supported offensive kits accept all three | Reduces authoring/validation burden |
| Outfit buffs | Cosmetic-first; at most minor passive | Avoids creating heavy gear progression |
| Inventory page | Keep as lightweight owned-item browser | Existing inventory actions and key items need a home |
| Attunement without item | Neutral/no-effect fallback | Safe start/load state |
| Fragment source of truth | Inventory-owned `Fragment` entries | Avoid duplicate collection state |
| Item previews | 3D only for character/weapon if available, otherwise icon | Feasible visual budget |
| UI visual effects | Static gradient + simple focus highlight | Low complexity and dependable UMG implementation |
| Rest unavailable actions | Disabled, visible only when useful | No empty click behavior pretending feature exists |
| Save compatibility | Verify first; explicit disposable test reset if no formal schema | Prevents overbuilding premature migration code |

## C. Questions to settle when implementation reaches them

### 1. SwordShield kit support

Which actual shield Actor/mesh is bound to each SwordShield offensive weapon? Could be fixed to a style or defined in a weapon-kit data asset. The user-facing requirement is clear; exact assembly implementation can follow the existing PlayerBase graphs.

### 2. Outfit representation

Whether the first outfit is a full skeletal mesh with additional follower pieces, or a data-defined list of existing armor meshes. The UI/data contract is one Outfit. Reuse the existing successful approach for the live assets instead of imposing one from docs.

### 3. Attunement combat effects

Specific numerical tuning and Light/Bleed special mechanics are not yet locked. Support the three *identities* and clean hooks first, then tune in dedicated combat tasks. Example descriptions/mockup numbers are not canon.

### 4. Talisman buffs and interaction strength

Exact passive effects and synergy multipliers remain content-design tasks. The architectural requirement is one independently authored passive with non-stacking deterministic application and removal.

### 5. Outfit gameplay contribution

Predominantly aesthetic. Whether each outfit offers one minor identity perk remains optional and must not recreate four-piece armor optimization.

### 6. Weapon growth and upgrade rank

The stat framework already supports attribute scaling. Whether to continue hidden defaults, introduce weapon rank, mastery or milestone growth is a **later balance decision**. Do not expose STR/DEX point allocation simply because it exists in the code.

### 7. Medicine depletion and rest refill

The existing Item/Tool potion pipeline may be quantity-driven with rest refills. Decide whether zero quantity means temporarily unavailable assigned potion or an unequipped slot after examining actual live behavior. In either case, never let an empty stack grant recovery.

### 8. Save/load schema and old test profiles

Discover whether `EquippedItemsBySlot`, `SelectedPotionID`, attributes, current rest state or CodeX fragments are serialized. Migrate only if necessary; destructive test save reset requires explicit acknowledgment.

### 9. Inventory scope

Long-term the Inventory tab could shrink further if every key item/artifact has its own page. Keep it for now so owned-item browsing and inventory actions remain coherent. Remove only after testing actual player need.

### 10. Artifact-to-skill relationship

Champion fragments may later unlock abilities or deepen the two-lane Skill Tree. For the first pass they are a collection/lore resource and **not currency or loadout buffs** unless separately specified.

### 11. Skill Tree and Travel

No screens, node counts, upgrade values, travel destinations or save systems are authorized by the continuation plan. Retain placeholder affordances only when safe; implement later as distinct features.

## D. How to resolve new uncertainties

Prefer the least disruptive implementation compatible with the settled contract. Record each real choice after a live audit in the documentation and label it **Implemented** only when compiled/saved. If a choice would require destructive refactors of unrelated systems, isolate it as a separate review, not an opportunistic edit.
