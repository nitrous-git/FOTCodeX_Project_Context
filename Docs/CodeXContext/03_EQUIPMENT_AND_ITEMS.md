# 03 — Equipment, inventory and item contracts

**Status: TARGET specification.** This file defines the player-facing **and backing-state** model for the CodeX continuation. Proposed new tags/DataAssets/function names do not yet necessarily exist in Unreal. See `02_EXPORT_AUDIT.md` for the implementation that must be migrated.

## 1. Authority and storage boundaries

```text
DT_InventoryItemData + per-item definitions     static identity and data
                    |
                    v
AC_InventoryComponent                          OWNED ItemIDs / quantities
                    | owned-item queries
                    v
AC_EquipmentComponent                          ASSIGNED slot → ItemID
    |                   |                       + active weapon index/slot
    |                   |                       + active medicine selection
    |                   +--> AC_EquipmentStatsComponent / player stats
    +--> BP_PlayerBase                          physical actors, visuals, animations
    +--> WBP_GameMenu pages / WBP_ItemWheelGrid read-only presentation + requests
```

**Rule:** UI widgets do not store a second authoritative loadout; `BP_PlayerBase` does not become the owner of an independent equipment map. Weapon actor refs in PlayerBase are runtime presentation and combat instances, not the owned-item record.

## 2. Canonical target assignment schema

One per-item assignment authority: `AC_EquipmentComponent.EquippedItemsBySlot : Map<GameplayTag, ItemID>`. Preserve this structure where practicable.

| Target tag (new, proposed) | Player-facing name | Allowed content | UI location |
|---|---|---|---|
| `Equipment.Slot.Weapon.Primary` | Primary Weapon | Offensive weapon kit | Equipment |
| `Equipment.Slot.Weapon.Secondary` | Secondary Weapon | Offensive weapon kit | Equipment |
| `Equipment.Slot.Outfit` | Outfit | Complete authored outfit | Character |
| `Equipment.Slot.Talisman` | Talisman | Passive Talisman | Equipment |
| `Equipment.Slot.Attunement` | Attunement | Fire, Light, Bleed | Equipment |
| `Equipment.Slot.Medicine.HP` | Medicine HP | Health recovery item only | Equipment |
| `Equipment.Slot.Medicine.EN` | Medicine EN | Energy recovery item only | Equipment |

**Seven authoritative assignments total**; only **six Equipment-page rows** because Outfit is selected on Character. The item wheel has **one active-weapon cycle** over the two weapon slots and **one Medicine selector** between HP and EN. Neither Talisman nor Attunement is a quick-wheel state.

Group tags are optional metadata (e.g. `Equipment.Group.Weapons`, `.Outfits`, `.Talismans`, `.Attunements`, `.Medicine`) and must not substitute for exact slot validity.

### No visible hand slots

Retain physical weapon sockets, actual meshes, spawned actors and `DA_CombatStyle` internal primary/secondary weapon actor concepts as needed. The player may still animate a shield in the left hand or twinblades in both hands. **Hand use is resolved inside the selected weapon kit**, never assigned as a second equipment position.

### Definition of a weapon kit

One Inventory `Weapon` record represents a playable combat configuration. It resolves an offensive weapon actor, a weapon family/CombatStyle mapping, and optional supporting visuals/actors (e.g. a shield for sword-and-shield style). A support actor does not require an independently equipable left-hand slot in CodeX.

For the first vertical slice:

- `SwordShield`: a one-slot kit containing sword and its guard/shield presentation. No shield-only loadout. Pure Sword (without supported guard locomotion) is not enabled until its combat implementation exists.
- `Katana`: one self-contained weapon style; no shield companion.
- `Twinblades`: one self-contained pair/style; no shield companion.
- Additional weapon families are introduced only with a working combat style and preview setup.

Whether companion shield visuals are represented by an extra class, actor child, part of a kit definition or an internal preset is an implementation choice; **the gameplay/UI contract is one ItemID per weapon slot**. Preserve working physical actors and collision; do not silently merge everything into one skeletal mesh if that breaks combat.

## 3. Weapon switching contract

**State proposal:** `ActiveWeaponSlot : GameplayTag` with only `Weapon.Primary` or `Weapon.Secondary` valid. Replace active-right-hand semantics once all callers are migrated. A boolean/index could be implemented internally, but expose a stable normalized query such as `GetActiveWeaponItem`.

Required behavior:

1. Equipping into either slot validates the ItemID is owned, offensive, has valid weapon definition/family, and is not already assigned to the other weapon slot. The same ItemID may not occupy both slots in phase one.
2. Equipping while the other slot is active **does not** unexpectedly switch the active kit. If no active kit exists, first valid assignment becomes active.
3. Cycle input swaps Primary ↔ Secondary **only among assigned/valid kits**. Empty slots are skipped; one populated slot remains active; zero populated slots resolves to Unarmed/safe no-weapon presentation.
4. Removing the **active** kit selects the other valid assigned kit immediately; otherwise resolves to Unarmed. Removing an inactive kit leaves active combat unchanged.
5. Switching requests pass through an input/action guard (death, cinematic, fog traversal, potion, combat lock, block transitions). Do not interrupt an unsafe action with a half-swapped actor/Animset.
6. The transition updates the resolved CombatStyle, applicable Animset/guard Animset, runtime weapon actor(s), equipped/unsheathed state as appropriate, player stats, Attunement VFX and wheel/HUD **consistently**.
7. Blocking must cleanly end or transition according to the current action framework, including guard locomotion restoration; switching cannot strand a held-block state.
8. Changed slots emit a single semantic assignment event. Active selection emits a separate active-weapon event. Consumers subscribe rather than poll every Tick.
9. No visual animation of a selected kit implies its stats are active until the transition has committed. If the switch is rejected, keep previous state unchanged.
10. A weapon swap must not erase the player's global Attunement, current HP or EN.

