# Feature Research

**Domain:** Spectrum analyzer with commercial desktop activation
**Researched:** 2026-06-22
**Confidence:** HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Correct log-frequency spectrum | Musical frequency inspection requires readable low/mid/high spacing | MEDIUM | Windowed FFT, dB conversion, calibrated floor/ceiling |
| Smooth, stable display | Raw FFT frames flicker and are hard to read | MEDIUM | Sensible attack/release or temporal smoothing defaults |
| Stereo-safe analysis | Plug-in and devices commonly supply stereo | MEDIUM | Explicit channel mix policy; never alter output |
| Host-safe VST3 pass-through | Analyzer must not change audio or destabilize a DAW | HIGH | Preserve buses, silence handling, variable block sizes |
| Standalone input selection | Users need microphone/interface monitoring | MEDIUM | Device list, permissions/errors, persisted selection |
| Standalone system-output capture | User explicitly needs Windows playback monitoring | HIGH | Native shared-mode WASAPI loopback adapter |
| Resizable, clear UI | Plug-in windows vary across hosts and DPI scales | MEDIUM | Responsive MUI shell; graphics renderer owns spectrum |
| Activation/status/deactivation | Paid software must explain entitlement state | HIGH | One machine, transfer, actionable errors |
| Offline launch | DAW use cannot depend on constant connectivity | HIGH | Signed local entitlement with seven-day grace |
| Reproducible cloud setup | Example must transfer between accounts | HIGH | Bootstrap, migration, deploy, verify, teardown documentation |

### Differentiators

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Transparent reference architecture | Teaches boundaries rather than hiding them in a monolith | MEDIUM | Architecture docs and focused modules/tests |
| One-command-ish Cloudflare bootstrap | Makes licensing demo genuinely reusable | HIGH | Automate D1 creation/config overlay/migrations/deploy |
| Native and web contract fixtures | Prevents bridge drift | MEDIUM | Typed messages and contract tests |
| Deterministic DSP tests | Demonstrates analyzer correctness | MEDIUM | Sine peaks, silence, multi-rate, block-size tests |
| Graceful WebView failure state | Avoids blank plug-in windows | MEDIUM | Native fallback diagnostics/activation affordance |

### Anti-Features

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Every analyzer control in v1 | Feels “complete” | Bloats bridge/state/test surface before core validity | Internal configuration with curated defaults and extension points |
| Hardware-ID absolutism | Appears to stop sharing | IDs change after upgrades and create support burden | Versioned privacy-conscious fingerprint plus transfer/recovery path |
| Always-online validation | Strong-looking enforcement | Breaks DAW sessions and makes outages product outages | Signed local token and bounded revalidation grace |
| Cloud dashboard click-through setup | Fast for original author | Cannot be reproduced or audited | Versioned config and bootstrap scripts |
| License checks inside `processBlock` | Immediate enforcement | Violates real-time guarantees | Resolve entitlement before audio starts; atomic status reads only |

## Feature Dependencies

```text
Reproducible native build
  -> DSP analyzer core
     -> VST3 pass-through integration
     -> standalone input capture
        -> WASAPI loopback capture
  -> WebView bridge contract
     -> React/MUI analyzer UI

Cloudflare bootstrap + D1 migrations
  -> signed Lemon webhook ingestion
  -> activation/deactivation/validation API
     -> C++ entitlement verification/storage
        -> activation UI and offline grace
```

## MVP Definition

### Launch With (v1)

- [ ] Windows VST3 and standalone builds from a clean checkout
- [ ] Correct, smooth spectrum with tested defaults and audio pass-through
- [ ] Input-device and WASAPI-loopback standalone capture
- [ ] React/TypeScript/MUI WebView UI with typed bridge
- [ ] One-machine activation, deactivation/transfer, signed local entitlement, seven-day offline grace
- [ ] Portable Cloudflare bootstrap/deploy/migrate/verify workflow
- [ ] Automated tests and manual host/device/security verification guides

### Add After Validation (v1.x)

- [ ] User-facing FFT/smoothing/decay/range controls
- [ ] Peak hold, freeze, and cursor readout
- [ ] Installer/signing pipeline
- [ ] Lost-machine administrative recovery tooling

### Future Consideration (v2+)

- [ ] macOS/AU and additional formats
- [ ] Multiple activation tiers
- [ ] Account portal and fleet administration
- [ ] Advanced metering, spectrogram, or comparison traces

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Analyzer correctness | HIGH | MEDIUM | P1 |
| VST3 host safety | HIGH | HIGH | P1 |
| Input + loopback standalone capture | HIGH | HIGH | P1 |
| WebView/MUI UI | HIGH | MEDIUM | P1 |
| Activation/offline transfer | HIGH | HIGH | P1 |
| Portable Cloudflare provisioning | HIGH | HIGH | P1 |
| Advanced analyzer controls | MEDIUM | MEDIUM | P2 |
| Cross-platform formats | MEDIUM | HIGH | P3 |

## Sources

- User-defined product behavior and activation diagram
- Context7 JUCE, WebView tutorial, and Material UI libraries
- [Microsoft WASAPI loopback recording](https://learn.microsoft.com/windows/win32/coreaudio/loopback-recording)
- [Lemon Squeezy License API](https://docs.lemonsqueezy.com/api/license-api)

---
*Feature research for: JUCE Spectrum Analyzer Example*
*Researched: 2026-06-22*
