#include "LumaScope/Analyzer/AnalyzerConfig.h"
#include "LumaScope/Analyzer/SpectrumAnalyzer.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace
{
int failures = 0;

void expect (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear (double actual, double expected, double tolerance, const char* message)
{
    if (std::abs (actual - expected) > tolerance)
    {
        std::cerr << "FAIL: " << message << " actual=" << actual
                  << " expected=" << expected << " tolerance=" << tolerance << '\n';
        ++failures;
    }
}

void testAnalyzerProfiles()
{
    using lumascope::AnalyzerProfile;
    using lumascope::WindowFunction;

    const auto defaultConfig = lumascope::makeAnalyzerConfig();
    const auto musical = lumascope::makeAnalyzerConfig (AnalyzerProfile::Musical);
    const auto measurement = lumascope::makeAnalyzerConfig (AnalyzerProfile::Measurement);
    const auto fast = lumascope::makeAnalyzerConfig (AnalyzerProfile::Fast);

    expect (defaultConfig.profile == AnalyzerProfile::Musical, "Musical is the default profile");
    expect (musical.fftSize == 4096, "Musical uses a 4096-point FFT");
    expect (musical.window == WindowFunction::Hann, "Musical uses a Hann window");
    expectNear (musical.minFrequencyHz, 20.0, 0.001, "Musical minimum frequency is 20 Hz");
    expectNear (musical.maxFrequencyHz, 20000.0, 0.001, "Musical maximum frequency is 20 kHz");
    expectNear (musical.snapshotRateHz, 30.0, 0.001, "Musical snapshot cadence is about 30 FPS");
    expect (musical.smoothingAttack > musical.smoothingRelease, "Musical has moderate attack/release smoothing");
    expect (musical.displayBinCount > 0, "Musical exposes a display bin count");

    expect (measurement.profile == AnalyzerProfile::Measurement, "Measurement profile exists");
    expect (fast.profile == AnalyzerProfile::Fast, "Fast profile exists");
    expect (measurement.fftSize > musical.fftSize, "Measurement uses a larger FFT than Musical");
    expect (fast.fftSize < musical.fftSize, "Fast uses a smaller FFT than Musical");
    expect (measurement.snapshotRateHz < musical.snapshotRateHz, "Measurement updates slower than Musical");
    expect (fast.snapshotRateHz > musical.snapshotRateHz, "Fast updates faster than Musical");
    expect (measurement.minDecibels < musical.minDecibels, "Measurement has a wider dB floor");
    expect (fast.smoothingAttack > musical.smoothingAttack, "Fast has a faster attack path");
}

void testSpectrumSnapshotContract()
{
    const auto config = lumascope::makeAnalyzerConfig();
    lumascope::SpectrumSnapshot snapshot;

    snapshot.sequence = 17;
    snapshot.profile = config.profile;
    snapshot.sampleRate = 48000.0;
    snapshot.fftSize = config.fftSize;
    snapshot.minFrequencyHz = config.minFrequencyHz;
    snapshot.maxFrequencyHz = config.maxFrequencyHz;
    snapshot.minDecibels = config.minDecibels;
    snapshot.maxDecibels = config.maxDecibels;
    snapshot.binCount = config.displayBinCount;
    snapshot.bins[0] = { 20.0f, -96.0f, 0.0f };
    snapshot.bins[1] = { 1000.0f, -6.0f, 0.9375f };

    expect (lumascope::SpectrumSnapshot::maxBins >= config.displayBinCount, "snapshot capacity contains profile bins");
    expect (snapshot.binCount <= lumascope::SpectrumSnapshot::maxBins, "snapshot bin count is bounded");
    expect (snapshot.sequence == 17, "snapshot carries sequence metadata");
    expect (snapshot.profile == lumascope::AnalyzerProfile::Musical, "snapshot carries profile metadata");
    expect (std::isfinite (snapshot.sampleRate), "snapshot sample rate is finite");
    expect (std::isfinite (snapshot.bins[0].frequencyHz), "snapshot bin frequency is finite");
    expect (std::isfinite (snapshot.bins[0].decibels), "snapshot bin dB is finite");
    expect (std::isfinite (snapshot.bins[0].normalisedValue), "snapshot bin value is finite");
    expect (snapshot.bins[1].normalisedValue >= 0.0f && snapshot.bins[1].normalisedValue <= 1.0f,
            "snapshot value is bounded");
}

bool pullSnapshot (lumascope::SpectrumAnalyzer& analyzer, lumascope::SpectrumSnapshot& snapshot)
{
    return analyzer.copyLatestSnapshot (snapshot);
}

void pushConstantBlock (lumascope::SpectrumAnalyzer& analyzer,
                        int channels,
                        int samples,
                        float value)
{
    juce::AudioBuffer<float> buffer (channels, samples);
    buffer.clear();

    for (int channel = 0; channel < channels; ++channel)
        buffer.fill (channel, 0, samples, value);

    analyzer.pushAudioBlock (buffer);
}

void pushSineBlocks (lumascope::SpectrumAnalyzer& analyzer,
                     double sampleRate,
                     double frequency,
                     int channels,
                     int totalSamples,
                     const std::vector<int>& blockSizes)
{
    double phase = 0.0;
    const auto increment = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    auto remaining = totalSamples;
    std::size_t blockIndex = 0;

    while (remaining > 0)
    {
        const auto requested = blockSizes[blockIndex++ % blockSizes.size()];
        const auto samples = std::min (requested, remaining);
        juce::AudioBuffer<float> buffer (channels, samples);

        for (int sample = 0; sample < samples; ++sample)
        {
            const auto value = static_cast<float> (std::sin (phase));
            phase += increment;

            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample (channel, sample, value);
        }

        analyzer.pushAudioBlock (buffer);
        remaining -= samples;
    }
}

lumascope::SpectrumBin findPeakBin (const lumascope::SpectrumSnapshot& snapshot)
{
    auto peak = snapshot.bins[0];

    for (std::size_t index = 1; index < snapshot.binCount; ++index)
        if (snapshot.bins[index].decibels > peak.decibels)
            peak = snapshot.bins[index];

    return peak;
}

void expectFiniteSnapshot (const lumascope::SpectrumSnapshot& snapshot, const lumascope::AnalyzerConfig& config)
{
    expect (snapshot.binCount == config.displayBinCount, "snapshot contains configured display bins");

    for (std::size_t index = 0; index < snapshot.binCount; ++index)
    {
        const auto& bin = snapshot.bins[index];
        expect (std::isfinite (bin.frequencyHz), "bin frequency is finite");
        expect (std::isfinite (bin.decibels), "bin decibels are finite");
        expect (std::isfinite (bin.normalisedValue), "bin value is finite");
        expect (bin.decibels >= config.minDecibels && bin.decibels <= config.maxDecibels, "bin decibels are bounded");
        expect (bin.normalisedValue >= 0.0f && bin.normalisedValue <= 1.0f, "bin value is bounded");
    }
}

void testSilenceAndDenormals()
{
    const auto config = lumascope::makeAnalyzerConfig();
    lumascope::SpectrumAnalyzer analyzer (config);
    analyzer.prepare (48000.0);

    pushConstantBlock (analyzer, 2, static_cast<int> (config.fftSize), 0.0f);

    lumascope::SpectrumSnapshot snapshot;
    expect (pullSnapshot (analyzer, snapshot), "silence produces a snapshot");
    expectFiniteSnapshot (snapshot, config);

    for (std::size_t index = 0; index < snapshot.binCount; ++index)
        expectNear (snapshot.bins[index].decibels, config.minDecibels, 0.001, "silence lands at the configured floor");

    analyzer.prepare (48000.0);
    pushConstantBlock (analyzer, 1, static_cast<int> (config.fftSize), std::numeric_limits<float>::denorm_min());
    expect (pullSnapshot (analyzer, snapshot), "denormal-sized input produces a snapshot");
    expectFiniteSnapshot (snapshot, config);
}

void testBinCenteredTonePlacement()
{
    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        const auto config = lumascope::makeAnalyzerConfig (lumascope::AnalyzerProfile::Musical);
        const auto binIndex = static_cast<int> (std::round (1000.0 * static_cast<double> (config.fftSize) / sampleRate));
        const auto frequency = static_cast<double> (binIndex) * sampleRate / static_cast<double> (config.fftSize);

        lumascope::SpectrumAnalyzer monoAnalyzer (config);
        monoAnalyzer.prepare (sampleRate);
        pushSineBlocks (monoAnalyzer, sampleRate, frequency, 1, static_cast<int> (config.fftSize), { 257, 509, 1024 });

        lumascope::SpectrumSnapshot monoSnapshot;
        expect (pullSnapshot (monoAnalyzer, monoSnapshot), "mono tone produces a snapshot");
        expectFiniteSnapshot (monoSnapshot, config);

        const auto monoPeak = findPeakBin (monoSnapshot);
        expect (std::abs (monoPeak.frequencyHz - static_cast<float> (frequency)) / static_cast<float> (frequency) < 0.06f,
                "mono peak is in the expected log-frequency bin");
        expectNear (monoPeak.decibels, 0.0, 1.5, "mono full-scale bin-centered sine is near 0 dBFS");

        lumascope::SpectrumAnalyzer stereoAnalyzer (config);
        stereoAnalyzer.prepare (sampleRate);
        pushSineBlocks (stereoAnalyzer, sampleRate, frequency, 2, static_cast<int> (config.fftSize), { 64, 333, 777, 2048 });

        lumascope::SpectrumSnapshot stereoSnapshot;
        expect (pullSnapshot (stereoAnalyzer, stereoSnapshot), "stereo tone produces a snapshot");
        const auto stereoPeak = findPeakBin (stereoSnapshot);
        expect (std::abs (stereoPeak.frequencyHz - static_cast<float> (frequency)) / static_cast<float> (frequency) < 0.06f,
                "stereo peak is in the expected log-frequency bin");
        expectNear (stereoPeak.decibels, 0.0, 1.5, "stereo full-scale bin-centered sine is near 0 dBFS");
    }
}

void testVariableBlocksAndSampleRateChanges()
{
    auto config = lumascope::makeAnalyzerConfig (lumascope::AnalyzerProfile::Fast);
    lumascope::SpectrumAnalyzer analyzer (config);
    analyzer.prepare (44100.0);
    pushConstantBlock (analyzer, 2, 0, 0.0f);
    pushSineBlocks (analyzer, 44100.0, 689.0625, 2, static_cast<int> (config.fftSize * 2), { 0, 1, 13, 2049, 97 });

    lumascope::SpectrumSnapshot snapshot;
    expect (pullSnapshot (analyzer, snapshot), "variable block sequence produces a snapshot");
    expectFiniteSnapshot (snapshot, config);
    expectNear (snapshot.sampleRate, 44100.0, 0.001, "snapshot records first sample rate");

    analyzer.prepare (96000.0);
    pushSineBlocks (analyzer, 96000.0, 937.5, 1, static_cast<int> (config.fftSize), { 4096 });
    expect (pullSnapshot (analyzer, snapshot), "sample-rate reset produces a new snapshot");
    expectFiniteSnapshot (snapshot, config);
    expectNear (snapshot.sampleRate, 96000.0, 0.001, "snapshot records changed sample rate");
}

void testSmoothingDecay()
{
    auto config = lumascope::makeAnalyzerConfig (lumascope::AnalyzerProfile::Musical);
    lumascope::SpectrumAnalyzer analyzer (config);
    analyzer.prepare (48000.0);

    pushSineBlocks (analyzer, 48000.0, 984.375, 2, static_cast<int> (config.fftSize), { static_cast<int> (config.fftSize) });

    lumascope::SpectrumSnapshot loudSnapshot;
    expect (pullSnapshot (analyzer, loudSnapshot), "loud tone produces a snapshot");
    const auto loudPeak = findPeakBin (loudSnapshot);

    pushConstantBlock (analyzer, 2, static_cast<int> (config.fftSize), 0.0f);
    lumascope::SpectrumSnapshot firstDecaySnapshot;
    expect (pullSnapshot (analyzer, firstDecaySnapshot), "first silence frame produces a decay snapshot");
    const auto firstDecayPeak = findPeakBin (firstDecaySnapshot);

    pushConstantBlock (analyzer, 2, static_cast<int> (config.fftSize), 0.0f);
    lumascope::SpectrumSnapshot secondDecaySnapshot;
    expect (pullSnapshot (analyzer, secondDecaySnapshot), "second silence frame produces a decay snapshot");
    const auto secondDecayPeak = findPeakBin (secondDecaySnapshot);

    expect (firstDecayPeak.decibels <= loudPeak.decibels, "decay does not rise after silence");
    expect (secondDecayPeak.decibels <= firstDecayPeak.decibels, "decay is monotonic across silence frames");
    expect (secondDecayPeak.decibels >= config.minDecibels, "decay remains above the configured floor");
}
}

int runSpectrumAnalyzerTests()
{
    testAnalyzerProfiles();
    testSpectrumSnapshotContract();
    testSilenceAndDenormals();
    testBinCenteredTonePlacement();
    testVariableBlocksAndSampleRateChanges();
    testSmoothingDecay();
    return failures;
}
