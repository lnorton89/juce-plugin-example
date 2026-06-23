# Phase 03: Standalone Windows Monitoring - Research

**Researched:** 2026-06-23  
**Domain:** Windows standalone audio capture, JUCE device management, WASAPI loopback, native/web source-state protocol  
**Confidence:** HIGH for JUCE/input/protocol boundaries, HIGH for WASAPI loopback fundamentals, MEDIUM for exact recovery UX until tested on real devices.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Source Model and Picker
- **D-01:** Standalone source selection uses two clear modes: `Input Device` and `System Output`.
- **D-02:** `Input Device` represents ordinary JUCE-managed input devices such as microphones and audio interfaces.
- **D-03:** `System Output` represents Windows render endpoints captured through shared-mode WASAPI loopback without requiring vendor "Stereo Mix" devices.
- **D-04:** Do not combine input devices and render endpoints into one ambiguous list. The user should understand whether they are monitoring a recording input or the system speaker/output mix.

### Failure, Removal, and Recovery
- **D-05:** If the selected source disappears, fails, or becomes invalid, standalone capture stops and the UI clearly asks the user to choose a source again.
- **D-06:** Do not silently auto-fallback to the default source after a selected source fails. Avoid surprising capture of a different microphone or output device.
- **D-07:** Automatic retry may be used internally only as part of bounded recovery for the same selected source, but any persistent failure must surface as a stopped/error state requiring user action.

### Silence and Idle State
- **D-08:** If the selected source is valid but currently silent, keep capture active and show a subtle "No signal detected" status.
- **D-09:** Treat silence as distinct from failure. The analyzer may settle to an empty/low spectrum, but the UI must not imply the device is broken.

### Standalone UI Surface
- **D-10:** Standalone source controls live in a compact control strip above the analyzer stage.
- **D-11:** The strip should include mode selection, source selection, and concise source/silence/error status without turning the app into a mixer or settings dashboard.
- **D-12:** Standalone-only controls must be omitted in VST3. The React/MUI app should branch from `host.info.hostMode` or an equivalent typed protocol signal rather than duplicating shells.

### Persistence and Startup Defaults
- **D-13:** On standalone startup, restore the last valid source if it is still available.
- **D-14:** If the saved source is unavailable, start stopped with a clear "Choose source" state.
- **D-15:** Do not auto-select a different default input/output on startup when the saved source is missing; the user must choose again.

