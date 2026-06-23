---
phase: "03-standalone-windows-monitoring"
plan: "01"
subsystem: "Standalone Capture"
tags: ["capture", "juce-input", "source-lifecycle", "bridge-protocol", "tdd"]
requires: ["02-04"]
provides: ["CAP-01", "CAP-03", "CAP-04", "UI-04-native"]
affects: ["PluginProcessor", "HostBridge", "bridge-protocol"]
tech-stack:
  added:
    - "juce::juce_audio_devices module for AudioDeviceManager/AudioIODeviceCallback"
  patterns:
    - "StandaloneSourceController interface with Impl factory"
    - "JuceInputSourceAdapter with preallocated scratch buffer for real-time callback"
    - "Closed protocol-v1 source.list/source.state payload builders"
key-files:
  created:
    - "plugin/include/LumaScope/Standalone/SourceModel.h"
    - "plugin/include/LumaScope/Standalone/StandaloneSourceController.h"
    - "plugin/source/Standalone/StandaloneSourceController.cpp"
    - "tests/native/StandaloneSourceControllerTests.cpp"
    - "tests/native/AudioConversionTests.cpp"
  modified:
    - "plugin/include/LumaScope/PluginProcessor.h"
    - "plugin/source/PluginProcessor.cpp"
    - "plugin/include/LumaScope/HostBridge.h"
    - "plugin/source/HostBridge.cpp"
    - "plugin/CMakeLists.txt"
    - "tests/native/CMakeLists.txt"
    - "tests/native/WebResourcesTests.cpp"
    - "tests/native/HostBridgeTests.cpp"
    - "docs/bridge-protocol.md"
decisions:
  - "Use pushStandaloneAudioBlock on processor to share analyzer with processBlock path"
  - "JUCE 8 uses audioDeviceIOCallbackWithContext (not audioDeviceIOCallback)"
  - "StandaloneSourceControllerImpl is a standalone class (not nested Impl) to avoid compile issues"
  - "SystemOutput selection returns not_implemented error; WASAPI loopback deferred to 03-02"
  - "SourceModel.h moved to Standalone/ subdirectory under plugin include"
  - "juce_audio_devices module linked in test CMakeLists.txt for adapter compilation"
metrics:
  duration: "30m"
  completed: "2026-06-23"
---

# Phase 3 Plan 1: Shared Source Lifecycle and JUCE Input Capture

Implemented the shared standalone source lifecycle contract and integrated selectable JUCE input-device capture with the existing analyzer ingress. The native side of UI-04 can now represent source list/state without exposing VST3 source controls.

## Tasks Completed

### Task 1: Source lifecycle contract (TDD) — `664c457`

- Defined `SourceMode` enum (`InputDevice`, `SystemOutput`), `SourceDescriptor`, `SourceSelection`, `SourceStateSnapshot` with lifecycle states (`stopped`, `starting`, `active`, `silent`, `error`)
- Defined `StandaloneSourceController` interface with `enumerateSources()`, `selectSource()`, `stop()`, `currentStateSnapshot()`
- Added factory function `createStandaloneSourceController()`
- Wrote 16 tests covering D-01 (separate modes), D-04 (not combined list), D-05 (error on failure), D-06 (no fallback), CAP-04 (lifecycle), D-08 (silent state), bounded fields, mode/state string conversion, empty lists, and invalid selection
- Updated `tests/native/CMakeLists.txt` and `WebResourcesTests.cpp`

### Task 2: JUCE input capture with analyzer ingress — `a840b83`

- Added `pushStandaloneAudioBlock()` to `LumaScopeAudioProcessor` — reuses the same analyzer and mailbox path as `processBlock`
- Implemented `JuceInputSourceAdapter` with JUCE 8 `audioDeviceIOCallbackWithContext`, preallocated scratch buffer, real-time-safe downmix (mono copy or multi-channel averaging), and no allocation/JSON/WebView in the callback
- Implemented `StandaloneSourceControllerImpl` with `juce::AudioDeviceManager` for input device enumeration, selection (open device, register adapter), and stop (remove callback, close device)
- `SystemOutput` mode gracefully returns `not_implemented` error — WASAPI loopback deferred to 03-02
- Wrote 11 `AudioConversionTests` covering mono/stereo/4-channel downmix, zero samples/channels, non-finite samples, source switching, shared analyzer state, and real-time safety shape
- Updated `plugin/CMakeLists.txt` and test build with new sources and `juce_audio_devices` link