Suggested interface **names only**, to be adapted to the existing Blueprint class:

```text
TryEquipItemToSlot(ItemID, SlotTag) -> Success, ErrorReason
UnequipItemFromSlot(SlotTag) -> Changed
GetEquippedItemAtSlot(SlotTag) -> ItemID, Found
GetActiveWeaponSlot() -> SlotTag, Found
GetActiveWeaponItem() -> ItemID, Found
TrySetActiveWeaponSlot(SlotTag) -> Success
CycleActiveWeapon() -> Changed
ReconcileActiveWeapon()  [internal]
OnEquipmentSlotChanged(SlotTag, PreviousItemID, NewItemID)
OnActiveWeaponChanged(PreviousSlotTag, NewSlotTag)
```

Preserve existing method signatures where changing them would cause broad breakage; the conceptual contract matters more than the proposed names.

## 4. Outfit model

One full outfit is equipped. Outfit choice is made on **Character**, and assigned using the equipment authority under `Equipment.Slot.Outfit`. It is **not** Head, Chest, Arms or Legs as player-facing positions.

**Proposed definition:** `DA_OutfitDefinition` containing a stable OutfitID/name, icon, description, complete mesh composition (full-body or compatible piece references), materials/overrides, headwear hide support, presentation pose/preview settings if necessary, and optionally one modest passive modifier bundle.

Recommended implementation strategy:

- Adapt the existing `DA_ArmorDefinition` mesh plus `AdditionalMeshes` follower pattern to a *single* named outfit visual application, preserving socket/skinned-mesh behavior where it already works.
- One assignment change updates every visual component **atomically**: clear previous outfit-only followers, apply all parts, update material overrides, refresh preview; do not leave old legs/gloves attached.
- Avoid deleting or breaking existing body skeletal mesh base, master pose/leader pose, cloth, or animation followers.
- A hidden-headwear setting should change presentation only; it does not alter ownership, buffs or the selected OutfitID.
- No requirement to have 4–5 outfits ready for the architecture migration. Support that eventual budget; one well-formed test outfit is sufficient for an initial functional stage.
- At most small, nonessential passive buffs. The outfit should not become a new multi-slot RPG progression system.

**Legacy armor:** Existing individual armor assets are input material, not the future item topology. Decide which combinations become complete Outfit definitions, remap starting equipment, then retire armor equipment-slot reliance. Never delete meshes/classes while preview/player consumers still reference them.

## 5. Talisman — truly passive equipment

One equipped Talisman; never a combat-wheel selection. It grants a passive resolved modifier and may enhance the global Attunement. It is **not** an alternate name for Attunement.

**Proposed definition:** `DA_TalismanDefinition` (or lightweight row data) with stable ID, DisplayName, Icon, Description, optional StatModifiers, optional AttunementModifiers and stacking policy. Keep effects data-driven but small. Do not build a generic effect scripting framework for 3–5 simple passive items unless a demonstrated need arises.

**Baseline behavior:** the effect applies once while equipped, updates when it changes, and is removed when unequipped or lost. Re-equipping the same item never stacks the modifier. Prefer stable modifier source IDs (e.g. `Talisman:<ItemID>`) through existing stat/effect hooks.

The export's `Talisman_Fire/Frost/Poison/Electric/Neutral` rows reference `DA_Attunement`; they must be reclassified/migrated as Attunements or retired from CodeX content. They are **not** valid examples of new passive Talismans.

## 6. Attunement — one global selection

Only **Fire, Light, Bleed** are selectable in the final CodeX experience. One is assigned in `Equipment.Slot.Attunement`, and the effective selection applies to the **currently active weapon**; changing weapon re-applies the same Attunement without an additional item-wheel cycle.

The existing `AC_PlayerAttunementComponent` owns actual application to weapon presentation. Equipment owns the assignment; the component is a **runtime applicator**, not a second independent equipment inventory. Synchronize by assignment event and active-weapon-change event; guard initialization order.

- Fire: later offers burn/pressure identity.
- Light: later offers a distinct Light identity (e.g. guard pressure/disruption); **not automatically synonymous with old Electric**.
- Bleed: later offers buildup and threshold/burst identity.

For the model migration, assignments/visual application hooks may use placeholders; full effects/VFX are **deferred**. An internal `Neutral` fallback is allowed for no Attunement. Existing Frost, Poison and Electric assets should not appear as CodeX selections after migration; remove from *active content*, not by blindly deleting their `.uasset` files.

