---
phase: 2
slug: end-to-end-vst3-analyzer
status: planned
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-23
---

# Phase 2 - Validation Strategy

> Per-phase validation contract for Nyquist feedback sampling during execution.

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest/native executable for DSP, processor, mailbox, editor polling, and VST3 passthrough; Vitest + React Testing Library for bridge parsing and canvas renderer |
| **Config files** | `tests/native/CMakeLists.txt`, `CMakePresets.json`, `ui/package.json`, `ui/vitest.config.ts` |
| **Quick run command** | `ctest --preset vs2019-debug --output-on-failure` for native changes; `npm.cmd --prefix ui run test:run` for UI changes |
| **Full suite command** | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` |
| **Estimated runtime** | Quick checks target under 60 seconds after configure/build cache is warm; full suite may take longer because it builds VST3 and attempts plugin validation |

## Nyquist Sampling Strategy

- **After every task commit:** Run the task's `<verify><automated>` command before moving to the next task.
- **After every wave:** Run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` once the wave's artifacts exist.
- **Before `$gsd-verify-work`:** Full suite green, pluginval attempted or recorded unavailable, and `02-HOST-SMOKE.md` contains Ableton-preferred or fallback DAW evidence.
- **Max feedback gap:** No behavior-producing Phase 2 task may proceed without a concrete automated check. Manual DAW smoke is additional evidence, not a replacement for automated checks.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirements | Threat Ref | Secure/Correct Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|--------------|------------|-------------------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | DSP-01, DSP-03, DSP-06 | T-02-01, T-02-02 | Profiles and snapshot contracts are bounded and deterministic | native unit | `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 1 creates `tests/native/SpectrumAnalyzerTests.cpp` | pending |
| 02-01-02 | 01 | 1 | DSP-01, DSP-02, DSP-03, DSP-04 | T-02-01, T-02-02 | FFT/window/log-bin output is finite, normalized, and stable across host rates/blocks | native unit | `cmake --preset vs2019-debug; cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 1 creates `tests/native/SpectrumAnalyzerTests.cpp` | pending |
| 02-01-03 | 01 | 1 | DSP-02, DSP-03, DSP-06 | - | DSP formulas, tolerances, and extension points are documented | docs/source | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1` | Missing - Wave 1 creates `docs/analyzer-dsp.md` | pending |
| 02-02-01 | 02 | 2 | DSP-05, DSP-06, VST3-03 | T-02-03, T-02-04 | Latest-snapshot handoff drops stale frames and uses bounded storage | native unit | `cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 2 creates `tests/native/RealtimeHandoffTests.cpp` | pending |
| 02-02-02 | 02 | 2 | DSP-01, DSP-04, DSP-05, VST3-02, VST3-03 | T-02-03, T-02-05 | `processBlock` observes audio without mutating samples or depending on editor lifetime | native unit/build | `cmake --preset vs2019-debug; cmake --build --preset vs2019-debug --target LumaScope_VST3 LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 2 creates `tests/native/PluginProcessorTests.cpp` | pending |
| 02-02-03 | 02 | 2 | DSP-05 | T-02-03 | Regression checks guard against prohibited audio-thread work | native unit/source | `ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 2 creates native regression tests | pending |
| 02-03-01 | 03 | 3 | DSP-03, DSP-05, VST3-01, UI-02 | T-02-06, T-02-07 | `spectrum.snapshot` schema is closed, bounded, and validated on native/web sides | native + Vitest | `npm.cmd --prefix ui run test:run; cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 3 creates bridge fixture/tests | pending |
| 02-03-02 | 03 | 3 | DSP-03, DSP-05, VST3-03 | T-02-06, T-02-08 | Editor polling emits newest snapshots at bounded cadence and survives close/reopen | native unit/build | `cmake --preset vs2019-debug; cmake --build --preset vs2019-debug --target LumaScope_Standalone LumaScope_VST3 LumaScopeNativeTests --parallel 4; ctest --preset vs2019-debug --output-on-failure` | Missing - Wave 3 creates `tests/native/EditorSnapshotPollingTests.cpp` | pending |
| 02-03-03 | 03 | 3 | UI-02 | T-02-07 | Renderer uses one bounded canvas and no per-bin DOM/MUI elements | Vitest/build | `npm.cmd --prefix ui run test:run; npm.cmd --prefix ui run build; npm.cmd --prefix ui run check:bundle` | Missing - Wave 3 creates `ui/src/components/SpectrumCanvas.test.tsx` | pending |
| 02-04-01 | 04 | 4 | VST3-01, VST3-04 | T-02-09, T-02-10 | pluginval is run when available and missing tools are recorded honestly | script/build | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1; powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` | Missing - Wave 4 creates `scripts/validate-plugin.ps1` | pending |
| 02-04-02 | 04 | 4 | DSP-01..DSP-06, VST3-01..VST3-04, UI-02 | T-02-10, T-02-11 | Host/lifecycle proof path records pass/fail/limitations without fabricating results | docs/source | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1` | Missing - Wave 4 creates `02-HOST-SMOKE.md` | pending |
| 02-04-03 | 04 | 4 | VST3-01, VST3-03, VST3-04 | T-02-10, T-02-11 | Human DAW smoke records Ableton-preferred or fallback host evidence | full suite + human | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` | Missing - Wave 4 creates and updates `02-HOST-SMOKE.md` | pending |

