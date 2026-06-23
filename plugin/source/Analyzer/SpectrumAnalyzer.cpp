#include "LumaScope/Analyzer/SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lumascope
{
namespace
{
int fftOrderForSize (std::size_t fftSize) noexcept
{
    auto order = 0;
    auto size = std::size_t { 1 };

    while (size < fftSize)
    {
        size <<= 1;
        ++order;
    }

    return order;
}

float clamp (float value, float lower, float upper) noexcept
{
    return std::min (std::max (value, lower), upper);
}

float decibelsToNormalised (float decibels, const AnalyzerConfig& config) noexcept
{
    return clamp ((decibels - config.minDecibels) / (config.maxDecibels - config.minDecibels), 0.0f, 1.0f);
}

float safeLogFrequency (double minHz, double maxHz, double ratio) noexcept
{
    return static_cast<float> (minHz * std::pow (maxHz / minHz, ratio));
}
}

SpectrumAnalyzer::SpectrumAnalyzer (AnalyzerConfig analyzerConfig)
    : config (isValidAnalyzerConfig (analyzerConfig) ? analyzerConfig : makeAnalyzerConfig())
{
    prepare (sampleRate);
}

void SpectrumAnalyzer::prepare (double newSampleRate)
{
    if (! std::isfinite (newSampleRate) || newSampleRate <= 0.0)
        newSampleRate = 48000.0;

    sampleRate = newSampleRate;
    fftOrder = fftOrderForSize (config.fftSize);
    coherentGain = 0.5f;
    fifoPosition = 0;
    hasProcessedFrame = false;
    hasSnapshot = false;

    fft = std::make_unique<juce::dsp::FFT> (fftOrder);
    window = std::make_unique<juce::dsp::WindowingFunction<float>> (
        config.fftSize,
        juce::dsp::WindowingFunction<float>::hann,
        false);

    fifo.assign (config.fftSize, 0.0f);
    fftData.assign (config.fftSize * 2, 0.0f);
    fftMagnitudes.assign ((config.fftSize / 2) + 1, 0.0f);
    smoothedDecibels.assign (config.displayBinCount, config.minDecibels);
    buildLogBins();

    latestSnapshot = {};
    latestSnapshot.profile = config.profile;
    latestSnapshot.sampleRate = sampleRate;
    latestSnapshot.fftSize = config.fftSize;
    latestSnapshot.minFrequencyHz = config.minFrequencyHz;
    latestSnapshot.maxFrequencyHz = config.maxFrequencyHz;
    latestSnapshot.minDecibels = config.minDecibels;
    latestSnapshot.maxDecibels = config.maxDecibels;
    latestSnapshot.binCount = config.displayBinCount;
}

void SpectrumAnalyzer::pushAudioBlock (const juce::AudioBuffer<float>& block) noexcept
{
    const auto channels = block.getNumChannels();
    const auto samples = block.getNumSamples();

    if (channels <= 0 || samples <= 0)
        return;

    for (int sample = 0; sample < samples; ++sample)
    {
        float mono = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
            mono += sanitizeSample (block.getSample (channel, sample));

        mono /= static_cast<float> (channels);
        fifo[fifoPosition++] = mono;

        if (fifoPosition == config.fftSize)
        {
            processFrame();
            const auto retainedSamples = config.fftSize - config.hopSize;
            if (retainedSamples > 0)
                std::move (fifo.end() - static_cast<std::ptrdiff_t> (retainedSamples), fifo.end(), fifo.begin());
            fifoPosition = retainedSamples;
        }
    }
}

bool SpectrumAnalyzer::copyLatestSnapshot (SpectrumSnapshot& destination) const noexcept
{
    if (! hasSnapshot)
        return false;

    destination = latestSnapshot;
    return true;
}

void SpectrumAnalyzer::processFrame() noexcept
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (fifo.begin(), fifo.end(), fftData.begin());

    window->multiplyWithWindowingTable (fftData.data(), config.fftSize);
    fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

    for (std::size_t fftBin = 0; fftBin < fftMagnitudes.size(); ++fftBin)
        fftMagnitudes[fftBin] = fftData[fftBin];

    latestSnapshot.sequence = nextSequence++;
    latestSnapshot.profile = config.profile;
    latestSnapshot.sampleRate = sampleRate;
    latestSnapshot.fftSize = config.fftSize;
    latestSnapshot.minFrequencyHz = config.minFrequencyHz;
    latestSnapshot.maxFrequencyHz = config.maxFrequencyHz;
    latestSnapshot.minDecibels = config.minDecibels;
    latestSnapshot.maxDecibels = config.maxDecibels;
    latestSnapshot.binCount = config.displayBinCount;

    for (std::size_t displayBin = 0; displayBin < config.displayBinCount; ++displayBin)
    {
        auto decibels = config.minDecibels;
        const auto start = logBinStarts[displayBin];
        const auto end = std::max (start + 1, logBinEnds[displayBin]);
        const auto binWidth = end - start;

        if (binWidth <= 2)
        {
            decibels = rawMagnitudeToDecibels (interpolateMagnitudeAtFrequency (logBinFrequencies[displayBin]), start);
        }
        else
        {
            for (auto fftBin = start; fftBin < end; ++fftBin)
                decibels = std::max (decibels, rawMagnitudeToDecibels (fftMagnitudes[fftBin], fftBin));
        }

        decibels = smoothDecibels (displayBin, decibels);
        latestSnapshot.bins[displayBin] = {
            logBinFrequencies[displayBin],
            decibels,
            decibelsToNormalised (decibels, config)
        };
    }

    hasProcessedFrame = true;
    hasSnapshot = true;
}

