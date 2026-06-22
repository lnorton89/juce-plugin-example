#include "LumaScope/PluginProcessor.h"
#include "LumaScope/PluginEditor.h"

LumaScopeAudioProcessor::LumaScopeAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool LumaScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void LumaScopeAudioProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
}

juce::AudioProcessorEditor* LumaScopeAudioProcessor::createEditor()
{
    return new LumaScopeAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LumaScopeAudioProcessor();
}

