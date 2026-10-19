# CodeX — Item, Inventory, Equipment and Game Menu Architecture

**Decision status:** Accepted target architecture, October 18–19, 2026.  
**Export inspected:** `main` at `89cb1449543df9d25465d194e6b1c7e575158cc6` (generated 2026-10-19 04:37:07 UTC).  
**Important provenance:** The user performed an additional **local cleanup after this export**. The live Unreal Editor is authoritative; a still-exported legacy node does **not** prove that it remains in the current project.

## Purpose and design principles

CodeX is adopting **SLF / Soulslike / Elden Ring-style menu semantics** while **retaining CodeX-native gameplay, item records and combat**. SLF is a *soft architectural and UX reference*, not a dependency or an instruction to transplant `AC_InventoryManager`, `AC_EquipmentManager`, its Blueprint widget logic, or its combat systems.

Four responsibilities must remain separate:

1. **Inventory** answers *What does the character own?* Storage, stack quantities, item definitions, item acquisition/removal. `WBP_Inventory` **displays** owned items; the widget never stores the authoritative inventory.
2. **Equipment** answers *What is assigned to each equipment position?* `AC_EquipmentComponent` owns loadout state; `WBP_Equipment` is the dedicated interactive loadout page. Selecting an equipment slot opens the **compatible-owned-item selection view**, not the general Inventory page.
3. **Game Menu** answers *Which page is open?* `WBP_GameMenu` hosts sibling Inventory, Equipment, Status and System pages. No crafting page in this foundation.
4. **Status/System** are independent pages; placeholders are acceptable until their dedicated functionality is implemented.

The permanent gameplay HUD `WBP_PlayerGUI` is **not** the Game Menu.

## Canonical ownership and package paths

All Unreal paths below start with `/Game/CodeX_Restruct`. Keep **one** consolidated Item System; do **not** create separate Inventory Framework, Equipment Framework and Game Menu Framework under `Frameworks/`.

```text
/Game/CodeX_Restruct/
  Frameworks/
    ItemSystemFramework/
      Inventory/
        AC_InventoryComponent
        FInventorySlotData
      Equipment/
        AC_EquipmentComponent
        Data/                         # future equipment rules, if needed
      Items/
        EItemCategory
        FInventoryItemDefinition
        DT_InventoryItemData
        Definitions/
          Weapons/
            DA_WeaponDefinition
            DA_Weapon_KnightSword
            DA_Weapon_IronSword
            DA_Weapon_KnightShield
          Armor/                      # future
          Talismans/                  # future
          Tools/                      # future
      Pickups/
        BP_ItemPickup_Master
        BP_Pickup_*                   # existing subclasses
    PlayerFramework/
      Core/
        BP_PlayerBase
        InventoryPreview/BP_PlayerBase_InventoryPreview
        DataAssets/DA_CombatStyle/    # existing player combat assets
        DataAssets/DA_Attunement/     # existing attunement assets
    ... existing unrelated frameworks ...
  Game/
    Items/Weapons/                    # physical weapon actors; retain as-is
    UI/
      GameMenu/
        WBP_GameMenu                  # PLANNED, MCP foundation
        WBP_GameMenuButton            # PLANNED
        EGameMenuPage                 # PLANNED, or equivalent local enum
        Inventory/
          WBP_Inventory               # PLANNED
          WBP_InventorySlot           # PLANNED
          WBP_InventoryAction         # PLANNED
          WBP_InventoryCategoryTab    # PLANNED, only if useful
        Equipment/
          WBP_Equipment               # PLANNED
          WBP_EquipmentSlot           # PLANNED
          WBP_EquipmentSelectionPanel # PLANNED
          WBP_EquipmentChoiceEntry    # PLANNED
          Data/
            FEquipmentSlotUIData      # PLANNED, UI metadata, not gameplay state
            DT_EquipmentSlots_UI      # PLANNED
        Status/WBP_Status             # PLANNED placeholder
        System/WBP_System             # PLANNED placeholder
        Shared/
          WBP_ItemDescriptionPanel    # PLANNED
          CharacterPreview/
            WBP_CharacterPreviewPanel # exported retained asset
            ... retained capture UI resources ...
      Widget/WBP_PlayerGUI            # existing gameplay HUD; KEEP
  Core/Controllers/BP_CPC_PlayerController
```

**Reality check:** GitHub's Blueprint exporter is not an inventory of all raw materials, textures or empty directories. Confirm live render-target/material locations before renaming or moving preview resources. Planned widget names are **contracts**, not claims that assets exist.

## Gameplay state: `AC_InventoryComponent`

Authoritative owner of inventory collections and item metadata. The current exported component provides:

