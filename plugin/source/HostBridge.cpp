#include "LumaScope/HostBridge.h"

namespace lumascope
{
const juce::Identifier HostBridge::uiReadyEvent { "ui.ready" };
const juce::Identifier HostBridge::hostInfoEvent { "host.info" };
const juce::Identifier HostBridge::bridgeErrorEvent { "bridge.error" };

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
}
