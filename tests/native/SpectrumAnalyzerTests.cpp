#include "LumaScope/Analyzer/AnalyzerConfig.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"

#include <cmath>
#include <iostream>
#include <string_view>

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
}

int runSpectrumAnalyzerTests()
{
    testAnalyzerProfiles();
    testSpectrumSnapshotContract();
    return failures;
}