- Arrays / capacities for `WeaponSlots`, `EquipmentSlots`, `AttunementSlots`, `ConsumableSlots`, `FragmentSlots` (all inventory collections, **not** actual loadout positions).
- `GetItemDefinition`, `GetSlotCountByCategory`, `GetSlotByCategoryAndIndex`, `SetSlotByCategoryAndIndex`, `AddItem`, `RemoveItemFromSlot`, `HasItem` and supporting collection functions.
- Inventory change notification (`OnInventoryChanged`).

`FInventorySlotData` represents *stored inventory entries*: `bIsEmpty`, `ItemID`, `ItemCategory`, `Quantity`, `SlotIndex`. Do not embed equipped positions, weapon actors or UMG selection flags in it.

`FInventoryItemDefinition` currently includes `ItemID`, `ItemCategory`, `ItemName`, `ItemDescription`, `ItemIcon`, `MaxQuantity`, `bCanEquip`, `bCanStack`, `bCanUse`, `WeaponData` plus existing legacy-specialized data and `WorldPickupClass`. It is a data record; the Data Table and weapon definitions remain CodeX's item source.

**The category naming is not fully migrated:** the exported `EItemCategory` contains `Consumable`, `Equipment`, `Others`, `CombatStyle`, `Attunement`, `Fragment`, `Weapon`. `EquipmentSlots` here means **owned legacy armor/clothing inventory items**, NOT map slots such as RightHand1. Future item-taxonomy changes (`Armor`, `Talisman`, `Tool`, etc.) should be a separate deliberate data migration. Avoid casually renaming enumerators or rewriting rows before that migration.

The user reports having removed remaining obsolete `FocusSlot`, old `OnEquipmentChanged`, old `EquipItemFromSlot` and player-owned inventory UI state *locally after the latest export*; **verify live**, do not re-create this obsolete design from the snapshot.

## Gameplay state: `AC_EquipmentComponent`

**Sole authoritative owner of equipped assignments**, attached to `BP_PlayerBase` alongside `AC_InventoryComponent`.

Current exported contract:

```text
InventoryRef                  : AC_InventoryComponent reference
EquippedItemsBySlot           : Map<GameplayTag, Name (ItemID)>
bEquipmentInitialized         : Boolean

GetEquippedItemAtSlot(Tag)    -> ItemID, Found
IsItemEquipped(ItemID)        -> Boolean
TryEquipItemToSlot(ItemID, Tag) -> Success
UnequipItemFromSlot(Tag)
ResolveEquippedWeaponClass(Tag) -> BP_WeaponBase class, Found
OnEquipmentSlotChanged(Tag, PreviousItemID, NewItemID)
```

Slot tags in the tested weapon slice:

- `Equipment.Slot.RightHand1`
- `Equipment.Slot.LeftHand1`

Illustrative assignment: `RightHand1 -> Weapon_IronSword`, `LeftHand1 -> Weapon_KnightShield`. These are **positions**; they are not inventory arrays. Inventory retains owned items when assigned to a loadout slot.

Equipping must validate a real owned item (via `InventoryRef.HasItem`), its definition and slot compatibility before updating `EquippedItemsBySlot`. Current weapon definitions are instances of `DA_WeaponDefinition`, which exposes `WeaponActorClass`, `WeaponFamilyTag`, and `AllowedEquipSlots`. Compatibility uses an **exact Gameplay Tag match** for each requested slot. The equipment component then broadcasts `OnEquipmentSlotChanged` with old/new item IDs; no UMG manipulation inside the gameplay component.

**Important integrity invariant:** Removing, selling, dropping, or consuming the last owned instance of an equipped item must either clear its assignment or reject the operation. This cross-component ownership policy is **not yet a documented fully verified feature**. Design it explicitly when implementing item removal and vendors. Another future concern is instance IDs when the game supports multiple separately upgraded copies of one ItemID.

## Player combat integration (preserve)

`BP_PlayerBase.SetCombatStyleWeapon` resolves weapon classes through `AC_EquipmentComponent.ResolveEquippedWeaponClass` for RightHand1 and LeftHand1; existing spawn, attach, transform, `PrimaryWeapon` / `SecondaryWeapon`, collision and ABP/CombatStyle logic remain CodeX-native. The player reacts to `OnEquipmentSlotChanged` (weapon-hand assignments) by refreshing the physical actors.

**Do not conflate equipped state with drawn/sheath state.** `DA_CombatStyle` remains responsible for attack/defense animation behavior and action contracts, not ownership of physical weapon identity. Preserve `AC_PlayerAttunementComponent`, associated VFX/weapon reapplication, locomotion and `ABP_PlayerMaster`. The general weapon-family-to-CombatStyle decision for all combinations is future work; the working sword-and-shield vertical slice does not establish every combination.

## UI ownership and navigation

