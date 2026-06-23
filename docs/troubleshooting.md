# Troubleshooting

## Environment check fails

- Visual Studio missing: install Visual Studio 2019 with Desktop development with C++. Do not substitute a toolchain silently.
- VS developer environment inactive: Visual Studio generator presets still work; activate `VsDevCmd.bat` only when using Ninja with MSVC command-line tools.
- Ninja missing: install Ninja or use `vs2019-*` presets.
- WebView2 SDK missing: CMake downloads pinned SDK 1.0.4022.49 into `.deps/webview2`; inspect network/TLS errors if this fails.
- WebView2 Runtime missing: install or repair Microsoft Edge WebView2 Evergreen Runtime, then reopen LumaScope.

## Vite server unavailable

The development build connects only to `http://127.0.0.1:5174`. Start it with:

```powershell
npm.cmd --prefix ui run dev -- --host 127.0.0.1 --port 5174 --strictPort
```

If port 5174 is occupied, stop the owning process only if it belongs to you; the smoke script will not terminate unrelated services. Rebuild `vs2019-debug` to use embedded assets. Release binaries never contain a development-server URL.

## Packaged assets or handshake fail

Rebuild the matching native target so the deterministic UI archive is regenerated. `host.info.buildMarker` and `uiSource` identify the loaded build. A protocol mismatch means native and UI came from different contracts; rebuild both from one checkout.

## pluginval unavailable or failing

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1
```

If the script reports that pluginval is missing, install or provide a trusted local pluginval executable outside Git, then rerun with `-PluginvalPath C:\path\to\pluginval.exe` or set `PLUGINVAL_EXE`. `scripts/test-all.ps1` uses `-AllowMissing` so day-to-day local verification can continue, but the output means automated plugin validation was skipped, not passed.

If pluginval cannot find the VST3, build it first:

```powershell
cmake --build --preset vs2019-debug --target LumaScope_VST3 --parallel 4
```

If validation fails, keep the command output and record the failure in `.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md`; do not replace it with a DAW smoke pass.

## DAW smoke issues

Ableton Live is the preferred Phase 2 host. If Ableton cannot scan the Debug artifact directly, copy the built `LumaScope.vst3` into a user VST3 folder that Ableton scans, rescan plug-ins, then insert it as an audio effect on a track with audible audio.

If Ableton is unavailable, use the best available Windows VST3 host and record the fallback host, version, reason, and limitations. A fallback result must not be described as an Ableton pass.

## WASAPI loopback output — no signal or error

The standalone application can monitor Windows system output through WASAPI loopback. If System Output mode is silent, check:

- **No audio playing**: The loopback stream captures all output on the selected render endpoint. If nothing is producing audio, the analyzer shows `silent` — this is a valid state, not a failure.
- **Endpoint removed or disabled**: If the selected output device is unplugged, disabled, or changes state, capture stops and the UI surfaces an error. Re-select the endpoint once it is available again.
- **Exclusive-mode application**: Some applications take exclusive control of the audio device, blocking loopback capture. Close the conflicting application or select a different render endpoint.

WASAPI loopback does not require a vendor "Stereo Mix" recording device. If no render endpoints appear in the System Output list, ensure one or more Windows audio output devices are enabled in the Sound control panel.

## Native fallback check

To inspect the last-resort panel without changing source, build Debug and launch with the documented diagnostic hook:

```powershell
$env:LUMASCOPE_SIMULATE_WEB_FAILURE = 'webview2'
& .\build\vs2019-debug\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
Remove-Item Env:LUMASCOPE_SIMULATE_WEB_FAILURE
```

The panel must explain WebView2 recovery rather than show a blank editor. Automated smoke also accepts `resource` and `handshake` to verify packaged-resource and protocol-timeout fallbacks. These hooks compile into Debug behavior only through the Debug smoke path and do not introduce remote navigation.