### the agent's Discretion
- The planner may choose the exact native class boundaries for shared source lifecycle, JUCE input capture, and WASAPI loopback capture.
- The planner may choose the exact persisted identifier format, as long as it avoids secrets, survives normal Windows device naming churn where practical, and fails visibly when no confident match exists.
- The planner may choose precise wording and visual severity for source states, as long as selected failure, stopped/no-source, and valid-but-silent are visibly distinct.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within Phase 3 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CAP-01 | User can select and monitor an available Windows audio input device in the standalone application. | Use `juce::AudioDeviceManager` for ordinary input-device capture and route callback input into the existing analyzer contract. [CITED: https://docs.juce.com/master/juce__AudioDeviceManager_8h.html] [VERIFIED: codebase] |
| CAP-02 | User can select and monitor a Windows render endpoint through shared-mode WASAPI loopback without requiring a vendor "Stereo Mix" device. | Use a Windows-specific WASAPI adapter over render `IMMDevice` endpoints with `AUDCLNT_STREAMFLAGS_LOOPBACK` in shared mode. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| CAP-03 | Standalone converts supported device channel/sample formats into the analyzer ingress contract without changing analyzer behavior. | Convert capture packets to `float` mono/stereo `juce::AudioBuffer<float>` blocks before calling `SpectrumAnalyzer::pushAudioBlock`. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat] |
| CAP-04 | Standalone safely handles source switching, no active source, device removal, default-device changes, capture invalidation, and restart. | Model stopped/starting/active/silent/error as native source lifecycle state; use JUCE change callbacks for input and `IMMNotificationClient` plus WASAPI error handling for render endpoints. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events] |
| CAP-05 | Standalone persists the last valid source preference and falls back visibly when that source is unavailable. | Persist only selected mode plus bounded non-secret device identifier/display metadata; restore only if the same source is enumerated and active. [VERIFIED: codebase] [ASSUMED] |
| UI-04 | UI exposes source selection and source/error state in standalone while omitting standalone-only controls in VST3. | Extend protocol v1 with closed source-list/state/request events and render a compact MUI strip only when `host.info.hostMode === "Standalone"`. [VERIFIED: codebase] [CITED: https://mui.com/material-ui/react-select.md] |
</phase_requirements>

## Summary

Phase 3 should keep the existing Phase 2 analyzer and bridge architecture intact. `LumaScopeAudioProcessor` already owns `SpectrumAnalyzer`, publishes through `SnapshotMailbox`, and lets `LumaScopeAudioProcessorEditor` emit bounded `spectrum.snapshot` events from the timer/message side; standalone capture should feed that same analyzer ingress instead of creating a parallel DSP path. [VERIFIED: codebase]

The standard split is two native capture adapters behind one source lifecycle controller: a JUCE input adapter using `AudioDeviceManager`/`AudioIODeviceCallback`, and a Windows-only WASAPI loopback adapter for render endpoints. JUCE documents `AudioDeviceManager` as the owner for audio I/O device state, `AudioIODeviceCallback` as the streaming callback interface, and `AudioProcessorPlayer` as a callback that can route device audio through an `AudioProcessor`. [CITED: https://docs.juce.com/master/juce__AudioDeviceManager_8h.html] [CITED: https://docs.juce.com/master/juce__AudioIODevice_8h.html] [CITED: https://docs.juce.com/master/juce__AudioProcessorPlayer_8h.html]

The loopback side cannot depend on vendor capture devices. Microsoft documents that WASAPI loopback captures audio played by a render endpoint, requires obtaining the render endpoint `IMMDevice`, initializes a loopback capture stream, and works only in shared mode. Microsoft also documents that vendor loopback capture devices such as "Stereo Mix" are inconsistent and not always available, which matches the project constraint. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

**Primary recommendation:** Build a standalone-only `SourceController` with `JuceInputSourceAdapter` and `WasapiLoopbackSourceAdapter`, feed preconverted float blocks into the existing analyzer path, and expose closed protocol-v1 source events/requests that React renders only in standalone mode. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

## Project Constraints (from AGENTS.md)

- Windows only for v1; do not research or plan macOS/Linux capture paths. [VERIFIED: AGENTS.md]
- VST3 consumes host audio; standalone owns selected input-device or WASAPI loopback capture. [VERIFIED: AGENTS.md]
- Audio callbacks must not allocate, block, perform network/disk I/O, or communicate directly with WebView. [VERIFIED: AGENTS.md]
- UI stack is React, TypeScript, Material UI, JUCE `WebBrowserComponent`, and WebView2. [VERIFIED: AGENTS.md]
- Build stays Modern CMake with deterministic dependency versions. [VERIFIED: AGENTS.md]
- Use the single project Context7 server and library IDs `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`; do not create per-library MCP servers or commit API keys. [VERIFIED: AGENTS.md]
- No source edits outside GSD workflow; this task writes only the requested research artifact. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Input device enumeration and capture | Native standalone host | UI | Device enumeration and callbacks belong to JUCE/Windows native APIs; UI only requests selection and displays state. [CITED: https://docs.juce.com/master/juce__AudioDeviceManager_8h.html] |
| Render endpoint loopback capture | Native Windows adapter | UI | WASAPI loopback requires Windows Core Audio COM interfaces and endpoint state handling. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| Format/channel conversion | Native capture adapter | Analyzer core | WASAPI/JUCE device packets must be converted to finite `float` blocks before analyzer ingress. [VERIFIED: codebase] |
| Analyzer snapshots | Analyzer core/native editor | UI renderer | Phase 2 already publishes bounded latest snapshots and emits WebView events from the editor timer. [VERIFIED: codebase] |
| Source controls and status | UI | Native protocol | React/MUI owns presentation; native owns authoritative source list/state/errors. [VERIFIED: codebase] [CITED: https://mui.com/material-ui/react-select.md] |
| Source preference persistence | Native standalone host | UI | The saved source preference affects startup capture behavior and should be validated against native enumeration before use. [ASSUMED] |

## Standard Stack

### Core

| Library/API | Version | Purpose | Why Standard |
|-------------|---------|---------|--------------|
| JUCE `juce_audio_utils` / `juce_audio_devices` | JUCE 8.0.14 pinned by project | Standalone plugin holder, audio device manager, input callback plumbing | Current project stack already links `juce_audio_utils`, and Context7/JUCE docs identify the relevant device and callback APIs. [VERIFIED: codebase] [CITED: https://docs.juce.com/master/classjuce_1_1StandalonePluginHolder.html] |
| Windows Core Audio WASAPI | Windows desktop SDK APIs | Render endpoint enumeration, shared-mode loopback capture, invalidation events | Microsoft documents loopback as a render-endpoint shared-mode WASAPI feature independent of vendor "Stereo Mix" devices. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| Existing `lumascope::SpectrumAnalyzer` | Project Phase 2 implementation | Shared analyzer ingress and display snapshot generation | Analyzer already handles mono/stereo float blocks, sample-rate resets, finite output, and bounded snapshots. [VERIFIED: codebase] |
| Existing protocol v1 bridge | Project Phase 1/2 implementation | Native/web source list/state additions and spectrum events | Protocol v1 already validates `hostMode`, errors, and `spectrum.snapshot`; source messages should follow that pattern. [VERIFIED: codebase] |

### Supporting

| Library/API | Version | Purpose | When to Use |
|-------------|---------|---------|-------------|
| `juce::StandalonePluginHolder` | JUCE 8.0.14 pinned by project | Existing standalone app device/processor ownership reference point | Use or wrap carefully when integrating with JUCE standalone lifecycle; it owns `AudioDeviceManager` and `AudioProcessorPlayer` in JUCE source. [VERIFIED: codebase] [CITED: https://docs.juce.com/master/classjuce_1_1StandalonePluginHolder.html] |
| `IMMNotificationClient` | Windows Core Audio | Device add/remove/state/default change notifications | Use for render endpoint list refresh and selected-source invalidation. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events] |
| `IAudioClient::GetMixFormat` | Windows Core Audio | Obtain shared-mode engine format for loopback stream setup and conversion | Use before initializing shared-mode loopback and record the format used by conversion tests. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat] |
| Material UI `ToggleButtonGroup`, `Select`, `Alert`/status text | `@mui/material` 9.1.1 installed | Compact mode picker, source picker, visible source status | Use in the standalone-only strip above the analyzer stage. [VERIFIED: codebase/npm ls] [CITED: https://mui.com/material-ui/react-toggle-button.md] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom WASAPI render-loopback adapter | Vendor "Stereo Mix" capture device through JUCE input list | Microsoft documents vendor loopback devices as inconsistent, not always present, and difficult for users to identify; do not use as the primary path. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| Shared source lifecycle controller | Direct UI calls into separate input/loopback implementations | Separate paths increase state drift; one controller can enforce stopped/error/silent semantics consistently. [VERIFIED: codebase] [ASSUMED] |
| Existing latest-snapshot bridge | Queue every capture block or every spectrum frame to UI | Queues can backlog; Phase 2 locked latest-complete-snapshot wins. [VERIFIED: codebase] |

**Installation:** No new external package install is recommended for Phase 3. Use existing JUCE, Windows SDK/Core Audio headers, and existing UI dependencies. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

**Version verification:** Existing frontend versions were checked with `npm --prefix ui ls`: React 19.2.7, TypeScript 6.0.3, Vite 8.0.16, Vitest 4.1.9, `@testing-library/react` 16.3.2, and `@mui/material` 9.1.1. [VERIFIED: codebase/npm ls]

## Architecture Patterns

### System Architecture Diagram

```text
React/MUI source strip
  |- source.list/request/select events (protocol v1)
  v
LumaScopeAudioProcessorEditor / HostBridge (message thread)
  |- emits source state/list/status to WebView
  |- polls latest analyzer snapshot at bounded cadence
  v
Standalone SourceController (standalone target only)
  |- mode = Input Device ---> JuceInputSourceAdapter ---> AudioDeviceManager callback
  `- mode = System Output --> WasapiLoopbackSourceAdapter --> IMMDevice + IAudioClient + IAudioCaptureClient
         |
         |- device notification / invalidation / restart decision
         v
Format + channel conversion to finite float blocks
  v
Existing SpectrumAnalyzer + SnapshotMailbox
  v
Existing spectrum.snapshot event
  v
SpectrumCanvas renderer
```

### Recommended Project Structure

```text
plugin/
|-- include/LumaScope/Standalone/     # source model, controller interfaces, persistence DTOs
|-- source/Standalone/                # JUCE input adapter and shared lifecycle implementation
|-- source/Standalone/windows/        # WASAPI loopback adapter and endpoint notification implementation
`-- source/HostBridge.cpp             # protocol-v1 source payload builders

ui/src/
|-- bridge/                           # source protocol parsing and provider state
|-- components/StandaloneSourceStrip.tsx
`-- components/                       # existing AnalyzerStage/StatusFooter integration

tests/native/
|-- StandaloneSourceControllerTests.cpp
|-- AudioConversionTests.cpp
`-- WasapiEndpointModelTests.cpp
```

### Pattern 1: Native Source Controller as the Single State Authority

**What:** Put selected mode/source, enumeration snapshots, lifecycle state, silence detection, persistence decisions, and error classification behind one native standalone controller. [ASSUMED]

**When to use:** Use for all standalone source requests and state events; VST3 should not instantiate or expose this controller. [VERIFIED: codebase]

**Example:**

```cpp
// Source: derived from existing HostBridge protocol pattern and Phase 3 decisions.
enum class SourceMode { inputDevice, systemOutput };
enum class SourceState { stopped, starting, active, silent, error };

struct SourceSelection
{
    SourceMode mode;
    juce::String stableId;
    juce::String displayName;
};

class StandaloneSourceController
{
public:
    SourceList enumerateSources();
    SourceStateSnapshot selectSource (const SourceSelection&);
    SourceStateSnapshot stop();
    void pushConvertedBlock (const juce::AudioBuffer<float>&) noexcept;
};
```

### Pattern 2: Keep Capture Callbacks Thin and Preallocated

**What:** Device callbacks copy/convert bounded input into preallocated buffers and call existing analyzer ingress; they do not allocate, query UI, persist settings, log unbounded strings, or recover devices inline. [VERIFIED: codebase] [ASSUMED]

**When to use:** Use for JUCE input callback and WASAPI loopback capture thread. [CITED: https://docs.juce.com/master/juce__AudioIODevice_8h.html] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

**Example:**

```cpp
// Source: project realtime constraint plus SpectrumAnalyzer::pushAudioBlock contract.
void CaptureAdapter::audioCallback (const float* const* inputs, int channels, int samples) noexcept
{
    auto block = scratch.wrapOrCopyFiniteFloatBlock (inputs, channels, samples);
    analyzer.pushAudioBlock (block);
    levelTracker.observe (block); // atomic or preallocated only
}
```

### Pattern 3: Closed Protocol-v1 Source Events

**What:** Add explicit `source.list`, `source.state`, and `source.select`/`source.stop` messages with bounded fields and parser tests; do not put source state into free-form bridge errors. [VERIFIED: codebase]

**When to use:** Use after `host.info` readiness, and render only when `hostMode` is `Standalone`. [VERIFIED: codebase]

**Example:**

```ts
// Source: existing ui/src/bridge/protocol.ts parser style.
export interface SourceState {
  protocolVersion: typeof protocolVersion;
  mode: 'InputDevice' | 'SystemOutput';
  state: 'stopped' | 'starting' | 'active' | 'silent' | 'error';
  selectedSourceId?: string;
  selectedSourceName?: string;
  code?: string;
  message?: string;
}
```

### Anti-Patterns to Avoid

- **One ambiguous device list:** It violates D-04 and hides the capture semantics from the user. [VERIFIED: phase context]
- **Fallback to a different default source after failure:** It violates D-06 and can capture a different microphone/output than the user selected. [VERIFIED: phase context]
- **WebView calls or JSON generation from capture callbacks:** It violates the project real-time constraint and Phase 2 bridge architecture. [VERIFIED: AGENTS.md] [VERIFIED: codebase]
- **Treating silence as an error:** D-08 and D-09 require valid-but-silent to remain active with subtle status. [VERIFIED: phase context]
- **Using JUCE input device enumeration as proof of system-output loopback:** WASAPI loopback is a render-endpoint capture path and not the same as capture-device enumeration. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Ordinary input-device capture | Raw WASAPI input stack for microphones/interfaces | `juce::AudioDeviceManager` plus `AudioIODeviceCallback`/`AudioProcessorPlayer` boundaries | JUCE already manages input device setup and callback dispatch for standalone apps. [CITED: https://docs.juce.com/master/juce__AudioDeviceManager_8h.html] |
| System-output capture | Vendor "Stereo Mix" dependency | WASAPI shared-mode loopback on render endpoints | Microsoft documents that hardware loopback devices are not always present and have inconsistent names. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| Spectrum rendering path | New renderer or raw FFT bridge | Existing `SpectrumAnalyzer`, `SnapshotMailbox`, and `SpectrumCanvas` | Phase 2 has validated bounded display-ready snapshots and latest-snapshot handoff. [VERIFIED: codebase] |
| UI component primitives | Custom inaccessible dropdown/segmented control | Material UI `Select`/`ToggleButtonGroup`/status components | Project stack already uses MUI and MUI documents accessible select labeling. [VERIFIED: codebase] [CITED: https://mui.com/material-ui/react-select.md] |

**Key insight:** The hard part is not FFT analysis; it is source lifecycle correctness around Windows endpoint churn while preserving Phase 2's real-time-safe analyzer handoff. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events]

## Common Pitfalls

### Pitfall 1: Treating JUCE Input Capture and WASAPI Loopback as the Same Device Class

**What goes wrong:** Render endpoints and recording endpoints become mixed in one UI and one implementation path. [ASSUMED]  
**Why it happens:** JUCE exposes Windows Audio devices through its device manager, while WASAPI loopback is opened against a rendering endpoint with loopback flags. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]  
**How to avoid:** Use separate `Input Device` and `System Output` modes and separate native adapters behind one controller. [VERIFIED: phase context]  
**Warning signs:** UI labels like "Stereo Mix" as the only system-output option, or source IDs without a mode prefix. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]

### Pitfall 2: Ignoring Device Invalidation and State Changes

**What goes wrong:** Capture threads continue after unplug/disable/default changes, producing stale state or crashes. [ASSUMED]  
**Why it happens:** Core Audio device add/remove/state/default changes arrive through `IMMNotificationClient`, and WASAPI calls can return invalidation errors. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events] [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat]  
**How to avoid:** Register endpoint notifications, filter for selected endpoint IDs, stop selected-source capture on persistent failure, and emit a visible source error requiring user action. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events] [VERIFIED: phase context]  
**Warning signs:** Auto-switching to a new default source or storing only display names without endpoint IDs. [VERIFIED: phase context]

### Pitfall 3: Unsafe Work on Capture Threads

**What goes wrong:** Dropouts, deadlocks, or host/app instability occur when callbacks allocate, block, persist settings, or emit WebView messages. [VERIFIED: AGENTS.md]  
**Why it happens:** Source switching and UI status tempt implementers to cross into GUI/persistence code from callbacks. [ASSUMED]  
**How to avoid:** Capture callbacks only convert/copy into preallocated buffers and feed the analyzer; controller state changes occur on non-audio threads. [VERIFIED: codebase]  
**Warning signs:** `juce::JSON`, `browser.emitEvent...`, file writes, COM enumeration, or locks inside audio callback bodies. [VERIFIED: codebase]

### Pitfall 4: Mishandling WASAPI Mix Format

**What goes wrong:** Loopback packets are interpreted with the wrong sample type, channel count, or frame stride. [ASSUMED]  
**Why it happens:** `GetMixFormat` returns the shared-mode engine format and may use `WAVEFORMATEXTENSIBLE`; Microsoft notes the caller owns freeing that allocation. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat]  
**How to avoid:** Normalize PCM/int/float formats into finite float blocks, downmix channels consistently, and test every conversion path without changing analyzer behavior. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat] [VERIFIED: codebase]  
**Warning signs:** Assuming all loopback data is stereo `float32` or skipping `WAVEFORMATEXTENSIBLE` channel masks. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat]

## Code Examples

Verified patterns from official and project sources:

### JUCE Input Device Callback Boundary

```cpp
// Source: Context7 /websites/juce_master, AudioDeviceManager + AudioIODeviceCallback docs.
juce::AudioDeviceManager deviceManager;
deviceManager.initialise (2, 0, nullptr, false);
deviceManager.addAudioCallback (&inputAdapter);
```

### WASAPI Loopback Setup Sequence

```cpp
// Source: Microsoft Loopback Recording + IAudioClient::Initialize docs.
// 1. Enumerate/select an eRender IMMDevice.
// 2. Activate IAudioClient on that render endpoint.
// 3. Read GetMixFormat for shared-mode format.
// 4. Initialize with AUDCLNT_SHAREMODE_SHARED and AUDCLNT_STREAMFLAGS_LOOPBACK.
// 5. Get IAudioCaptureClient and read packets on the capture thread.
```

### Standalone-only UI Branch

```tsx
// Source: existing AppShell hostMode branch and MUI Select accessibility docs.
{bridge.state === 'ready' && bridge.hostInfo.hostMode === 'Standalone' ? (
  <StandaloneSourceStrip sourceState={bridge.sourceState} />
) : null}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Vendor hardware loopback capture devices such as "Stereo Mix" | WASAPI shared-mode loopback on render endpoints | Microsoft loopback documentation current as of 2025-04-16 page update | Phase 3 should not require users to enable or identify vendor loopback devices. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording] |
| Queueing every analysis frame to UI | Latest-complete snapshot wins through fixed mailbox and timer emission | Locked by Phase 2 project decisions | Source capture must not introduce UI backlog. [VERIFIED: codebase] |
| Source UI in native audio settings dialog only | Compact React/MUI standalone strip | Locked by Phase 3 decisions | Users get phase-specific source controls without exposing VST3-only or generic device settings. [VERIFIED: phase context] |

**Deprecated/outdated:**
- Relying on "Stereo Mix" or "What You Hear" as the system-output path is inappropriate for this project because Microsoft documents those vendor loopback devices as optional and inconsistently named. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]
- Emitting source updates through free-form `bridge.error` is inappropriate because protocol v1 payloads are closed and parser-tested. [VERIFIED: codebase]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Persisted source preference should use mode plus endpoint/device ID plus display metadata, with no alternate default fallback. | Phase Requirements, Responsibility Map | If Windows/JUCE IDs are less stable than expected, restore may fail more often and require clearer UX. |
| A2 | A single `SourceController` abstraction is the best implementation boundary. | Architecture Patterns | If JUCE standalone holder internals make this hard to own cleanly, planner may need a thinner integration boundary. |
| A3 | Capture callbacks can use preallocated conversion scratch buffers for all expected block sizes. | Common Pitfalls | If WASAPI packet sizes exceed expected scratch bounds, implementation must include a bounded resize outside capture start or reject that configuration visibly. |

## Open Questions

1. **Should Phase 3 customize JUCE `StandalonePluginHolder` or bypass it for standalone capture ownership?**
   - What we know: JUCE source shows `StandalonePluginHolder` owns `AudioDeviceManager` and `AudioProcessorPlayer`; the current project uses `juce_add_plugin(... FORMATS VST3 Standalone ...)`. [VERIFIED: codebase]
   - What's unclear: The least invasive hook point for adding standalone-only source lifecycle without fighting generated standalone client code. [ASSUMED]
   - Recommendation: Plan an early spike/task to inspect generated standalone entry points and choose either a custom standalone app wrapper or a controlled extension around holder/device manager. [ASSUMED]

2. **How much WASAPI endpoint behavior can be automated in tests on CI?**
   - What we know: Conversion, state-machine, parser, and persistence tests can be deterministic without real endpoints. [VERIFIED: codebase]
   - What's unclear: CI availability of render endpoints and event-driven loopback behavior. [ASSUMED]
   - Recommendation: Automate model/conversion/protocol tests and keep a required manual Windows device matrix for real loopback, unplug/disable, default changes, and silence. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events]

3. **Build toolchain mismatch risk**
   - What we know: AGENTS recommends Visual Studio 2022, while current `CMakePresets.json` uses Visual Studio 16 2019 presets and local `vswhere` reports a 16.11 toolset. [VERIFIED: AGENTS.md] [VERIFIED: codebase]
   - What's unclear: Whether Phase 3 should preserve existing VS2019 presets or align presets to the stack document. [ASSUMED]
   - Recommendation: Do not change presets in Phase 3 unless required for WASAPI/Windows SDK compilation; record any mismatch in validation evidence. [ASSUMED]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Native configure/build/test | yes | 4.0.3 | Existing presets require 3.22+. [VERIFIED: local command] |
| CTest | Native test execution | yes | 4.0.3 | None needed. [VERIFIED: local command] |
| Node.js | Frontend tests/build | yes | 22.18.0 | None needed. [VERIFIED: local command] |
| npm | Frontend dependency scripts | yes | 10.9.3 | None needed. [VERIFIED: local command] |
| Windows PowerShell | Project scripts | yes | 5.1.19041.6456 | Use Windows PowerShell scripts; `pwsh` is absent. [VERIFIED: local command] |
| PowerShell 7 `pwsh` | Optional shell | no | - | Use Windows PowerShell 5.1. [VERIFIED: local command] |
| Visual Studio C++ tools on PATH (`cl`, `MSBuild`) | Native command-line builds | no on current PATH | - | Use configured VS generator/developer environment; validate before implementation. [VERIFIED: local command] |
| Visual Studio installation via `vswhere` | Native builds | yes | 16.11.37327.17 | Toolset exists but does not match AGENTS VS2022 recommendation. [VERIFIED: local command] |
| pluginval | Later VST3 validation | no | - | Not required for Phase 3 capture tests; keep honest skip if invoked. [VERIFIED: local command] |

**Missing dependencies with no fallback:**
- None for research. [VERIFIED: local command]

**Missing dependencies with fallback:**
- `pwsh`: use Windows PowerShell 5.1. [VERIFIED: local command]
- `pluginval`: not needed for standalone capture research; do not claim pluginval coverage. [VERIFIED: local command]
- `cl`/`MSBuild` on PATH: run from a Visual Studio developer environment or through CMake generator integration. [VERIFIED: local command]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Native framework | CTest target `LumaScopeNativeTests` built from custom C++ test executable. [VERIFIED: codebase] |
| Native config file | `tests/native/CMakeLists.txt`. [VERIFIED: codebase] |
| Frontend framework | Vitest 4.1.9 with React Testing Library 16.3.2 and jsdom. [VERIFIED: codebase/npm ls] |
| Frontend config file | `ui/vitest.config.ts`. [VERIFIED: codebase] |
| Quick native command | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` [VERIFIED: codebase] |
| Quick frontend command | `npm --prefix ui run test:run -- --runInBand` is not a Vitest-standard flag; use `npm --prefix ui run test:run` unless planner adds targeted commands. [VERIFIED: codebase] |
| Full suite command | `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` if unchanged by planner. [VERIFIED: codebase] |

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| CAP-01 | Input source selection and JUCE callback feeds analyzer | native unit/integration | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` | No; Wave 0 add `StandaloneSourceControllerTests.cpp`. [VERIFIED: codebase] |
| CAP-02 | Render endpoint loopback selection model and adapter startup errors | native unit + manual Windows smoke | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` | No; Wave 0 add model tests and manual matrix. [VERIFIED: codebase] |
| CAP-03 | Sample format/channel conversion to analyzer ingress | native unit | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` | No; Wave 0 add `AudioConversionTests.cpp`. [VERIFIED: codebase] |
| CAP-04 | Switching, no source, removal, invalidation, restart state | native unit + manual Windows smoke | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` | No; Wave 0 add state-machine tests. [VERIFIED: codebase] |
| CAP-05 | Preference restore and unavailable saved source fallback | native unit | `ctest --test-dir build/vs2019-debug -C Debug -R LumaScopeNativeTests --output-on-failure` | No; Wave 0 add persistence tests. [VERIFIED: codebase] |
| UI-04 | Standalone-only source controls and source/error state | frontend component/protocol | `npm --prefix ui run test:run` | No; add parser/provider/strip tests. [VERIFIED: codebase] |

### Sampling Rate

- **Per task commit:** Run native targeted CTest for changed native files or `npm --prefix ui run test:run` for UI/protocol changes. [VERIFIED: codebase]
- **Per wave merge:** Run both native and frontend suites. [VERIFIED: codebase]
- **Phase gate:** Run full project verification plus a manual Windows matrix covering microphone/input, render loopback, source disappearance, default-device change, silence, restart, and saved-source missing. [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events]

### Wave 0 Gaps

- [ ] `tests/native/StandaloneSourceControllerTests.cpp` - covers CAP-01, CAP-04, CAP-05. [VERIFIED: codebase]
- [ ] `tests/native/AudioConversionTests.cpp` - covers CAP-03. [VERIFIED: codebase]
- [ ] `tests/native/WasapiEndpointModelTests.cpp` - covers CAP-02 and CAP-04 without requiring live hardware. [ASSUMED]
- [ ] `ui/src/bridge/SourceProtocol.test.ts` - covers source protocol parsing and malformed payload rejection for UI-04. [VERIFIED: codebase]
- [ ] `ui/src/components/StandaloneSourceStrip.test.tsx` - covers standalone-only controls, VST3 omission, source/error/silent states. [VERIFIED: codebase]
- [ ] Manual verification document or checklist for real Windows input/render endpoint behavior. [ASSUMED]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | Licensing/authentication is Phase 6; Phase 3 should not add activation UI. [VERIFIED: roadmap] |
| V3 Session Management | no | No user session or network service is introduced. [VERIFIED: roadmap] |
| V4 Access Control | limited | Hide standalone controls in VST3 through trusted `host.info.hostMode`; do not expose VST3 capture controls. [VERIFIED: codebase] |
| V5 Input Validation | yes | Closed protocol-v1 parsers with bounded strings, finite numeric validation, and stable error envelopes. [VERIFIED: codebase] |
| V6 Cryptography | no | No cryptographic feature is in Phase 3. [VERIFIED: roadmap] |

### Known Threat Patterns for Native/Web Audio Source Control

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed WebView source-select payload | Tampering | Validate protocol version, mode enum, bounded source ID, and reject unknown fields before native action. [VERIFIED: codebase] |
| Unintended microphone/output capture after fallback | Privacy | Never auto-fallback to another source after selected-source failure; require user selection. [VERIFIED: phase context] |
| Unbounded native error text into React | Information disclosure / DoS | Bound codes/messages like existing `bridge.error` and avoid raw COM/system strings beyond concise diagnostics. [VERIFIED: codebase] |
| Capture callback crossing to UI or disk | Denial of service | Keep callbacks realtime-safe and move state persistence/UI events to non-audio threads. [VERIFIED: AGENTS.md] |

## Sources

### Primary (HIGH confidence)

- Context7 `/websites/juce_master` - `AudioDeviceManager`, `AudioIODeviceCallback`, `AudioProcessorPlayer`, `StandalonePluginHolder` queried 2026-06-23.
- Microsoft Learn Loopback Recording - WASAPI render endpoint loopback, shared-mode requirement, vendor loopback limitations, page updated 2025-04-16: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording
- Microsoft Learn Device Events - `IMMNotificationClient` device add/remove/default/state callbacks: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events
- Microsoft Learn `IAudioClient::GetMixFormat` - shared-mode mix format and invalidation errors, page updated 2025-09-05: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat
- Microsoft Learn `IAudioClient::Initialize` - shared-mode/event/loopback initialization constraints: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-initialize
- Microsoft Learn `DEVICE_STATE_XXX` - active/disabled/not-present/unplugged endpoint states: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-state-xxx-constants
- Context7 `/websites/mui_material-ui` - MUI Select accessibility, ToggleButtonGroup, Alert import patterns queried 2026-06-23.
- Codebase files required by prompt, especially `PluginProcessor`, `PluginEditor`, `HostBridge`, `SpectrumAnalyzer`, `protocol.ts`, `BridgeProvider.tsx`, `AppShell.tsx`, `AnalyzerStage.tsx`, `StatusFooter.tsx`, `plugin/CMakeLists.txt`, and `tests/native/CMakeLists.txt`. [VERIFIED: codebase]

### Secondary (MEDIUM confidence)

- Local vendored JUCE source under `.deps/cpm-cache/juce/f73e/modules` - implementation check for standalone holder ownership, `AudioDeviceManager` callback dispatch, and JUCE WASAPI support. [VERIFIED: codebase]

### Tertiary (LOW confidence)

- Implementation-boundary assumptions in the Assumptions Log, pending planner/executor validation against generated standalone target internals. [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Uses locked project stack, Context7 JUCE docs, Microsoft Core Audio docs, and installed package checks. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording]
- Architecture: HIGH - Existing analyzer/bridge ownership is verified in code; only exact class names remain discretionary. [VERIFIED: codebase]
- Pitfalls: HIGH for real-time/protocol/device invalidation categories, MEDIUM for exact Windows device recovery behavior until manual endpoint matrix runs. [VERIFIED: codebase] [CITED: https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-events]

**Research date:** 2026-06-23  
**Valid until:** 2026-07-23 for JUCE/project architecture; recheck Microsoft WASAPI docs and package versions before implementation if planning slips past 30 days.
