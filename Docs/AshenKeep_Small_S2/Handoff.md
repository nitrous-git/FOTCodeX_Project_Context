# AshenKeep_Small — S2 construction handoff

Implemented and saved November 9, 2026 through the live Unreal MCP connection.

**Map:** `/Game/CodeX_Restruct/Game/Worlds/Level01/AshenKeep_Small`

The map contains 1,802 architectural actors organized in `AshenKeep/A_Entrance` through `K_Integration`, plus five integration markers. Existing sky/weather actors remain. The walkable layout is approximately **115 × 205 m**, intentionally narrower than the suggested envelope. No Landscape was created or modified.

## Architecture and routes

- **A–C:** A 35 m stone approach, paired gate towers, enclosed lower ward with broken cover, and a 50 × 45 m fountain courtyard.
- **D:** Western stairs, an entrance overlook and a supported elevated arcade. The route looks back over the courtyard and reconnects at the upper staging area.
- **E:** A small partially vaulted chapel with an effigy. Its separate return stair reconnects to the lower ward.
- **F–H:** Four turning main-ascent flights and terraces, a monumental keep threshold, a dominant rear belfry, and an enclosed open-air boss arena. The arena shell floor is 45 × 50 m; reserve approximately 40 × 43 m centrally for combat, keeping the threshold and perimeter piers outside that space.
- **Shortcut:** Courtyard west opening → descending sally stair → passage beneath the western construction → external return connector. **Currently open, without unlock behavior.** `AKS_Shortcut_InnerUnlock_IntegrationRequired` marks the future interaction junction.

The primary route is entrance → lower ward → courtyard → eastern folded ascent → pre-boss staging → arena. The western alternative and chapel return are real geometry connections, not decorative paths.

## Landscape interfaces

Coordinates are Unreal centimetres. Rotation below is `(Pitch, Yaw, Roll)`; scale is `(1,1,1)`.

| Interface | Location XYZ | Rotation | Width |
|---|---|---|---|
| Main entrance, facing into castle along −Y | `(0, 500, 1000)` | `(0, −90, 0)` | 1,000 cm |
| Sally-port external return, facing outward +Y | `(−4000, −1000, 1000)` | `(0, 90, 0)` | 1,000 cm |
| Shortcut courtyard-side junction | `(−2500, −8000, 1600)` | route descends toward −X | approximately 460 cm between stair cheeks |

Keep an exterior tie-in area around **X −1000…1000, Y 500…2500** available for the main approach. Connect the sally return to the future bonfire-side terrain separately. No graveyard, bonfire or natural starting area was built.

Main floor datums: **Z 1000 / 1600 / 2800 / 4000 cm**. Intermediate ascent landings: **2200 and 3400 cm**. Total entrance-to-arena gain: **30 m**.

## Assets and validation

Gothic Castle supplies paving, substrate masonry, walls, gates, tower Blueprints, stairs, cheek walls, vaults, piers and arcade supports. Dark Fantasy Castle supplies foundation rocks, the courtyard fountain, statues and broken-column dressing. Source pack definitions and all reference maps were left untouched. No gameplay Blueprint definitions were modified.

The existing CodeX GameMode is assigned to this map, with PlayerStart on the entrance platform. Final map save succeeded and the package reported clean.

- Initial complete-route checks: **844 ground samples**, no gaps or torso obstructions.
- Additional torso/head traces at ±65 cm exposed a western stair-side conflict. The flight and landings were revised; the final affected-junction retest passed **216 checks**. The rest of that route had already passed its wider retest.
- PIE: the existing player spawned and grounded correctly. Fourteen teleported grounding checks covered all stair flights, the arena, chapel and sally return. The revised western flight was checked again with its actual runtime position confirmed and `MOVE_Walking` / walkable floor reported.

These are collision probes and PIE grounding checks, **not a continuous input-driven playthrough**. A manual full-route walk/run/dodge and camera pass is still required. Traversal duration, enemy navigation and boss combat have not been balanced or validated.

## Remaining integration and polish

Implement the shortcut lock/unlock and boss fog/boss activation using existing CodeX systems at the named markers. No boss or fog gameplay actor was added. Next priorities: manual traversal/camera refinement, AI navigation and encounters, Landscape tie-ins, then lighting/material cohesion and selective ruin dressing. Inspect foundation seams again from the eventual terrain viewpoints.

Captures: [overview](Overview.png), [courtyard](Courtyard.png), [western route](WesternRampart.png), [upper keep](UpperKeep.png).

The task placement record is `D:/Unreal Projects/FOTCodeX_SLF_583/Saved/AshenKeep_Small_S2_Manifest.json`. It is a reference for later edits; live Unreal actors remain authoritative. Do not blindly rerun its placements over the saved map.
