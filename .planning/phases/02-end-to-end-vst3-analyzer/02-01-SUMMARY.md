---
phase: 02-end-to-end-vst3-analyzer
plan: 01
subsystem: native-dsp
tags: [juce, juce-dsp, fft, spectrum-analyzer, ctest]

requires:
  - phase: 01-reproducible-product-shell
    provides: Pinned JUCE/CMake native target and CTest harness
provides:
  - AnalyzerProfile and AnalyzerConfig contracts for Musical, Measurement, and Fast
  - Bounded SpectrumSnapshot model for later real-time handoff and WebView bridge work
  - JUCE FFT/Hann/log-bin/smoothing analyzer core with deterministic native tests
  - DSP contract documentation for normalization, tolerances, and extension points
affects: [02-02-realtime-handoff, 02-03-web-spectrum-renderer, vst3-analyzer]

tech-stack:
  added: [juce_dsp]
  patterns: [processor-independent analyzer core, bounded snapshot model, display-ready logarithmic bins]

key-files:
  created: [plugin/include/LumaScope/Analyzer/AnalyzerConfig.h, plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h, plugin/include/LumaScope/Analyzer/SpectrumSnapshot.h, plugin/source/Analyzer/AnalyzerConfig.cpp, plugin/source/Analyzer/SpectrumAnalyzer.cpp, tests/native/SpectrumAnalyzerTests.cpp, docs/analyzer-dsp.md]
  modified: [plugin/CMakeLists.txt, tests/native/CMakeLists.txt, tests/native/WebResourcesTests.cpp]

key-decisions:
  - "Keep Musical as the default profile with 4096 FFT, Hann window, 20 Hz to 20 kHz range, 30 Hz snapshot cadence, and moderate smoothing."
  - "Represent Measurement and Fast as real native profile configs now, while leaving user-facing analyzer controls to v2."
  - "Publish display-ready logarithmic snapshot bins from the DSP core rather than exposing raw FFT bins to later UI code."

patterns-established:
  - "Analyzer processing is independent of processor/editor lifetime and can be connected to the real-time handoff in Plan 02-02."
  - "Native DSP tests own the documented frequency placement and +/-1.5 dB display tolerance."

requirements-completed: [DSP-01, DSP-02, DSP-03, DSP-04, DSP-06]

duration: 9 min
completed: 2026-06-23
---

# Phase 02 Plan 01: Native Spectrum Analyzer Core Summary

**JUCE FFT/Hann spectrum core with Musical, Measurement, and Fast profiles plus bounded logarithmic snapshots**

## Performance

- **Duration:** 9 min
- **Started:** 2026-06-23T03:06:16Z
- **Completed:** 2026-06-23T03:15:13Z
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- Added native analyzer profile/config contracts with Musical as the default and real Measurement/Fast alternatives.
- Implemented JUCE `dsp::FFT` plus Hann windowing, single-sided dB normalization, logarithmic display-bin mapping, and smoothing/decay.
- Added deterministic CTest coverage for silence, denormals, mono/stereo tones, sample rates, variable block sizes, sample-rate reset, and smoothing decay.
- Documented the DSP formulas, tolerances, defaults, and phase boundaries in `docs/analyzer-dsp.md`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Define analyzer profiles and snapshot contracts** - `de6c23f` (test RED), `9952b61` (feat GREEN)
2. **Task 2: Implement FFT, normalization, log mapping, smoothing, and edge handling** - `13b3237` (test RED), `38782d6` (feat GREEN)
3. **Task 3: Document DSP contracts and normalization** - `3c09bc2` (docs)

## Files Created/Modified

- `plugin/include/LumaScope/Analyzer/AnalyzerConfig.h` - Profile/config contract and validation entry points.
- `plugin/include/LumaScope/Analyzer/SpectrumSnapshot.h` - Fixed-capacity snapshot/bin model.
- `plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h` - Processor-independent analyzer API.
- `plugin/source/Analyzer/AnalyzerConfig.cpp` - Musical, Measurement, and Fast profile values plus range validation.
- `plugin/source/Analyzer/SpectrumAnalyzer.cpp` - JUCE FFT/window/log-bin/smoothing implementation.
- `tests/native/SpectrumAnalyzerTests.cpp` - Native coverage for DSP-01 through DSP-04 and DSP-06.
- `tests/native/CMakeLists.txt` - Analyzer sources and `juce_dsp` linkage in the native test target.
- `plugin/CMakeLists.txt` - Analyzer sources and `juce::juce_dsp` linkage in the product target.
- `docs/analyzer-dsp.md` - DSP contract, formula, tolerances, and deferred scope.

## Decisions Made

- Used 160 display-ready logarithmic bins with a fixed 256-bin snapshot capacity so later bridge work has a bounded payload.
- Treated the first completed FFT frame as unsmoothed, then applied one-pole attack/release smoothing to subsequent frames. This keeps tone normalization deterministic while preserving display stability.
- Kept all dynamic storage in construction/prepare paths; the current `pushAudioBlock` path reuses preallocated buffers and avoids JSON, WebView, file, and network work.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the native test helper to use the JUCE AudioBuffer API**
- **Found during:** Task 2
- **Issue:** The new RED test helper used `AudioBuffer::fill`, which is not available in the pinned JUCE API surface used by this project.
- **Fix:** Replaced it with explicit `setSample` writes in the helper.
- **Files modified:** `tests/native/SpectrumAnalyzerTests.cpp`
- **Verification:** `cmake --preset vs2019-debug; cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure`
- **Committed in:** `38782d6`

---

**Total deviations:** 1 auto-fixed (1 bug).  
**Impact on plan:** The fix was limited to the new test helper and did not change scope.

## Issues Encountered

- Build output still prints the known non-fatal `pwsh.exe` missing line from the JUCE/CMake helper path on this Windows PowerShell 5.1 machine. Native build and CTest exit successfully.

## Known Stubs

None. Empty numeric/default initializers in analyzer structs are intentional defaults, not UI-flowing placeholder data.

## Threat Flags

None. This plan added native DSP and bounded in-memory snapshot structures only; it did not introduce network endpoints, auth paths, file access, schema changes, or new trust-boundary persistence.

## Verification Evidence

- `cmake --preset vs2019-debug` - passed.
- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4` - passed.
- `ctest --preset vs2019-debug --output-on-failure` - 1/1 native test passed.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1` - passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 02-02 can attach the analyzer to the real-time ingress and latest-snapshot handoff without changing the DSP math or snapshot shape. Standalone capture, licensing, WebView rendering, plugin validation, and v2 controls remain outside this plan.

## Self-Check: PASSED

- Key files exist: analyzer headers/sources, native tests, and DSP documentation were found on disk.
- Task commits exist: `de6c23f`, `9952b61`, `13b3237`, `38782d6`, and `3c09bc2`.
- Plan-level verification commands passed.

---
*Phase: 02-end-to-end-vst3-analyzer*
*Completed: 2026-06-23*
