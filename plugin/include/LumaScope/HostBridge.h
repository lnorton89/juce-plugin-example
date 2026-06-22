#pragma once

#include <juce_core/juce_core.h>

namespace lumascope
{
struct BridgeResponse
{
    juce::Identifier eventId;
    juce::var payload;
    bool ready = false;
};

class HostBridge
{
public:
    static constexpr int protocolVersion = 1;
    static const juce::Identifier uiReadyEvent;
    static const juce::Identifier hostInfoEvent;
    static const juce::Identifier bridgeErrorEvent;

    HostBridge (juce::String hostMode, juce::String uiSource, juce::String productVersion,
                juce::String buildMarker);
    BridgeResponse handleUiReady (const juce::var& payload) const;
    juce::var makeHostInfo() const;

private:
    static BridgeResponse error (juce::String code, juce::String message);
    static bool isBoundedString (const juce::var& value);

    juce::String hostMode;
    juce::String uiSource;
    juce::String productVersion;
    juce::String buildMarker;
};
}
