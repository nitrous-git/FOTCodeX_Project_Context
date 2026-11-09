# CodeX — L2A: Ashen Keep Main-Spine & Castle Macro Blockout

**Model:** Astra High  
**Engine:** Unreal Engine 5.8.3  
**Project:** FOTCodeX_SLF_583  
**Method:** Unreal MCP, editor assets and editor-side scripts where available  
**Mission:** BUILD A PLAYABLE MACRO BLOCKOUT DIRECTLY IN UNREAL, not another concept document.

## Role and design intent

Act as Senior Soulslike Level Designer and Senior Unreal Environment Artist. Take ownership of the 3D composition and macro architecture. Preserve our established Ashen Keep / Ashen Spire visual identity: start on a high bonfire overlook, descend into a broad lowland, converge at the Fallen Hamlet, ascend through a switchback castle approach, cross a monumental ravine bridge, enter a gatehouse, reach the courtyard hub and climb to the Great Hall / final boss summit. Create a coherent world with strong vista reveals, meaningful elevation, architectural tension, and physical continuity. Don't invent a different map.

You have autonomy to adapt dimensions, angles, bridge endpoints, construction details, and local path geometry based on editor evidence. Preserve the spatial relationships, core zone anchors and route/shortcut intent. Use real modular kit meshes rather than relying entirely on cubes or arbitrary primitive architecture.

## Required inputs and authority

1. `Docs/AshenKeep/L2A_References/REFERENCE_INDEX.md` — read first, including reference precedence.
2. `Docs/AshenKeep/L2A_References/AshenKeep_L1A_SpatialLayout.json` — **numeric layout source**. All positions are METRES; UE5 Actor transforms are CENTIMETRES.
3. `Docs/AshenKeep/L2A_References/AshenKeep_L1A_SpatialTerrainSpec.md` — zones, benches, route contracts and buildability.
4. `Docs/AshenKeep/L2A_References/02_L1B_ConstructionOverlay_AUTHORITATIVE_TERRAIN.png` and `03_L1B_Oblique3D_ActualHeightmap.png` — physical terrain context.
5. `Docs/AshenKeep/L2A_References/00_LevelStructurePlan_AUTHORITATIVE_DESIGN.png` — progression and two-way spatial logic.
6. `Docs/AshenKeep/L2A_References/04_Mockup_StartingOverlook_ART_DIRECTION.png` and `05_Mockup_GreatBridge_ART_DIRECTION.png` — mood and silhouette; *not* exact geometry.
7. `Docs/AshenKeep_L0_FantasyCastleReferenceAudit.md`, `Docs/AshenKeep_L0_GothicCastleReferenceAudit.md`, and the two `*Assemblies.json` files — actual construction methods, measurements, reusable assets and source transforms. Inspect only the catalog sections relevant to this stage.
8. Existing repo screenshots `Docs/AshenKeep_FantasyCastle_Overview.png`, `Docs/AshenKeep_GothicCastle_Overview.png`, plus gate/approach images where useful — architectural realism of imported modular kits.

**Precedence on conflict:** measured imported UE5 Landscape and real asset dimensions > L1A JSON anchor contracts > L1B terrain previews > authored structure reference > cinematic mockups. Report necessary deviations. The images depict the intended feeling, not photogrammetric terrain.

## Setup safety and orientation

- Inspect the live project and locate the intended Ashen Keep working map. Use a dedicated map or a clearly isolated copy; preserve all asset-pack demonstration maps and existing CodeX gameplay maps.
- Verify a 2017×2017 Landscape with XY/Z scale 100/100/100 (if this is the configured foundation), coordinate orientation, Brushify setup and UDS actors where already configured. Verify three asymmetric reference landmarks by querying terrain Z; the starting overlook is southwest and castle is north. Do not silently rebuild, reimport or paint an existing Landscape. If the expected heightmap is not imported or spatially inconsistent, first attempt safe import into a NEW isolated map from the supplied primary 16-bit PNG; preserve source terrain/maps, and record the decision. If import is technically blocked, stop destructive construction and report the blocker rather than placing architecture on a wrong coordinate frame.
- Preserve game framework, combat and existing Blueprints. Do not modify source asset packs, demo scenes, or their materials.

## Execution — do this in Unreal in the current task

**Pass A: verify and stage**
- Record actual Landscape bounds, XYZ datum, the 17 anchor locations, important slope changes and buildable benches. Place temporary, easily removable zone/route markers in a dedicated folder or data layer.
- Establish a reproducible actor ownership convention (`L2A_...` labels/folders) and a stable world coordinate contract. Build with batch operations where efficient and avoid thousands of one-at-a-time MCP calls.

