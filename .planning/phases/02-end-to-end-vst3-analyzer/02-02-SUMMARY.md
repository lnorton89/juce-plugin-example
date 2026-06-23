---
phase: 02-end-to-end-vst3-analyzer
plan: 02
subsystem: native-vst3-realtime-handoff
tags: [juce, vst3, realtime, analyzer, mailbox, ctest]

requires:
  - phase: 02-01-native-spectrum-analyzer-core
    provides: Processor-independent analyzer core and bounded SpectrumSnapshot model
provides:
  - Fixed-slot latest-snapshot-wins mailbox for SpectrumSnapshot handoff
  - Processor-owned analyzer state independent of editor lifetime
  - Transparent processBlock analyzer ingress with sample-equivalent audio passthrough
  - Native regression tests for mailbox semantics, processor passthrough, sample-rate resets, and realtime safety shape
affects: [02-03-web-spectrum-renderer, vst3-analyzer, realtime-audio-boundary]

tech-stack:
  added: []
  patterns: [fixed-slot atomic mailbox, processor-owned analyzer, editor-facing snapshot watermark reads]

key-files:
  created: [plugin/include/LumaScope/SnapshotMailbox.h, tests/native/RealtimeHandoffTests.cpp, tests/native/PluginProcessorTests.cpp]
  modified: [plugin/include/LumaScope/PluginProcessor.h, plugin/source/PluginProcessor.cpp, tests/native/CMakeLists.txt, tests/native/WebResourcesTests.cpp]

key-decisions:
  - "Use a fixed two-slot atomic mailbox where the latest complete snapshot wins and stale snapshots are dropped rather than queued."
  - "Keep analyzer ownership in LumaScopeAudioProcessor so analysis continues while editors are closed or recreated."
  - "Have processBlock observe the actual callback buffer shape and leave samples untouched, preserving mono/stereo passthrough and malformed edge-block robustness."

patterns-established:
  - "Message/UI consumers read snapshots with a caller-owned last-seen sequence watermark."
  - "Realtime safety regressions are guarded by native tests that inspect the narrow processBlock body for obvious prohibited work."

requirements-completed: [DSP-01, DSP-04, DSP-05, DSP-06, VST3-02, VST3-03]

duration: 12 min
completed: 2026-06-23
---

# Phase 02 Plan 02: VST3 Analyzer Handoff Summary

**Processor-owned analyzer ingress with fixed-slot latest-snapshot handoff and transparent VST3 passthrough**

## Performance

- **Duration:** 12 min
- **Started:** 2026-06-22T23:35:36Z
- **Completed:** 2026-06-22T23:47:29Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments

- Added `SnapshotMailbox`, a bounded two-slot handoff for complete `SpectrumSnapshot` values with latest-snapshot-wins semantics.
- Connected `LumaScopeAudioProcessor::processBlock` to the analyzer core without clearing, scaling, reordering, or otherwise modifying audio samples.
- Added processor lifecycle tests for mono/stereo passthrough, variable block sizes, sample-rate resets, zero-size blocks, and editor-closed operation.
- Added regression checks for obvious realtime contract violations in `processBlock` and mailbox storage.

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement latest-snapshot mailbox** - `b0281e5` (test RED), `9590965` (feat GREEN)
2. **Task 2: Attach analyzer ingress to processor without changing audio** - `f5f78b8` (test RED), `d1d84db` (feat GREEN)
3. **Task 3: Add real-time safety regression checks** - `978b7da` (test)

## Files Created/Modified

- `plugin/include/LumaScope/SnapshotMailbox.h` - Fixed-slot atomic latest snapshot handoff with validation and reset support.
- `plugin/include/LumaScope/PluginProcessor.h` - Processor-owned analyzer/mailbox state and editor-facing snapshot read API.
- `plugin/source/PluginProcessor.cpp` - `prepareToPlay`, `releaseResources`, and `processBlock` analyzer ingress while preserving audio passthrough.
- `tests/native/RealtimeHandoffTests.cpp` - Mailbox sequence/drop/read, malformed snapshot, bounded storage, and no-queue regression tests.
- `tests/native/PluginProcessorTests.cpp` - Processor passthrough, lifecycle, sample-rate reset, editor-closed, and realtime-shape tests.
- `tests/native/CMakeLists.txt` - Native test target wiring for processor and new regression source checks.
- `tests/native/WebResourcesTests.cpp` - Native test runner registration for the new suites.

## Decisions Made

- Latest snapshot wins across the audio-to-UI boundary; the mailbox never grows a queue or preserves stale frames.
- The processor owns analyzer state, so analysis continues without an editor and later UI consumers can catch up from the newest snapshot.
- `processBlock` uses the actual callback buffer supplied by the host/test harness for analysis and does not write to audio samples.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Avoided crashing on mono/malformed callback buffers**
- **Found during:** Task 2 (Attach analyzer ingress to processor without changing audio)
- **Issue:** The first GREEN implementation used `getBusBuffer` based on the configured processor bus. The new mono processor test supplied a one-channel callback buffer while the default bus layout was stereo, causing the native test executable to segfault.
- **Fix:** Changed `processBlock` to validate and analyze the actual callback buffer shape directly, preserving passthrough behavior and improving malformed edge-block robustness.
- **Files modified:** `plugin/source/PluginProcessor.cpp`
- **Verification:** `cmake --build --preset vs2019-debug --target LumaScope_VST3 LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure`
- **Committed in:** `d1d84db`

---

**Total deviations:** 1 auto-fixed (1 bug).  
**Impact on plan:** The fix stayed inside the planned processor ingress boundary and strengthened DSP-04/VST3-02 coverage without adding standalone capture, licensing, or v2 controls.

## Issues Encountered

- Build output still prints the known non-fatal `pwsh.exe` missing line from the JUCE/CMake helper path on this Windows PowerShell 5.1 machine. Native build, VST3 build, and CTest exit successfully.

## Known Stubs

None. Empty/default initializers in snapshot and mailbox structures are intentional bounded state, not UI-flowing placeholder data.

## Threat Flags

None. This plan implemented the planned host audio callback -> analyzer ingress and audio thread -> message/UI handoff boundaries already covered by T-02-03 through T-02-05.

## Verification Evidence

- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4` - Task 1 RED failed as expected before implementation, then passed after `SnapshotMailbox`.
- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4` - Task 2 RED failed as expected on missing `readLatestSpectrumSnapshot`, then passed after processor integration.
- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` - Task 3 passed.
- `cmake --preset vs2019-debug` - passed.
- `cmake --build --preset vs2019-debug --target LumaScope_VST3 LumaScopeNativeTests --parallel 4` - passed.
- `ctest --preset vs2019-debug --output-on-failure` - 1/1 native test passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 02-03 can attach bounded editor polling and the WebView spectrum renderer to `readLatestSpectrumSnapshot` without changing analyzer ownership or the audio-thread handoff. Standalone capture, licensing, pluginval, and manual DAW smoke remain later phase/plan scope.

## Self-Check: PASSED

- Key files exist: mailbox header, processor header/source, new native tests, and SUMMARY were found on disk.
- Task commits exist: `b0281e5`, `9590965`, `f5f78b8`, `d1d84db`, and `978b7da`.
- Plan-level verification commands passed.

---
*Phase: 02-end-to-end-vst3-analyzer*
*Completed: 2026-06-23*
