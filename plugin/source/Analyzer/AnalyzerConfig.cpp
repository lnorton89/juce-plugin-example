#include "LumaScope/Analyzer/AnalyzerConfig.h"

#include <cmath>

namespace lumascope
{
namespace
{
constexpr bool isPowerOfTwo (std::size_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}
}

AnalyzerConfig makeAnalyzerConfig (AnalyzerProfile profile) noexcept
{
    AnalyzerConfig config;
    config.profile = profile;

    switch (profile)
    {
        case AnalyzerProfile::Musical:
            config.fftSize = 4096;
            config.hopSize = 1024;
            config.snapshotRateHz = 45.0;
            config.minDecibels = -96.0f;
            config.smoothingAttack = 0.72f;
            config.smoothingRelease = 0.32f;
            break;

        case AnalyzerProfile::Measurement:
            config.fftSize = 8192;
            config.hopSize = 2048;
            config.snapshotRateHz = 20.0;
            config.minDecibels = -120.0f;
            config.smoothingAttack = 0.30f;
            config.smoothingRelease = 0.06f;
            break;

        case AnalyzerProfile::Fast:
            config.fftSize = 2048;
            config.hopSize = 512;
            config.snapshotRateHz = 60.0;
            config.minDecibels = -90.0f;
            config.smoothingAttack = 0.85f;
            config.smoothingRelease = 0.25f;
            break;
    }

    return config;
}

bool isValidAnalyzerConfig (const AnalyzerConfig& config) noexcept
{
    return isPowerOfTwo (config.fftSize)
        && config.fftSize >= 512
        && config.fftSize <= 16384
        && config.hopSize > 0
        && config.hopSize <= config.fftSize
        && config.window == WindowFunction::Hann
        && std::isfinite (config.minFrequencyHz)
        && std::isfinite (config.maxFrequencyHz)
        && config.minFrequencyHz >= 1.0
        && config.maxFrequencyHz > config.minFrequencyHz
        && config.snapshotRateHz > 0.0
        && config.snapshotRateHz <= 120.0
        && config.minDecibels < config.maxDecibels
        && config.minDecibels >= -160.0f
        && config.maxDecibels <= 24.0f
        && config.smoothingAttack >= 0.0f
        && config.smoothingAttack <= 1.0f
        && config.smoothingRelease >= 0.0f
        && config.smoothingRelease <= 1.0f
        && config.displayBinCount > 0
        && config.displayBinCount <= 256;
}

const char* toString (AnalyzerProfile profile) noexcept
{
    switch (profile)
    {
        case AnalyzerProfile::Musical: return "Musical";
        case AnalyzerProfile::Measurement: return "Measurement";
        case AnalyzerProfile::Fast: return "Fast";
    }

    return "Musical";
}
}
