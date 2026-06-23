#pragma once

#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/Standalone/SourceModel.h"

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
    static const juce::Identifier spectrumSnapshotEvent;
    static const juce::Identifier sourceListEvent;
    static const juce::Identifier sourceStateEvent;
    static const juce::Identifier sourceSelectEvent;
    static const juce::Identifier sourceStopEvent;

    HostBridge (juce::String hostMode, juce::String uiSource, juce::String productVersion,
                juce::String buildMarker);
    BridgeResponse handleUiReady (const juce::var& payload) const;
    juce::var makeHostInfo() const;
    static juce::var makeSpectrumSnapshot (const SpectrumSnapshot& snapshot);
    static juce::var makeSourceList (const SourceList& list);
    static juce::var makeSourceStateSnapshot (const SourceStateSnapshot& state);
    static juce::var makeError (juce::String code, juce::String message);

private:
    static BridgeResponse error (juce::String code, juce::String message);
    static bool isBoundedString (const juce::var& value);

    juce::String hostMode;
    juce::String uiSource;
    juce::String productVersion;
    juce::String buildMarker;
};
}