**Pass B: construct the MAIN gameplay spine**
- Build the traversable route from Starting Overlook → Lowland → Fallen Hamlet → Castle Approach switchbacks → South Great Bridge seat → Great Bridge → Main Gate → Courtyard → Great Hall → Boss Summit.
- Establish actual walkable geometry at each key elevation. Main bridge approx from (0,+287,+112)m to (0,+522,+112)m. Gate at (0,+536,+114)m, courtyard architectural floor around +132m, Great Hall +153m, boss arena platform ~+181m. These are starting targets; use collision-tested interfaces and measure real walkable deck positions.
- Create a clear castle sightline from the Starting Overlook and another strong reveal from the Hamlet. Provide defensible path widths, turns and landings suitable for third-person melee combat and a future enemy group encounter. Do not place permanent enemies yet.
- Use simple placeholder passages where the modular kit cannot achieve immediate walkability, but identify them visibly and log the exact gap. Do not claim the full main route is traversable without testing.

**Pass C: large-scale castle massing**
- Construct a recognizably gothic, terraced fortress silhouette using actual Dark Fantasy and Gothic pack architecture. Use Dark Fantasy for monumental shell/walls, great bridge and major gates where suitable; Gothic kit for modular courtyard, stairs, terraces, gate returns, accessible towers and interiors. Treat this as a flexible recommendation, not a prohibition on mixing.
- Make the main gate/clock-gate entrance, exterior curtain walls, primary towers, courtyard and Great Hall architecturally legible from player viewpoints. Use approved measured meshes and Blueprints. Prefer adapting small source assemblies or building project-owned Level Instances; do NOT duplicate the full 10k-actor and 4.6k-actor demo scenes.
- Build castle foundations with modular rock faces/retaining structures where needed. Terrain benches are NOT a single flat plateau. Leave geometry clean, sectional and editable.
- Reserve and mark (but do not fully build yet) the West Graveyard/Chapel route, East Forest/Cliff alternate route, Broken Wall side entrance, Courtyard West/East interior wings and two castle shortcuts. Avoid building walls that permanently prevent their future connection.

**Pass D: review and make useful corrections**
- Capture at least: Starting Overlook toward Castle; Hamlet toward Castle; pre-bridge toward gate; Great Bridge deck toward gate; Courtyard toward Great Hall; overhead castle layout.
- Compare these real viewport captures against the selected reference images, focusing on scale, silhouette, spatial readability and terrain integration. Adjust substantive issues using the existing construction tools before stopping.
- Validate walking along the main spine via PIE wherever supported (or report exact missing links); basic collision, fatal gaps, view obstruction, and intended route width. Validate actor transforms and that the working map saves and reloads correctly.

## Stop line (do NOT spend effort on this yet)

No detailed Landscape paint, heavy foliage/PCG, fine rubble dressing, atmosphere relighting, complete interiors, enemy encounters, bonfire logic, boss triggers, loot, persistent gate logic or optimization overhaul. Respect existing UDS and Brushify if configured. Avoid fake 'finished' detail that hides blocked routes.

## Deliverables

- Saved dedicated Ashen Keep blockout map with the main route and dramatic fortress massing actually constructed.
- Concise `Docs/AshenKeep/L2A_ImplementationReport.md`: assets used, significant placements, actual versus planned anchors, traversal status, blockers and next steps.
- `Docs/AshenKeep/L2A_ActorManifest.json`: generated/owned actors or Level Instances with labels, asset paths and transforms. Do not dump all demo actors.
- Six comparison viewport screenshots in `Docs/AshenKeep/L2A_Captures/` (or another documented location).
- Any reusable structural assemblies under a project-owned `CodeX_Restruct` environment path, not under third-party asset folders.

## Acceptance

1. Start-to-courtyard-to-boss progression is physically intelligible; if incomplete, identify exact broken segments.
2. The great bridge is supported and its deck is placed correctly above the ravine.
3. Castle silhouette and approach have high visual impact, without excessive decoration.
4. Major architecture aligns with the *actual imported terrain*, not just paper coordinates.
5. The intended secondary routes/shortcuts have preserved openings and attachment points.
6. Existing CodeX gameplay and designer demo maps remain untouched.

Be decisive and autonomous within these boundaries. This is a 3D construction task: prioritize an impressive, coherent editor result over verbose preliminary analysis. Do the work, validate it, save it, then report clearly.
