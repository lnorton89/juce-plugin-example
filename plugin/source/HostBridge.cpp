#include "LumaScope/HostBridge.h"

namespace lumascope
{
const juce::Identifier HostBridge::uiReadyEvent { "ui.ready" };
const juce::Identifier HostBridge::hostInfoEvent { "host.info" };
const juce::Identifier HostBridge::bridgeErrorEvent { "bridge.error" };
const juce::Identifier HostBridge::spectrumSnapshotEvent { "spectrum.snapshot" };
const juce::Identifier HostBridge::sourceListEvent { "source.list" };
const juce::Identifier HostBridge::sourceStateEvent { "source.state" };
const juce::Identifier HostBridge::sourceSelectEvent { "source.select" };
const juce::Identifier HostBridge::sourceStopEvent { "source.stop" };

HostBridge::HostBridge (juce::String hostModeIn, juce::String uiSourceIn, juce::String productVersionIn,
                        juce::String buildMarkerIn)
    : hostMode (std::move (hostModeIn)),
      uiSource (std::move (uiSourceIn)),
      productVersion (std::move (productVersionIn)),
      buildMarker (std::move (buildMarkerIn))
{
    jassert (hostMode == "VST3" || hostMode == "Standalone");
    jassert (uiSource == "embedded" || uiSource == "vite");
    jassert (productVersion.length() <= 128);
    jassert (buildMarker.isNotEmpty() && buildMarker.length() <= 128);
}

bool HostBridge::isBoundedString (const juce::var& value)
{
    return value.isString() && value.toString().isNotEmpty() && value.toString().length() <= 128;
}

BridgeResponse HostBridge::error (juce::String code, juce::String message)
{
    auto payload = juce::var (new juce::DynamicObject());
    payload.getDynamicObject()->setProperty ("code", code.substring (0, 64));
    payload.getDynamicObject()->setProperty ("message", message.substring (0, 256));
    payload.getDynamicObject()->setProperty ("protocolVersion", protocolVersion);
    return { bridgeErrorEvent, std::move (payload), false };
}

juce::var HostBridge::makeHostInfo() const
{
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("protocolVersion", protocolVersion);
    object->setProperty ("productName", "LumaScope");
    object->setProperty ("companyName", "Signal Foundry Audio");
    object->setProperty ("productVersion", productVersion);
    object->setProperty ("hostMode", hostMode);
    object->setProperty ("uiSource", uiSource);
    object->setProperty ("buildMarker", buildMarker);
    return result;
}

juce::var HostBridge::makeSpectrumSnapshot (const SpectrumSnapshot& snapshot)
{
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("protocolVersion", protocolVersion);
    object->setProperty ("sequence", static_cast<int> (snapshot.sequence));
    object->setProperty ("profile", toString (snapshot.profile));
    object->setProperty ("sampleRate", snapshot.sampleRate);
    object->setProperty ("fftSize", static_cast<int> (snapshot.fftSize));
    object->setProperty ("minFrequencyHz", snapshot.minFrequencyHz);
    object->setProperty ("maxFrequencyHz", snapshot.maxFrequencyHz);
    object->setProperty ("minDecibels", snapshot.minDecibels);
    object->setProperty ("maxDecibels", snapshot.maxDecibels);

    juce::Array<juce::var> bins;
    const auto boundedBinCount = std::min (snapshot.binCount, SpectrumSnapshot::maxBins);
    bins.ensureStorageAllocated (static_cast<int> (boundedBinCount));

    for (std::size_t index = 0; index < boundedBinCount; ++index)
    {
        const auto& source = snapshot.bins[index];
        auto bin = juce::var (new juce::DynamicObject());
        auto* binObject = bin.getDynamicObject();
        binObject->setProperty ("frequencyHz", source.frequencyHz);
        binObject->setProperty ("decibels", source.decibels);
        binObject->setProperty ("normalisedValue", source.normalisedValue);
        bins.add (std::move (bin));
    }

    object->setProperty ("bins", juce::var (bins));
    return result;
}

BridgeResponse HostBridge::handleUiReady (const juce::var& payload) const
{
    const auto* object = payload.getDynamicObject();
    if (object == nullptr || object->getProperties().size() != 1)
        return error ("malformed_payload", "ui.ready must be an object containing only protocolVersion.");

    const auto version = object->getProperty ("protocolVersion");
    if (! version.isInt() && ! version.isInt64())
        return error ("malformed_payload", "protocolVersion must be an integer.");
    if (static_cast<int> (version) != protocolVersion)
        return error ("protocol_mismatch", "The UI and native host use different protocol versions.");

    const auto hostInfo = makeHostInfo();
    const auto* hostObject = hostInfo.getDynamicObject();
    if (hostObject == nullptr || ! isBoundedString (hostObject->getProperty ("productName"))
        || ! isBoundedString (hostObject->getProperty ("companyName"))
        || ! isBoundedString (hostObject->getProperty ("productVersion"))
        || ! isBoundedString (hostObject->getProperty ("buildMarker")))
        return error ("runtime_error", "Native host information is invalid.");
    return { hostInfoEvent, hostInfo, true };
}

juce::var HostBridge::makeSourceList (const SourceList& list)
{
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("protocolVersion", protocolVersion);
    object->setProperty ("event", sourceListEvent.toString());

    // Input devices
    juce::Array<juce::var> inputEntries;
    for (const auto& desc : list.inputDevices)
    {
        auto entry = juce::var (new juce::DynamicObject());
        entry.getDynamicObject()->setProperty ("id", desc.id.substring (0, 256));
        entry.getDynamicObject()->setProperty ("displayName", desc.displayName.substring (0, 256));
        entry.getDynamicObject()->setProperty ("mode", toString (desc.mode));
        inputEntries.add (std::move (entry));
    }
    object->setProperty ("inputDevices", juce::var (inputEntries));

    // System outputs
    juce::Array<juce::var> outputEntries;
    for (const auto& desc : list.systemOutputs)
    {
        auto entry = juce::var (new juce::DynamicObject());
        entry.getDynamicObject()->setProperty ("id", desc.id.substring (0, 256));
        entry.getDynamicObject()->setProperty ("displayName", desc.displayName.substring (0, 256));
        entry.getDynamicObject()->setProperty ("mode", toString (desc.mode));
        outputEntries.add (std::move (entry));
    }
    object->setProperty ("systemOutputs", juce::var (outputEntries));

    return result;
}

juce::var HostBridge::makeError (juce::String code, juce::String message)
{
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("code", code.substring (0, 64));
    object->setProperty ("message", message.substring (0, 256));
    object->setProperty ("protocolVersion", protocolVersion);
    return result;
}

juce::var HostBridge::makeSourceStateSnapshot (const SourceStateSnapshot& state)
{
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("protocolVersion", protocolVersion);
    object->setProperty ("event", sourceStateEvent.toString());
    object->setProperty ("mode", toString (state.mode));
    object->setProperty ("state", toString (state.state));
    object->setProperty ("selectedSourceId", state.selectedSourceId.substring (0, 256));
    object->setProperty ("selectedSourceName", state.selectedSourceName.substring (0, 256));
    object->setProperty ("code", state.code.substring (0, 64));
    object->setProperty ("message", state.message.substring (0, 256));

    return result;
}
}
