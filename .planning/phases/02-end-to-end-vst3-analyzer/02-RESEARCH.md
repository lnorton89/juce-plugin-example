# Phase 2: End-to-End VST3 Analyzer - Research

**Researched:** 2026-06-23  
**Domain:** JUCE VST3 real-time spectrum analysis, native/web snapshot bridge, React canvas rendering  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Analyzer Presets and Defaults
- **D-01:** Implement three analyzer preset profiles in the DSP/model layer: `Musical`, `Measurement`, and `Fast`.
- **D-02:** The default visible Phase 2 behavior is `Musical`.
- **D-03:** `Musical` defaults should be a balanced mixing-oriented analyzer: 4096-point FFT, Hann window, logarithmic 20 Hz-20 kHz display, bounded ~30 FPS UI snapshots, and moderate smoothing/decay so the display is stable rather than twitchy.
- **D-04:** `Measurement` and `Fast` must be real preset profiles/configuration paths in the core, not just names, but full user-facing mode switching may remain an extension point unless it is trivial and does not displace the Phase 2 MVP.

### Real-Time Audio and Snapshot Handoff
- **D-05:** Use a latest-snapshot-wins handoff. If UI rendering lags, stale frames are dropped rather than queued.
- **D-06:** Audio callbacks must never call WebView, perform JSON work, allocate dynamically, block, perform disk/network I/O, or rely on locks that can contend with the UI.
- **D-07:** Analyzer processing should publish complete snapshots through a preallocated real-time-safe boundary. The UI/editor consumes the newest complete snapshot at its bounded display rate.
- **D-08:** The analyzer must continue running while the editor is closed. Reopening the editor should catch up quickly from the latest available snapshot without requiring audio processing to restart.

### Spectrum Rendering
- **D-09:** Render the first real spectrum as a smooth filled spectral curve with subtle glow/fill, matching the existing dark LumaScope instrument-panel look.
- **D-10:** Rendering must be efficient: no DOM/MUI element per FFT bin. Prefer canvas, SVG path, or another bounded-node strategy suitable for WebView2.
- **D-11:** Keep the existing central analyzer stage as the visual home for the renderer and preserve room for future labels/controls without implementing v2 controls in this phase.

### VST3 Behavior and Host Proof
- **D-12:** The VST3 is an analyzer/effect passthrough. Output must be sample-equivalent to input within floating-point tolerance for supported mono/stereo layouts.
- **D-13:** The analyzer must not alter gain, latency, channel count, or samples; it only observes audio.
- **D-14:** Automated tests should cover silence, deterministic tones, mono/stereo layouts, sample-rate changes, block-size variation, passthrough equivalence, editor-open/editor-closed operation, and malformed/edge conditions.
- **D-15:** Ableton Live is the preferred manual DAW smoke target for Phase 2. If Ableton is unavailable or cannot be verified enough in the environment, the executor must record that honestly and use the best available fallback without claiming Ableton passed.
- **D-16:** Automated plugin validation remains required where practical, with pluginval/VST3 validation preferred and any environmental limitations documented explicitly.

### the agent's Discretion
- Exact `Measurement` and `Fast` preset numerical values, provided they are meaningfully distinct from `Musical` and covered by tests.
- Exact FFT overlap/hop strategy, smoothing coefficient model, dB floor/ceiling, interpolation, and log-bin count, provided deterministic tone tests verify frequency placement and level tolerance.
- Exact snapshot payload shape and renderer implementation technology, provided the protocol remains typed/versioned, bounded, and efficient.
- Exact DAW fallback order if Ableton Live is not available.

