#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/PluginProcessor.h"

#include <cmath>
#include <iostream>
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

void expectNear (float actual, float expected, float tolerance, const char* message)
{
    if (std::abs (actual - expected) > tolerance)
    {
        std::cerr << "FAIL: " << message << " actual=" << actual
                  << " expected=" << expected << " tolerance=" << tolerance << '\n';
        ++failures;
    }
}

juce::AudioBuffer<float> makeSineBuffer (int channels, int samples, double sampleRate, double frequency)
{
    juce::AudioBuffer<float> buffer (channels, samples);
    double phase = 0.0;
    const auto increment = juce::MathConstants<double>::twoPi * frequency / sampleRate;

    for (int sample = 0; sample < samples; ++sample)
    {
        const auto value = static_cast<float> (std::sin (phase));
        phase += increment;

        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, sample, value * (channel == 0 ? 1.0f : 0.5f));
    }

    return buffer;
}

std::vector<float> copySamples (const juce::AudioBuffer<float>& buffer)
{
    std::vector<float> samples;
    samples.reserve (static_cast<std::size_t> (buffer.getNumChannels() * buffer.getNumSamples()));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            samples.push_back (buffer.getSample (channel, sample));

    return samples;
}

void expectUnchanged (const juce::AudioBuffer<float>& buffer,
                      const std::vector<float>& before,
                      const char* message)
{
    auto index = std::size_t { 0 };

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            expectNear (buffer.getSample (channel, sample), before[index++], 0.0f, message);
}

bool readSnapshot (LumaScopeAudioProcessor& processor, lumascope::SpectrumSnapshot& snapshot, std::uint32_t& lastSeen)
{
    return processor.readLatestSpectrumSnapshot (snapshot, lastSeen);
}

void processBuffer (LumaScopeAudioProcessor& processor, juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
}

void testMonoAndStereoPassthrough()
{
    for (const auto channels : { 1, 2 })
    {
        LumaScopeAudioProcessor processor;
        processor.prepareToPlay (48000.0, 512);
        auto buffer = makeSineBuffer (channels, 512, 48000.0, 1000.0);
        const auto before = copySamples (buffer);

        processBuffer (processor, buffer);
        expectUnchanged (buffer, before, "processBlock preserves mono/stereo samples");
    }
}

void testVariableBlocksEventuallyPublishSnapshot()
{
    LumaScopeAudioProcessor processor;
    processor.prepareToPlay (48000.0, 256);

    for (const auto samples : { 0, 1, 17, 2048, 333, 4096 })
    {
        auto buffer = makeSineBuffer (2, samples, 48000.0, 937.5);
        const auto before = copySamples (buffer);
        processBuffer (processor, buffer);
        expectUnchanged (buffer, before, "variable block processing preserves samples");
    }

    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;
    expect (readSnapshot (processor, snapshot, lastSeen), "enough variable blocks publish a snapshot");
    expect (snapshot.sequence > 0, "published snapshot has a sequence");
    expect (snapshot.binCount > 0, "published snapshot has bins");
}

void testSampleRateChangeResetsAnalyzer()
{
    LumaScopeAudioProcessor processor;
    processor.prepareToPlay (44100.0, 512);

    auto first = makeSineBuffer (2, 4096, 44100.0, 689.0625);
    processBuffer (processor, first);

    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;
    expect (readSnapshot (processor, snapshot, lastSeen), "first sample rate publishes");
    expect (std::abs (snapshot.sampleRate - 44100.0) < 0.001, "first snapshot records first sample rate");

    processor.prepareToPlay (96000.0, 128);
    expect (! readSnapshot (processor, snapshot, lastSeen), "sample-rate reset clears stale snapshots");

    auto second = makeSineBuffer (1, 4096, 96000.0, 937.5);
    processBuffer (processor, second);
    expect (readSnapshot (processor, snapshot, lastSeen), "changed sample rate publishes new snapshot");
    expect (std::abs (snapshot.sampleRate - 96000.0) < 0.001, "snapshot records changed sample rate");
}

void testProcessingContinuesWithoutEditor()
{
    LumaScopeAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    for (int block = 0; block < 3; ++block)
    {
        auto buffer = makeSineBuffer (2, 2048, 48000.0, 984.375);
        processBuffer (processor, buffer);
    }

    lumascope::SpectrumSnapshot snapshot;
    std::uint32_t lastSeen = 0;
    expect (readSnapshot (processor, snapshot, lastSeen), "processor publishes snapshots without an editor");
    expect (! readSnapshot (processor, snapshot, lastSeen), "later consumer receives only newer snapshots");
}
}

int runPluginProcessorTests()
{
    testMonoAndStereoPassthrough();
    testVariableBlocksEventuallyPublishSnapshot();
    testSampleRateChangeResetsAnalyzer();
    testProcessingContinuesWithoutEditor();
    return failures;
}
