# 08 — Data migration register: observed source → intended target

**Status:** Planning register based on November 14 export. These are not completed migrations. For each row, a later implementation must verify the live asset, apply the appropriate UE editor operation, update referencers, compile and report.

## 1. Slot schema

### Retire from player-facing model

| Observed slot tags | New target | Notes |
|---|---|---|
| `Equipment.Slot.RightHand1` / `RightHand2` / `RightHand3` | `Equipment.Slot.Weapon.Primary` / `.Secondary` | Three old right-hand assignments collapse into two explicit weapon-kit assignments. Migrate starter selection deterministically; never silently discard owned items. |
| `Equipment.Slot.LeftHand1` | None | Supporting shield may remain internal to SwordShield kit. No independently equippable shield. |
| `Equipment.Slot.Head` / `Chest` / `Arms` / `Legs` | `Equipment.Slot.Outfit` | Convert chosen parts into an Outfit definition; one actual equipment assignment. |
| `Equipment.Slot.Tool01` ... `Tool05` | `Equipment.Slot.Medicine.HP` / `.EN` | Only true recovery items map here; fragments must migrate to collection. |

### Proposed new explicit target Gameplay Tags

```text
Equipment.Slot.Weapon.Primary
Equipment.Slot.Weapon.Secondary
Equipment.Slot.Outfit
Equipment.Slot.Talisman
Equipment.Slot.Attunement
Equipment.Slot.Medicine.HP
Equipment.Slot.Medicine.EN
```

These are **suggested target spellings**. They must be registered in Unreal's Gameplay Tag system; old tag references may need bridging during migration. `DT_EquipmentSlots_UI` should show six Equipment rows while Character shows Outfit separately.

### Current `DT_EquipmentSlots_UI`

The exported table has 13 rows (all enabled): RightHand1/2/3, LeftHand1, Head/Chest/Arms/Legs, Tool01..05, with groups Weapons/Armor/Tools and existing CodeX-owned UI icon paths. The target should use stable new row keys such as `WeaponPrimary`, `WeaponSecondary`, `Talisman`, `Attunement`, `MedicineHP`, `MedicineEN`, backed by `FEquipmentSlotUIData`. Exact visual ordering follows the six-row Equipment spec. Store Outfit UI metadata separately or in a broader data registry if needed; do not create a seventh Equipment-page row.

## 2. Item categories / record mapping

### Observed `EItemCategory` members

`Tool`, `Armor`, `Talisman`, `CombatStyle`, `Attunement`, `Fragment`, `Weapon`.

**Recommended first migration:** keep enum modifications scoped. If category values are re-ordered/deleted, all DataTables, Blueprint comparison nodes and serialized default values must be checked. Do not replace enum members en masse by text or assume UE internal `NewEnumeratorN` values are stable.

### Target meaning

| User-facing category | Target meaning | How to implement minimally |
|---|---|---|
| Weapon | Assigned two-kit weapons | Reuse current `Weapon` category and `WeaponData`; add kit resolver as necessary |
| Outfit | Full authored costumes | New Outfit definition/category or owned-item metadata; one equipped slot |
| Talisman | Passive buff items | Separate `DA_TalismanDefinition`; *do not reuse* old Attunement records |
| Attunement | Fire, Light, Bleed | Existing `Attunement` category and `DA_Attunement` integration |
| Medicine | HP and EN recovery items | Existing Tool category may be an internal temporary alias, but typed Medicine filter is mandatory |
| Fragment | Champion artifacts | Existing `Fragment` enum member, with UI routed to Artifacts |
| Key Item | Non-equip narrative / progression object | Optional new category; defer if no such items exist |

If adding an `Outfit` or `Medicine` enum entry is not needed for the first functioning UI (e.g. category metadata/tag can express it safely), prefer less intrusive internal change. **Semantics and validation matter more than cosmetic enum renaming.**

## 3. Current item rows requiring semantic changes

### Legacy `Talisman_*` (actually Attunements)

