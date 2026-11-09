# CodeX — Ashen Keep L1B: UE5 Landscape import and verification

**Status:** Procedurally generated **terrain foundation v1.0**. No in-editor import, player traversal test, collision test or material painting has been performed. The design is based on the L1A coordinates and the two castle kit audits, not a direct conversion of a perspective illustration.

## 1. Recommended production files

- **`AshenKeep_Heightmap_2017x2017_16bit.png`**: primary one-channel 16-bit heightmap; import **this** first.
- `AshenKeep_Heightmap_2017x2017_16bit_YFlipped_ALTERNATIVE.png`: alternate encoding of the same terrain if the north/south axis appears reversed in UE.
- `AshenKeep_Heightmap_2017x2017_16bit.r16`: headerless, little-endian, unsigned 16-bit raw data; fallback if the PNG importer has issues.
- `AshenKeep_L1B_TopographicPlan.png`: terrain with route/landmark overlays, **not** a heightmap.
- `AshenKeep_L1B_TerrainPreview.png`: terrain-only hillshade preview, **not** a heightmap.
- `AshenKeep_L1B_Oblique3D.png`: 3D interpretation for terrain composition inspection.
- `AshenKeep_L1B_ConstructionOverlay.png`: planned foundations, routes, river and intended crossing positions.
- `AshenKeep_L1B_QA.json`: sampled heights, grade checks, river intersections and sightline caveats.
- `AshenKeep_L1A_SpatialLayout.json`: original authoritative layout; all intended world coordinates in metres.
- `AshenKeep_L1B_Generate.py`: reproducible generator (seed **18673**) using Python, NumPy, SciPy, Pillow, matplotlib and Shapely.

## 2. Unreal Landscape configuration

Use an **isolated new map or backed-up Landscape** for the first import. Do not overwrite a painted production Landscape without a snapshot.

| Setting | Value |
|---|---|
| Heightmap resolution | **2017 × 2017** samples |
| Section size | **63 × 63 quads** |
| Sections per component | **2 × 2** |
| Components | **16 × 16 (256)** |
| X scale | **100** (Unreal units = centimetres) |
| Y scale | **100** |
| Z scale | **100** |
| Landscape Z origin | **0 cm** for this encoding |
| Actual span | **2016 × 2016 metres** |
| Expected terrain elevation range | approximately **−42 to +186 m** |
| 16-bit encoding | `u16 = round(32768 + terrain_z_m × 128)` |
| Raster orientation in supplied primary image | row 0 corresponds to **world Y = −1008 m (south)**; last row to **Y = +1008 m (north)** |

### Option A — create the Landscape from the file (safest first test)

1. Open a separate Ashen Keep test map. Enter **Landscape Mode → Manage → Import from File**.
2. Select the **primary 16-bit PNG**. Confirm Unreal detects a single 2017 × 2017 heightmap.
3. Choose 63 quads, 2×2 sections, 16×16 components if these do not auto-populate.
4. Set **Scale X/Y/Z = 100/100/100**; ensure Landscape **world Z origin is 0 cm**.
5. Enable Landscape Edit Layers, if using that workflow. Import the heightmap into a named base sculpt layer where available.
6. Create/import and save the map. Inspect in top-down and player-level views before applying any expensive material, foliage or lighting.

**XY placement:** The L1A convention assumes the terrain covers X,Y = −1008…+1008 m. Unreal Landscape location conventions differ by create/import workflow. Set the actor transform or offset so the **physical world bounds match these coordinates**, rather than blindly applying a fixed location. If the import behaves as a minimum-corner anchored grid, the actor location may be around **(−100800, −100800, 0) cm**. If it imports centered at the origin, keep the actor centered; **verify with landmarks**.

### Option B — use an already configured Landscape

1. Back up the map and verify resolution **2017×2017** and scale **100/100/100**.
2. Import the height data into a suitable dedicated sculpt/edit layer via your version's supported Landscape editing/import procedure. Some workflows require reimport or creating a new Landscape instead; don't assume a selected material paint layer accepts a heightmap.
3. Recheck Layer Info assets, auto-material setup and painted weights before painting further. This heightmap package contains **no paint weightmaps**.

## 3. Three-point orientation and calibration test

