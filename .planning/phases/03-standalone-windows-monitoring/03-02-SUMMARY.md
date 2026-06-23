---
phase: 03-standalone-windows-monitoring
plan: 02
type: execute
wave: 2
subsystem: "standalone source"
tags:
  - "wasapi"
  - "loopback"
  - "endpoint-notifications"
  - "format-conversion"
requires:
  - "03-01"
provides:
  - "CAP-02 (render endpoint enumeration)"
  - "CAP-03 (WASAPI mix format conversion)"
  - "CAP-04 (endpoint notifications and recovery)"
affects:
  - "StandaloneSourceController"
  - "SourceModel.h"
tech-stack:
  added:
    - "Windows Core Audio API (MMDeviceEnumerator, IAudioClient, IMMNotificationClient)"
    - "WASAPI shared-mode loopback (AUDCLNT_STREAMFLAGS_LOOPBACK)"
    - "Format conversion: float32 PCM, int16/24/32 PCM, WAVEFORMATEXTENSIBLE"
    - "IMMNotificationClient for render endpoint change monitoring"
  patterns:
    - "Opaque void* for Windows SDK types in headers (avoids JUCE Windows.h conflict)"
    - "Thread-safe atomic error/silence state machine (hasHardwareError, recentLevel, consecutiveSilentFrames)"
    - "All COM setup outside capture hot path"
    - "Bounded preallocated scratch buffers for packet conversion"
key-files:
  created:
    - "plugin/include/LumaScope/Standalone/WasapiLoopbackSourceAdapter.h"
    - "plugin/source/Standalone/windows/WasapiLoopbackSourceAdapter.cpp"
    - "plugin/include/LumaScope/Standalone/WasapiDeviceNotifications.h"
    - "plugin/source/Standalone/windows/WasapiDeviceNotifications.cpp"
    - "plugin/source/Standalone/windows/WasapiEndpointEnumerator.cpp"
    - "tests/native/WasapiEndpointModelTests.cpp"
  modified:
    - "plugin/include/LumaScope/Standalone/SourceModel.h"
    - "plugin/source/Standalone/StandaloneSourceController.cpp"
    - "plugin/CMakeLists.txt"
    - "tests/native/AudioConversionTests.cpp"
    - "tests/native/RealtimeHandoffTests.cpp"
    - "tests/native/CMakeLists.txt"
    - "tests/native/WebResourcesTests.cpp"
    - "docs/troubleshooting.md"
decisions:
  - "Use void* for WAVEFORMATEX and HANDLE in public headers to avoid Windows.h/JUCE Notification class conflict"
  - "Pre-opaque-type casting in .cpp: static_cast<const WAVEFORMATEX*>, static_cast<HANDLE>"
  - "Downmix: group source channels evenly into dest channels and average"
  - "Silence detection: exponential moving average (alpha=0.1) with 1e-6 threshold and 20-frame hold"
  - "Error state: atomic hasHardwareError set by capture thread on GetNextPacketSize/GetBuffer failure"
  - "D-07 bounded retry: maxRetryAttempts=3, target only same endpoint ID (no fallback)"
duration: "~3 hours"
completed: "2026-06-23"
plan_start: "2026-06-23T14:00:00Z"
plan_end: "2026-06-23T17:20:00Z"
requirements:
  - "CAP-02"
  - "CAP-03"
  - "CAP-04"
---

# Phase 3 Plan 02: WASAPI Loopback Capture and Render Endpoint Notifications

**One-liner:** Implemented native Windows WASAPI loopback capture with shared-mode render endpoint enumeration, mix format conversion (float32/int16/int24/int32 PCM + WAVEFORMATEXTENSIBLE), silence detection, endpoint change notifications, and bounded hardware error recovery — completing CAP-02, CAP-03, and CAP-04 for the standalone application's System Output mode.

## Tasks

### Task 3 of 3 completed

| Task | Type | Name | Commit | Key Files |
|------|------|------|--------|-----------|
| 1 | auto | Render endpoint enumeration | `863fe48` | WasapiEndpointEnumerator.cpp, SourceModel.h, WasapiEndpointModelTests.cpp |
| 2 | auto | Loopback capture and format conversion | `dd1ca53` | WasapiLoopbackSourceAdapter.h/.cpp, AudioConversionTests.cpp, StandaloneSourceController.cpp |
| 3 | auto | Notifications, invalidation, silence, and restart | `09633d5` | WasapiDeviceNotifications.h/.cpp, WasapiEndpointModelTests.cpp, docs/troubleshooting.md |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed `NOMINMAX` missing before `Windows.h` in WasapiLoopbackSourceAdapter.cpp**
- **Found during:** Task 2 build
- **Issue:** `<Windows.h>` macros `min`/`max` conflicted with `std::min` and JUCE's `Notification` class
- **Fix:** Added `#define NOMINMAX` before `<Windows.h>` and `#undef NOMINMAX` after
- **Files:** `plugin/source/Standalone/windows/WasapiLoopbackSourceAdapter.cpp`

