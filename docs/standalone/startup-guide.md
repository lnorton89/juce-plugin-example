# Standalone Startup Guide

## Prerequisites

- Windows 10 20H1+ or Windows 11
- [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) (Evergreen)
- VST3 (optional — only if using the host)
- Build artifacts from a successful CMake build (see the repository [README](../../README.md))

## Quick Start

### Build the standalone target

```powershell
# Configure
cmake --preset vs2019-vite

# Build standalone
cmake --build --preset vs2019-vite --target LumaScope_Standalone --parallel 4
```

### Run

```powershell
.\build\vs2019-vite\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

### Run via launcher script

```powershell
.\scripts\start-lumascope-standalone.ps1 -Config Debug
```

## Source Selection

### Input Device Mode

1. After launch, the source strip shows "Input Device" mode by default.
2. Open the **Select source** dropdown to see available input devices (microphones, interfaces).
3. Click a device name to start capture.
4. The spectrum analyzer displays the input signal in real time.

### System Output (Loopback) Mode

1. Change the **Source mode** dropdown from "Input Device" to "System Output".
2. The **Select source** dropdown now shows render endpoints (speakers, headphones, HDMI).
3. Click an endpoint name to start loopback capture.
4. The spectrum analyzer displays the system-wide mix.

## Source Preference Persistence

LumaScope saves the last valid source selection (mode + ID + display name) to a JSON file at:

```
%APPDATA%\LumaScope\source-preference.json
```

On startup, the application restores the saved source if it is still available in the current device enumeration. If the source is no longer available, the application starts in the stopped (choose-source) state.

To clear the saved preference, delete the `source-preference.json` file while the application is closed.

Product name, artifact directory, standalone target name, AppData directory, and preference filename are sourced from `project-config.json` via `scripts/config.ps1`; see [configuration reference](../CONFIG-REFERENCE.md).

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| "No source state" shown | No source selected | Choose a source from the dropdown |
| "endpoint_lost" error | WASAPI endpoint removed | Check if the render device is still connected; reselect |
| "endpoint_changed" error | Default render device changed | Reselect the desired endpoint |
| "device_init_failed" | Input device unavailable or busy | Try a different device or wait for the active exclusive-mode app to release the device |
| "invalid_selection" | Empty or malformed source ID | Use the UI dropdown rather than manual input |
| Empty device list | No audio devices available | Check Windows Sound settings; ensure devices are enabled |
| No spectrum after source select | Audio silent or below threshold | The silence detector (20 consecutive quiet frames) may trigger; play audio to verify |