Test the **unflipped primary PNG first**. Unreal's raster-to-world-Y orientation must be confirmed in the actual editor workflow. If the **north and south are inverted**, import the separately labelled **YFlipped_ALTERNATIVE** file into a fresh landscape / fresh sculpt layer instead; do **not** rotate the Landscape 180° to correct just one axis.

To test: start/bonfire is **southwest** at (−285, −790), graveyard is **west** at (−515, −305), forest is **east** at (+505, −350) and castle foundations are in the **north** (Y ≈ +535…+900). Test at least three asymmetric landmarks.

| Landmark | World X,Y (m) | Terrain Z expected (m) | Unreal world position in cm (reference) |
|---|---|---:|---|
| Starting bonfire | −285, −790 | +39 | (−28500, −79000, +3900) |
| Lowland hub | −90, −480 | +10 | (−9000, −48000, +1000) |
| Graveyard | −515, −305 | +39 | (−51500, −30500, +3900) |
| Ruined Chapel | −565, −72 | +63 | (−56500, −7200, +6300) |
| Forest Trail | +505, −350 | +36 | (+50500, −35000, +3600) |
| Hamlet bonfire | −45, −68 | +46 | (−4500, −6800, +4600) |
| Castle Approach | +5, +165 | +83 | (+500, +16500, +8300) |
| Bridge south seat | 0, +287 | +112 | (0, +28700, +11200) |
| Bridge north seat | 0, +522 | +113 | (0, +52200, +11300) |
| Great Bridge riverbed | 0, +390 | about −16 | (0, +39000, −1600) |

**Important:** landscape terrain at the courtyard is near **+129 m**, while the architectural courtyard *floor* is planned near **+132 m**. Great Hall is supported near **+148 m** with intended finished floor **+153 m**; final boss finished arena **+181 m** will be built from meshes, not terrain.

## 4. What Astra should build, not sculpt

- The ~235 m deck connection between bridge seats; source Dark Fantasy great bridge is approximately 242 m long including architectural supports. Source mesh scale, pivots, and collision must be tested against these anchors.
- Bridge supports and bridge-end abutments; riverbed around 128 m below bridge deck.
- Gates, castle walls, stairs, towers, retaining walls, walkable courtyards, Great Hall and final boss platform.
- Steep cliff faces, overhangs, caves and exposed retaining geometry using rock meshes and modular architecture; Landscapes cannot produce genuine overhangs.
- Three marked river crossings for optional ground routes (see QA/ConstructionOverlay), as separate small bridge or ford geometry.
- Recheck main-route grading around Hamlet: generated road centerline mostly stays near 11% or less, but has a short localized slope spike to ~25% near the village. This needs smoothing or a modular stair/ramp in L2.

## 5. Terrain-specific cautions

- The river uses dramatically different water levels along its path. **Do not** fill it with one constant-height Water Body plane; it needs graded river segments, rapids or waterfalls. No water actors were generated.
- Heightmap smooth terracing is only an **underlay**. Structural mesh floors and walls create precise gameplay geometry.
- Full lower gate/courtyard elevations are **not necessarily visible** from every viewpoint due to intervening terrain; the opening view to the upper keep's silhouette is terrain-clear, but all sightlines must be revalidated with actual castle architecture and the player camera in PIE.
- Autosculpted roads here are terrain **grades**, not final painted roads; surface painting is intentionally left to the user.
- Do not spawn the 10,778 demo actors into the level. Prefer approved reusable assemblies, Blueprints and Level Instances, using the source JSON catalogs.
- Confirm the target project's Landscape streaming/rendering and Brushify performance; 256 components cost more than the original 64-component option.

## 6. Acceptance check before L2 Astra

- [ ] PNG imports without an 8-bit quantization or stepped height artifact.
- [ ] Landscape physical extents, orientation and major anchor heights match the JSON.
- [ ] South and north bridge banks remain separate with an open ravine.
- [ ] Castle is supported by multiple terrain benches, not one giant flat rectangle.
- [ ] Starting bonfire has a view toward the northern upper keep skyline.
- [ ] Main valley route and both optional branches can be physically connected (with bridges/stairs where needed).
- [ ] Manual material painting is deferred until blockout placements are stable.
- [ ] Map is backed up before Astra begins any structural modification.

**Result expected:** a reliable terrain foundation for Astra's complete modular castle and traversal blockout, not yet a finished playable level.
