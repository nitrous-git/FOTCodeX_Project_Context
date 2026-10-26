# G4.0/G4.1 — native character stats and Souls

Implemented October 26, 2026 in `FOTCodeX_SLF_583`. Live assets are authoritative.

## Ownership and public contract

- Player Core `Components/AC_CharacterStatsComponent`: eight integer base/effective attributes, MaxHP, current/max FP and Stamina, and maximum equipment-load capacity. No current-HP variable.
- Player Core `Components/AC_ProgressionComponent`: Level, integer Souls, configurable next-level cost. No Inventory currency dependency or level purchases.
- `BP_PlayerBase.GetCharacterStatusSnapshot()` returns `Structs/FCharacterStatusSnapshot`; `OnCharacterStatusChanged` forwards the two components' change dispatchers. Read once on widget initialization, then subscribe. No per-frame polling.
- `bEquipmentStatsAvailable=false` explicitly marks G4.3 calculations as absent. Snapshot has no fabricated attack, defense or equipped-weight values.

## HP boundary for G4.4

`BP_PlayerBase.Health` remains the sole legacy current-HP value. Snapshot reads a copy clamped to `[0, MaxHP]`; no new code writes Health or changes damage/death handling. The audited live base has Health default 0; its AnyDamage chain handles camera/movement feedback, and no Health nodes were found in its listed graphs. Do not infer active HP depletion from older descriptions. `NotifyLegacyHealthChanged()` is the notification hook for future legacy writers. Audit/redirect any actual writers during G4.4 before migrating ownership. The present default snapshot therefore reports current HP 0, not a fabricated full-health value.

## Attributes, balance and modifiers

Attribute identifiers are Name values: Vigor, Mind, Endurance, Strength, Dexterity, Intelligence, Faith, Arcane. Unknown identifiers return Found=false; debug modification rejects them. Base defaults are 10 in the component (standalone struct defaults are 0).

APIs: `GetPrimaryAttribute`, `GetEffectiveAttribute`, `DebugSetBaseAttribute`, `InitializeStats`, `RecalculateDerivedStats`, `SetModifierForSource`, `RemoveModifiersBySource`, `DebugPrintStats`; dispatcher `OnStatsChanged`.

`ModifiersBySource` maps String source IDs to flat `FPrimaryAttributes` contributions. Setting an existing source replaces it; removing a source removes its entire contribution. Empty IDs are rejected. Limit: 64 sources; each contribution clamps to [-999,999]. Recalculation sums all contributions before clamping effective attributes, so iteration order cannot affect results. Base/effective bounds use configurable AttributeLimit (safety range 1–999).

`DataAssets/DA_CharacterStatBalance_Default` supplies base values, per-segment rate vectors (X/Y/Z) and two soft caps. Formula: Base + min(Attribute,Cap1)*X + clamp(Attribute-Cap1,0,Cap2-Cap1)*Y + max(Attribute-Cap2,0)*Z. Output clamps to [0,1000000]. Defaults: caps 40/60, attribute limit 99; HP base50/rates5,2,1; FP base20/rates3,1,0.5; Stamina base80/rates2,1,0.5; load base30/rates1,0.5,0.25.

At attributes10: MaxHP100, MaxFP50, MaxStamina100, MaxEquipmentLoad40. Initial FP/Stamina fill once; later recalculation clamps without refilling. Components initialize through their own BeginPlay and do not tick.

## Progression and future integration

APIs: `InitializeProgression`, `GetLevel`, `GetSouls`, `GetSoulsRequiredForNextLevel`, `GrantSouls`, `CanSpendSouls`, `SpendSouls`, `DebugValidateProgression`; dispatcher `OnProgressionChanged`. Grants/spends require positive amounts. Grants reject int32 overflow, spending rejects insufficient balance. Default Level1 costs160 using exposed base100/linear50/quadratic10 coefficients. No automatic currency grant.

Future G4.3 should subscribe to `AC_EquipmentComponent.OnEquipmentSlotChanged(EquipmentSlot, PreviousItemID, NewItemID)` and set stable source IDs; inventory ownership changes alone must not apply bonuses. G4.2 can consume the snapshot and dispatcher. Future Level Up previews should calculate without mutating committed stats.

## Validation

All new/modified Blueprints compiled with warnings treated as errors and saved. Twenty-two transient-component checks passed: defaults, Mind/Endurance changes, invalid attribute rejection, modifier replacement/removal, repeated recalculation, bounds, soft-cap boundaries, resource clamp/no-refill, Souls validation and level cost. `DebugPrintCharacterStatusSnapshot` is callable on the player.

PIE validation passed after proceeding past the existing SLF `W_Settings_KeyMapping` compile dialog: both new components initialized, unified snapshot returned expected defaults, progression self-test passed on the actual player, and the existing ABP_PlayerMaster instance remained active. Sword, shield and all four armor slots each passed unequip/re-equip with their assignments and visuals restored; the status snapshot remained unchanged. PIE was stopped afterward. The debug attribute-print helper was corrected and recompiled/saved. Full movement/combat input regression was not exercised; the existing SLF compilation warning remains unrelated and unresolved.