**Compatibility policy:** first continuation allows any supported offensive weapon kit with any of three Attunements unless a specific engine limitation is demonstrated. Avoid authoring a full per-weapon compatibility matrix before it is useful.

## 7. Medicine HP and EN

Exactly two equipment assignments, one Health and one Energy. Both items are owned by Inventory and consumed via the established potion action/montage pipeline.

- `Medicine.HP` accepts only an HP recovery item; `Medicine.EN` accepts only an EN recovery item. Validate explicit recovery kind, not display text or Icon.
- The quick Medicine selector has exactly two selectable categories (skip an unavailable/empty assignment as appropriate); preserve current input and HUD event flow where possible.
- Consumption occurs at the existing confirmed montage notify/commit point; interrupted actions do not silently consume an extra item.
- Item quantities remain Inventory-owned, stack rules stay authoritative, and empty stacks must not appear useable.
- The input/action layer checks whether a potion use is valid (death, cinematic, already using a potion, unable to perform action, full resource if desired).
- On depletion, the assignment can remain as a **typed empty medicine reference** if refill rules require it, or clear it consistently. The first migration should explicitly select and test one model; never let a stale ItemID grant infinite use.
- `Tool_FragmentChampion*` are NOT medicines despite living in the current Tool category.

## 8. Inventory behavior

Keep `AC_InventoryComponent`'s category-based arrays, item definition lookup, `AddItem`, `RemoveItemFromSlot`, quantities, pickups and amount-action validation unless concrete problems demand an isolated change.

Player-facing inventory categories (target): **Weapons, Talismans, Attunements, Medicine, Key Items**. Outfits can be browsed on Character rather than separately listed. Champion fragments appear primarily on Artifacts, not the Medicine grid. If a category has no meaningful items, hide it rather than displaying meaningless tabs. Backend enum names can be migrated incrementally, but category semantics must be correct.

Inventory is a **browser**: selection shows description/preview and contextual actions. It is not the Equipment page's eligible-item chooser and should not directly assign an equipment slot when clicking an item.

Equipment selection receives a `SlotTag`, queries Inventory-owned entries, filters by **explicit category and supported slot**, and returns a selected ItemID to `AC_EquipmentComponent`. One shared picker implementation can support the six Equipment rows and Outfit page while preserving distinct filtering.

### Quantity and equip integrity

- Ownership check at equip time, not only when displaying the picker.
- A final-copy drop, sell, discard or consumed item cannot silently leave active applied effects. Reconcile or reject before successful removal; ensure Player and UI hear the resulting change.
- Duplicate ItemIDs as independent upgraded instances are not yet supported by the current Map contract. Defer item-instance identity migration unless multiple owned copies with distinct properties become a game requirement.
- Do not infer inventory capacity from number of equipment slots; inventory storage count is a separate configurable concern.
- Preserve item pickup/drop actions and world-pickup classes as needed; test them after any category changes.

## 9. Artifacts

Artifacts are collected **Champion Fragments**; `Artifacts` is a separate full-screen presentation page. The authoritative owned/discovered set may be derived from Inventory's `Fragment` category for the first implementation. Do not create a second unsynchronized fragment array purely for the widget.

Target fields: stable FragmentID, champion association, Title, Icon, collectible preview visual (mesh/texture where available), Lore, IsDiscovered / collection state. Locked entries can be silhouettes. Discovery is persistent when save support exists. Artifacts are neither equippable nor consumable Medicine.

The three current `Tool_FragmentChampion01/02/03` rows should retain stable IDs if possible and be remapped to category `Fragment`, with corresponding inventory and UI routing updates. No new boss rewards or narrative unlock system is required for the first migration.

## 10. Explicit failure modes to handle

| Operation | Required result |
|---|---|
| Equip ItemID not owned | Reject, no state/event mutation |
| Assign two identical ItemIDs | Reject duplicate or deterministic policy; default **reject** |
| Equip incompatible category | Reject before changing Map |
| Assign unsupported weapon family | Reject or show unavailable, never fall back to arbitrary Animset |
| Cycle with one valid weapon | No change; retain same active item |
| Unequip active weapon | Switch to other equipped kit or Unarmed |
| Unequip inactive weapon | Active weapon unaffected |
| Equip Outfit | Exactly one complete set of visuals, no leftover pieces |
| Unequip Talisman | Remove passive modifier exactly once |
| Swap weapon under global Attunement | Preserve Attunement and reapply visual/effect to new active kit |
| Use depleted Medicine | Reject; no extra recovery |
| Lose last owned equipped item | Reject removal or reconcile atomically; never stale applied state |
| Equip while input locked | Stable rejection or safe deferred transition, never partial swap |

## 11. Persistence and migration

**VERIFY LIVE:** current save serialization and profile schema. If existing saved ItemIDs/tags are persisted, design a remap for old slot tags and category semantics before deleting any references. If no production save format exists, explicitly clear incompatible test-save state and document that decision. Never assume a clean UI means stale save data is gone.

Do not change the item storage and equipment assignment simultaneously without a checkpoint, source backups and a reversible migration map.
