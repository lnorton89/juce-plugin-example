#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "LumaScope/HostBridge.h"
#include "LumaScope/PluginProcessor.h"
#include "LumaScope/WebResources.h"

class LumaScopeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void handleUiReady (const juce::var& payload);
    void handleBrowserNetworkError (const juce::String& errorInfo);

private:
    static juce::String hostMode();
    static juce::String configuredDevServer();
    static bool isCanonicalDevServer (const juce::String& url);
    void timerCallback() override;
    void showFallback (juce::String code, juce::String message);
    void writeSmokeResult (juce::String status, juce::String errorCode = {});

    class Browser final : public juce::WebBrowserComponent
    {
    public:
        Browser (LumaScopeAudioProcessorEditor&, const juce::WebBrowserComponent::Options&);
        bool pageLoadHadNetworkError (const juce::String& errorInfo) override;
    private:
        LumaScopeAudioProcessorEditor& owner;
    };

    lumascope::WebResources resources;
    lumascope::HostBridge bridge;
    Browser browser;
    juce::String uiSource;
    juce::String fallbackCode;
    juce::String fallbackMessage;
};
