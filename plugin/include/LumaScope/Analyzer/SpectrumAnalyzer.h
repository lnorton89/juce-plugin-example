#pragma once

#include "LumaScope/Analyzer/AnalyzerConfig.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>
#include <vector>

namespace lumascope
{
class SpectrumAnalyzer
{
public:
    explicit SpectrumAnalyzer (AnalyzerConfig config = makeAnalyzerConfig());

    const AnalyzerConfig& getConfig() const noexcept { return config; }
    void prepare (double newSampleRate);
    void pushAudioBlock (const juce::AudioBuffer<float>& block) noexcept;
    bool copyLatestSnapshot (SpectrumSnapshot& destination) const noexcept;

private:
    void processFrame() noexcept;
    void buildLogBins();
    float sanitizeSample (float sample) const noexcept;
    float rawMagnitudeToDecibels (float magnitude, std::size_t fftBin) const noexcept;
    float smoothDecibels (std::size_t displayBin, float decibels) noexcept;

    AnalyzerConfig config;
    double sampleRate = 48000.0;
    int fftOrder = 12;
    float coherentGain = 0.5f;
    std::uint32_t nextSequence = 1;
    std::size_t fifoPosition = 0;
    bool hasProcessedFrame = false;
    bool hasSnapshot = false;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> smoothedDecibels;
    std::vector<std::size_t> logBinStarts;
    std::vector<std::size_t> logBinEnds;
    std::vector<float> logBinFrequencies;
    SpectrumSnapshot latestSnapshot;
};
}
