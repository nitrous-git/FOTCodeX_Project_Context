# Ashen Keep — L1A Spatial Terrain Specification

**Status:** Spatial design proposal v1.0; ready for heightmap planning, not yet a production-ready heightfield.  
**Project:** FOTCodeX_SLF_583, Unreal Engine 5.8.3.  
**Target:** a compact, layered Soulslike region with a spectacular castle skyline and reconnecting exterior paths.

## 1. Design intent

Preserve the three established Ashen Keep / Ashen Spire maps: **starting overlook → descent to lowlands → graveyard/chapel and forest/cliff branches → Fallen Hamlet reconnection → castle approach / great bridge / gate → castle courtyard hub → interior branches and physical shortcuts → Great Hall → final boss summit**. The visual references are creative guides, not orthographic surveys.

This specification is an **authored translation** of that topology into internally consistent coordinates; no source map provides exact world-space positions. The designer's castle audits provide constraints, **not** a mandate to duplicate either demo map.

## 2. World coordinates and Unreal import contract

- Landscape: **2017 × 2017** samples, **2016 × 2016 metres** at X/Y scale 100 (1 m per sample).
- Sections: 63×63 quads, 2×2 sections/component; **16×16 = 256 components**.
- Coordinate origin: world centre `(X=0, Y=0, Z=0)` metres. **+X East; +Y North; +Z Up.** North is up in our map.
- Landscape lower-left vertex: `(-1008, -1008)` m. Place Landscape origin at `(-100800,-100800,0)` cm if the imported vertex 0 aligns with its actor origin; validate in Unreal.
- Heightmap vertical scale: **Z scale 100**; the intended terrain targets approximately **−55 to +165 m**. Some traversable elevations are higher because architectural meshes sit on terrain.
- Heightmap encoding at this Z scale, Landscape Z origin 0: `uint16 ≈ round(32768 + height_m * 128)`. Confirm using calibration markers before importing the full production heightmap.
- Heightmap format: **single-channel 16-bit PNG or R16**. Do not export 8-bit grayscale or gamma-altered height images.
- Orientation test: Start must lie **SW**, graveyard **W**, forest **E**, castle **N**. Check Y-flip on import with 3 asymmetric markers.

## 3. Castle scale derived from L0

| Source audit | Measured architectural envelope | Practical inference |
|---|---:|---|
| Dark Fantasy Castle demo | ~585 × 472 m with bridge | Strong reference for great bridge, monumental walls, clock gate and hall |
| Gothic Castle demo | ~271 × 310 m structural area | Strong reference for playable courtyards, towers, stairs, wings and interior bays |
| **Ashen Keep working envelope** | **X −345…+345, Y +285…+900 = 690 × 615 m** | Reserved construction region; includes bridge and side spurs; **never flatten this rectangle** |

The 690×615m envelope exceeds the Dark Fantasy audit's 630×525m reservation so that the two kits can combine without compressing entry, branches and boss ascent. It is a **3D site envelope**, not a requested contiguous flat mesa. The heightmap will shape several compact foundation benches separated by rock shoulders and the great-bridge chasm.

## 4. Spatial layout

![Top-down construction map](AshenKeep_L1A_TopDown.png)

All centres and radii below are planning approximations in metres. Radii outline environment influence areas, not exact polygons or mandatory boundaries.

| Zone | XY centre | XY radii | Terrain Z target | Role |
|---|---|---|---|---|
| Z01 Starting Overlook | (-285, -790) | 110 × 84 | 34–43 m | Bonfire and unobstructed castle reveal |
| Z02 Open Lowland | (-90, -480) | 360 × 200 | 0–20 m | Primary open exploration and route split |
| Z03 Graveyard | (-515, -305) | 155 × 135 | 28–49 m | Optional skeleton challenge |
| Z04 Ruined Chapel | (-565, -72) | 120 × 113 | 52–73 m | Optional landmark/loot and reconnection |
| Z05 Forest Trail | (505, -350) | 190 × 240 | 16–58 m | Ambush and eastern alternate climb |
| Z06 Cliff Overlook | (485, 108) | 100 × 92 | 68–93 m | View to castle and optional reconnection |
| Z07 Fallen Hamlet | (-45, -68) | 174 × 132 | 37–54 m | Midpoint bonfire, village combat and branch convergence |
| Z08 Castle Approach | (5, 165) | 195 × 142 | 61–111 m | Switchback ascent and gate foreshadowing |
| Z09 Bridge Chasm | (12, 400) | 255 × 67 | -35–65 m | River gorge beneath great bridge; cliff mesh dressing required |
| Z10 Castle Outer Walls | (0, 570) | 327 × 100 | 105–139 m | Gate foundations and flanking wall returns |
| Z11 Castle Courtyard | (0, 681) | 115 × 77 | 124–137 m | Main interior/exterior hub; architectural finished floor at ~132m |
| Z12 West Chapel/Barracks Spur | (-208, 722) | 127 × 163 | 121–156 m | Branch architecture and shortcut 1 |
| Z13 East Watchtower/Barracks Spur | (202, 710) | 130 × 153 | 122–156 m | Alternate entry and shortcut 2 |
| Z14 Upper Keep Foundations | (5, 847) | 147 × 67 | 139–157 m | Great Hall/upper boss ascent supported by masonry |

