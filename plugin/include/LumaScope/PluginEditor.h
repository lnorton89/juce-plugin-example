#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <limits>
#include "LumaScope/HostBridge.h"
#include "LumaScope/PluginProcessor.h"
#include "LumaScope/WebResources.h"

namespace lumascope
{
class EditorSnapshotPoller
{
public:
    EditorSnapshotPoller() = default;

    template <typename Reader, typename Emitter>
    bool poll (double nowMilliseconds, Reader&& readLatestSnapshot, Emitter&& emitSnapshot)
    {
        if (nowMilliseconds - lastEmitMilliseconds < minIntervalMilliseconds)
            return false;

        SpectrumSnapshot snapshot;
        if (! readLatestSnapshot (snapshot, lastSeenSequence))
            return false;

        minIntervalMilliseconds = intervalForProfile (snapshot.profile);
        lastEmitMilliseconds = nowMilliseconds;
        emitSnapshot (snapshot);
        return true;
    }

private:
    static double intervalForProfile (AnalyzerProfile profile) noexcept
    {
        const auto config = makeAnalyzerConfig (profile);
        return 1000.0 / config.snapshotRateHz;
    }

    std::uint32_t lastSeenSequence = 0;
    double lastEmitMilliseconds = -std::numeric_limits<double>::infinity();
    double minIntervalMilliseconds = intervalForProfile (AnalyzerProfile::Musical);
};
}

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
    lumascope::EditorSnapshotPoller snapshotPoller;
    Browser browser;
    juce::String uiSource;
    juce::String fallbackCode;
    juce::String fallbackMessage;
    bool bridgeReady = false;
};
