# 07 — Validation and acceptance matrix

**Policy:** Agent performs compile/save, data consistency, graph/referencer and targeted editor structural checks; **user performs PIE**, including animation/weapon/UI checks. The agent must not claim manual PIE results. Checkpoint outcomes should be recorded after each phase, with clear Pass/Fail/Not Run states.

## 1. Pre-implementation baseline

| ID | Case | Agent evidence |
|---|---|---|
| B01 | Confirm live UE version and correct CodeX project | Report engine/project identity |
| B02 | Capture old slot tags and all references | Tag/Blueprint consumers named |
| B03 | Capture category enum, Table rows and references | Item category/row migration map |
| B04 | Check `DA_CombatStyle` family mapping | Matching styles/weapon definitions |
| B05 | Check actual UMG/preview ownership | Existing preview asset + callsites |
| B06 | Backup/commit/checkpoint before structural edits | Source revision or project backup recorded |

## 2. Equipment assignment (C1/C2)

| ID | Manual PIE scenario / static contract | Expected result |
|---|---|---|
| E01 | Assign a valid owned weapon to Primary | Slot stores ItemID, updates row/HUD, valid active kit |
| E02 | Assign a different weapon to Secondary | Both assignments retained, active selection stable |
| E03 | Attempt same ItemID in both weapon slots | Rejected without mutating existing state |
| E04 | Try assign wrong category to Weapon slot | Rejected; no actor or animation changes |
| E05 | Try assign item not owned | Rejected, informative/consistent UI feedback |
| E06 | Cycle two occupied weapon slots | Exactly two states, each resolves correct weapon kit |
| E07 | Cycle with one assigned weapon | Remains active, no Unarmed flicker |
| E08 | Remove inactive weapon | Active kit remains unchanged |
| E09 | Remove active weapon | Falls back to other assigned kit or Unarmed |
| E10 | Remove last equipped weapon | Valid Unarmed fallback, no stale weapon actor |
| E11 | Assign complete Outfit | Full visual ensemble updates; no leftover previous meshes |
| E12 | Hide/show headwear | Cosmetic only, no modifier/ownership changes |
| E13 | Equip/unequip Talisman | Passive effect applies/removes exactly once |
| E14 | Select Fire/Light/Bleed Attunement | Single global selection, correct effect/preview identity |
| E15 | Switch weapon with Attunement equipped | Same global selection applies to newly active kit |
| E16 | Reinitialize equipment/HUD | No duplicate event binding or passive stacking |
| E17 | Remove final owned equipped item | Rejection/reconciliation contract prevents stale loadout |
| E18 | Invalid/unregistered Gameplay Tag | Stable handling; no impossible Map entry |

## 3. Combat and actions (C3)

| ID | Manual PIE scenario | Expected result |
|---|---|---|
| C01 | SwordShield basic/heavy attacks | Correct moveset, actors, hit traces |
| C02 | SwordShield hold and release guard | Guard enters/exits, locomotion restored |
| C03 | Switch out of blocking state | No held defense latch; intended action gate/cleanup |
| C04 | Katana attack/guard behavior | Correct Katana CombatStyle/Animset, no shield |
| C05 | Twinblades attack/guard behavior | Correct Twinblades CombatStyle/Animset, no shield |
| C06 | Weapon switch near attack/montage | No half-swapped actor, invalid state or orphan montage |
| C07 | Switch during locked cinematic/death/fog | Rejected safely, input state restored later |
| C08 | Switch during Shadow Strike aim/travel/execution | Safe rejection or explicitly supported transition |
| C09 | Spawn/attach/detach weapon actors | No invisible active kit, duplication or leaked actors |
| C10 | Damage and stat resolution after switch | Only active weapon contributes offensive values |
| C11 | HP damage/healing; EN and stamina regeneration | Existing resource logic unchanged |
| C12 | Potion HP use with available quantity | Existing montage, notify, consumption and recovery |
| C13 | Potion EN use with available quantity | Appropriate EN recovery, no HP crossover |
| C14 | Interrupt potion animation before commit | No erroneous double consumption/recovery |
| C15 | Use depleted potion | No recovery, no negative count |
| C16 | Medicine quick-slot cycle | Only HP/EN choices, no Fragment or unrelated Tool |
| C17 | Enemy and boss combat regression | Lock-on, hit outcomes, enemy death/boss states still operate |

