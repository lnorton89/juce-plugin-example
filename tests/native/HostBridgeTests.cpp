#include "LumaScope/HostBridge.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include <juce_core/juce_core.h>
#include <iostream>

namespace
{
juce::var readFixture (const char* path)
{
    return juce::JSON::parse (juce::File (path));
}

lumascope::SpectrumSnapshot makeSnapshot()
{
    lumascope::SpectrumSnapshot snapshot;
    snapshot.sequence = 42;
    snapshot.profile = lumascope::AnalyzerProfile::Musical;
    snapshot.sampleRate = 48000.0;
    snapshot.fftSize = 4096;
    snapshot.minFrequencyHz = 20.0;
    snapshot.maxFrequencyHz = 20000.0;
    snapshot.minDecibels = -96.0f;
    snapshot.maxDecibels = 0.0f;
    snapshot.binCount = 3;
    snapshot.bins[0] = { 20.0f, -96.0f, 0.0f };
    snapshot.bins[1] = { 1000.0f, -12.0f, 0.875f };
    snapshot.bins[2] = { 20000.0f, -48.0f, 0.5f };
    return snapshot;
}
}

int runHostBridgeTests()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    lumascope::HostBridge bridge ("Standalone", "embedded", "0.1.0", "0.1.0-Debug");
    const auto ready = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_UI_READY));
    expect (ready.ready && ready.eventId == lumascope::HostBridge::hostInfoEvent, "valid ui.ready succeeds");
    expect (juce::JSON::toString (ready.payload, true) == juce::JSON::toString (readFixture (LUMASCOPE_FIXTURE_HOST_INFO), true), "host.info matches fixture");
    const auto unsupported = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_UNSUPPORTED));
    expect (! unsupported.ready && unsupported.payload["code"] == "protocol_mismatch", "unsupported version is typed error");
    const auto malformed = bridge.handleUiReady (readFixture (LUMASCOPE_FIXTURE_MALFORMED));
    expect (! malformed.ready && malformed.payload["code"] == "malformed_payload", "malformed payload is typed error");

    const auto spectrumPayload = lumascope::HostBridge::makeSpectrumSnapshot (makeSnapshot());
    expect (lumascope::HostBridge::spectrumSnapshotEvent.toString() == "spectrum.snapshot", "spectrum event id is stable");
    expect (juce::JSON::toString (spectrumPayload, true)
            == juce::JSON::toString (readFixture (LUMASCOPE_FIXTURE_SPECTRUM_SNAPSHOT), true),
            "spectrum.snapshot matches fixture");
    return failures;
}
