#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "LumaScope/HostBridge.h"
#include "LumaScope/PluginProcessor.h"
#include "LumaScope/WebResources.h"

class LumaScopeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void handleUiReady (const juce::var& payload);

private:
    static juce::String hostMode();

    lumascope::WebResources resources;
    lumascope::HostBridge bridge;
    juce::WebBrowserComponent browser;
};
