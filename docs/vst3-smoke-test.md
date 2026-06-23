# VST3 smoke test

This checklist is the manual Phase 2 host proof for the LumaScope VST3 analyzer. Ableton Live is the preferred host. If Ableton is not available or cannot complete the check, use the best available Windows VST3 host and record that fallback honestly in `.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md`.

Do not mark Ableton as passed unless Ableton Live actually loaded the built VST3 and completed the smoke steps below.

## Automated setup

From the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1
cmake --build --preset vs2019-debug --target LumaScope_VST3 --parallel 4
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1
```

If pluginval is not on `PATH`, pass `-PluginvalPath C:\path\to\pluginval.exe` or set `PLUGINVAL_EXE` to the absolute executable path. If pluginval is unavailable, record `skipped - pluginval unavailable` in the evidence file. That is an explicit limitation, not a pass.

The default Debug VST3 artifact is:

```text
build\vs2019-debug\plugin\LumaScope_artefacts\Debug\VST3\LumaScope.vst3
```

## Ableton-preferred smoke

1. Open Ableton Live.
2. Make sure Ableton scans the Debug VST3 location above, or copy/install the built `LumaScope.vst3` into a user VST3 folder that Ableton scans.
3. Create or open a set with audible audio on an audio or instrument track.
4. Insert LumaScope as an audio effect on that track.
5. Verify audio remains unchanged by bypassing/unbypassing the plug-in and listening for no level, latency, channel, or tone change attributable to LumaScope.
6. Open the LumaScope editor and verify the embedded UI reaches `Bridge ready`.
7. Play audio and verify a smooth filled logarithmic spectrum appears in the analyzer stage.
8. Close and reopen the editor while playback continues. The spectrum should resume without restarting playback.
9. Resize the editor and verify the canvas remains inside the analyzer stage with no overlapping UI text.
10. Remove and reinsert the plug-in, then verify it loads and renders again.

## Fallback host smoke

Use this only when Ableton Live is unavailable or cannot be verified. Pick the best available Windows VST3 host and record:

- Host name and version.
- Why Ableton was unavailable or not used.
- Whether the VST3 loaded.
- Whether routed audio stayed unchanged.
- Whether the spectrum rendered from real routed audio.
- Whether editor close, reopen, resize, and destroy/recreate stayed stable.
- Any limitation that prevents treating the fallback as equivalent to Ableton.

## Evidence record

Update `.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md` immediately after the smoke. Required statuses are `pass`, `fail`, `skipped`, or `blocked`; free-form notes should include exact commands, host name/version, observed behavior, and timestamp.
