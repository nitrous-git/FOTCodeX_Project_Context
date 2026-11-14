# 05 — Five-page GameMenu and practical visual specification

**Status: TARGET design, optimized for real UMG construction.** This is **not** an instruction to recreate high-cost cinematic concept art literally. It translates the chosen style into reusable native widgets, static UI materials and existing preview systems.

## 1. Design language

**Visual target:** sober, premium, dark action-adventure. A mostly black/charcoal background with a subtle vignette, warm off-white body text and a restrained bronze accent for active/focused controls. The large negative spaces and clean columns are more important than decorative effects.

**Implementation-friendly visual tokens (initial suggestions, not strict pixel authority):**

| Token | Suggested treatment |
|---|---|
| Background | Single fullscreen near-black fill (`#111113` approximate) with subtle gray vertical/radial gradient; optional faint translucent world blur only if cheap |
| Panel | Translucent charcoal `#19191C` / alpha as needed; no complicated scenic 3D background |
| Primary text | Warm off-white `#E4DED5` |
| Secondary text | Desaturated gray `#9D9892` |
| Accent | Muted bronze `#B59A70` |
| Dividers | 1–2px thin rule, low opacity |
| Selected item | One light line/outline or muted warm fill + slight contrast boost |
| Hover/focus | Same component system as selected, but visually distinguish focused from committed selection |
| Typography | One clean serif for titles + existing legible body UI font; avoid embedding text as textures |
| Icons | Small consistent monochrome silhouettes, tinted by common theme; avoid item-specific hand-painted chrome |
| Transitions | Short opacity fades and subtle position changes, not heavy render effects |

Use existing CodeX-owned UI materials/fonts/icons when suitable and **confirmed live**. Non-Blueprint visual resources may not be in the export. Prefer one reusable `WBP_MenuHeader`, one slot-card style, one item tile, one detail panel, one keycap treatment and one divider treatment. Design for 16:9 first and scale with UMG Anchors/SizeBoxes/ScaleBoxes; avoid a fragile absolute Canvas everywhere.

## 2. Common screen shell

```text
WBP_GameMenu
 ├── BackgroundGradient/Image (single reusable surface)
 ├── Header [Character | Equipment | Inventory | Artifacts | System]
 ├── ContentSwitcher (one visible page)
 │    ├── WBP_Character             [new page asset or repurposed shell]
 │    ├── WBP_Equipment             [existing, reworked]
 │    ├── WBP_Inventory             [existing, simplified]
 │    ├── WBP_Artifacts             [new page]
 │    └── WBP_System                [existing, integrated]
 └── ContextualBottomHelp (page-aware controls)
```

Five tabs in this **exact order**. `EGameMenuPage` currently contains Inventory/Equipment/Status/System, so its serialized values and referencers require a safe migration. The existing `WBP_GameMenu`, `WBP_GameMenuButton`, `RefreshPage`, navigation dispatchers and controller page handling should be **retained/adapted**, not cloned into a parallel menu.

**Navigation behavior:**

- Q/E switch previous/next tab using the existing menu input context or appropriately mapped equivalent.
- Mouse click and gamepad/keyboard focus activate pages reliably; focus and selected page are separate states.
- Opening a menu chooses a deterministic default/last valid page; returning from internal picker restores Equipment and the selected slot.
- Escape/Back closes internal picker first, then applicable modal, then GameMenu (consistent LIFO stack).
- No item Wheel, combat, or movement input can accidentally fire while navigating menu.
- Page widgets initialize from gameplay state and subscribe to changes; avoid per-frame polling.
- Controller owns opening/closing/input mode/focus, not the menu child widgets.

## 3. Character page

**Purpose:** choose a complete Outfit; no character-leveling screen.

Layout:

```text
[LEFT: 4–5 Outfit cards]    [CENTER: full-body player preview]    [RIGHT: outfit details]
```

- Left: simple grid or compact vertical list of available outfits; selected Outfit shows one clean border. Locked/unowned outfits dim or hide according to inventory policy.
- Center: **reuse the existing character preview capture/RT pipeline**, not a separate full 3D scene. Maintain good character framing, cloth/weapon visibility and an unobtrusive background.
- Right: Name, one short paragraph of lore, optional modest passive effect, equipped indication.
- Optional headwear toggle is purely cosmetic; distinguish inspect-preview toggle from persistent chosen appearance.
- Outfit click leads to an eligible Outfit choice or equips directly after eligibility check, then refreshes world character and preview together.
- **Do not** show four armor-piece grids, STR/DEX allocation or a complex body-part editor.

## 4. Equipment page

**Purpose:** inspect/edit six assignment rows and preview each selected item's relevant content.

Layout:

```text
[LEFT ~32%]               [CENTER ~35%]                [RIGHT ~33%]
Primary Weapon            Selected weapon              Name / type
Secondary Weapon          3D preview or icon           Compact properties
Talisman                  (appropriate content)        Traits / effect / lore
Attunement
Medicine HP
Medicine EN
```

For non-weapon items, preview can be an icon/2D illustration rather than a new 3D viewer. This is both cheaper and more consistent with a lightweight item roster.