### Deferred Ideas (OUT OF SCOPE)
- Full user-facing analyzer controls for FFT size, windows, smoothing, decay, hold/freeze, and cursor inspection remain v2 `CTRL-*` scope.
- Standalone audio input and WASAPI loopback source selection remain Phase 3.
- Activation/licensing UI remains Phase 6.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DSP-01 | Analyzer accepts mono or stereo floating-point audio at host/device sample rates without modifying source audio. | Use mono/stereo bus-only layouts and pass input through unchanged while observing the main bus. [VERIFIED: codebase grep] |
| DSP-02 | Analyzer applies a documented window, FFT, single-sided magnitude normalization, and decibel conversion that place deterministic test tones at expected frequency and level tolerance. | Use JUCE `dsp::FFT` and `dsp::WindowingFunction`; add deterministic tone tests for bin placement and dB tolerance. [CITED: https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html] |
| DSP-03 | Analyzer publishes a logarithmic-frequency spectrum with fixed FFT size, visible range, smoothing, and decay defaults. | Implement fixed profile configs and a log-bin snapshot payload rather than raw FFT bins. [VERIFIED: 02-CONTEXT.md] |
| DSP-04 | Analyzer handles silence, denormals, changing block sizes, sample-rate changes, and channel-layout changes without invalid output or crashes. | JUCE documents variable and zero-sample `processBlock` calls; tests must cover zero, small, large, and changing block sequences. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html] |
| DSP-05 | Audio callbacks perform no allocation, blocking synchronization, file/network access, JSON work, or WebView calls. | Use preallocated ring/FFT buffers and atomic latest snapshot publication; emit WebView events only from editor timer. [VERIFIED: 02-CONTEXT.md] |
| DSP-06 | Analyzer configuration and snapshot interfaces allow future controls without replacing DSP core. | Store `AnalyzerProfile` and `AnalyzerConfig` as typed native models with Musical/Measurement/Fast presets. [VERIFIED: 02-CONTEXT.md] |
| VST3-01 | User can load VST3 as an audio effect in a compatible Windows host and see routed audio spectrum. | Build existing JUCE VST3 target, extend processor/editor bridge, then validate with pluginval plus a real DAW smoke. [VERIFIED: codebase grep] |
| VST3-02 | Plug-in output is sample-equivalent to input within floating-point tolerance. | Add processor passthrough tests that compare input/output buffers after `processBlock`. [VERIFIED: codebase grep] |
| VST3-03 | Plug-in remains stable when editor closes/reopens/resizes/destroys while audio continues. | Keep analyzer state processor-owned; editor only subscribes/polls latest snapshots. [VERIFIED: codebase grep] |
| VST3-04 | Plug-in passes automated VST3 validation and documented smoke tests in at least one real Windows DAW. | `pluginval` is preferred but missing locally; planner must add an install/checkpoint or document limitation. [VERIFIED: environment probe] |
| UI-02 | Spectrum renders smoothly at bounded display frame rate without DOM/MUI element per FFT bin. | Use one canvas renderer mounted in `#spectrum-renderer-mount`; Material UI remains shell/layout. [VERIFIED: 02-CONTEXT.md] |
</phase_requirements>

## Summary

Phase 2 should implement a processor-owned analyzer pipeline that observes host audio, never changes samples, and publishes complete spectrum snapshots through a preallocated latest-snapshot-wins boundary. [VERIFIED: 02-CONTEXT.md] The editor should poll that boundary at a bounded cadence and emit typed `spectrum.snapshot` events to the WebView only from the UI/message side, preserving JUCE's real-time rule that `processBlock` must not interact with the UI. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]

The DSP core should use JUCE's pinned `juce_dsp` module for Hann windowing and FFT, then normalize to single-sided dB magnitudes and resample into a fixed logarithmic display grid. [CITED: https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html] Use `Musical` as the visible default and implement real `Measurement` and `Fast` profiles behind the same config model so future controls do not require a core rewrite. [VERIFIED: 02-CONTEXT.md]