## 5. Construction anchors and elevations

**All listed XY/Z values use world metres**, not Unreal centimetres. Convert to cm by multiplying by 100. If `surface` indicates architecture, the Z is the *walkable mesh floor / deck*, not required Landscape height.

| ID | Anchor | World (X, Y, Z), m | Surface |
|---|---|---|---|
| A_START | Starting bonfire | (-285, -790, 39) | landscape |
| A_LOWLAND | Lowland hub | (-90, -480, 10) | landscape |
| A_GRAVE | Graveyard | (-515, -305, 39) | landscape |
| A_CHAPEL | Ruined Chapel | (-565, -72, 63) | landscape |
| A_FOREST | Forest Trail | (505, -350, 36) | landscape |
| A_CLIFF | Cliff Overlook | (485, 108, 81) | landscape |
| A_HAMLET | Hamlet bonfire | (-45, -68, 46) | landscape |
| A_APPROACH | Castle approach | (5, 165, 83) | landscape |
| A_BRIDGE_S | Bridge south abutment | (0, 287, 112) | landscape supported by structural masonry |
| A_BRIDGE_N | Bridge north abutment | (0, 522, 112) | landscape supported by structural masonry |
| A_GATE | Main gate | (0, 536, 114) | architectural threshold over terrace |
| A_BROKEN_WALL | Broken wall side entry | (274, 535, 119) | architectural threshold over terrace |
| A_COURT | Courtyard | (0, 679, 132) | architectural floor over terrace |
| A_WEST_WING | Chapel/barracks | (-195, 735, 143) | architectural floor over terrace |
| A_EAST_WING | Watchtower/barracks | (190, 729, 146) | architectural floor over terrace |
| A_HALL | Great Hall | (75, 804, 153) | architectural floor, terrain foundation approx +145m |
| A_BOSS | Final boss arena | (0, 874, 181) | architectural platform, NOT heightmap at +181m |

**Important height distinctions:**

- Starting bonfire ground **+39m**, lowland **+10m**, Hamlet ground **+46m**, castle approach **+83m**.
- Bridge deck **+112m** with chasm bed beneath it at roughly **−13m** where river crosses. Architectural bridge supports and cliff meshes handle this exceptional height difference.
- Gate floor **+114m**; courtyard finished floor **+132m**, with approximate **+129m** terrain bench underneath.
- Great Hall walkable floor **+153m**, supported by an approximately **+146m** site terrace.
- Final boss **+181m** is a **modular architectural arena**, not a heightmap plateau at that exact elevation.
- Towers can extend **far above** the playable terrain; demo tower AABBs must not be mistaken for mountain sculpt requirements.

![Elevation targets](AshenKeep_L1A_ElevationProfile.png)

## 6. Protected flat/graded masks for L1B

These are the ONLY large-area targets to be deliberately levelled, and even their edges should feather naturally. Actual actor footprint validation can change them slightly.

| ID | Target | Bounds (m) | Target terrain Z (m) | Priority |
|---|---|---|---:|---|
| T01 | Starting vista pad | X -355…-215, Y -844…-746 | 39 | hard |
| T02 | Hamlet buildable core | X -175…105, Y -155…10 | 46 | medium |
| T03 | South bridge abutment | X -77…77, Y 270…308 | 112 | hard |
| T04 | North bridge/gate abutment | X -95…95, Y 506…554 | 113 | hard |
| T05 | Courtyard foundation | X -106…106, Y 631…729 | 129 | medium |
| T06 | West architectural spur | X -315…-132, Y 606…832 | 138 | medium |
| T07 | East architectural spur | X 130…316, Y 602…817 | 138 | medium |
| T08 | Upper keep foundation | X -130…139, Y 790…897 | 148 | medium |

The castle should consist of **multiple benches and shelves**, not a 690×615 m flat rectangle. Use clean compact pads beneath player-facing spaces; let rocks, modular walls, retaining structures and cliff meshes handle steep rises, overhangs and masonry transitions.

## 7. Routes, branch logic and encounter pacing

