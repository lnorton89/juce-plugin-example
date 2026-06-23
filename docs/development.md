# Development workflow

## Prerequisites and caches

LumaScope's verified baseline is 64-bit Windows 10, Visual Studio 2019 16.11 with Desktop development with C++, CMake 3.22+, Node/npm, Ninja, and the Evergreen WebView2 Runtime. Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-environment.ps1 -Json
```

`scripts/bootstrap.ps1` never installs system software. It uses `npm ci`, downloads exact JUCE 8.0.14 and WebView2 SDK 1.0.4022.49 dependencies when absent, and stores them under ignored `.deps/` and `ui/node_modules/`. If VS2019 compatibility fails, stop and diagnose it; do not silently switch compilers or JUCE versions.

## Embedded mode

Embedded assets are authoritative for normal Debug and every Release build:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap.ps1 -Preset vs2019-debug
& .\build\vs2019-debug\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

Expected footer: `Embedded UI · Bridge ready`.

## Vite mode

Vite is allowed only at `http://127.0.0.1:5174`. Hostnames, remote addresses, alternate ports, credentials, paths, queries, fragments, HTTPS, and malformed URLs are rejected. Terminal 1:

```powershell
npm.cmd --prefix ui run dev -- --host 127.0.0.1 --port 5174 --strictPort
```

Terminal 2:

```powershell
cmake --preset vs2019-vite
cmake --build --preset vs2019-vite --target LumaScope_Standalone --parallel 4
& .\build\vs2019-vite\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

Expected footer: `Vite development · Bridge ready`. Edit `ui/src/components/AnalyzerStage.tsx` while both processes run to verify hot reload. Stop Vite with Ctrl+C. Return to embedded assets without source edits by rebuilding/running `vs2019-debug`.

## Verification

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-web-modes.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest
```

`scripts/test-all.ps1` builds the native targets, runs native and UI tests, checks packaged WebView modes, and invokes `scripts/validate-plugin.ps1 -AllowMissing`. If pluginval is not installed, the full suite reports automated VST3 validation as skipped rather than passed. For the release-quality Phase 2 VST3 gate, run pluginval without the allow-missing flag:

```powershell
cmake --build --preset vs2019-debug --target LumaScope_VST3 --parallel 4
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1
```

If pluginval is not on `PATH`, pass `-PluginvalPath C:\path\to\pluginval.exe` or set `PLUGINVAL_EXE` to the absolute executable path. The wrapper prints the resolved executable and VST3 artifact path before validation so the recorded result is auditable.

Codex uses exactly one Context7 MCP server from `.codex/config.toml`. Use `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`; an optional Context7 API key belongs only in the user's environment.
