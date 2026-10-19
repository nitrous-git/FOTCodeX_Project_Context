# CodeX — B1.1 SLF Game Menu Reference Audit (READ-ONLY)

**Status:** current next MCP task, 2026-10-19. This is **analysis and reporting only**, NOT the build phase.

## Exact objective

Inspect SLF's original `W_GameMenu` and `W_GameMenu_Button` **in the live Unreal 5.8.3 editor**, including Designer hierarchy, widget properties, brush/material/texture/font references, selected/hovered/pressed states and relevant event/delegate/animation behavior. Compare each visual resource needed for a CodeX reconstruction against the already user-prepared CodeX copy. Produce a specific, implementable reconstruction specification for later **B2**; do not modify anything.

### Read-only source and target paths

```text
ORIGINAL SLF, reference only:
/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu
/Game/SoulslikeFramework/Widgets/HUD/W_GameMenu_Button
/Game/SoulslikeFramework/Widgets/_Materials/
/Game/SoulslikeFramework/Widgets/_Textures/
/Game/SoulslikeFramework/Fonts/

CODEX copied visual resource library (user states already migrated and internally checked):
/Game/CodeX_Restruct/External/AssetContent/UI/SLF/

CODEX B0 target widget shells, inspect only:
/Game/CodeX_Restruct/Game/UI/GameMenu/WBP_GameMenu
/Game/CodeX_Restruct/Game/UI/GameMenu/WBP_GameMenuButton
```

The original SLF source widgets are **not** children/templates for new CodeX widgets. Never modify their graphs, copy full source `W_` widgets, or import SLF runtime navigation/combat/system dependencies. Read and document them only.

## Context that the archived SLF Blueprint export already supports

The following are **export observations**, not an exhaustive live inspection:

- `W_GameMenu` root is `MainOverlay`. It includes `LeftSideSizer/BorderLeft`, `GameMenuButtonsBox` (SLF Inventory, Equipment, **Crafting**, Status, System), Select/Back input hints, `RightSideSizer/BorderRight`, and an additional `SizeBox_73`. The faded left/right panels reference `MI_FadingBorder_R_Soft` and `MI_FadingBorder_L_Soft` from `Widgets/_Materials`.
- `W_GameMenu_Button` root `OL` includes `SelectedBorder`, clickable `EscMenuButton`, and a horizontal icon/text composition (`IconSizer`, `ImgBorder`, `Bg`, `Img`, `BtnText`). Its selected background references `MI_FadingBorder_R_Soft`; the default icon on the reference asset is `T_Inventory`. Text references `F_LiberationSerif-Regular`.
- The button has `SetGameMenuButtonSelected` logic that changes selected state, icon/border tints and the `SelectedBorder` visibility; `Event PreConstruct` configures button text/icon; it has press and selected dispatchers. `W_GameMenu` uses menu-closed/request delegates and SLF navigation/input dependencies. Its exported EventGraph contains composite/collapsed nodes, so **inspect the live inner logic** before describing full behavior.
- Exported `W_GameMenu` dependency classes include `BFL_Helper`, `BPI_Controller`, `IA_NavigableMenu_Back`, `IA_NavigableMenu_Ok`, `W_Navigable`, `W_Navigable_InputReader` and the SLF child button. These are **NOT** CodeX reconstruction dependencies.
- Exported widget-tree serialization may not expose all transitions/animations, dimensions or runtime style changes. Do not invent those details; inspect live and mark gaps.

## Requested audit — inspection checklist

### A. Original widget anatomy

For both reference widgets, record a concise ordered widget tree (widget name, widget class, parent, `IsVariable` if available). Inspect slot/container sizing and behavior: `SizeBox` width/height overrides, panel offsets, padding, alignment, horizontal/vertical fill, anchoring, clipping, safe zone/DPI and render transforms **where determinable**. Record the intended relative layout separately from possible animation start offsets such as `BorderLeft` translated left and `BorderRight` translated right.

### B. Visual style and assets

Inspect each relevant `FSlateBrush` resource and record:

