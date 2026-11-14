# 01 — Game direction and scope

**Status: TARGET specification — approved continuation philosophy.**

## One game, one focus

*Fragment of the CodeX* (CodeX / FOTCodeX) is an original dark-fantasy **action-adventure Soulslike** built on a mature Unreal combat and exploration foundation. Its distinguishing value is expressive melee combat, responsive enemies and bosses, Shadow Strike, a selective Attunement system, intriguing champions and artifacts, and authored interconnected worlds.

The project is not meant to match the full equipment volume or attribute min-maxing breadth of an orthodox RPG. Content is curated so effort can concentrate on the systems a player actually feels: animations, collision/impact, transitions, encounter pacing, boss movesets, level exploration and presentation.

## Product pillars

### 1. Combat expression over build complexity

Weapons feel substantially different through their family Animset, timing, defense, motion and effects. The player carries exactly two assignable weapon loadouts and swaps the active one. A Talisman and a global Attunement add tactical variation without becoming another inventory-management minigame.

### 2. Curated identity rather than modular armor farming

The protagonist selects one authored, complete **Outfit** from a small collection (initial creative budget: approximately four to five). Each is a strong, recognizable silhouette; internal mesh composition may remain modular for implementation flexibility. Outfits are predominantly appearance/lore, with at most small passive differentiation if later justified. Headwear may be hidden cosmetically.

### 3. Attunements as distinct combat choices

One global Attunement at a time: **Fire, Light or Bleed**. It affects the active weapon's presentation and resolved effect rules. Talismans are separate passive amplifiers. The first migration need only establish correct ownership, assignment, display and integration points; sophisticated elemental combat behavior comes later.

### 4. Champion narrative and artifact collection

Defeated or discovered champions yield distinct fragments of the CodeX. They are permanent collection entries and narrative discoveries, not equippable armor or random Tools. Each can reveal lore and progression state, with optional later mechanical unlocks. Initially the Artifacts screen can be a viewer using existing item records.

### 5. High polish with modest implementation cost

Menus adopt a restrained modern dark-fantasy language: near-black surfaces, a soft charcoal gradient, warm bronze/off-white accents, thin dividers, adequate negative space, a central preview, and deliberate selected/focused states. No complex scenic background environment, custom heavy blur pipeline or animated set dressing is required to achieve the desired result.

## The intended player experience

A player explores interconnected authored areas; encounters enemies; masters two distinct weapon kits, Shadow Strike and selected Attunement; collects Medicine and occasional Talismans; fights champions and bosses; recovers CodeX fragments; rests to recover and later upgrade discrete combat abilities; discovers traversal shortcuts and story context.

The player's regular meaningful loadout questions are:

1. Which **two weapon styles** do I carry?
2. Which **Attunement** do I want globally active?
3. Which **Talisman** complements that choice?

Outfit choice is primarily an aesthetic identity decision. HP/EN Medicine must remain readily accessible in gameplay, not buried in a menu.

## Target equipment surface

| Player-facing surface | Count | Notes |
|---|---:|---|
| Outfits equipped | 1 | Selected in Character page; full ensemble |
| Weapons assigned | 2 | Primary and Secondary; only one active at a time |
| Passive Talisman | 1 | Buff; no combat wheel |
| Global Attunement | 1 | Fire, Light, Bleed; no combat wheel |
| Medicine assignments | 2 | Dedicated HP and EN items, active selection on Medicine quick wheel |
| Artifacts | Collection | Champion fragments, not equipment |

No independent left/right-hand slots appear in UI. Weapon family definitions still determine socket/actor/animation behavior internally.

## Menu architecture

**Main tabs (fixed order):** Character · Equipment · Inventory · Artifacts · System.

- **Character**: outfit thumbnail selection, full character preview, short name/description; no attribute allocation.
- **Equipment**: six visible loadout entries excluding outfit (Primary, Secondary, Talisman, Attunement, Medicine HP, Medicine EN); item-specific selection; large preview; essential stats/traits.
- **Inventory**: owned-item browser with categories; separate from equipment slot picking; lightweight, not mandatory for every equip action.
- **Artifacts**: collected champion fragments, lore, discovery progress.
- **System**: preserve the existing functioning settings flows, adopt shared navigation/styling.

**Rest:** Skill Tree / Travel / Leave. Initially unavailable future actions can be disabled or omitted deliberately; current rest/heal/leave/sit/fade behaviors should be retained. No time-of-day selector or manual Level Up.

## Progression position

There is **no manually distributed STR/DEX/Vigor/etc level-up loop**. Underlying game math may still use resolved attributes, health and stamina. The eventual small Skill Tree has two distinct lanes: **Attunements** and **Shadow Strike**. Weapon upgrade ranks or combat milestone buffs may be layered in later, but do not create a hidden multi-axis RPG in phase one.

## Explicitly excluded from the first migration

- New boss fights, enemy animations, new locales or story content.
- Implementing the Skill Tree UI/upgrade logic or fast-travel destination system.
- Adding new weapon families/animations or every Fire/Light/Bleed VFX/status effect.
- Rebalancing all player damage/defense/economy values.
- Rebuilding the entire System or Main Menu if already functional.
- Rewriting inventory storage, enemy AI, boss orchestration or traversal systems.

## Production decisions for the first playable continuation

Prefer adapting existing `AC_EquipmentComponent`, `AC_InventoryComponent`, `AC_EquipmentStatsComponent`, `BP_PlayerBase` and UMG widgets over new parallel frameworks. Keep each conversion slice compilable and manually testable; structural refactor precedes presentation polish. A separate design can be revisited later only when gameplay needs justify it.