| Current `DT_InventoryItemData` row | OBSERVED | TARGET disposition |
|---|---|---|
| `Talisman_Neutral` | Category `Talisman`, references `DA_Attunement_Neutral` | Neutral is internal no-effect fallback; optionally retain as a non-player-facing default. |
| `Talisman_Fire` | Category `Talisman`, references `DA_Attunement_Fire` | Convert to genuine Fire Attunement record; migrate category, rename/display as desired, preserve stable identity if safe. |
| `Talisman_Frost` | Category `Talisman`, refs Frost Attunement | Retire from active CodeX roster; keep asset until unused. |
| `Talisman_Poison` | Category `Talisman`, refs Poison Attunement | Retire from active CodeX roster; keep asset until unused. |
| `Talisman_Electric` | Category `Talisman`, refs Electric Attunement | Retire from active CodeX roster. **Light is a new identity**, not an unreviewed rename of Electric. |

**Additional TARGET rows:** a Light Attunement and a Bleed Attunement item/definition. If items are represented as discoveries, record ownership in Inventory and assignment in Equipment. New passive Talisman items must have **new passives**, not `AttunementData` masquerading as effects.

### Tools and fragments

| Current row | OBSERVED | TARGET disposition |
|---|---|---|
| `Tool_HPPotion` | Tool, equippable, stack max 10 | Typed Medicine.HP assignment, keep potion action/quantity contract |
| `Tool_ENPotion` | Tool, equippable, stack max 10 | Typed Medicine.EN assignment |
| `Tool_FragmentChampion01` | Tool, not equippable | `Fragment` collectible; Artifacts page |
| `Tool_FragmentChampion02` | Tool, not equippable | `Fragment` collectible; Artifacts page |
| `Tool_FragmentChampion03` | Tool, not equippable | `Fragment` collectible; Artifacts page |

Do not infer that all Tool items are medicines. The current category design illustrates why a literal Tool→Medicine enum rename is incorrect.

### Weapons

| Current row | TARGET role |
|---|---|
| `Weapon_KnightSword` | Candidate SwordShield kit (must resolve shield/supporting animation coherently). |
| `Weapon_IronSword` | Another existing sword item; assign a supported kit/family or hide until supported. |
| `Weapon_Katana` | **Present in November 14 export**; self-contained Katana kit with `Weapon.Family.Katana`. |
| `Weapon_KnightShield` | No direct Primary/Secondary assignment. Can be retained as internal support/visual content or converted/remapped if save/story requires. |
| `Weapon_Twinblades` | Self-contained Twinblades kit. Current export uses `Weapon.Family.Twinblade` (singular); verify actual tag registration and style mapping before any rename. |

Current `DA_WeaponDefinition` includes `AllowedEquipSlots`, `WeaponFamilyTag`, `WeaponActorClass`, `AttributeRequirements`, `ScalingRules`, base attack and load weight. Keep it and adapt the allowed slots to Primary/Secondary for valid offensive kits. Avoid making shield-only `bOffensiveWeapon=true` by mistake. `Weapon_Katana` now exists in the export and references `DA_Weapon_Katana` / `BP_Katana`. Validate its combat-style mapping in the live editor.

### Armor/outfits

The latest export has **4 Armor rows** representing individual Head, Chest, Arms and Legs items (down from 23 in the prior snapshot). Do not transfer those four rows directly into four CodeX outfits. Author/assemble a **small selected set** of complete ensembles (initial creative goal: 4–5). Preserve usable body meshes, `AdditionalMeshes` and material references. Legacy piece items should be inactive in the new CodeX equipment UI after a verified mapping; whether to keep them as asset library or remove them is decided by referencer audit, not by this spec.

## 4. Definition class and enum proposals

