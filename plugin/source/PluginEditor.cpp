#include "LumaScope/PluginEditor.h"

LumaScopeAudioProcessorEditor::LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor& processor)
    : AudioProcessorEditor (&processor)
{
    setSize (960, 600);
}

void LumaScopeAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff0b0f12));
    graphics.setColour (juce::Colour (0xfff2f7f8));
    graphics.setFont (24.0f);
    graphics.drawFittedText ("LumaScope", getLocalBounds(), juce::Justification::centred, 1);
}