## Requirement Coverage

| Requirement | Primary Proof | Secondary Proof |
|-------------|---------------|-----------------|
| DSP-01 | `tests/native/SpectrumAnalyzerTests.cpp`, `tests/native/PluginProcessorTests.cpp` | Host smoke passthrough observation |
| DSP-02 | Deterministic tone tests with documented frequency-bucket and +/-1.5 dB display tolerance | `docs/analyzer-dsp.md` normalization formula |
| DSP-03 | Profile/log-bin/smoothing tests and snapshot fixture | Canvas renderer fixture |
| DSP-04 | Silence, denormal, zero/changing/large block, sample-rate reset tests | pluginval lifecycle run where available |
| DSP-05 | Realtime handoff and processBlock regression tests | Plan threat models and source checks |
| DSP-06 | Analyzer config/profile tests | DSP documentation extension point section |
| VST3-01 | Built VST3 plus pluginval/manual host smoke | `02-HOST-SMOKE.md` |
| VST3-02 | Processor passthrough tests | Manual DAW output unchanged observation |
| VST3-03 | Editor polling close/reopen tests | DAW lifecycle smoke |
| VST3-04 | `scripts/validate-plugin.ps1` and DAW smoke evidence | Explicit limitation record if pluginval or Ableton unavailable |
| UI-02 | Vitest renderer tests and UI build | Manual WebView smooth spectrum smoke |

## Wave 0 Requirements

- [ ] `tests/native/SpectrumAnalyzerTests.cpp` - covers DSP-01 through DSP-04 and DSP-06.
- [ ] `tests/native/RealtimeHandoffTests.cpp` - covers latest-snapshot-wins mailbox behavior.
- [ ] `tests/native/PluginProcessorTests.cpp` - covers passthrough, processBlock edge cases, and editor-closed analyzer operation.
- [ ] `tests/native/EditorSnapshotPollingTests.cpp` - covers bounded cadence, newest-only emission, and editor close/reopen behavior.
- [ ] `tests/fixtures/bridge/spectrum-snapshot-v1.json` - covers protocol extension fixture data.
- [ ] `ui/src/bridge/SpectrumSnapshot.test.ts` - covers snapshot parser validation.
- [ ] `ui/src/components/SpectrumCanvas.test.tsx` - covers bounded renderer mount and no per-bin DOM.
- [ ] `scripts/validate-plugin.ps1` - pluginval runner with explicit missing-tool behavior.

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ableton-preferred DAW smoke | VST3-01, VST3-03, VST3-04 | Requires a licensed/usable local DAW and real host UI | Load the built VST3 in Ableton Live if available, route audible audio, verify unchanged output and smooth spectrum, close/reopen/resize editor, and record exact result in `02-HOST-SMOKE.md` |
| Fallback host smoke | VST3-01, VST3-04 | Only needed when Ableton is unavailable or cannot be verified | Use the best available Windows VST3 host, record host name/version and limitations, and do not claim Ableton passed |
| WebView visual polish | UI-02 | Canvas appearance and smoothness need human inspection | Verify the filled spectral curve fits the existing dark LumaScope stage, does not overlap text, and remains smooth at bounded cadence |

## Validation Sign-Off

- [x] All anticipated tasks have automated verification or explicit Wave 0 test-artifact creation.
- [x] Sampling continuity: no behavior-producing task relies solely on manual verification.
- [x] Wave 0 covers all missing test infrastructure named by research and plans.
- [x] pluginval and Ableton availability are treated as recorded evidence paths, not assumed passes.
- [x] `nyquist_compliant: true` set in frontmatter.