**Primary recommendation:** Plan four slices in the roadmap order: DSP core/tests, real-time handoff, VST3/editor/renderer integration, then pluginval plus Ableton-or-fallback host proof. [VERIFIED: .planning/ROADMAP.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Audio passthrough | API / Backend (JUCE processor) | Browser / Client: none | `processBlock` owns audio buffers and must preserve samples before any UI involvement. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html] |
| FFT/window/magnitude/log-bin DSP | API / Backend (native DSP core) | Browser / Client: render only | Deterministic tone behavior and real-time constraints belong in native tests and processor-owned state. [VERIFIED: codebase grep] |
| Latest snapshot handoff | API / Backend (processor/editor boundary) | Frontend Server: none | Audio thread publishes preallocated data; editor consumes on message/timer side. [VERIFIED: 02-CONTEXT.md] |
| Snapshot protocol | API / Backend and Browser / Client | — | Existing bridge is typed on both native and TypeScript sides; Phase 2 must extend both parsers together. [VERIFIED: docs/bridge-protocol.md] |
| Spectrum rendering | Browser / Client | API / Backend supplies data | WebView should draw the spectral curve from bounded snapshot data; MUI remains layout/theme shell. [VERIFIED: 02-CONTEXT.md] |
| Host validation | Test/Tooling | API / Backend | pluginval/DAW smoke validates VST3 loading, editor lifecycle, and audio-thread safety externally. [CITED: https://github.com/Tracktion/pluginval] |

## Project Constraints (from AGENTS.md)

- Windows-only v1; do not broaden host/device/platform scope in Phase 2. [VERIFIED: AGENTS.md]
- VST3 and standalone are both built, but Phase 2's user-facing scope is VST3 host audio; standalone capture remains Phase 3. [VERIFIED: AGENTS.md]
- Audio callback must not allocate, block, perform network/disk I/O, or communicate directly with WebView. [VERIFIED: AGENTS.md]
- UI stack remains React, TypeScript, Material UI, JUCE `WebBrowserComponent`, and WebView2. [VERIFIED: AGENTS.md]
- Build system remains modern CMake with pinned deterministic dependencies. [VERIFIED: AGENTS.md]
- No secrets/account-specific values in Git; Phase 2 should not introduce any. [VERIFIED: AGENTS.md]
- Use one project Context7 server and the configured IDs `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`. [VERIFIED: AGENTS.md]
- Do not create per-library MCP servers or commit API keys. [VERIFIED: AGENTS.md]
- GSD workflow rules apply; research writes planning artifacts only. [VERIFIED: AGENTS.md]

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.14 pinned | VST3 processor/editor, bus layouts, WebView, DSP modules | Already pinned in `cmake/Dependencies.cmake`; JUCE documents `AudioProcessor`, `dsp::FFT`, and windowing APIs needed here. [VERIFIED: codebase grep] [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html] |
| `juce_dsp` | From JUCE 8.0.14 | FFT and Hann windowing | The JUCE WebView tutorial explicitly links `juce::juce_dsp` when DSP is used. [CITED: https://github.com/janwilczek/juce-webview-tutorial/blob/main/plugin/CMakeLists.txt] |
| C++20 | Project standard | Analyzer core, preallocated buffers, atomic publication | Root CMake sets C++20 as required. [VERIFIED: codebase grep] |
| React | 19.2.7 | WebView application shell and renderer component | Existing locked UI dependency; do not add a visualization framework for Phase 2. [VERIFIED: npm registry] |
| Material UI | 9.1.1 | Shell layout/theme/status controls | Existing locked UI dependency and project-selected UI system. [VERIFIED: npm registry] [CITED: /websites/mui_material-ui] |
| Vite/Vitest | Vite 8.0.16, Vitest 4.1.9 | UI build and unit tests | Existing package scripts already run build/test paths. [VERIFIED: npm registry] |

### Supporting

| Library / Tool | Version | Purpose | When to Use |
|----------------|---------|---------|-------------|
| WebView2 SDK/runtime | SDK 1.0.4022.49 pinned; Evergreen runtime required | Hosts React UI in JUCE | Already configured in Phase 1; Phase 2 only extends bridge events. [VERIFIED: codebase grep] |
| CTest | CMake 4.0.3 locally | Native DSP/passthrough tests | Add DSP, processBlock, and snapshot tests to existing `LumaScopeNativeTests`. [VERIFIED: environment probe] |
| pluginval | Missing locally | VST3 validation | Add optional script path/checkpoint; fail honestly if unavailable. [VERIFIED: environment probe] [CITED: https://github.com/Tracktion/pluginval] |
| Ableton Live | Folder present, usability unverified | Preferred DAW smoke | Human/executor must verify actual executable/license and record results. [VERIFIED: environment probe] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| JUCE `dsp::FFT` | FFTW/KissFFT | Not needed for Phase 2; JUCE DSP is pinned and adequate for deterministic analyzer tests. [VERIFIED: AGENTS.md] |
| Canvas renderer | SVG path | SVG is acceptable if it uses one or a few paths, but canvas is preferred for stable high-frequency redraws in WebView2. [ASSUMED] |
| Latest snapshot | Queued frames | Queues preserve history but can lag and grow stale; locked decision is latest snapshot wins. [VERIFIED: 02-CONTEXT.md] |

**Installation:**

```bash
# No new npm/CMake packages should be installed for Phase 2.
# Add juce::juce_dsp to target_link_libraries from the already pinned JUCE dependency.
```

**Version verification:** `npm view` confirmed `@mui/material` 9.1.1, `vite` 8.0.16, and `vitest` 4.1.9 on 2026-06-23. [VERIFIED: npm registry]

## Package Legitimacy Audit

Phase 2 should not install new external packages. [VERIFIED: codebase grep] Existing dependencies remain governed by Phase 1's pinned dependency policy, and `juce_dsp` comes from the already pinned JUCE dependency. [VERIFIED: cmake/Dependencies.cmake]

| Package | Registry | Age | Downloads | Source Repo | slopcheck | Disposition |
|---------|----------|-----|-----------|-------------|-----------|-------------|
| None new | — | — | — | — | Not run | Approved: no new package install recommended. |

**Packages removed due to slopcheck [SLOP] verdict:** none  
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```text
VST3 host audio
  -> JUCE processBlock(buffer, midi)
  -> explicit passthrough/no sample mutation
  -> mono/stereo analyzer ingress copy into preallocated FIFO/window accumulator
  -> enough samples for hop?
       no  -> return immediately
       yes -> window + FFT + magnitude + dB + log-bin map + smoothing/decay
  -> publish complete native SpectrumSnapshot into double-buffered latest slot
  -> editor timer at bounded FPS reads newest sequence if present
  -> HostBridge emits spectrum.snapshot native event
  -> BridgeProvider validates typed payload
  -> Canvas renderer draws filled logarithmic curve
```

### Recommended Project Structure

```text
plugin/
├── include/LumaScope/Analyzer/
│   ├── AnalyzerConfig.h       # profiles, ranges, bin counts
│   ├── SpectrumAnalyzer.h     # preallocated DSP core
│   └── SpectrumSnapshot.h     # POD-like snapshot model
├── source/Analyzer/
│   ├── AnalyzerConfig.cpp
│   └── SpectrumAnalyzer.cpp
├── include/LumaScope/
│   ├── SnapshotMailbox.h      # latest-snapshot-wins handoff
│   └── HostBridge.h           # add spectrum snapshot event constants/parsers
└── source/
    ├── PluginProcessor.cpp    # passthrough + analyzer ingress
    └── PluginEditor.cpp       # bounded polling and WebView emission

ui/src/
├── bridge/protocol.ts         # SpectrumSnapshot schema/parser
├── bridge/BridgeProvider.tsx  # subscribe to snapshot events
└── components/SpectrumCanvas.tsx
```

### Pattern 1: Processor-Owned Analyzer State

**What:** `LumaScopeAudioProcessor` owns the analyzer, config, and latest snapshot mailbox; editors are disposable consumers. [VERIFIED: 02-CONTEXT.md]  
**When to use:** Always in Phase 2, because audio continues when the editor is closed. [VERIFIED: 02-CONTEXT.md]

```cpp
// Source: JUCE AudioProcessor docs + existing PluginProcessor structure.
void LumaScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    auto input = getBusBuffer (buffer, true, 0);
    analyzer.pushAudioBlock (input);     // preallocated, non-blocking, no JSON/WebView
    // Leave buffer samples unchanged for analyzer passthrough.
}
```

### Pattern 2: Fixed Preset Config Model

**What:** Encode `Musical`, `Measurement`, and `Fast` as real `AnalyzerProfile` values with deterministic config fields. [VERIFIED: 02-CONTEXT.md]  
**When to use:** Immediately, even if UI switching is deferred. [VERIFIED: 02-CONTEXT.md]

| Profile | FFT | Window | Visible Range | UI FPS | Smoothing / Decay Recommendation |
|---------|-----|--------|---------------|--------|----------------------------------|
| Musical | 4096 | Hann | 20 Hz-20 kHz | 30 | Attack 0.45, release 0.12 per frame; floor -96 dB, ceiling 0 dB. [ASSUMED] |
| Measurement | 8192 | Hann | 20 Hz-20 kHz | 20 | Slower smoothing/decay for stable readings; floor -120 dB, ceiling 0 dB. [ASSUMED] |
| Fast | 2048 | Hann | 20 Hz-20 kHz | 45 | Faster attack/release for responsiveness; floor -90 dB, ceiling 0 dB. [ASSUMED] |

Recommended log grid: 160 display bins, each bin stores center frequency, min/max span, and normalized dB value. [ASSUMED]

### Pattern 3: Latest-Snapshot-Wins Double Buffer

**What:** Publish only complete snapshots using two preallocated slots plus atomic sequence/slot indicators. [ASSUMED]  
**When to use:** When a real-time producer and UI consumer have mismatched rates. [VERIFIED: 02-CONTEXT.md]

```cpp
// Source: Phase 2 locked decision; exact implementation should be tested with TSAN-style reasoning.
struct SnapshotMailbox
{
    std::array<SpectrumSnapshot, 2> slots;
    std::atomic<uint32_t> publishedSequence { 0 };
    std::atomic<int> publishedSlot { 0 };

    void publishFromAudioThread (const SpectrumSnapshot& snapshot) noexcept;
    bool readLatestOnMessageThread (SpectrumSnapshot& out, uint32_t& lastSeen) noexcept;
};
```

### Pattern 4: Canvas Renderer, MUI Shell

**What:** Mount one `<canvas>` inside `#spectrum-renderer-mount`, resize with `ResizeObserver`, redraw through `requestAnimationFrame`, and draw one filled spectral path per snapshot. [ASSUMED]  
**When to use:** For the continuous spectrum curve; use MUI only for surrounding layout/status. [VERIFIED: 02-CONTEXT.md]

```tsx
// Source: Phase 2 UI decision; MUI remains the layout shell.
export function SpectrumCanvas({ snapshot }: { snapshot: SpectrumSnapshot | null }) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  useSpectrumCanvasRenderer(canvasRef, snapshot);
  return <canvas ref={canvasRef} aria-label="Live frequency spectrum" />;
}
```

### Anti-Patterns to Avoid

- **Queueing every FFT frame:** It creates visual lag and violates latest-snapshot-wins. [VERIFIED: 02-CONTEXT.md]
- **Calling `emitEventIfBrowserIsVisible` from `processBlock`:** JUCE says UI interaction in `processBlock` is out of the question. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]
- **Allocating `std::vector`/`juce::var`/JSON in audio callback:** Violates real-time safety and Phase 2 decisions. [VERIFIED: 02-CONTEXT.md]
- **One DOM/MUI node per bin:** Violates UI-02 and creates unnecessary layout cost. [VERIFIED: .planning/REQUIREMENTS.md]
- **Fake spectrum data in ready state:** Phase 1 intentionally avoided fabricated analyzer output; Phase 2 should render only live or explicit test fixtures. [VERIFIED: 01-VERIFICATION.md]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| FFT engine | Custom DFT/FFT | `juce::dsp::FFT` | JUCE provides a pinned FFT API and `performFrequencyOnlyForwardTransform`. [CITED: https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html] |
| Window coefficients | Manual Hann tables | `juce::dsp::WindowingFunction` | JUCE provides windowing functions for spectrum analyzers. [CITED: https://docs.juce.com/master/juce__Windowing_8h.html] |
| VST3 validation harness | Bespoke host simulator only | pluginval plus native tests | pluginval is designed for headless plugin validation in CI. [CITED: https://github.com/Tracktion/pluginval] |
| Bridge schema guesses | Ad hoc object reads | Existing closed protocol parsers + fixtures | Protocol v1 requires coordinated native/UI parser updates. [VERIFIED: docs/bridge-protocol.md] |
| High-frequency UI with MUI nodes | Hundreds of bars/components | Canvas or bounded SVG path | UI-02 forbids DOM/MUI per FFT bin. [VERIFIED: .planning/REQUIREMENTS.md] |

**Key insight:** The hard part is not computing an FFT; it is keeping every cross-thread and host lifecycle boundary boring, bounded, and testable. [ASSUMED]

## Common Pitfalls

### Pitfall 1: Block Size Assumptions
**What goes wrong:** FFT input code assumes every callback has the same size as `prepareToPlay`. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]  
**Why it happens:** `maximumExpectedSamplesPerBlock` is a hint, not a guarantee. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessorGraph.html]  
**How to avoid:** Use an accumulator that accepts arbitrary blocks, including zero samples and blocks larger than expected. [ASSUMED]  
**Warning signs:** Clicks, crashes, out-of-bounds writes, or analyzer stalls during host transport changes. [ASSUMED]

### Pitfall 2: Analyzer Changes Audio
**What goes wrong:** Mono summing or output clearing accidentally modifies the host buffer. [ASSUMED]  
**Why it happens:** JUCE buffers can include input/output channel details and garbage in extra output channels. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]  
**How to avoid:** Restrict supported layouts to matching mono/stereo input/output and add bit/tolerance passthrough tests. [VERIFIED: codebase grep]  
**Warning signs:** Null tests fail or host gain/latency meters change when inserting LumaScope. [ASSUMED]

### Pitfall 3: Editor Lifetime Coupled to DSP
**What goes wrong:** Closing the editor stops analyzer state or destroys buffers still used by audio. [ASSUMED]  
**Why it happens:** UI objects feel like the natural place to store display data, but hosts can destroy editors while processing continues. [VERIFIED: 02-CONTEXT.md]  
**How to avoid:** Processor owns analyzer and mailbox; editor stores only last-seen sequence and UI render state. [VERIFIED: 02-CONTEXT.md]  
**Warning signs:** Reopening editor shows no spectrum until playback restarts. [ASSUMED]

### Pitfall 4: dB Normalization Drift
**What goes wrong:** A full-scale sine appears at surprising dB values or different sample rates shift tone levels. [ASSUMED]  
**Why it happens:** Window coherent gain and single-sided magnitude scaling are easy to omit. [ASSUMED]  
**How to avoid:** Document exact normalization formula and assert deterministic sine levels within explicit tolerance. [VERIFIED: .planning/REQUIREMENTS.md]  
**Warning signs:** 1 kHz test frequency lands correctly but level tolerance fails. [ASSUMED]

### Pitfall 5: WebView Event Flooding
**What goes wrong:** Native emits snapshots faster than WebView can render. [ASSUMED]  
**Why it happens:** FFT hop rate and display rate are conflated. [ASSUMED]  
**How to avoid:** Editor timer emits at profile FPS only when sequence changes; renderer drops stale snapshots naturally. [VERIFIED: 02-CONTEXT.md]  
**Warning signs:** High CPU, delayed UI, or bridge messages piling up. [ASSUMED]

## Code Examples

### Snapshot Payload Shape

```json
{
  "protocolVersion": 1,
  "sequence": 42,
  "profile": "Musical",
  "sampleRate": 48000,
  "fftSize": 4096,
  "minHz": 20,
  "maxHz": 20000,
  "minDb": -96,
  "maxDb": 0,
  "bins": [
    { "hz": 20.0, "db": -96.0, "value": 0.0 },
    { "hz": 1000.0, "db": -6.1, "value": 0.936 }
  ]
}
```

Source: Existing protocol-v1 extension rules require closed schema updates on both sides. [VERIFIED: docs/bridge-protocol.md]

### Test Tone Expectations

```cpp
// Source: DSP-02 requirement.
// Generate sine at exact bin-centered frequency: binIndex * sampleRate / fftSize.
// Assert the maximum display bin center is within its log-bin span and level tolerance is documented.
```

### Renderer Draw Loop

```tsx
// Source: UI-02 requirement.
// Keep React state at snapshot granularity; do imperative canvas drawing inside a hook.
requestAnimationFrame(() => {
  drawFilledSpectrumCurve(context, snapshot.bins, devicePixelRatio);
});
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| UI thread directly reads audio data | Real-time producer publishes bounded immutable snapshots | Locked in Phase 2 context | Avoids blocking and lifetime hazards. [VERIFIED: 02-CONTEXT.md] |
| Many DOM bars | Canvas or bounded SVG path | Locked in Phase 2 context | Satisfies UI-02 and avoids layout churn. [VERIFIED: .planning/REQUIREMENTS.md] |
| Custom WebView JS eval | JUCE native events and typed protocol fixtures | Phase 1 | Snapshot events must extend existing bridge style. [VERIFIED: 01-VERIFICATION.md] |

**Deprecated/outdated:**
- Projucer as source of truth: project uses CMake. [VERIFIED: AGENTS.md]
- Floating dependency updates: project pins dependencies. [VERIFIED: AGENTS.md]
- Vendor “Stereo Mix” capture: out of scope for Phase 2 and disallowed as primary standalone loopback path. [VERIFIED: AGENTS.md]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Canvas is preferred over SVG for this Phase 2 renderer. | Alternatives / Pattern 4 | Planner may choose SVG path if it stays bounded; low product risk. |
| A2 | Suggested profile numeric smoothing/decay/floor/ceiling values are appropriate defaults. | Pattern 2 | Tone/display tests may require tuning before locking values. |
| A3 | 160 log display bins is a good initial grid size. | Pattern 2 | UI may look too coarse/fine; adjust through tests and visual checkpoint. |
| A4 | Double-buffered atomic mailbox is sufficient. | Pattern 3 | Implementation must be carefully reviewed for torn reads; sequence checks may need refinement. |
| A5 | Pitfall warning signs are likely failure signatures. | Common Pitfalls | Verification matrix may need additional host-specific probes. |

## Open Questions

1. **Exact dB tolerance for deterministic tones**
   - What we know: Requirement demands expected frequency and level tolerance. [VERIFIED: .planning/REQUIREMENTS.md]
   - What's unclear: The accepted dB tolerance is not locked. [VERIFIED: 02-CONTEXT.md]
   - Recommendation: Start with ±1.5 dB for display bins and tighter native raw-bin tests where bin-centered tones and coherent gain allow it. [ASSUMED]

2. **pluginval acquisition path**
   - What we know: `pluginval` is missing locally, and official repo documents headless validation. [VERIFIED: environment probe] [CITED: https://github.com/Tracktion/pluginval]
   - What's unclear: Whether the executor may install/download a binary or should require a human-provided path. [ASSUMED]
   - Recommendation: Add a script parameter/env var for pluginval path and a human checkpoint if unavailable. [ASSUMED]

3. **Ableton smoke availability**
   - What we know: Ableton folders exist, but a real usable Ableton Live installation/session was not verified. [VERIFIED: environment probe]
   - What's unclear: Whether the installed copy can load the VST3 and run a smoke test. [ASSUMED]
   - Recommendation: Plan Ableton as preferred manual smoke, then fallback to any available Windows VST3 host without claiming Ableton passed. [VERIFIED: 02-CONTEXT.md]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Windows PowerShell | Scripts | ✓ | 5.1 implied by `powershell.exe`; `--version` not emitted | — |
| PowerShell 7 `pwsh` | None required | ✗ | — | Use Windows PowerShell 5.1. [VERIFIED: 01-VERIFICATION.md] |
| CMake | Native configure/build/test | ✓ | 4.0.3 | — |
| CTest | Native tests | ✓ | 4.0.3 | — |
| Node | UI build/tests | ✓ | v22.18.0 | — |
| npm | UI build/tests | ✓ | 10.9.3 | — |
| Ninja | Alternate presets | ✓ | 1.13.1 | VS2019 preset. |
| pluginval | VST3 validation | ✗ | — | Human-provided path or documented skipped validation. |
| Ableton Live | Preferred DAW smoke | ? | Folder found only | Use best available Windows VST3 host and document fallback. |

**Missing dependencies with no fallback:**
- None for research/planning. [VERIFIED: environment probe]

**Missing dependencies with fallback:**
- `pluginval`: planner should add availability check and skip/fail policy. [VERIFIED: environment probe]
- `pwsh`: scripts should remain Windows PowerShell-compatible. [VERIFIED: 01-VERIFICATION.md]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | CTest/native executable, Vitest 4.1.9, React Testing Library 16.3.2. [VERIFIED: npm registry] |
| Config file | `tests/native/CMakeLists.txt`, `ui/package.json`. [VERIFIED: codebase grep] |
| Quick run command | `ctest --preset vs2019-debug --output-on-failure` and `npm.cmd --prefix ui run test:run`. [VERIFIED: docs/development.md] |
| Full suite command | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1`. [VERIFIED: docs/development.md] |

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DSP-01 | Mono/stereo host blocks accepted and passed through | native unit | `ctest --preset vs2019-debug --output-on-failure` | ❌ Wave 0 |
| DSP-02 | Window/FFT/dB deterministic tones | native unit | `ctest --preset vs2019-debug --output-on-failure` | ❌ Wave 0 |
| DSP-03 | Log bins, smoothing, decay defaults | native unit + UI fixture | `ctest --preset vs2019-debug --output-on-failure`; `npm.cmd --prefix ui run test:run` | ❌ Wave 0 |
| DSP-04 | Silence, denormals, variable/zero blocks, sample-rate changes | native unit | `ctest --preset vs2019-debug --output-on-failure` | ❌ Wave 0 |
| DSP-05 | No prohibited audio-thread work | native review/test + pluginval | `ctest --preset vs2019-debug --output-on-failure`; `pluginval --validate <vst3>` when available | ❌ Wave 0 |
| DSP-06 | Profile/config extension points | native unit | `ctest --preset vs2019-debug --output-on-failure` | ❌ Wave 0 |
| VST3-01 | VST3 loads and shows spectrum in host | pluginval + manual | pluginval command plus Ableton smoke checklist | ❌ Wave 0 |
| VST3-02 | Output equals input | native unit | `ctest --preset vs2019-debug --output-on-failure` | ❌ Wave 0 |
| VST3-03 | Editor lifecycle safe | native/pluginval/manual | pluginval plus DAW smoke | ❌ Wave 0 |
| VST3-04 | Automated validation and DAW smoke | pluginval/manual | pluginval command plus checklist | ❌ Wave 0 |
| UI-02 | Smooth bounded renderer | Vitest + visual smoke | `npm.cmd --prefix ui run test:run`; manual WebView smoke | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `ctest --preset vs2019-debug --output-on-failure` for native changes or `npm.cmd --prefix ui run test:run` for UI-only changes. [VERIFIED: docs/development.md]
- **Per wave merge:** `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1`. [VERIFIED: scripts/test-all.ps1]
- **Phase gate:** Full suite green, pluginval attempted with recorded result, and Ableton-or-fallback DAW smoke recorded before `$gsd-verify-work`. [VERIFIED: 02-CONTEXT.md]

### Wave 0 Gaps

- [ ] `tests/native/SpectrumAnalyzerTests.cpp` — covers DSP-01 through DSP-04 and DSP-06.
- [ ] `tests/native/PluginProcessorTests.cpp` — covers VST3-02 passthrough and edge process blocks.
- [ ] `tests/fixtures/bridge/spectrum-snapshot-v1.json` — covers protocol extension.
- [ ] `ui/src/bridge/SpectrumSnapshot.test.ts` — covers snapshot parser.
- [ ] `ui/src/components/SpectrumCanvas.test.tsx` — covers bounded renderer mount and no per-bin DOM.
- [ ] `scripts/validate-plugin.ps1` — optional pluginval runner with explicit missing-tool behavior.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | Phase 2 has no auth surface. [VERIFIED: .planning/ROADMAP.md] |
| V3 Session Management | no | Phase 2 has no sessions. [VERIFIED: .planning/ROADMAP.md] |
| V4 Access Control | no | Phase 2 has no privileged operations. [VERIFIED: .planning/ROADMAP.md] |
| V5 Input Validation | yes | Validate closed native/web snapshot schema, bounded arrays, finite numbers, and protocol version. [VERIFIED: docs/bridge-protocol.md] |
| V6 Cryptography | no | Licensing crypto is Phase 6. [VERIFIED: .planning/ROADMAP.md] |

### Known Threat Patterns for JUCE/WebView Snapshot Bridge

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed native snapshot payload crashes UI | Tampering / DoS | TypeScript parser rejects wrong version, non-finite values, oversized bin arrays, and bad profile names. [VERIFIED: docs/bridge-protocol.md] |
| Unbounded event payload exhausts WebView memory | DoS | Fixed bin count and bounded string/array lengths. [VERIFIED: docs/bridge-protocol.md] |
| Debug-only smoke hooks leak to Release | Information Disclosure | Preserve Phase 1 Debug-only simulation/smoke behavior. [VERIFIED: 01-VERIFICATION.md] |

## Plan Slicing Guidance

| Plan | Scope | Must Prove |
|------|-------|------------|
| 02-01 | Analyzer config, FFT/window/log mapping, smoothing/decay, native tests | Tone placement/level, silence, profiles, sample rates, block variation. [VERIFIED: .planning/ROADMAP.md] |
| 02-02 | Processor integration and real-time latest-snapshot mailbox | No allocations/locks/WebView in `processBlock`; editor-closed analyzer continues. [VERIFIED: 02-CONTEXT.md] |
| 02-03 | Bridge snapshot event and canvas renderer | Typed protocol fixtures, bounded payload, smooth filled curve, no per-bin DOM. [VERIFIED: 02-CONTEXT.md] |
| 02-04 | Validation and host proof | pluginval attempted/passing when available; Ableton preferred smoke or honest fallback; lifecycle checklist. [VERIFIED: 02-CONTEXT.md] |

## Sources

### Primary (HIGH confidence)
- `/websites/juce_master` - `AudioProcessor::processBlock`, `prepareToPlay`, bus buffers, `dsp::FFT`, `WindowingFunction`. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]
- `/janwilczek/juce-webview-tutorial` - WebView2 compile definitions, binary web assets, and `juce::juce_dsp` linkage when DSP is used. [CITED: https://github.com/janwilczek/juce-webview-tutorial/blob/main/plugin/CMakeLists.txt]
- Project files listed in the user request - current processor/editor/bridge/UI/test/script state. [VERIFIED: codebase grep]

### Secondary (MEDIUM confidence)
- Tracktion/pluginval official GitHub - headless plugin validation and CI use. [CITED: https://github.com/Tracktion/pluginval]
- npm registry - existing UI dependency versions and publish metadata for MUI/Vite/Vitest. [VERIFIED: npm registry]

### Tertiary (LOW confidence)
- Assumed DSP profile numeric constants and canvas-vs-SVG preference; planner should allow small implementation-time tuning. [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Phase 1 pins dependencies and Context7/JUCE docs confirm APIs. [VERIFIED: codebase grep]
- Architecture: HIGH - Phase 2 context locks processor-owned analyzer and latest-snapshot handoff. [VERIFIED: 02-CONTEXT.md]
- Pitfalls: MEDIUM - Core pitfalls are verified by JUCE docs; visual/default tuning remains assumed until tests and checkpoint. [CITED: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html]

**Research date:** 2026-06-23  
**Valid until:** 2026-07-23 for JUCE/CMake architecture; 2026-06-30 for npm/pluginval version details.

## RESEARCH COMPLETE