### Task 3: Native source protocol payloads — `9113f03`

- Added `sourceListEvent` and `sourceStateEvent` identifiers to `HostBridge`
- Implemented `makeSourceList()` — builds closed protocol-v1 payload with separate `inputDevices`/`systemOutputs` arrays, bounded IDs and display names (256 chars)
- Implemented `makeSourceStateSnapshot()` — builds payload with mode, state, selected source, bounded error code (64 chars) and message (256 chars)
- Added `<vector>` include fix to `SourceModel.h`
- Wrote 10 `HostBridgeTests` for source list/state payloads, error/stopped/silent states, bounded truncation, event ID stability, empty lists, and event field matching
- Updated `docs/bridge-protocol.md` with source.list, source.state, source.select, source.stop event documentation, state table, error codes, and D-04/D-05/D-06/D-08 references

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing dependency] `juce_audio_devices` required for tests**
- **Found during:** Task 2 build
- **Issue:** `StandaloneSourceController.cpp` uses `juce::AudioDeviceManager` and `juce::AudioIODeviceCallback` from `juce_audio_devices`, which was not linked in the test CMakeLists.txt
- **Fix:** Added `juce::juce_audio_devices` to `target_link_libraries` for `LumaScopeNativeTests`
- **Files modified:** `tests/native/CMakeLists.txt`
- **Commit:** `a840b83`

**2. [Rule 2 - API adaptation] JUCE 8 uses different callback signature**
- **Found during:** Task 2 build
- **Issue:** `audioDeviceIOCallback` was removed in JUCE 8; replacement is `audioDeviceIOCallbackWithContext` with `const juce::AudioIODeviceCallbackContext&` parameter
- **Fix:** Changed callback override signature and added `juce::` namespace prefix to context type
- **Files modified:** `plugin/source/Standalone/StandaloneSourceController.cpp`
- **Commit:** `a840b83`

**3. [Rule 2 - API adaptation] `AudioDeviceManager::initialise` uses different pattern**
- **Found during:** Task 2 build
- **Issue:** `initialise()` returns `juce::String` (error message) directly, not a bool with a String* output parameter
- **Fix:** Changed to use return value check: `errorResult = deviceManager.initialise(...)` then `if (errorResult.isNotEmpty())`
- **Files modified:** `plugin/source/Standalone/StandaloneSourceController.cpp`
- **Commit:** `a840b83`

**4. [Rule 2 - Compile issue] `StandaloneSourceController::Impl` nested class pattern**
- **Found during:** Task 2 build
- **Issue:** `Impl` as a nested class of `StandaloneSourceController` didn't compile because no nested declaration was in the header
- **Fix:** Used standalone `StandaloneSourceControllerImpl` class instead, keeping the factory function
- **Files modified:** `plugin/source/Standalone/StandaloneSourceController.cpp`
- **Commit:** `a840b83`

## Threat Surface Scan

No new threat flags found. The `SystemOutput` mode returns a bounded `not_implemented` error, which is defined in the plan's threat register as accept/not-applicable for this plan. All source protocol payloads use closed enums and bounded fields per T-03-01-01 mitigation. The capture callback uses preallocated buffers per T-03-01-03 mitigation. No auto-fallback per T-03-01-02 mitigation.

## Known Stubs

No stubs. The `SystemOutput` mode returning `not_implemented` is intentional behavior for the deferred WASAPI loopback capture (03-02), not a stub.

## Self-Check: PASSED

- All 14 created/modified files verified on disk ✓
- All 3 commit hashes verified in git log ✓
- No unwanted stub patterns found ✓
- No threat flags outside plan's threat register ✓
- Build and all tests pass ✓
- Project verification script passes ✓

## Verification Results

- `cmake --preset vs2019-debug` — configured successfully
- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4` — built successfully
- `ctest --preset vs2019-debug --output-on-failure` — 1/1 test passed (all 37 test cases across 5 test modules)
- `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` — passed (all 5 negative probes)