## 4. Menu/UI (C4)

| ID | Scenario | Expected result |
|---|---|---|
| U01 | Open GameMenu from play | Correct input mode, readable five-page shell |
| U02 | Cycle Q/E through tabs | Character → Equipment → Inventory → Artifacts → System, wraps correctly |
| U03 | Mouse/keyboard/gamepad focus | No accidental page changes on hover; focus visible |
| U04 | Character outfit browse/equip | Correct set/preview/body update; one Outfit at a time |
| U05 | Equipment select each of six rows | Correct allowed-item filter and description |
| U06 | Equipment picker cancel/back | Prior selection restored, no input traps |
| U07 | Empty eligible-item list | Graceful state, no out-of-range selection |
| U08 | Active weapon marker vs focus marker | Semantically distinct |
| U09 | Inventory category filtering | Items actually owned and correctly typed |
| U10 | Inventory action modal | No bypass of equipment validation, closes correctly |
| U11 | Artifacts discovery and locked entries | Appropriate visible/hidden state, correct count |
| U12 | System settings audio/camera/graphics | Existing options remain functional |
| U13 | System Save & Quit / confirmation | Existing safe flow; not overwritten by new shell |
| U14 | Close GameMenu while subview/modal open | UI releases focus, game input restored |
| U15 | Character/weapon preview lifetime | No actor/RT leaks or game actor mutation from preview |
| U16 | 16:9 sizing and readable type | No clipping, overlap or unreadable focus states |

## 5. Rest and progression (C5)

| ID | Scenario | Expected result |
|---|---|---|
| R01 | Discover/enter rest site | Existing notification, sitting, fade and lock sequence |
| R02 | Open Rest Menu | Only approved current choices accessible |
| R03 | Select unavailable Skill Tree/Travel | Clearly disabled/placeholder; cannot call nonexistent system |
| R04 | Leave rest | Correct location, camera, input and movement return |
| R05 | Rest while wounded/depleted | Recovery/refill behavior matches existing intended rules |
| R06 | Death/interruption while resting | Controller cleanup, no stuck lock/fade |
| R07 | Search for old Level Up/time selectors | Unreachable from CodeX UI, backend not accidentally deleted |

## 6. Save/state compatibility

| ID | Scenario | Expected result |
|---|---|---|
| S01 | Load/continue with new equipment schema | No old invalid hand/armor/tool tag state |
| S02 | Persist chosen Outfit/weapon/Talisman/Attunement/Medicine | Same assignments after reload, if save support exists |
| S03 | Persist collected Artifact | Collection count/discovery not lost |
| S04 | Old slot/tag/ItemID migration | Documented remap or deliberate test-save invalidation |
| S05 | Missing/invalid saved item | Safe fallback, no crash or malformed UI |

Mark S01–S05 **N/A** if no save persistence contract exists yet, rather than claiming they passed.

## 7. Agent static acceptance checks

For each changed asset set:

1. Modified Blueprints compile in live Unreal without introduced errors; record any pre-existing warnings.
2. Assets are saved; do not infer save from a successful graph mutation alone.
3. `DT_InventoryItemData`, `DT_EquipmentSlots_UI`, enums, gameplay tags and new DataAsset references resolve.
4. No illegal `AllowedEquipSlots` remains on content intended to be selectable; legacy content is either hidden/disabled or deliberately migrated.
5. All direct Blueprint referencers of renamed/retired contracts have been inspected or left under a deliberate temporary adapter.
6. Active weapon/Medicine and equipment assignment event chains have no circular feedback loops or duplicate broadcasts from a single user action.
7. New UI pages have valid designer widget trees, indexes and bindable properties; dynamic creation does not leak widgets.
8. No new runtime dependency on a separate external gameplay framework is introduced.
9. Report what was checked by graph/data inspection and **what still requires user PIE**.

## 8. Issue reporting template

For each manual failure, capture: **Case ID**, current/expected behavior, affected equipped ItemIDs/slot tags, active weapon family, player action state, reproduction steps, any Blueprint errors/logs, and if possible a screenshot/video. Change the smallest relevant asset set and repeat the failed case plus the nearest regressions.

**Done criterion:** do not mark migration complete until all critical C/E/U/R cases have passed user PIE or are explicitly deferred with a reason.