- Each row: existing `WBP_EquipmentSlot` can be restyled; thumbnail/icon + short label + assigned name.
- Selecting a row opens existing `WBP_EquipmentSelectionPanel` and filters **owned + category-valid** items. The picker is **not `WBP_Inventory`**.
- The right panel uses data relevant to that category: weapons (damage, style, traits, Attunement); Talisman (passive); Attunement (effect); Medicine (resource/recovery). Do not show the entire legacy stat spreadsheet.
- Buttons/keys: Equip/Replace, Unequip if allowed, Switch active weapon, Back. Gray out invalid actions with a concise reason.
- A selected slot's appearance and active weapon slot are **not identical**. Active kit receives a small distinct indicator.
- No visible right-hand/left-hand nomenclature, no four armor slots, no Tool01..05 labels.

**Integration:** `WBP_Equipment` already has `SelectedEquipmentSlot`, local choice-item lists, equipment events, stat sidebar and preview info functions. Reuse these hooks, remove only the outdated visual/navigation content once replacements compile.

## 5. Inventory page

**Purpose:** browse **owned** items, quantities, descriptions and contextual actions. A simpler page is acceptable; Equipment remains the dedicated assignment UI.

Layout:

```text
[Categories]  [Small item grid or list]  [Optional 2D preview]  [Detail panel]
```

- Expected relevant categories: Weapons, Talismans, Attunements, Medicine, Key Items. Artifacts shown in their own collection, while implementation can still reference Inventory ownership internally.
- Item grid/list uses existing `WBP_InventorySlot`, `WBP_InventoryCategoryTab` and `WBP_InventoryAction` where functional.
- Only show contextual actions actually implemented (Use, Drop, Discard, Details, etc.). Do not introduce an Equip shortcut that bypasses `AC_EquipmentComponent`'s slot compatibility validation.
- Reuse `WBP_ItemDescriptionPanel` and `WBP_Inventory`'s current selection/focus events; reduce visual density instead of reconstructing data ownership.
- The grid may be sparse; do not render dozens of inert empty slots merely because inventory arrays have fixed capacity.

## 6. Artifacts page

**Purpose:** narrative record of discovered Champion Fragments.

Layout:

```text
[LEFT: discovered/locked fragment tiles] [CENTER: selected relic preview] [RIGHT: name + lore + progress]
```

A fragment may use a 2D item illustration instead of a custom 3D mesh. A small `Collected X / Total Y` display is enough. No attack/defense statistics; no equip action. A selected undiscovered entry may be hidden with a neutral placeholder.

Existing `Tool_FragmentChampion01..03` content should be reclassified under real Artifact/Fragment semantics before being shown. Avoid creating fabricated champion lore as if it were verified game canon; placeholder text must be labeled as placeholder during implementation.

## 7. System page

Preserve `WBP_System`'s existing selection, preferences and confirmation flows. Put it under the shared five-tab shell; adjust backgrounds, spacing, typography and selection controls only as needed. The exported widget has graphics, audio, camera, preferences, exit/confirmation and title-context logic. Do not recreate settings state from screenshots.

System uses a settings list and a small explanatory panel, not the large item preview. Keep existing Save & Quit / settings application logic intact and test it explicitly.

## 8. Character/item previews

- Character: keep and reuse CodeX's existing preview actor, scene capture, render target, lighting and rotation, verifying actual asset references live.
- Weapon: prefer an existing weapon-preview method or one small reusable capture actor with a clean neutral background. Do not build a complex environment scene per item.
- Talisman/Attunement/Medicine: start with icon/texture; only implement 3D previews when a real asset exists and adds meaning.
- Preview changes are visual only; they may not spawn gameplay weapons, apply combat damage or modify equipped ItemIDs.
- On page close, release any preview-only actor/material instance references according to current component lifecycle.

## 9. UMG interaction/state guidelines

- Responsive root layout via overlays/horizontal boxes/size boxes, not thousands of pixel-offset Canvas elements.
- Shared slot component with data-binding/update function; no one-off hand-painted frame per slot.
- Hover/focus/selected/disabled states are data-driven; align highlight visuals consistently.
- Event-driven changes (`OnEquipmentSlotChanged`, `OnInventoryChanged`, status/preview changes), not polling.
- Avoid asset casts in every tile at Tick; resolve once per refresh.
- Support keyboard/mouse now; controller focus should remain valid even when no items populate a category.
- Back key and menu switching cannot leave a modal selector hidden but input-blocking.
- Keep item icon/tile dimensions stable despite long names; truncate gracefully in UI.
- Confirm visual behavior in a real UMG/PIE preview before claiming reference fidelity. The agent performs structural/compile checks; **user performs PIE testing**.

## 10. Visual acceptance definition

The result should look polished without a large rendering budget. Success means consistent grid/column alignment, typography, contrast, selection language, a working central preview, clear item descriptions, and predictable input. It does **not** require fog, embers, parallax ruins, volumetric lighting or material-heavy brush effects from earlier conceptual imagery.

The previous mockups are **art-direction references**; their sample items, numbers and lore are not gameplay data contracts. Do not reproduce example descriptions/stat values unless they are explicitly adopted into project data.
