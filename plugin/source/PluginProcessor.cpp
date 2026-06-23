#include "LumaScope/PluginProcessor.h"

#ifndef LUMASCOPE_NATIVE_TESTS
 #include "LumaScope/PluginEditor.h"
#endif

LumaScopeAudioProcessor::LumaScopeAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void LumaScopeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    analyzer.prepare (sampleRate);
    snapshotMailbox.clear();
    lastPublishedAnalyzerSequence = 0;
}

void LumaScopeAudioProcessor::releaseResources()
{
    snapshotMailbox.clear();
    lastPublishedAnalyzerSequence = 0;
}

bool LumaScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void LumaScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);

    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return;

    analyzer.pushAudioBlock (buffer);

    lumascope::SpectrumSnapshot snapshot;
    if (analyzer.copyLatestSnapshot (snapshot) && snapshot.sequence != lastPublishedAnalyzerSequence)
    {
        if (snapshotMailbox.publish (snapshot))
            lastPublishedAnalyzerSequence = snapshot.sequence;
    }
}

void LumaScopeAudioProcessor::pushStandaloneAudioBlock (const juce::AudioBuffer<float>& block) noexcept
{
    // Reuses the same analyzer ingress contract used by processBlock.
    // Must be called from a non-audio-thread context, or from a JUCE capture
    // callback that is already real-time safe.
    if (block.getNumChannels() <= 0 || block.getNumSamples() <= 0)
        return;

    analyzer.pushAudioBlock (block);

    lumascope::SpectrumSnapshot snapshot;
    if (analyzer.copyLatestSnapshot (snapshot) && snapshot.sequence != lastPublishedAnalyzerSequence)
    {
        if (snapshotMailbox.publish (snapshot))
            lastPublishedAnalyzerSequence = snapshot.sequence;
    }
}

bool LumaScopeAudioProcessor::readLatestSpectrumSnapshot (lumascope::SpectrumSnapshot& snapshot,
                                                          std::uint32_t& lastSeenSequence) const noexcept
{
    return snapshotMailbox.readLatest (snapshot, lastSeenSequence);
}

juce::AudioProcessorEditor* LumaScopeAudioProcessor::createEditor()
{
   #ifdef LUMASCOPE_NATIVE_TESTS
    return nullptr;
   #else
    return new LumaScopeAudioProcessorEditor (*this);
   #endif
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LumaScopeAudioProcessor();
}
