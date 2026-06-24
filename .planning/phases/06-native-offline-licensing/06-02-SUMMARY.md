# 06-02-SUMMARY: Local Entitlement Store, Grace Model, Licensing State & Core

**Completed:** 2026-06-23

## What Was Built

### LocalEntitlementStore (LIC-04)
- DPAPI-encrypted file storage at `%APPDATA%/LumaScope/entitlement.dat`
- `CryptProtectData`/`CryptUnprotectData` with CRYPTPROTECT_UI_FORBIDDEN
- `write()`, `read()`, `clear()`, `exists()` interface
- Rejects empty or >64KB content
- Returns `nullopt` on missing/corrupt/decrypt-failure (never crashes)
- Inject `setBasePathForTesting()` for deterministic tests
- Tested: roundtrip, missing file, corrupt file, clear, empty/oversize rejection, DPAPI blob verified non-human-readable

### GraceModel (LIC-05, LIC-07)
- `checkOfflineGrace()` — 7-day window, elapsedDays calculation, three status outputs (active/warning/expired)
- `checkOfflineGraceWithRollback()` — compares current time against lastKnownSystemTime; >1 hour backward jump triggers rollback path using stored system time as reference
- `calculateRemainingDays()` — generous partial-day rounding via `floor(remaining + 0.999999)`
- Warning threshold at 1 day remaining

### LicensingState (LIC-09)
- `std::atomic<LicenseStatus>` with release/acquire ordering — lock-free from audio thread
- Sequence counter for change detection via `hasChanged()`
- 10 states: uninitialized, not_activated, activating, activated, offline_grace, revalidation_required, revoked, corrupt, service_unavailable, deactivating
- `toString()`, `isTransitionState()`, `isErrorState()` helpers

### LicensingCore (LIC-05, LIC-06, LIC-08, D-11, D-12, D-13)
- Unified state machine facade over all licensing operations
- `loadFromDisk()` — parses and verifies stored token, checks grace, correct status on exit
- `handleActivationResponse()` — verifies signature, machine match, saves to store
- `handleValidationResponse()` — same as activation but for revalidation
- `handleDeactivationResponse()` — clear store AND transition (D-11: only on server success)
- `handleServerError()` — preserves entitlement, transitions to service_unavailable (D-12)
- `handleAuthoritativeFailure()` — maps error codes to revoked/not_activated, clears store (D-13)
- `checkGrace()` — periodic re-evaluation from editor timer
- Inject `setMachineIdForTesting()` for deterministic test behavior
- Thread-safe `currentDetail()` with mutex, atomic `currentStatus()`

## Files Created/Modified

| File | Purpose |
|------|---------|
| `plugin/include/LumaScope/Licensing/LocalEntitlementStore.h` | DPAPI store interface |
| `plugin/source/Licensing/LocalEntitlementStore.cpp` | CryptProtectData/CryptUnprotectData implementation |
| `plugin/include/LumaScope/Licensing/GraceModel.h` | Grace period & rollback interface |
| `plugin/source/Licensing/GraceModel.cpp` | Grace calculation logic |
| `plugin/include/LumaScope/Licensing/LicensingState.h` | Atomic cross-thread status |
| `plugin/source/Licensing/LicensingState.cpp` | Status string/classifier helpers |
| `plugin/include/LumaScope/Licensing/LicensingCore.h` | Unified state machine interface |
| `plugin/source/Licensing/LicensingCore.cpp` | State machine implementation |
| `tests/native/Licensing/LocalEntitlementStoreTests.cpp` | 10 DPAPI store tests |
| `tests/native/Licensing/GraceModelTests.cpp` | 9 grace/rollback tests |
| `tests/native/Licensing/LicensingCoreTests.cpp` | 15+ state machine tests |
| `tests/native/CMakeLists.txt` | Updated sources + crypt32 link |

## Bugs Fixed

1. **`parseSignedEntitlement` canonical reconstruction** — canonical was always recomputed from payload instead of using the stored canonical. Updated to use fixture's canonical when available, preserving signature integrity.

2. **Fixture JSON missing `algorithm`/`kid`** — `entitlement-v1.json` lacked top-level `algorithm` and `kid` fields. Added them matching the payload values.

3. **`FileOutputStream` non-truncating behavior** — Corruption test overwrote DPAPI blob with short string but stream didn't truncate, leaving residual DPAPI bytes. Fixed by writing corrupt data directly to new file.

4. **`LicensingCore` machine identity in tests** — Fixture token has fixed machineId, but `deriveMachineIdentifier()` returns actual machine identity. Added `setMachineIdForTesting()` injection point.

5. **GraceModel test data errors** — Rollback test used `lastVerified = now - 3 days` and `lastKnownSystem = now - 3 days`, making elapsed = 0 (expected 3). Fixed test data so elapsed correctly produces 4 days remaining.

## Test Results

```
All 30+ licensing tests pass (LocalEntitlementStore, GraceModel, LicensingCore, plus all prior tests)
Both LumaScopeNativeTests and LumaScope_Standalone build cleanly
```
