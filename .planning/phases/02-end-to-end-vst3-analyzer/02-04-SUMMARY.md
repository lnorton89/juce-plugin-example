---
phase: 02-end-to-end-vst3-analyzer
plan: 04
subsystem: vst3-validation
tags: [juce, vst3, pluginval, ableton-live, validation, host-smoke]

requires:
  - phase: 02-03-web-spectrum-renderer
    provides: Typed spectrum.snapshot bridge, editor polling, and canvas renderer
provides:
  - pluginval discovery and validation wrapper with honest missing-tool behavior
  - full Phase 2 verification wiring through scripts/test-all.ps1 and scripts/verify-project.ps1
  - Ableton-preferred VST3 smoke checklist and troubleshooting guidance
  - recorded Ableton Live smoke evidence with approved retest after analyzer responsiveness repair
  - overlapping FFT hop and responsive Musical profile repair for host-visible lag/accuracy
affects: [phase-03-standalone-monitoring, phase-07-release-handoff, vst3-analyzer, validation-docs]

tech-stack:
  added: []
  patterns: [optional external-tool validation wrapper, honest manual smoke evidence, overlapping FFT display updates]

key-files:
  created: [scripts/validate-plugin.ps1, docs/vst3-smoke-test.md, .planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md]
  modified: [scripts/test-all.ps1, scripts/verify-project.ps1, docs/development.md, docs/troubleshooting.md, README.md, docs/analyzer-dsp.md, plugin/include/LumaScope/Analyzer/AnalyzerConfig.h, plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h, plugin/source/Analyzer/AnalyzerConfig.cpp, plugin/source/Analyzer/SpectrumAnalyzer.cpp, tests/native/EditorSnapshotPollingTests.cpp, tests/native/SpectrumAnalyzerTests.cpp]

key-decisions:
  - "Treat pluginval absence as an explicit unavailable/skipped validation state, never as a pass."
  - "Use Ableton Live as the preferred real-host smoke target and record fallback hosts only when Ableton cannot be used."
  - "Fix the failed Ableton smoke inside Phase 2 by making the existing Musical analyzer profile more responsive, not by adding v2 controls."

patterns-established:
  - "External validation tools are resolved from an explicit path, environment variable, or PATH, with the resolved path printed before use."
  - "Manual smoke evidence records pass, fail, skipped, and limitations separately so release claims stay auditable."
  - "The analyzer can keep a 4096-point Musical FFT while reducing display lag through overlapping 1024-sample hops."

requirements-completed: [DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-06, VST3-01, VST3-02, VST3-03, VST3-04, UI-02]

duration: 9h 28m
completed: 2026-06-23
---

# Phase 02 Plan 04: VST3 Validation and Host Smoke Summary

**pluginval-ready validation workflow plus approved Ableton Live VST3 smoke after analyzer responsiveness repair**

## Performance

- **Duration:** 9h 28m
- **Started:** 2026-06-23T00:07:16Z
- **Completed:** 2026-06-23T09:35:54Z
- **Tasks:** 3
- **Files modified:** 15

## Accomplishments

- Added `scripts/validate-plugin.ps1`, with pluginval lookup from `-PluginvalPath`, `PLUGINVAL_EXE`, or PATH, and clear missing-tool behavior.
- Wired `scripts/test-all.ps1` to build/test the analyzer path and attempt plugin validation without pretending missing pluginval passed.
- Added VST3 smoke documentation and a structured host evidence record for pluginval, Ableton, fallback hosts, lifecycle checks, limitations, and approval.
- Diagnosed and fixed the failed Ableton smoke by adding overlapping FFT hops, more responsive Musical smoothing/cadence, and narrow-bin interpolation.
- Recorded the approved Ableton retest after repair commit `5c95579`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add plugin validation and full-suite Phase 2 gates** - `307b66d` (feat)
2. **Task 2: Document lifecycle, pluginval, and DAW smoke matrix** - `9551c55` (docs)
3. **Checkpoint repair: Reduce analyzer lag in host smoke** - `5c95579` (fix)
4. **Task 3: Record approved Ableton smoke retest** - `06a41bd` (docs)

## Files Created/Modified

