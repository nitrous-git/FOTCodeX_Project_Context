# Inventory and UI — current architecture reference

## Exported asset map

```text
CodeX_Restruct/Frameworks/InventorySystemFramework/InventorySystemRefactor/
  Inventory/AC_InventoryComponent/
  Inventory/FInventorySlotData/
  Item/DT_InventoryItemData/
  Item/FInventoryItemDefinition/
  Item/BP_ItemPickup_Master/
  UI/WBP_PlayerInventoryHUD_manual/
  UI/WBP_InventorySlot/
  UI/WBP_SlotContainer/
  UI/WBP_SlotContainer_wName/
  UI/WBP_DescriptionBox/
  UI/WBP_CharacterPreviewPanel/

CodeX_Restruct/Game/UI/Widget/WBP_PlayerGUI/
```

## Current design boundary

The existing inventory workflow uses selection of equipment/weapon/Attunement via the inventory, with a relatively restrained equipment model. `AC_InventoryComponent` already references the Player framework's `DA_CombatStyle` and `DA_Attunement` as well as item data. The project contains multiple widgets that use or reference this component.

The current documentation goal is **not** to migrate another framework's inventory or to create separate weapon-hand and armor slots. Treat more modular equipment/UI layouts as product design questions, not assumptions about what the current implementation must become.

## Relevant exported dependencies

`AC_InventoryComponent/asset.json` lists dependencies on `FInventorySlotData`, `DT_InventoryItemData`, `FInventoryItemDefinition`, `BP_PlayerBase`, `AC_PlayerAttunementComponent`, `DA_Attunement` and `DA_CombatStyle`. Referencers include `BP_ItemPickup_Master`, inventory slots/containers, description widget, inventory HUD and inventory preview actor.

## Agent investigation protocol

For inventory or UMG issues, inspect the item definition and slot structure, `AC_InventoryComponent` API, all consuming widgets, and `BP_PlayerBase` integration. Verify widgets in the actual editor: exported graphs and UMG metadata can be incomplete representations of widget layout, animation and binding behavior. Do not invent panel or equipment-slot capabilities absent from the exported snapshot/live asset.
