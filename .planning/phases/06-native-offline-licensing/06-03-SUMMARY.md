# Plan 06-03 Summary: Activation Client and UI Integration

**Completed:** 2026-06-23
**Duration:** ~2 hours

## Goal

Implement the non-real-time activation HTTP client and connect it with the React/MUI activation/deactivation/status UI, bridging through the existing HostBridge protocol.

## What Was Built

### ActivationClient (`plugin/source/Licensing/ActivationClient.cpp`)

- Queue-based HTTP client using `juce::TimeSliceThread` for non-blocking activation requests
- `enqueueActivate()` / `enqueueValidate()` / `enqueueDeactivate()` — thread-safe enqueue methods
- `cancelAll()` — cancels pending and in-flight requests
- Configurable `baseUrl` (constructor-injected for testing) and optional `HttpFactory` for mock HTTP
- On completion/deactivation, invokes the callback with `SignedEntitlementToken` or error info
- Handles HTTP error status codes and connection failures gracefully

### ActivationClientTests (`tests/native/Licensing/ActivationClientTests.cpp`)

- 8 tests covering construction, activation, validate, deactivate, cancelAll, request body format, multiple requests, and network error
- Custom `MockHttpStream` for status code propagation (JUCE doesn't expose status codes on `MemoryInputStream`)

### CMake Integration

- `plugin/CMakeLists.txt`: Added `tweetnacl.c`, linked `bcrypt` and `crypt32`
- Android/iOS excluded for Windows-only build

### HostBridge Extension

- Added `EventId::makeLicenseStatusPayload()` — constructs `license.status` payload
- Added `EventId::licenseActivate`, `licenseDeactivate`, `licenseValidate` — UI→native request events
- Added `canHandleNativeEvent("license.")` routing path

### PluginProcessor Integration

- Owns `LicensingCore` and `ActivationClient` as class members
- `handleBridgeMessage()` routes `license.*` events to `ActivationClient` and `LicensingCore`
- `onActivationComplete` / `onActivationError` / `onDeactivationComplete` callbacks update `LicensingCore` state and emit `license.status` events
- Timer callback polls licensing state changes and pushes to WebView

### PluginEditor Integration

- Overrides `timerCallback()` to handle licensing state timers
- Emits `license.status` events to WebView on state changes

### TypeScript Protocol (`ui/src/bridge/protocol.ts`)

- Added `LicensingStateEnum` — maps all `LicenseStatus` enum values
- Added `LicenseStatusPayload` interface matching native payload shape
- Added `parseLicenseStatus()` — validates and returns typed payload
- Added event IDs: `license.status`, `license.activate`, `license.deactivate`, `license.validate`

### BridgeProvider (`ui/src/bridge/BridgeProvider.tsx`)

- Exports `LicenseContext` with status and request helpers
- Exports `useLicenseStatus()` — returns current `LicenseStatusPayload`
- Exports `useLicenseRequest()` — returns `{ status, isPending, error, activate, deactivate, validate }`
- Exports `useBridgeRequest()` for generic bridge request emitting

### React/MUI Licensing Components

- **`ActivationStatus.tsx`** — Chip showing 9-state license status with MUI colors/icons per UI-SPEC Section 4.1
- **`ActivationDialog.tsx`** — License key entry dialog with progress, error severity mapping, success state
- **`DeactivationDialog.tsx`** — Confirmation dialog with D-11 enforcement (server success clears local)
- **`GraceWarnAlert.tsx`** — Dismissible Collapse Alert at threshold boundaries (≤2 days, ≤1 day, expired)
- **`StatusFooter.tsx`** — Extended with `licensing` prop and `ActivationStatus` chip
- **`AppShell.tsx`** — Dialog state machine for activation/deactivation, `GraceWarnAlert` between source strip and analyzer

### Plan 06-04 (Integration and Verification)

- VST3 and Standalone targets compile with full licensing integration
- `ActivationClientTests` all pass (8/8)
- All pre-existing tests continue to pass (only `processBlock avoids prohibited realtime token` pre-existing failure remains)
- Added 5 integration tests to `LicensingCoreTests`:
  - loadFromDisk with valid fixture → `activated`
  - loadFromDisk with tampered signature → `corrupt`
  - deactivate then restart → `not_activated`
  - grace expiry after 8 days → `revalidation_required`
  - validation response extends grace → still `activated`
- Updated `docs/bridge-protocol.md` with license status states and UI request event schemas

## Files Created
- `plugin/source/Licensing/ActivationClient.cpp`
- `plugin/include/LumaScope/Licensing/ActivationClient.h`
- `tests/native/Licensing/ActivationClientTests.cpp`
- `ui/src/components/ActivationStatus.tsx`
- `ui/src/components/ActivationDialog.tsx`
- `ui/src/components/DeactivationDialog.tsx`
- `ui/src/components/GraceWarnAlert.tsx`

## Files Modified
- `plugin/CMakeLists.txt` — tweetnacl.c, bcrypt, crypt32
- `plugin/source/PluginProcessor.cpp` — LicensingCore + ActivationClient ownership
- `plugin/include/LumaScope/PluginProcessor.h` — header declarations
- `plugin/source/PluginEditor.cpp` — licensing event handlers
- `plugin/include/LumaScope/PluginEditor.h` — header declarations
- `plugin/source/Licensing/LicensingCore.cpp` — `handleServerError` preserves status on valid cached
- `plugin/include/LumaScope/Licensing/LicensingCore.h` — `currentDetail()`, test helpers
- `plugin/include/LumaScope/Licensing/LicensingState.h` — `LicensingDetail` struct
- `plugin/source/HostBridge.cpp` — licensing event routing
- `plugin/include/HostBridge.h` — `EventId` licensing enums
- `ui/src/bridge/protocol.ts` — licensing event types
- `ui/src/bridge/BridgeProvider.tsx` — licensing context/hooks
- `ui/src/components/StatusFooter.tsx` — licensing chip
- `ui/src/app/AppShell.tsx` — dialogs, grace alert
- `tests/native/Licensing/LicensingCoreTests.cpp` — 5 integration tests

## Verification
- `npm run test` — 60 passing (0 failing) in UI
- `npm run typecheck` — TypeScript compiles clean
- Native tests: LicensingCore (all pass), ActivationClient (all pass)
- Worker tests: 61/61 passing
- VST3 target: compiles
- Standalone target: compiles
