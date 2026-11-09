# CodeX — Ashen Keep Astra High L2A Reference Index

## Source-of-truth priority
1. **Actual Unreal working Landscape** (once anchor positions and north/south orientation are verified).
2. `AshenKeep_L1A_SpatialLayout.json` for anchor positions, route logic, and zone IDs; `AshenKeep_L1B_Manifest.json` for import/scale.
3. `02_L1B_ConstructionOverlay_AUTHORITATIVE_TERRAIN.png` and `03_L1B_Oblique3D_ActualHeightmap.png` for designed terrain features.
4. `00_LevelStructurePlan_AUTHORITATIVE_DESIGN.png` for authored progression graph, exterior and castle interior relationships. NOT a survey or literal metric raster.
5. Creative/style references (`01_AshenSpire_GlobalVisualNorthStar.png`, `04_Mockup_StartingOverlook_ART_DIRECTION.png`, `05_Mockup_GreatBridge_ART_DIRECTION.png`, `06_Mockup_Courtyard_OPTIONAL.png`). These are **aspirational** art, not ground-truth geometry.

## Core images to view first
- `00_LevelStructurePlan_AUTHORITATIVE_DESIGN.png` — zone order and loops
- `02_L1B_ConstructionOverlay_AUTHORITATIVE_TERRAIN.png` — terrain foundation sites and route/crossings
- `03_L1B_Oblique3D_ActualHeightmap.png` — real terrain massing
- `04_Mockup_StartingOverlook_ART_DIRECTION.png` — opening vista and atmosphere
- `05_Mockup_GreatBridge_ART_DIRECTION.png` — heroic castle approach and architectural scale
- Existing repo `Docs/AshenKeep_FantasyCastle_Overview.png` and `Docs/AshenKeep_GothicCastle_Overview.png` — what the actual available kits build, not conceptual image-generation guesses.

## Primary structural sources in the existing repo
- `Docs/AshenKeep_L0_FantasyCastleReferenceAudit.md`
- `Docs/AshenKeep_L0_GothicCastleReferenceAudit.md`
- `Docs/AshenKeep_FantasyCastleAssemblies.json`
- `Docs/AshenKeep_GothicCastleAssemblies.json`
- `Docs/AshenKeep_FantasyCastle_Approach.png`
- `Docs/AshenKeep_FantasyCastle_Gate.png`
- `Docs/AshenKeep_GothicCastle_Entry.png`

The assembly JSON files contain editor-world actor names. These are **not** a substitute for imported asset paths and do not guarantee editor GroupActor membership. Rebuild or reuse only approved subassemblies in project-owned assets; never clone the entire demo scenes.

## Critical location anchors, world metres
- Starting Overlook: (-285, -790, 39)
- Fallen Hamlet: (-45, -68, 46)
- Castle Approach: (5, 165, 83)
- Bridge S: (0, 287, 112), Bridge N: (0, 522, 112)
- Main Gate: (0, 536, 114)
- Courtyard: (0, 679, 132) finished architectural floor, terrain underlay ~129
- Great Hall: (75, 804, 153) finished architectural floor
- Boss: (0, 874, 181) finished architectural platform; terrain is lower

**Units:** JSON uses metres; Unreal transforms use centimetres (×100). Heightmap 2017×2017; XY 100 cm; Z 100; terrain extent ±1008 m, if imported correctly. Confirm orientation via southwest start, west graveyard and north castle. Do not impose coordinates on a misaligned Landscape.

The bridge spans approximately 235 m between terrain seats (designer source great-bridge mesh ~242 m overall); fit actual walking deck and abutments separately. Existing river gorge is ~128 m below deck around center. Roads have three other water crossings and a locally steep hamlet approach; these can be tackled in the next pass after the main spine.
