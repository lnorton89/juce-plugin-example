# Analyzer DSP contract

Phase 2 introduces the native spectrum-analysis core only. It observes mono or
stereo floating-point audio, produces bounded spectrum snapshots, and does not
own audio passthrough, WebView delivery, standalone capture, licensing, or v2
user controls.

## Profiles

`AnalyzerProfile::Musical` is the default visible profile for Phase 2.

| Profile | FFT size | Hop size | Window | Range | Snapshot target | Floor | Smoothing |
| --- | ---: | ---: | --- | --- | ---: | ---: | --- |
| Musical | 4096 | 1024 | Hann | 20 Hz to 20 kHz | 45 Hz | -96 dB | attack 0.72, release 0.32 |
| Measurement | 8192 | 2048 | Hann | 20 Hz to 20 kHz | 20 Hz | -120 dB | attack 0.30, release 0.06 |
| Fast | 2048 | 512 | Hann | 20 Hz to 20 kHz | 60 Hz | -90 dB | attack 0.85, release 0.25 |

All profiles currently use 160 logarithmic display bins and a 0 dB ceiling. The
profile/config model intentionally exposes FFT size, hop size, window, display
range, snapshot cadence, dB bounds, smoothing attack/release, and display bin
count so future controls can reuse the same DSP core. User-facing controls
remain v2 scope.

## FFT And Normalization

The analyzer uses overlapping FFT windows. `hopSize` controls how many new input
samples advance between frames; the retained tail of the previous window is
reused for the next analysis frame. The Musical default keeps a 4096-point FFT
for useful low-frequency resolution while publishing a new analysis frame every
1024 samples when the host provides enough audio.

Each frame uses JUCE `juce::dsp::WindowingFunction<float>` with a Hann window,
then `juce::dsp::FFT::performFrequencyOnlyForwardTransform` on a zero-padded
buffer sized at twice the FFT size.

Magnitude normalization is single-sided:

```text
linear_amplitude = (2 * fft_magnitude) / (fft_size * hann_coherent_gain)
decibels = 20 * log10(max(linear_amplitude, 1e-12))
decibels = clamp(decibels, minDecibels, maxDecibels)
```

The current Hann coherent gain is treated as 0.5. A full-scale bin-centered sine
therefore lands near 0 dBFS after windowing and single-sided normalization. The
native tests allow +/-1.5 dB for display-bin level tolerance because the visible
log bins aggregate FFT bins and smoothing is part of the snapshot contract.

## Log Bins

Snapshots are display-ready. They do not expose raw FFT bins. Each configured
display bin covers a logarithmic span between the profile minimum and the lower
of the profile maximum or Nyquist. Very narrow display bins interpolate the FFT
magnitude at the logarithmic center frequency so the low end does not repeat the
same FFT bin across several visible points. Wider display bins use the maximum
FFT-bin magnitude inside their span so narrow peaks remain visible. The bin
stores:

- `frequencyHz`: the logarithmic center frequency.
- `decibels`: the interpolated or span-maximum clamped dB value for that bin.
- `normalisedValue`: the dB value mapped to 0..1 between the profile floor and
  ceiling.

The snapshot model has a fixed capacity of 256 bins. Current profiles use 160
bins, leaving room for future profile changes without changing the bridge shape.

## Smoothing And Decay

The first completed FFT frame is published without smoothing so deterministic
tone tests can validate normalization directly. Later frames use one-pole
smoothing per display bin:

```text
coefficient = rawDb > previousDb ? smoothingAttack : smoothingRelease
smoothedDb = previousDb + coefficient * (rawDb - previousDb)
```

The result is clamped to the configured dB range. Silence and denormal-sized
inputs resolve to the profile floor and never publish NaN or infinity.

## Test Tolerances

Native tests cover:

- Musical defaults and distinct Measurement/Fast profiles.
- Bounded finite snapshot metadata and bins.
- Silence and denormal-sized input at the configured floor.
- Bin-centered mono and stereo sine tones at 44.1, 48, and 96 kHz.
- Frequency placement within the expected logarithmic display bin.
- Full-scale sine level within +/-1.5 dB of 0 dBFS.
- Zero-sized, small, changing, and larger-than-FFT block sequences.
- Sample-rate reset behavior.
- Overlapping hop publication after the first full FFT window.
- Monotonic responsive decay after signal stops.

These tolerances document DSP-02 and DSP-03 behavior without requiring another
developer to reverse-engineer the implementation.

## Phase Boundaries

This plan does not connect the analyzer to `AudioProcessor::processBlock`, emit
WebView bridge events, render a canvas spectrum, validate VST3 hosts, add
standalone input or WASAPI loopback capture, or add licensing UI. Those are owned
by later Phase 2 plans, Phase 3, and Phase 6.
