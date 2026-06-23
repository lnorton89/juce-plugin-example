# Phase 2 Host Smoke Evidence

This file records Phase 2 pluginval and DAW smoke evidence. Leave fields as `not recorded` until the command or human host check has actually been run.

## Environment

| Field | Value |
|-------|-------|
| Date/time | 2026-06-23 |
| Windows version | Microsoft Windows 10 Pro 10.0.19045 |
| Build preset | vs2019-debug |
| VST3 artifact | `build\vs2019-debug\plugin\LumaScope_artefacts\Debug\VST3\LumaScope.vst3` |
| Commit tested | `9551c55` |

## Automated Validation

| Check | Status | Evidence |
|-------|--------|----------|
| `scripts/test-all.ps1` | not recorded | not recorded |
| `scripts/validate-plugin.ps1` | failed - pluginval unavailable | User ran `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1`; script reported `pluginval executable not found. Provide -PluginvalPath, set PLUGINVAL_EXE, or add pluginval to PATH.` |
| pluginval executable | skipped - pluginval unavailable | No `pluginval.exe` was available on PATH and no `-PluginvalPath`/`PLUGINVAL_EXE` was provided. This is recorded as unavailable, not passed. |

## Ableton Live Smoke

| Check | Status | Evidence |
|-------|--------|----------|
| Ableton availability | available | User provided Ableton Live screenshot with LumaScope loaded alongside Ableton Spectrum. |
| Ableton version | not recorded | Screenshot confirms Ableton Live was used; exact version not recorded. |
| VST3 loaded as audio effect | passed | User reported "vst works" and provided screenshot showing LumaScope loaded in Ableton. |
| Routed audio unchanged | not recorded | not recorded |
| Smooth live spectrum visible | failed | User reported the LumaScope VST is "laggy and not very accurate compared to abletons"; screenshot shows LumaScope spectrum diverging from Ableton Spectrum. |
| Editor close/reopen while audio continues | not recorded | not recorded |
| Editor resize remains visually stable | not recorded | not recorded |
| Plug-in remove/reinsert remains stable | not recorded | not recorded |

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
| Overall status | failed - needs analyzer accuracy/performance fix |
| Approved by | not approved |
| Limitations | Ableton smoke revealed LumaScope loads, but rendered spectrum is laggy and materially less accurate than Ableton Spectrum reference. |