```text
WBP_GameMenu
  Navigation buttons (Inventory | Equipment | Status | System)
  WidgetSwitcher_Page
    [0] WBP_Inventory
    [1] WBP_Equipment
    [2] WBP_Status
    [3] WBP_System

WBP_Inventory
  Category filter; owned-item cells; description; inventory actions
  Reads AC_InventoryComponent only for storage/query contracts
  SelectedInventoryItemID lives here (presentation state)

WBP_Equipment
  WidgetSwitcher_Equipment
    [0] EquipmentSlotsView
        weapons / armor / talismans / quick-tool groups
        reusable WBP_EquipmentSlot instances
    [1] WBP_EquipmentSelectionPanel
        compatible owned items (WBP_EquipmentChoiceEntry)
  SelectedEquipmentSlot : GameplayTag (UI-only transient state)
  Reads AC_EquipmentComponent and inventory queries
  Equips only after explicit selection of a compatible owned ItemID
```

The root Game Menu owns navigation/page selection, **not equipped items or inventory arrays**. The Equipment page owns `SelectedEquipmentSlot`, not the root Game Menu, Inventory page, or a gameplay component. `WBP_InventorySlot` should not automatically equip a weapon on click. `WBP_EquipmentSlot` should only report which tag was clicked; the parent manages selection and requests the equipment API.

Preferred interaction:

```text
Click WBP_EquipmentSlot(Tag)
  -> WBP_Equipment stores SelectedEquipmentSlot
  -> Switch equipment subview from slot grid to selection panel
  -> Fetch inventory-owned, exact-tag-compatible items
  -> Select ItemID / Unequip / Cancel
  -> AC_EquipmentComponent.TryEquipItemToSlot(ItemID, Tag)
  -> OnEquipmentSlotChanged
  -> BP_PlayerBase refreshes relevant equipment
  -> WBP_Equipment refreshes; return to slot grid
```

Inventory browse is a **different page**, not the `WBP_EquipmentSelectionPanel`. The selection subview replaces the equipment-slot view **inside** the Equipment page, matching SLF's `W_Equipment` conceptual structure. UI should show empty placeholders for unassigned positions and fallback art for ItemIcon=None.

### Scalable slot configuration

Plan reusable slot UI metadata (`FEquipmentSlotUIData` / `DT_EquipmentSlots_UI`) with `SlotTag` (unique key), label, group, default empty icon, row/column or ordering and enabled state. The **equipment Gameplay Tag must be unique per position**; duplicated tags cannot coexist as independent entries in a Map keyed by tag. `Equipment.Group.*` tags, if used, describe UI groups, not equipped Map keys.

Initial **functional scope**: `RightHand1`, `LeftHand1` with Knight Sword, Iron Sword, and Knight Shield. Reserve the UI structure for additional hands, four armor pieces (Head, Chest, Arms/Gloves, Legs/Greaves), talismans, quick tools, and potential ammunition, but do not implement unrequested categories/rows. Filter owned candidates through an Inventory/Equipment-side helper, not by hardcoding weapon-only logic in UMG. The inventory's present `WeaponSlots` array is the immediate source for weapon candidates.

### Character preview and permanent HUD

Keep the existing `BP_PlayerBase_InventoryPreview`, `WBP_CharacterPreviewPanel`, capture material/render target and rotation controls. Preview should ultimately resolve the **same equipment assignments** as the real player; avoid a duplicate authoritative appearance/loadout state. Latest locally polished capture assets may exceed the export's coverage: inspect live before altering. Do not rebuild/revert working transparent compositing, preview lighting/gradient, capture framing or rotation behaviors in the foundation task.

`WBP_PlayerGUI` remains the in-game HUD and must not be reparented under `WBP_GameMenu`.

## Status and implementation boundary

- **Export verified (2026-10-19):** consolidated item folder structure; both Actor Components; `EquippedItemsBySlot` exists only in Equipment; player weapon resolver targets Equipment; old HUD widget classes absent; retained preview widget exported.
- **User reported local changes after export:** final obsolete-focus/event/direct-equip cleanup. Treat the live project as up to date and confirm only if a targeted operation depends on the detail.
- **Not yet built in export:** root Game Menu, separate Inventory/Equipment pages and reusable equipment-selection widgets. They are the **next MCP foundation assignment**, not completed code.
- **Next stage:** create the clean UMG/widget hierarchy and event/variable contracts in `Game/UI/GameMenu` without reviving old widgets. See `GameMenuFoundationSpec.md`.

## MCP guidance

For Game Menu tasks read this file plus `GameMenuFoundationSpec.md`, then inspect the specific live assets. The user explicitly authorized creating the base widgets and hierarchy. Proceed with routine live asset creation/Designer operations, compile and save; avoid editing exported JSON or renaming `.uasset` files on disk. Do not transplant SLF Blueprints, rebuild player combat, auto-create every speculative future feature, or claim runtime tests based solely on exported graph presence. Consult `AGENTS.md` and `DevelopmentRules.md` for escalation rules.
