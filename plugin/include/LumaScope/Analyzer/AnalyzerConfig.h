#pragma once

#include <cstddef>
#include <cstdint>

namespace lumascope
{
enum class AnalyzerProfile : std::uint8_t
{
    Musical,
    Measurement,
    Fast
};

enum class WindowFunction : std::uint8_t
{
    Hann
};

struct AnalyzerConfig
{
    AnalyzerProfile profile = AnalyzerProfile::Musical;
    std::size_t fftSize = 4096;
    std::size_t hopSize = 1024;
    WindowFunction window = WindowFunction::Hann;
    double minFrequencyHz = 20.0;
    double maxFrequencyHz = 20000.0;
    double snapshotRateHz = 30.0;
    float minDecibels = -96.0f;
    float maxDecibels = 0.0f;
    float smoothingAttack = 0.45f;
    float smoothingRelease = 0.12f;
    std::size_t displayBinCount = 160;
};

AnalyzerConfig makeAnalyzerConfig (AnalyzerProfile profile = AnalyzerProfile::Musical) noexcept;
bool isValidAnalyzerConfig (const AnalyzerConfig& config) noexcept;
const char* toString (AnalyzerProfile profile) noexcept;
}
