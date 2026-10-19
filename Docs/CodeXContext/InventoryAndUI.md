# CodeX — Inventory, Equipment and Game Menu architecture

**Status:** Approved CodeX architecture; **B0 foundation completed 2026-10-19**; B1.1 SLF reference audit next. The running UE 5.8.3 editor is authoritative. The GitHub Blueprint exporter may omit non-Blueprint UI resources, new local changes or empty folders.

## Goals

Adopt the **Soulslike Framework (SLF)** style of menu information architecture and selectively reuse its visual assets **without importing SLF gameplay logic or making CodeX depend on original SLF widget classes**. Keep CodeX-native player combat, inventory/equipment components, Attunement and item definitions. The old direct-equip Inventory HUD has been retired; do not re-create it.

- **Inventory:** "What items do I own?" `AC_InventoryComponent` holds arrays, quantities and definitions. `WBP_Inventory` is only a browser/actions view.
- **Equipment:** "What is assigned to each equipped position?" `AC_EquipmentComponent` alone owns the slot assignment Map. `WBP_Equipment` shows slots and opens its **own** compatible-owned-item selector.
- **Game Menu:** `WBP_GameMenu` navigates four sibling pages: Inventory, Equipment, Status and System. It does not own inventory arrays or equipment assignments.
- **Permanent HUD:** `WBP_PlayerGUI` remains separate. The working character preview/rotation/capture system must be preserved.

## Canonical UE folder structure

```text
/Game/CodeX_Restruct/
  Frameworks/
    ItemSystemFramework/
      Inventory/
        AC_InventoryComponent
        FInventorySlotData
      Equipment/
        AC_EquipmentComponent
      Items/
        EItemCategory
        FInventoryItemDefinition
        DT_InventoryItemData
        Definitions/Weapons/
          DA_WeaponDefinition
          DA_Weapon_KnightSword
          DA_Weapon_IronSword
          DA_Weapon_KnightShield
      Pickups/
        BP_ItemPickup_Master
        BP_Pickup_*
    PlayerFramework/
      Core/BP_PlayerBase
      Core/InventoryPreview/BP_PlayerBase_InventoryPreview
      # combat, attunement and other frameworks retained
  Game/
    Items/Weapons/                     # existing physical weapon actors
    UI/
      Widget/WBP_PlayerGUI              # permanent gameplay HUD
      GameMenu/
        WBP_GameMenu                    # B0 COMPLETE: empty shell
        WBP_GameMenuButton              # B0 COMPLETE: empty shell
        EGameMenuPage                   # B0 COMPLETE: enum
        Inventory/
          WBP_Inventory
          WBP_InventorySlot
          WBP_InventoryAction
          WBP_InventoryCategoryTab
        Equipment/
          WBP_Equipment
          WBP_EquipmentSlot
          WBP_EquipmentSelectionPanel
          WBP_EquipmentChoiceEntry
          Data/FEquipmentSlotUIData
          Data/DT_EquipmentSlots_UI
        Status/WBP_Status
        System/WBP_System
        Shared/WBP_ItemDescriptionPanel
        Shared/CharacterPreview/WBP_CharacterPreviewPanel  # EXISTING, preserve
  External/
    AssetContent/
      UI/SLF/                           # user-prepared CodeX copies; live-only resources
  Core/Controllers/BP_CPC_PlayerController

# Original source assets — reference only; must not become CodeX runtime dependencies
/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu
/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu_Button
/Game/SoulslikeFramework/Widgets/_Materials/
/Game/SoulslikeFramework/Widgets/_Textures/
/Game/SoulslikeFramework/Fonts/
```

The `External/AssetContent/UI/SLF` subtree contains **user-copied textures, materials and fonts**. The user reports checking material/material-function dependency resolution back into CodeX. **Exact individual copied asset object paths have not been exported; B1.1 must inspect them live.** Do not assume a particular folder nesting or duplicate resources by guessed names.

## Authoritative gameplay data

### `AC_InventoryComponent`

Owns inventory collection slots/quantities and item queries: e.g., `WeaponSlots`, `EquipmentSlots` (legacy-named *owned clothing/armor collection*, not equipped positions), `AttunementSlots`, `ConsumableSlots`, `FragmentSlots`, `AddItem`, `RemoveItemFromSlot`, `GetItemDefinition`, `HasItem`, inventory-change events. `FInventorySlotData`: `bIsEmpty`, `ItemID`, `ItemCategory`, `Quantity`, `SlotIndex`. It must not store the loadout map or UMG focus state.

Current exported `EItemCategory` historically includes `Consumable`, `Equipment`, `Others`, `CombatStyle`, `Attunement`, `Fragment`, `Weapon`; future `Armor`, `Talisman`, `Tool` taxonomy migration is **deferred**. Do not modify `EItemCategory` during Game Menu visual work. `FInventoryItemDefinition` contains item metadata and a `WeaponData` definition reference; actual Data Table rows remain CodeX-native.

### `AC_EquipmentComponent`

**Single loadout authority**, attached to `BP_PlayerBase`:

```text
InventoryRef : AC_InventoryComponent object reference
EquippedItemsBySlot : Map<GameplayTag, Name(ItemID)>
GetEquippedItemAtSlot(SlotTag) -> ItemID, Found
TryEquipItemToSlot(ItemID, SlotTag) -> Success
UnequipItemFromSlot(SlotTag)
IsItemEquipped(ItemID) -> Boolean
ResolveEquippedWeaponClass(SlotTag) -> BP_WeaponBase Class, Found
OnEquipmentSlotChanged(SlotTag, PreviousItemID, NewItemID)
```