- `scripts/validate-plugin.ps1` - pluginval discovery/run wrapper with explicit unavailable state.
- `scripts/test-all.ps1` - full suite now invokes plugin validation in allow-missing mode.
- `scripts/verify-project.ps1` - verifies Phase 2 validation artifacts and documentation hooks.
- `docs/vst3-smoke-test.md` - Ableton-preferred smoke checklist and fallback rules.
- `docs/development.md`, `docs/troubleshooting.md`, `README.md` - validation, pluginval, and host smoke guidance.
- `.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md` - pluginval and Ableton smoke evidence.
- `plugin/include/LumaScope/Analyzer/AnalyzerConfig.h` and `plugin/source/Analyzer/AnalyzerConfig.cpp` - added hop size and responsive profile values.
- `plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h` and `plugin/source/Analyzer/SpectrumAnalyzer.cpp` - overlapping FFT hops and narrow-bin interpolation.
- `tests/native/SpectrumAnalyzerTests.cpp` and `tests/native/EditorSnapshotPollingTests.cpp` - coverage for overlapping hop publication and updated bounded cadence.
- `docs/analyzer-dsp.md` - documented hop sizes, interpolation, and responsive smoothing.

## Decisions Made

- pluginval remains preferred for automated VST3 validation, but local absence is recorded as unavailable/not passed.
- Ableton Live was used as the real-host smoke target. The first smoke failed due to lag/accuracy, and the repaired retest passed.
- The fix stayed inside existing analyzer defaults and validation scope: no standalone capture, licensing, or v2 user controls were added.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed laggy/inaccurate host-visible analyzer response**
- **Found during:** Task 3 (Human DAW smoke checkpoint)
- **Issue:** User-tested Ableton smoke showed the VST3 loaded, but the LumaScope spectrum was visibly laggy and materially less accurate than Ableton Spectrum.
- **Fix:** Added 1024-sample overlapping FFT hops for the 4096-point Musical profile, increased Musical snapshot cadence and smoothing responsiveness, interpolated very narrow log bins to avoid low-frequency stair-step plateaus, updated DSP docs, and added native regression coverage.
- **Files modified:** `plugin/include/LumaScope/Analyzer/AnalyzerConfig.h`, `plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h`, `plugin/source/Analyzer/AnalyzerConfig.cpp`, `plugin/source/Analyzer/SpectrumAnalyzer.cpp`, `tests/native/SpectrumAnalyzerTests.cpp`, `tests/native/EditorSnapshotPollingTests.cpp`, `docs/analyzer-dsp.md`, `.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md`
- **Verification:** Native/UI/full suite passed; Ableton retest approved by user.
- **Committed in:** `5c95579`

---

**Total deviations:** 1 auto-fixed (1 bug).  
**Impact on plan:** The fix was required to satisfy VST3-01/VST3-04 host smoke honestly. It did not add out-of-scope standalone capture, licensing, or v2 controls.

## Issues Encountered

- pluginval is not installed/configured locally. `scripts/validate-plugin.ps1` fails clearly without `-AllowMissing`, and `scripts/test-all.ps1` reports pluginval as skipped, not passed.
- The first Ableton smoke failed on analyzer responsiveness/accuracy. The repair in `5c95579` passed the human retest.
- Build output still prints the known non-fatal `pwsh.exe` missing message on this Windows PowerShell 5.1 machine.

## Known Stubs

None blocking. `02-HOST-SMOKE.md` still contains `not recorded` for Ableton version, unused fallback-host fields, and expanded lifecycle rows; those are honest evidence limitations after an Ableton pass, not product stubs.

## Threat Flags

None beyond the planned trust boundaries. This plan touched local script execution and manual evidence recording as specified in T-02-09 through T-02-11; no network endpoints, auth paths, persistent secrets, database schema, or new external-service credentials were introduced.

## Verification Evidence

- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` - passed after Task 1, before the initial checkpoint, after repair `5c95579`, and after Task 3 evidence update.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` - passed through `scripts/test-all.ps1`.
- `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` - passed after analyzer repair.
- `npm.cmd --prefix ui run test:run; npm.cmd --prefix ui run build; npm.cmd --prefix ui run check:bundle` - passed after analyzer repair.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1` - failed clearly because pluginval is unavailable; not counted as passed.
- Human Ableton Live smoke - initially failed at `9551c55`, then passed after repair commit `5c95579`.

## User Setup Required

pluginval remains optional for local development but required for a true automated VST3 validation pass. Provide a trusted pluginval executable with `-PluginvalPath` or `PLUGINVAL_EXE` and rerun `scripts/validate-plugin.ps1` when available.

## Next Phase Readiness

Phase 2 is complete from the VST3 analyzer perspective: DSP, realtime handoff, WebView rendering, validation scripts, and Ableton smoke evidence are in place. Phase 3 can add standalone input and WASAPI loopback capture without changing the VST3 analyzer proof path.

## Self-Check: PASSED

- Key files exist: summary, pluginval wrapper, VST3 smoke docs, host smoke evidence, analyzer repair source, and native tests were found on disk.
- Task and repair commits exist: `307b66d`, `9551c55`, `5c95579`, and `06a41bd`.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1` passed after summary creation.
