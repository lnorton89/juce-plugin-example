# LumaScope

LumaScope is a Windows JUCE 8 example that builds a VST3 and standalone application with the same embedded React/TypeScript/Material UI interface. Phase 1 contains the product shell and bridge handshake only; analyzer DSP, audio-device capture, and licensing arrive in later phases.

## Quick start

From a fresh PowerShell terminal at the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-environment.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap.ps1 -Preset vs2019-debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1
```

The bootstrap checks prerequisites, runs `npm ci`, configures CMake, and builds Standalone, VST3, and native tests. It does not install system software. Pinned dependencies are cached in ignored `.deps/` and `ui/node_modules/` directories.

Launch the authoritative embedded Debug application:

```powershell
& .\build\vs2019-debug\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

For Vite hot reload, start the fixed loopback server in one terminal:

```powershell
npm.cmd --prefix ui run dev -- --host 127.0.0.1 --port 5174 --strictPort
```

Then configure, build, and launch the explicit development preset in another terminal:

```powershell
cmake --preset vs2019-vite
cmake --build --preset vs2019-vite --target LumaScope_Standalone --parallel 4
& .\build\vs2019-vite\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

See [development](docs/development.md), [bridge protocol](docs/bridge-protocol.md), and [troubleshooting](docs/troubleshooting.md) for complete commands and diagnostics.