Currently tested hand slots: `Equipment.Slot.RightHand1`, `Equipment.Slot.LeftHand1`; compatible weapon definitions have `AllowedEquipSlots` and a physical `WeaponActorClass`. Equipping validates ownership and exact-tag compatibility, updates Map, broadcasts the change; the player separately refreshes spawned/attached weapon Actors. **Do not make UI spawn weapons or implement SLF's equipment backend.** `DA_CombatStyle` continues to own combat animation/moveset choices; equipping and drawing/sheathing are distinct.

Future required integrity contract: selling, dropping or removing the final owned copy of an equipped item must resolve the assignment consistently. Multi-instance upgraded weapons will need persistent instance identities. Neither is part of B1.1.

## Game Menu UX contract — not built yet

```text
WBP_GameMenu                         # CodeX page navigation
  Inventory                           # WBP_Inventory: owned-item browser
  Equipment                           # WBP_Equipment: slot editor
  Status                              # WBP_Status
  System                              # WBP_System

WBP_Equipment
  EquipmentSlotsView                  # generic WBP_EquipmentSlot per position
    click Equipment.Slot.RightHand1
       -> SelectedEquipmentSlot stored *in WBP_Equipment*
       -> compatible-item subview (WBP_EquipmentSelectionPanel)
       -> player picks eligible owned ItemID / Unequip / Cancel
       -> AC_EquipmentComponent interface call
       -> OnEquipmentSlotChanged / player and UI refresh
```

Inventory browsing must **not** act as the Equipment selection subview. `WBP_InventorySlot` does not equip on click. `WBP_GameMenu` and the button receive transient navigation state only. The full UMG layout, navigation wiring, controller input and runtime item filtering remain *planned later*, not part of B0/B1.1.

`EGameMenuPage` holds `Inventory`, `Equipment`, `Status`, `System`. `WBP_GameMenu.ActivePage` uses that enum. `WBP_GameMenuButton.Page` and `OnPageRequested(Page)` were created and typed during B0. The complete asset and dispatcher contract lives in `GameMenuFoundationSpec.md`.

### Initial equipment UI metadata (B0 complete)

`FEquipmentSlotUIData`: `SlotTag : GameplayTag`, `SlotLabel : Text`, `SlotGroup : GameplayTag`, `EmptyIcon : Texture2D object`, `GridRow/Column : Integer`, `bEnabled : Boolean`. `DT_EquipmentSlots_UI` has exactly `RightHand1` and `LeftHand1`. Both `EmptyIcon = None`, and at B0 completion `SlotGroup` remains unset because `Equipment.Group.Weapons` was not yet registered. This optional UI group tag does **not** replace equipment position tags. No speculative future slot records in B1.1.

## SLF visual resource adaptation — B1.1 current scope

The user has migrated selected SLF UI **textures, materials and fonts** to CodeX's **External** subtree, and locally verified that relevant material parents/functions resolve there. Preserve the original reference widgets under `/Game/SoulslikeFramework` for comparison only.

B1.1 is an **inspection-only** task. Compare the live reference `W_GameMenu`, `W_GameMenu_Button`, their widget trees/Brush styles, colors, alignments, selected/hovered states, inputs and animation behavior to the prepared CodeX asset copies. The SLF original includes a Crafting button and its own navigation helpers; CodeX deliberately has **four** pages and its own enum/dispatcher contracts. Do not clone SLF `W_Navigable`, `W_Navigable_InputReader`, SLF controllers/interfaces/actions, or insert SLF widget subclasses into CodeX.

B1.1 should identify the exact local CodeX asset counterparts of the original faded-panel materials, navigation icon textures, fonts and dependencies. **No additional asset copying in B1.1. No CodeX widget layout creation or EventGraph edits.** Distinguish reported user checks from MCP-verified paths. For source visuals confirmed in the SLF Blueprint export, see `GameMenuReferenceAudit.md`.

### Preview and game HUD protection

Preserve `BP_PlayerBase_InventoryPreview`, the retained `WBP_CharacterPreviewPanel`, working transparent capture/compositing resources, lighting/gradient, camera framing and rotation. They are not part of `WBP_GameMenuButton`/`WBP_GameMenu` reference audit or immediate visual reconstruction. Likewise leave `WBP_PlayerGUI` alone.

## Work status and next steps

- **A1–A4:** user-completed Item System restructuring, component separation, legacy UI removal. Newer local changes may not be represented in an older exported Blueprint graph; live Editor wins.
- **B0:** MCP reported creation/save of 16 assets; 13 empty Widget Blueprints compiled, typed variables/dispatchers and two-row slot table verified. Exported definitions confirm the foundation exists.
- **B1 preparation:** user moved/copied SLF visuals to `External/AssetContent/UI/SLF` and checked material graph dependencies. Non-Blueprint files not guaranteed visible in the exporter; trust user report until read-only live confirmation.
- **B1.1 (now):** read-only reference audit and exact reconstruction specification, per `GameMenuReferenceAudit.md`.
- **B1.2 (conditional):** fix only demonstrably missing/misdirected copied visual assets after approval.
- **B2 (future separate task):** implement `WBP_GameMenuButton` then `WBP_GameMenu` with CodeX-native navigation and CodeX-only resource references; only after visual review connect controller input, build other pages and later equipment/inventory logic.

Read `AGENTS.md`, the frozen `GameMenuFoundationSpec.md` and `GameMenuReferenceAudit.md` before scoped Game Menu work. Do not treat the foundation as an instruction to generate assets again.
