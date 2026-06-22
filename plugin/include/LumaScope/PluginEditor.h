#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LumaScope/PluginProcessor.h"

class LumaScopeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor&);
    void paint (juce::Graphics&) override;
};