**2. [Rule 1 - Bug] Fixed `void*` member casting for `mixFormat` and `captureEvent`**
- **Found during:** Task 2 build (after opaque-type refactoring in header)
- **Issue:** `mixFormat` stored as `void*` needed `static_cast<const WAVEFORMATEX*>` in capture thread; `captureEvent` stored as `void*` needed `static_cast<HANDLE>` for `SetEventHandle`, `WaitForSingleObject`, etc.
- **Fix:** Added explicit casts in `start()` and `captureThreadFunc()`

**3. [Rule 1 - Bug] Fixed `convertPacket` signature mismatch (const void* vs const WAVEFORMATEX*)**
- **Found during:** Task 2 build
- **Issue:** Header declared `convertPacket` with `const void* format` but .cpp had `const WAVEFORMATEX*`
- **Fix:** Updated .cpp signature and added internal `static_cast<const WAVEFORMATEX*>` cast

**4. [Rule 1 - Bug] Fixed use-after-free: `device` COM interface released before passing to thread**
- **Found during:** Task 2 code review
- **Issue:** `safeReleaseCom(device)` called on line 496, then the dangling pointer passed to `captureThreadFunc` on line 500
- **Fix:** Removed `safeReleaseCom(device)` before thread creation (capture thread function owns the release)

**5. [Rule 1 - Bug] Fixed test data overflow in `makeInterleavedInt16` and `makeInterleavedInt24` helpers**
- **Found during:** Task 2 test failures
- **Issue:** `makeInterleavedInt16(channels, frames, 16384)` produced `value * (1 + ch)` = 32768 for ch1, overflowing `std::int16_t` to -32768
- **Fix:** Changed test helper calls to use values that don't overflow (e.g., 8192 base gives ch1=16384 within range)

**6. [Rule 1 - Bug] Fixed zero-frame test using empty vector (nullptr data() on MSVC)**
- **Found during:** Task 2 test failures
- **Issue:** `std::vector<uint8_t>()` has `data()` returning nullptr on MSVC, causing `source == nullptr` check to fail
- **Fix:** Changed test to use a non-empty buffer

**7. [Rule 3 - Fix] Added WasapiLoopbackSourceAdapter.cpp and WasapiDeviceNotifications.cpp to test target CMakeLists.txt**
- **Found during:** Task 2/3 linker errors
- **Issue:** Linker couldn't find WasapiLoopbackSourceAdapter symbols because .cpp wasn't compiled for the test target
- **Fix:** Added source files to `tests/native/CMakeLists.txt`

## Threat Flags

None — all threat register mitigations (T-03-02-01, T-03-02-02, T-03-02-03) were addressed in implementation.

## Success Criteria

- [x] **CAP-02**: Render endpoint enumeration produces `SystemOutput` source descriptors with `wasapi-loopback-` prefix via `enumerateRenderEndpoints()`, distinct from `juce-input-` device IDs.
- [x] **CAP-03**: WASAPI mix format conversion tests cover float32 PCM, int16/24/32 PCM, WAVEFORMATEXTENSIBLE, silent buffers, unsupported formats, zero-frame bounds, 5.1 downmix, negative values, computeLevel, and capture-thru-analyzer integration.
- [x] **CAP-04**: Endpoint change notifications monitored via `IMMNotificationClient`; adapter reports hardware errors atomically; error state surfaces `endpoint_lost`/`endpoint_changed` codes; no silent fallback to another endpoint (D-06); same-source bounded retry supported (D-07, maxRetryAttempts=3).
- [x] **D-05**: Endpoint removal or failure transitions capture to error state requiring user action.
- [x] **D-08/D-09**: Valid-but-silent capture remains active with `silent` state (not error); silence != failure.
- [x] All tests pass: `ctest --preset vs2019-debug --output-on-failure` reports 100%.
- [x] Full verification: `verify-project.ps1 -SelfTest` passes all artifact, Context7, identity, secrets, remote assets, and release-mode checks.

## Self-Check: PASSED

- [x] LumaScopeNativeTests.exe exists in build output
- [x] All 3 commits exist in git log
- [x] ctest reports 100% passed
- [x] verify-project.ps1 reports all checks passed
- [x] No untracked files remain (only .planning/ files, which are gitignored)