- Image/Border/Material/Texture reference, tint, draw type, size, image alpha, corner/brush behavior.
- Text font object, size, weight/typeface, color, shadow and letter spacing.
- Default/normal, hover, focused/selected, pressed and disabled visuals; clarify which states are button style vs custom selected-state UI.
- Any widget animations/UMG tracks and runtime material-instance parameter changes actually present.
- Select/Back input icons and whether controller/keyboard layouts drive image swaps; do not bring SLF input actions or interfaces into CodeX.

### C. Complete resource dependency comparison

For each **required visual** in the source reference, document exact original object path, class, proposed/current CodeX counterpart object path and verified dependency chain, including material-instance **parent materials**, embedded material functions, texture samplers and font assets. Inspect hard and relevant soft references; avoid assuming ResourceObject is the full dependency set.

Use a table with columns: `SLF source asset`, `Type / UI usage`, `CodeX copy`, `Parent/function/texture chain`, `Result (verified / missing / uncertain)`, `Notes`. Read-only inspection must **not** create missing copies, update references, or change material parents.

### D. Behavior and navigation adaptation

Describe original button press, hover/focus, selection updates, selected-color changes, keyboard/gamepad navigability, menu animations and back/close dispatch behavior without wholesale graph duplication. Show the minimal corresponding **CodeX** contracts:

```text
WBP_GameMenuButton.Page : EGameMenuPage
WBP_GameMenuButton.OnPageRequested(Page)
WBP_GameMenu.ActivePage : EGameMenuPage
CodeX pages: Inventory, Equipment, Status, System (NO Crafting)
```

Propose only **future** Blueprint functions/dispatcher wiring needed to drive four buttons and a page switcher in B2. Do not implement the proposal in B1.1. SLF-specific controller interfaces, input actions, W_Navigable family and library calls are reference context, not runtime dependencies.

### E. Comparison to current CodeX shells

Verify `WBP_GameMenu` and `WBP_GameMenuButton` exist, are currently B0 empty shells, and have their typed variables/dispatchers (without altering them). Audit whether prepared CodeX resource copies are adequate for building the reference look. No Designer construction, no compiling, no saving, no PIE, no `CreateWidget` or controller input integration in B1.1.

## Mandatory report format

Return a **single focused technical report** with:

1. **Evidence/provenance:** assets inspected live, export-only observations, user-reported steps; read-only actions taken.
2. **Reference Designer hierarchy:** original `W_GameMenu` and `W_GameMenu_Button` trees and layout-relevant properties.
3. **Visual asset/dependency map:** exact SLF object path → exact already-copied CodeX path, class and parent/function/texture links; flag missing/uncertain.
4. **State and input behavior:** original hover/selected/pressed/disabled and any discovered animations, focus navigation and input prompts.
5. **CodeX adaptation:** four pages, no Crafting, reuse B0 `EGameMenuPage`, `ActivePage`, `Page`, `OnPageRequested`, avoid SLF logic links. Proposed Designer hierarchy and future minimal event flow; **no implementation**.
6. **Suggested B2 order:** first generic `WBP_GameMenuButton`, then `WBP_GameMenu`; note questions worth settling before edits.
7. **Blind spots:** tool-limited UMG properties, unsupported animation inspection, unverified copied assets and exact screenshot requests for manual review.

## Strict stop conditions

- **No content mutation** anywhere: no copying/moving/renaming/deleting assets, saving packages, adding/removing widget elements, applying styles, compiling, or modifying settings/tags.
- Do not launch a build task automatically. Stop once report and conditional resource fixes (if needed) are proposed.
- Avoid scanning entire SLF content tree. Inspect only these two widgets and their actual visual/dependency closure.
- Never call the original SLF runtime `W_GameMenu` as a subclass/child of CodeX `WBP_GameMenu`; study, then reconstruct later.

**Acceptance:** an accurate original-widget reconstruction blueprint *on paper*, asset mapping to existing CodeX copies, and a list of verified unknowns to inspect manually. No `.uasset` changed.
