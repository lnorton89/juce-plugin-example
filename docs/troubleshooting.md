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

## Native fallback check

To inspect the last-resort panel without changing source, build Debug and launch with the documented diagnostic hook:

```powershell
$env:LUMASCOPE_SIMULATE_WEB_FAILURE = 'webview2'
& .\build\vs2019-debug\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
Remove-Item Env:LUMASCOPE_SIMULATE_WEB_FAILURE
```

The panel must explain WebView2 recovery rather than show a blank editor. Automated smoke also accepts `resource` and `handshake` to verify packaged-resource and protocol-timeout fallbacks. These hooks compile into Debug behavior only through the Debug smoke path and do not introduce remote navigation.