| Route ID | Connection | Kind | XY control points | Implementation rule |
|---|---|---|---:|---|
| R_MAIN | Main progression | main | 15 | Stage 8->9 is a switchback, not a direct steep ramp; bridge 287->522 is architectural deck; last two legs are interior architecture |
| R_WEST | Graveyard and chapel | secondary | 6 | Optional left-side route, loops back to hamlet |
| R_EAST | Forest and cliff path | secondary | 9 | Alternate uphill entry through broken wall; route must remain traversable through elevated cliffs |
| R_EAST_RETURN | Cliff reconnect to hamlet | unlockable_loop | 4 | Optional rejoining route; design explicit one-way drop/ladder or proper trail, not arbitrary teleport |
| R_SHORTCUT_1 | Courtyard-west to lower gate | castle_shortcut | 3 | Lift / stairs / door; persistent state via CodeX interaction later |
| R_SHORTCUT_2 | Courtyard-east to broken wall | castle_shortcut | 3 | Opens a fast route into east forest/cliff track |

**Important route rules:**

- R_MAIN: Starting Overlook → Lowland → Hamlet → switchback Castle Approach → great bridge → Main Gate → Courtyard → Great Hall → Boss. The main valley ascent must use **enough route length, switchbacks and architectural stairs** to avoid unnaturally steep Landscape roads.
- R_WEST: optional graveyard and chapel branch reconnecting near the Hamlet. Keep it tactically distinct and rewarding.
- R_EAST: forest and cliff route rises to the **broken-wall side entry**, not a teleport or inaccessible cliff. Provide a physical trail with stairs/bridges as needed.
- R_EAST_RETURN: a separate physical loop to the Hamlet, potentially one-way until activated.
- Shortcut 1: western castle wing to the lower gate approach; Shortcut 2: eastern wing to broken-wall side entry. Persist unlock state later through CodeX, not in the heightmap.
- Main Gate progression (Gate Knight vs locked-from-behind mechanism) remains a design decision, not something the heightmap must hard-code.

## 8. River, ravines and vistas

River concept: NE upper drainage → **below the great bridge around (X=0,Y=390)** → western gorge → lowland drainage. River water elevation must monotonically decrease; where relief is extreme, use waterfalls/rapids or structural cliff meshes rather than forcing continuous shallow Landscape slopes. The west graveyard terrace should remain safely above the main drainage.

Sightline priorities: (1) Starting Overlook → upper keep skyline; (2) lowland → main gate; (3) east cliff overlook → layered court and side wall; (4) Hamlet → ascending castle approach. Avoid creating ridges that completely obscure the upper keep from the starting bonfire. Check with the actual CodeX player camera, not only a free editor camera.

## 9. Heightmap-generation algorithm brief (next stage L1B)

1. Establish continuous macro ridges and basins by combining broad hand-authored elevation fields with local plateau masks. Keep the central lowland readable.
2. Blend pad targets with smooth transitions. Roads receive independently planned corridors; use spline profiles and actual grades, not straight-line interpolation between anchor heights.
3. Cut a dramatic E–W river gorge under the great bridge, without filling the bridge span. Keep steep terrain behind intended cliff meshes.
4. Apply moderate multi-scale noise and erosion-style detail outside protected pads, roads and playable combat surfaces; suppress high-frequency bumps in traversal corridors.
5. Produce a low-resolution height preview, contour/elevation preview, calibrated 16-bit heightmap and import-orientation test. Validate all anchor heights numerically and inspect an oblique 3D view.
6. Retain procedural source code, seed and configuration so a changed gate location does not unpredictably alter the entire map.

**Do not** implement foliage, fine road paint layers, castle geometry, or gameplay objects in L1B. These remain manual painting and Astra-driven modular blockout tasks.

## 10. L1A approval checklist

- [ ] Main castle is large enough for the two kits and still readable from the Starting Overlook.
- [ ] Start, lowland, graveyard, forest, Hamlet, castle approach and inner keep form the correct map relationships.
- [ ] Castle foundations are separated into terraces, while the chasm is preserved.
- [ ] Bridge elevations and supports are consistent with the intended asset scale.
- [ ] Both side routes and both castle shortcuts can physically reconnect.
- [ ] Enough visual and collision space exists for CodeX melee, ranged enemies and boss camera.
- [ ] All future image/map imports use the same XY/Z convention.

## 11. Files and references

- `AshenKeep_L1A_SpatialLayout.json`: authoritative structured world specification and route coordinates.
- `AshenKeep_L1A_TopDown.png` / `.svg`: top-down planning overlay, to scale.
- `AshenKeep_L1A_ElevationProfile.png`: conceptual elevation targets for major milestones.
- Source audits: [Dark Fantasy Castle](https://github.com/nitrous-git/FOTCodeX_Project_Context/blob/main/Docs/AshenKeep_L0_FantasyCastleReferenceAudit.md) and [Gothic Castle](https://github.com/nitrous-git/FOTCodeX_Project_Context/blob/main/Docs/AshenKeep_L0_GothicCastleReferenceAudit.md).

**Next:** L1B — generate, preview and iterate a real 2017×2017 terrain heightmap using this JSON as the geographic control contract. Only then import to UE and hand off castle construction to Astra.
