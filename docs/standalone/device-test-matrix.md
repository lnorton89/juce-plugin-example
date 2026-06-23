# Standalone Device Test Matrix

Use this matrix to verify LumaScope standalone with various Windows audio input devices and render endpoints.

## Format

| # | Device Type | Device Name | Source Mode | Expected | Actual | Notes |
|---|-------------|-------------|-------------|----------|--------|-------|
|   |             |             |             |          |        |       |

## Scenarios to Cover

### Input Devices (CAP-01)

| # | Scenario | Steps | Expected Result |
|---|----------|-------|-----------------|
| 1 | Default microphone | Open standalone → Source mode "Input Device" → select default mic | Spectrum shows for the microphone |
| 2 | USB microphone/interface | Connect USB audio interface → open standalone → select it | Spectrum shows from USB input |
| 3 | No input device available | Open standalone with no microphone connected | Source strip shows "stopped" state; no active capture |
| 4 | Device hotplug | Connect microphone while standalone is running → source list should show it (on next source.list request) | Controller must not crash; next enumerate returns the new device |
| 5 | Device removal during capture | Disconnect the active input device while capturing | Controller transitions to error/endpoint_lost state; user can reselect |

### System Output / WASAPI Loopback (CAP-02)

| # | Scenario | Steps | Expected Result |
|---|----------|-------|-----------------|
| 6 | Default speakers | Source mode "System Output" → pick default render endpoint | Spectrum shows system audio output |
| 7 | Headphones vs speakers | Switch render output device at system level during capture | Controller detects change and transitions to endpoint_changed error |
| 8 | HDMI/Virtual cable endpoint | Select non-default render device (HDMI, VB-Cable, etc.) | Spectrum shows from that output |
| 9 | System output with no speakers | Disable all render devices | Device list returns empty; UI shows no sources available |
| 10 | Render endpoint reconnection | Re-plug HDMI/display while running | Controller recovers (or signals error); user can reselect |

### Source Persistence (CAP-05, D-13, D-14, D-15)

| # | Scenario | Steps | Expected Result |
|---|----------|-------|-----------------|
| 11 | Save and restore input device | Select Input Device → microphone → close standalone → reopen | Microphone automatically selected (D-13) |
| 12 | Save and restore system output | Select System Output → speakers → close standalone → reopen | Speakers automatically selected (D-13) |
| 13 | Saved source unavailable | Select microphone → close → disconnect mic → reopen | No auto-fallback; started state with no source |
| 14 | No saved preference (first run) | Fresh install → open standalone | Started stopped; user must choose source |

### Switching / Transitions (CAP-04)

| # | Scenario | Steps | Expected Result |
|---|----------|-------|-----------------|
| 15 | Switch from input to loopback | Capture mic → switch mode to System Output → select endpoint | Mic stops; loopback starts cleanly |
| 16 | Switch between inputs | Capture mic A → stop → select mic B | B starts; A was stopped |
| 17 | Rapid source switching | Rapidly select different sources in succession | Controller does not crash; last selection wins |
| 18 | Stop while no source selected | Click Stop while no source is active | No-op; state remains stopped |

### Error / Diagnostics

| # | Scenario | Steps | Expected Result |
|---|----------|-------|-----------------|
| 19 | Invalid source ID | Send source.select with empty/unknown ID via dev tools | Error state with "invalid_selection" or "device_init_failed" |
| 20 | Loopback permission denied | Run without audio loopback privilege (if Windows restricts it) | Error state; user can retry with different source |
| 21 | Audio device initialization fail | Run with device in use by exclusive-mode application | Error state visible in source strip |

## Device Inventory Template

List devices available on the test machine:

```
Machine: _______________
OS: Windows _____ Build _______
Date: _______________

Input Devices:
- _______________________________
- _______________________________

Render Endpoints:
- _______________________________
- _______________________________

Notes:
_______________
```