void SpectrumAnalyzer::buildLogBins()
{
    logBinStarts.assign (config.displayBinCount, 1);
    logBinEnds.assign (config.displayBinCount, 2);
    logBinFrequencies.assign (config.displayBinCount, static_cast<float> (config.minFrequencyHz));

    const auto maxFftBin = config.fftSize / 2;
    const auto binHz = sampleRate / static_cast<double> (config.fftSize);
    const auto nyquist = sampleRate * 0.5;
    const auto maxHz = std::min (config.maxFrequencyHz, nyquist);

    for (std::size_t displayBin = 0; displayBin < config.displayBinCount; ++displayBin)
    {
        const auto lowerRatio = static_cast<double> (displayBin) / static_cast<double> (config.displayBinCount);
        const auto upperRatio = static_cast<double> (displayBin + 1) / static_cast<double> (config.displayBinCount);
        const auto centreRatio = (lowerRatio + upperRatio) * 0.5;
        const auto lowerHz = safeLogFrequency (config.minFrequencyHz, maxHz, lowerRatio);
        const auto upperHz = safeLogFrequency (config.minFrequencyHz, maxHz, upperRatio);

        auto start = static_cast<std::size_t> (std::floor (lowerHz / binHz));
        auto end = static_cast<std::size_t> (std::ceil (upperHz / binHz)) + 1;

        start = std::clamp<std::size_t> (start, 1, maxFftBin);
        end = std::clamp<std::size_t> (end, start + 1, maxFftBin + 1);

        logBinStarts[displayBin] = start;
        logBinEnds[displayBin] = end;
        logBinFrequencies[displayBin] = safeLogFrequency (config.minFrequencyHz, maxHz, centreRatio);
    }
}

float SpectrumAnalyzer::sanitizeSample (float sample) const noexcept
{
    if (! std::isfinite (sample) || std::abs (sample) < std::numeric_limits<float>::min())
        return 0.0f;

    return sample;
}

float SpectrumAnalyzer::rawMagnitudeToDecibels (float magnitude, std::size_t) const noexcept
{
    if (! std::isfinite (magnitude) || magnitude <= 0.0f)
        return config.minDecibels;

    const auto normalised = (2.0f * magnitude) / (static_cast<float> (config.fftSize) * coherentGain);
    const auto decibels = 20.0f * std::log10 (std::max (normalised, 1.0e-12f));
    return clamp (decibels, config.minDecibels, config.maxDecibels);
}

float SpectrumAnalyzer::interpolateMagnitudeAtFrequency (float frequencyHz) const noexcept
{
    const auto binPosition = static_cast<double> (frequencyHz) * static_cast<double> (config.fftSize) / sampleRate;
    if (! std::isfinite (binPosition) || binPosition <= 0.0)
        return 0.0f;

    const auto lowerBin = static_cast<std::size_t> (std::floor (binPosition));
    const auto upperBin = std::min (lowerBin + 1, fftMagnitudes.size() - 1);
    if (lowerBin >= fftMagnitudes.size())
        return fftMagnitudes.back();

    const auto blend = static_cast<float> (binPosition - static_cast<double> (lowerBin));
    return fftMagnitudes[lowerBin] + blend * (fftMagnitudes[upperBin] - fftMagnitudes[lowerBin]);
}

float SpectrumAnalyzer::smoothDecibels (std::size_t displayBin, float decibels) noexcept
{
    if (! hasProcessedFrame)
    {
        smoothedDecibels[displayBin] = decibels;
        return decibels;
    }

    const auto previous = smoothedDecibels[displayBin];
    const auto coefficient = decibels > previous ? config.smoothingAttack : config.smoothingRelease;
    const auto smoothed = previous + coefficient * (decibels - previous);
    smoothedDecibels[displayBin] = clamp (smoothed, config.minDecibels, config.maxDecibels);
    return smoothedDecibels[displayBin];
}
}
