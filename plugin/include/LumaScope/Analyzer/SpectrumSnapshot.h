#pragma once

#include "LumaScope/Analyzer/AnalyzerConfig.h"

#include <array>
#include <cstdint>

namespace lumascope
{
struct SpectrumBin
{
    float frequencyHz = 0.0f;
    float decibels = -96.0f;
    float normalisedValue = 0.0f;
};

struct SpectrumSnapshot
{
    static constexpr std::size_t maxBins = 256;

    std::uint32_t sequence = 0;
    AnalyzerProfile profile = AnalyzerProfile::Musical;
    double sampleRate = 0.0;
    std::size_t fftSize = 0;
    double minFrequencyHz = 20.0;
    double maxFrequencyHz = 20000.0;
    float minDecibels = -96.0f;
    float maxDecibels = 0.0f;
    std::size_t binCount = 0;
    std::array<SpectrumBin, maxBins> bins {};
};
}
