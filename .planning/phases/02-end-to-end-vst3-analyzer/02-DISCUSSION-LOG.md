# Phase 2: End-to-End VST3 Analyzer - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-23
**Phase:** 2-End-to-End VST3 Analyzer
**Areas discussed:** DSP defaults, audio-to-UI handoff, spectrum renderer, VST3 host proof, passthrough correctness, editor lifecycle

---

## Gray Area Selection

| Option | Description | Selected |
|--------|-------------|----------|
| All areas | Clarify DSP defaults, audio-to-UI handoff, renderer behavior, and VST3 validation in one pass. | ✓ |
| DSP core only | Focus on FFT/window/normalization/smoothing choices; let planner decide renderer and validation details. | |
| UI + host proof | Focus on renderer behavior, DAW/pluginval proof, and lifecycle expectations; let planner decide DSP internals. | |

**User's choice:** All areas.
**Notes:** The user wanted all major Phase 2 analyzer decisions covered before planning.

---

## DSP Defaults

| Option | Description | Selected |
|--------|-------------|----------|
| Musical real-time analyzer | Smooth, stable, useful for mixing: Hann window, sensible FFT size, log frequency bins, decay/averaging so it does not flicker wildly. | partial |
| Measurement-style analyzer | More literal/technical: tighter level/frequency accuracy, less smoothing, more lab-instrument feel. | partial |
| Fast visual meter | Lower latency and more motion, less concern about exact dB/frequency presentation. | partial |

**User's choice:** All three as separate modes.
**Notes:** Captured as `Musical`, `Measurement`, and `Fast` preset profiles. The visible default remains `Musical` unless future UI controls expose switching.

---

## Musical Preset Values

| Option | Description | Selected |
|--------|-------------|----------|
| Balanced/default | 4096 FFT, Hann window, ~20 Hz-20 kHz, ~30 FPS UI snapshots, moderate smoothing/decay. | ✓ |
| More detailed lows | 8192 FFT, better bass resolution, a little more latency and CPU. | |
| Faster response | 2048 FFT, snappier motion, rougher low-frequency accuracy. | |
| You decide | Let the agent lock a balanced default. | |

**User's choice:** Balanced/default.
**Notes:** This becomes the locked default `Musical` profile direction.

---

## Audio-to-UI Handoff

| Option | Description | Selected |
|--------|-------------|----------|
| Latest snapshot wins | Audio/DSP publishes into a preallocated handoff; UI consumes the latest available snapshot and drops stale frames. | ✓ |
| Small queue | Keep a short queue of snapshots for smoother motion, with backlog/staleness risk. | |
| UI pulls on timer | Editor asks for snapshots at display rate, requiring care to avoid audio-thread coupling. | |
| You decide | Let planner decide. | |

**User's choice:** Latest snapshot wins.
**Notes:** Real-time safety is the controlling priority. Staleness is worse than dropped visual frames.

---

## Spectrum Renderer

| Option | Description | Selected |
|--------|-------------|----------|
| Filled spectral curve | Smooth line with subtle fill/glow, professional audio-tool feel, matches the dark/cyan/lime LumaScope motif. | ✓ |
| Bar spectrum | Classic FFT bars, easier to read per band, more meter-plugin feel. | |
| Thin technical trace | Minimal line graph with grid labels, more measurement-oriented and restrained. | |
| You decide | Let planner decide. | |

**User's choice:** Filled spectral curve.
**Notes:** Renderer should be efficient and avoid one DOM/MUI element per bin.

---

## VST3 Host Proof

| Option | Description | Selected |
|--------|-------------|----------|
| REAPER | Lightweight, common for plugin testing, easy to document. | |
| Ableton Live | Strong real-world Windows DAW target if installed. | ✓ |
| FL Studio | Common Windows DAW target if it is the user's main host. | |
| Use whatever is installed | Detect installed DAWs and pick the best available, with REAPER as preferred documented target. | |

**User's choice:** Ableton Live.
**Notes:** Ableton Live is preferred for the manual DAW smoke target. If unavailable, the executor should document the limitation honestly and use the best available fallback.

---

## Passthrough Correctness

| Option | Description | Selected |
|--------|-------------|----------|
| Sample-equivalent passthrough | Output equals input within floating-point tolerance for supported mono/stereo layouts. | ✓ |
| Bit-exact where possible | Try for exact equality in simple cases, with tolerance only where host/layout conversion makes exactness unrealistic. | |
| Audibly unchanged | Practical manual standard, less strict than automated tests. | |
| You decide | Let planner decide. | |

**User's choice:** Sample-equivalent passthrough.
**Notes:** Analyzer observes audio only and must not change samples, gain, latency, or channel layout.

---

## Editor Lifecycle

| Option | Description | Selected |
|--------|-------------|----------|
| Analyzer keeps running, UI catches up on reopen | DSP/snapshot pipeline continues safely without WebView; reopening shows the latest spectrum quickly. | ✓ |
| Pause UI-facing snapshots while closed | Analysis can keep internal state, but no UI snapshots are prepared until the editor exists. | |
| Stop analyzer while editor is closed | Simplest, but weaker as a VST3 lifecycle proof. | |
| You decide | Let planner decide. | |

**User's choice:** Analyzer keeps running, UI catches up on reopen.
**Notes:** This gives the phase a stronger VST3 lifecycle proof and keeps DSP/editor ownership properly separated.

---

## Agent's Discretion

- Exact `Measurement` and `Fast` preset values.
- Exact FFT overlap/hop strategy, dB display floor/ceiling, interpolation, and log-bin count.
- Exact snapshot payload shape and renderer technology.
- Exact DAW fallback order if Ableton Live is unavailable.

## Deferred Ideas

- Full user-facing analyzer controls remain v2 `CTRL-*` scope.
- Standalone source selection and WASAPI loopback remain Phase 3.
- Licensing/activation UI remains Phase 6.
