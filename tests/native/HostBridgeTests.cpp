#include "LumaScope/HostBridge.h"
#include <juce_core/juce_core.h>
#include <iostream>

namespace
{
juce::var readFixture (const char* path)
{
    return juce::JSON::parse (juce::File (path));
}
}

int runHostBridgeTests()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    lumascope::HostBridge bridge ("Standalone", "embedded", "0.1.0");
    const auto ready = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_UI_READY));
    expect (ready.ready && ready.eventId == lumascope::HostBridge::hostInfoEvent, "valid ui.ready succeeds");
    expect (juce::JSON::toString (ready.payload, true) == juce::JSON::toString (readFixture (LUMASCOPE_FIXTURE_HOST_INFO), true), "host.info matches fixture");
    const auto unsupported = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_UNSUPPORTED));
    expect (! unsupported.ready && unsupported.payload["code"] == "protocol_mismatch", "unsupported version is typed error");
    const auto malformed = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_MALFORMED));
    expect (! malformed.ready && malformed.payload["code"] == "malformed_payload", "malformed payload is typed error");
    return failures;
}