| Existing / proposed asset | Suggested action | Reason |
|---|---|---|
| `FInventoryItemDefinition` | Extend selectively or add appropriate definition references | Existing ownership/lookup contract is valuable |
| `DA_WeaponDefinition` | Extend for weapon kit support if necessary | Already binds actor + family + combat stats |
| `DA_ArmorDefinition` | Retain temporarily; selectively reuse visual fields | Outfit construction/preview can use existing mesh follower code |
| `DA_OutfitDefinition` | **Proposed new asset** | One named full-body look and optional lightweight bonus |
| `DA_TalismanDefinition` | **Proposed new asset** | True passive buffs independently from Attunement |
| `DA_Attunement` | Keep and add Fire/Light/Bleed records | Existing visual applicator references it |
| `EItemCategory` | Controlled semantic migration only | Broad enum referencers/serialized defaults |
| `EGameMenuPage` | Replace four-page navigation with five pages, carefully | Existing UMG/controller referencers |
| `FEquipmentSlotUIData` | Reuse | Generic slot metadata already suitable |
| `DT_EquipmentSlots_UI` | Replace 13 legacy display rows with six target rows | Explicit target Equipment UI contract |

Names `DA_OutfitDefinition` and `DA_TalismanDefinition` are targets, not existing assets verified in export.

## 5. GameMenu enum and references

**Observed `EGameMenuPage`:** `Inventory`, `Equipment`, `Status`, `System`.  
**Target:** `Character`, `Equipment`, `Inventory`, `Artifacts`, `System`.

Affected: `WBP_GameMenu.ActivePage`, `WBP_GameMenuButton.Page`, request dispatchers, widget switcher/page indices, controller menu functions, `WBP_Status` references and any full-screen page routing. Do not trust old numerical indices after enum change. Verify navigation from every entry point, including close/back when a choice panel is open.

## 6. Functional symbol mapping (rename only when ready)

| Current symbol | Target role | Approach |
|---|---|---|
| `ActiveRightHandSlot` | Active weapon selection | New normalized slot/query + staged migration of listeners |
| `OnActiveRightHandSlotChanged` | Active weapon event | Migrate HUD/Stats/Player/controller listeners |
| `GetActiveRightHandItem` | Get active weapon ItemID | Preserve underlying item lookup semantics |
| `IsValidRightHandAssignment` | Validate offensive kit | Replace hand-specific combination policy |
| `ActiveToolSlot` | Active Medicine selection | Reduce to typed HP/EN variants |
| `CycleActiveToolSlot` | Cycle Medicine | Remove 5-slot traversal/fragment leakage |
| `RefreshAllArmorVisuals` | Apply current Outfit visuals | Reuse atomic clear/apply behavior if sound |
| `GetAssignedRightHandWeight` | Active weapon stats only | EquipmentStats logic must update |
| `ApplyArmorSource` | Outfit/passive modifiers | Remove multi-piece armor assumptions carefully |
| `WBP_Status` page | No main tab | Retain stats backend; widget cleanup after referencer check |
| `WBP_RestTimeEntry` | Not present in target UI | Deactivate old flow; do not delete rest controller indiscriminately |

## 7. Save/recovery migration guidance

- If already persistent, old `EquippedItemsBySlot` tags need explicit mapping. A preexisting `RightHand3` equipped item cannot all fit into two slots; decide migration precedence deterministically and leave remaining owned items in Inventory.
- Old `LeftHand1` shield item does not become a stand-alone secondary weapon; either associate support visual with a SwordShield kit or retain it owned but not assignable.
- Head/Chest/Arms/Legs combinations may not correspond to an authored Outfit. Select a defined default Outfit and preserve those older armor items as owned until save strategy is clear.
- Tool01..05 assignments become distinct HP/EN Medicine assignments only if corresponding owned potion item is present.
- Current `Talisman_*` save records must map to Attunement semantics rather than becoming arbitrary passive Talismans.
- Champion fragment discovery must survive the switch from Tool to Fragment if persisted.

**VERIFY LIVE FIRST:** no guarantees are made that gameplay save/load serializes these fields yet. If it does not, the migration can explicitly reset disposable test saves rather than implement a full migration engine.

## 8. Final data gate

No category migration is accepted until all three agree: **definition category**, **equipment-slot validity**, and **display/picker classification**. A correct looking UI with wrong backend categories is a failed migration.
