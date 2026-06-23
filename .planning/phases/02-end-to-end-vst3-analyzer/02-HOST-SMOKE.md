# Phase 2 Host Smoke Evidence

This file records Phase 2 pluginval and DAW smoke evidence. Leave fields as `not recorded` until the command or human host check has actually been run.

## Environment

| Field | Value |
|-------|-------|
| Date/time | 2026-06-23 |
| Windows version | Microsoft Windows 10 Pro 10.0.19045 |
| Build preset | vs2019-debug |
| VST3 artifact | `build\vs2019-debug\plugin\LumaScope_artefacts\Debug\VST3\LumaScope.vst3` |
| Commit tested | `5c95579` |

## Automated Validation

| Check | Status | Evidence |
|-------|--------|----------|
| `scripts/test-all.ps1` | passed | Ran after repair commit `5c95579`; UI tests, native tests, VST3 build, WebView smoke checks, and verifier self-test passed. |
| `scripts/validate-plugin.ps1` | failed - pluginval unavailable | User ran `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1`; script reported `pluginval executable not found. Provide -PluginvalPath, set PLUGINVAL_EXE, or add pluginval to PATH.` |
| pluginval executable | skipped - pluginval unavailable | No `pluginval.exe` was available on PATH and no `-PluginvalPath`/`PLUGINVAL_EXE` was provided. This is recorded as unavailable, not passed. |

## Ableton Live Smoke

| Check | Status | Evidence |
|-------|--------|----------|
| Ableton availability | available | User provided Ableton Live screenshot with LumaScope loaded alongside Ableton Spectrum. |
| Ableton version | not recorded | Screenshot confirms Ableton Live was used; exact version not recorded. |
| VST3 loaded as audio effect | passed | User reported "vst works" and provided screenshot showing LumaScope loaded in Ableton. |
| Routed audio unchanged | passed | User approved the human retest after repair commit `5c95579`; no passthrough problem was reported. |
| Smooth live spectrum visible | passed after repair | Initial smoke at `9551c55` failed because LumaScope was laggy/inaccurate versus Ableton Spectrum. Repair commit `5c95579` added overlapping FFT hops, more responsive Musical smoothing/cadence, and narrow-bin interpolation. User approved the Ableton retest after that repair. |
| Editor close/reopen while audio continues | passed | User approved the repaired Ableton smoke checkpoint after the lifecycle checklist was re-presented. |
| Editor resize remains visually stable | passed | User approved the repaired Ableton smoke checkpoint after the lifecycle checklist was re-presented. |
| Plug-in remove/reinsert remains stable | passed | User approved the repaired Ableton smoke checkpoint after the lifecycle checklist was re-presented. |

## Fallback Host Smoke

Use this section only if Ableton Live is unavailable or cannot complete the smoke.

| Check | Status | Evidence |
|-------|--------|----------|
| Fallback host name/version | not recorded | not recorded |
| Reason Ableton was not used | not recorded | not recorded |
| VST3 loaded as audio effect | not recorded | not recorded |
| Routed audio unchanged | not recorded | not recorded |
| Smooth live spectrum visible | not recorded | not recorded |
| Editor lifecycle stable | not recorded | not recorded |
| Limitations versus Ableton | not recorded | not recorded |

## Lifecycle Observations

| Scenario | Status | Notes |
|----------|--------|-------|
| Editor closed during playback | not recorded | not recorded |
| Editor reopened during playback | not recorded | not recorded |
| Editor resized during playback | not recorded | not recorded |
| Plug-in destroyed and recreated | not recorded | not recorded |

## Result

| Field | Value |
|-------|-------|
| Overall status | passed after repair commit `5c95579` |
| Approved by | User human retest approval |
| Limitations | pluginval remains unavailable and is not counted as passed. Ableton version was not recorded. |
