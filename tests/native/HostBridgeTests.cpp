#include "LumaScope/HostBridge.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/Standalone/SourceModel.h"
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

lumascope::SourceList makeTestSourceList()
{
    lumascope::SourceList list;
    list.inputDevices.push_back ({ "juce-input-mic-1", "Microphone (Realtek Audio)", lumascope::SourceMode::inputDevice });
    list.inputDevices.push_back ({ "juce-input-mic-2", "Microphone (USB Camera)", lumascope::SourceMode::inputDevice });
    list.systemOutputs.push_back ({ "wasapi-speaker-1", "Speakers (Realtek Audio)", lumascope::SourceMode::systemOutput });
    return list;
}

lumascope::SourceStateSnapshot makeTestSourceState()
{
    lumascope::SourceStateSnapshot state;
    state.mode = lumascope::SourceMode::inputDevice;
    state.state = lumascope::SourceState::active;
    state.selectedSourceId = "juce-input-mic-1";
    state.selectedSourceName = "Microphone (Realtek Audio)";
    return state;
}

void testMakeSourceListPayload()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    const auto list = makeTestSourceList();
    const auto payload = lumascope::HostBridge::makeSourceList (list);
    const auto* obj = payload.getDynamicObject();

    expect (obj != nullptr, "source.list payload is a DynamicObject");
    expect (static_cast<int> (obj->getProperty ("protocolVersion")) == 1, "source.list has protocolVersion");
    expect (obj->getProperty ("event") == "source.list", "source.list event field");

    const auto& inputDevices = obj->getProperty ("inputDevices");
    expect (inputDevices.isArray(), "source.list inputDevices is an array");
    expect (inputDevices.size() == 2, "source.list inputDevices has 2 entries");

    const auto& outputDevices = obj->getProperty ("systemOutputs");
    expect (outputDevices.isArray(), "source.list systemOutputs is an array");
    expect (outputDevices.size() == 1, "source.list systemOutputs has 1 entry");

    // Verify first input device has expected fields
    const auto firstInput = inputDevices[0];
    const auto* firstInputObj = firstInput.getDynamicObject();
    expect (firstInputObj != nullptr, "first input device is a DynamicObject");
    expect (firstInputObj->getProperty ("id").toString() == "juce-input-mic-1", "first input id matches");
    expect (firstInputObj->getProperty ("displayName").toString() == "Microphone (Realtek Audio)", "first input name matches");
    expect (firstInputObj->getProperty ("mode").toString() == "InputDevice", "first input mode is InputDevice");

    // Verify system output
    const auto firstOutput = outputDevices[0];
    const auto* firstOutputObj = firstOutput.getDynamicObject();
    expect (firstOutputObj != nullptr, "first system output is a DynamicObject");
    expect (firstOutputObj->getProperty ("mode").toString() == "SystemOutput", "system output mode is SystemOutput");
}

void testMakeSourceStatePayload()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    const auto state = makeTestSourceState();
    const auto payload = lumascope::HostBridge::makeSourceStateSnapshot (state);
    const auto* obj = payload.getDynamicObject();

    expect (obj != nullptr, "source.state payload is a DynamicObject");
    expect (static_cast<int> (obj->getProperty ("protocolVersion")) == 1, "source.state has protocolVersion");
    expect (obj->getProperty ("event") == "source.state", "source.state event field");
    expect (obj->getProperty ("mode").toString() == "InputDevice", "source.state mode matches");
    expect (obj->getProperty ("state").toString() == "active", "source.state is active");
    expect (obj->getProperty ("selectedSourceId").toString() == "juce-input-mic-1", "source.state selectedSourceId matches");
    expect (obj->getProperty ("selectedSourceName").toString() == "Microphone (Realtek Audio)", "source.state selectedSourceName matches");
}

void testMakeSourceStateStopped()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    lumascope::SourceStateSnapshot state;
    state.state = lumascope::SourceState::stopped;

    const auto payload = lumascope::HostBridge::makeSourceStateSnapshot (state);
    const auto* obj = payload.getDynamicObject();

    expect (obj != nullptr, "stopped state payload is a DynamicObject");
    expect (obj->getProperty ("state").toString() == "stopped", "stopped state");
    expect (obj->getProperty ("selectedSourceId").toString().isEmpty(), "stopped has no source ID");
}

void testMakeSourceStateError()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    lumascope::SourceStateSnapshot state;
    state.mode = lumascope::SourceMode::inputDevice;
    state.state = lumascope::SourceState::error;
    state.selectedSourceId = "juce-input-mic-1";
    state.selectedSourceName = "Microphone";
    state.code = "source_lost";
    state.message = "The selected device was disconnected.";

    const auto payload = lumascope::HostBridge::makeSourceStateSnapshot (state);
    const auto* obj = payload.getDynamicObject();

    expect (obj != nullptr, "error state payload is a DynamicObject");
    expect (obj->getProperty ("state").toString() == "error", "error state");
    expect (obj->getProperty ("code").toString() == "source_lost", "error code matches");
    expect (obj->getProperty ("message").toString() == "The selected device was disconnected.", "error message matches");
    expect (obj->getProperty ("mode").toString() == "InputDevice", "error state has mode");
}

void testMakeSourceStateSilent()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    lumascope::SourceStateSnapshot state;
    state.mode = lumascope::SourceMode::inputDevice;
    state.state = lumascope::SourceState::silent;
    state.selectedSourceId = "juce-input-mic-1";
    state.selectedSourceName = "Microphone";

    const auto payload = lumascope::HostBridge::makeSourceStateSnapshot (state);
    const auto* obj = payload.getDynamicObject();

    expect (obj != nullptr, "silent state payload is a DynamicObject");
    expect (obj->getProperty ("state").toString() == "silent", "silent state");
    expect (obj->getProperty ("code").toString().isEmpty(), "silent state has no error code");
}

void testMakeSourceStateBoundedFields()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // Test that oversized fields are truncated
    lumascope::SourceStateSnapshot state;
    state.state = lumascope::SourceState::error;
    state.code = juce::String().paddedRight ('X', 200);
    state.message = juce::String().paddedRight ('Y', 500);

    const auto payload = lumascope::HostBridge::makeSourceStateSnapshot (state);
    const auto* obj = payload.getDynamicObject();

    expect (obj->getProperty ("code").toString().length() <= 64, "error code truncated to 64");
    expect (obj->getProperty ("message").toString().length() <= 256, "error message truncated to 256");
}

void testMakeSourceListBoundedFields()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // Test that oversized source descriptor fields are truncated
    lumascope::SourceList list;
    lumascope::SourceDescriptor oversized;
    oversized.id = juce::String().paddedRight ('I', 500);
    oversized.displayName = juce::String().paddedRight ('N', 500);
    oversized.mode = lumascope::SourceMode::inputDevice;
    list.inputDevices.push_back (oversized);

    const auto payload = lumascope::HostBridge::makeSourceList (list);
    const auto* obj = payload.getDynamicObject();
    const auto& devices = obj->getProperty ("inputDevices");
    const auto device = devices[0];

    expect (device.getDynamicObject()->getProperty ("id").toString().length() <= 256,
            "source id truncated to 256");
    expect (device.getDynamicObject()->getProperty ("displayName").toString().length() <= 256,
            "source displayName truncated to 256");
}

void testEventIdentifiersStable()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    expect (lumascope::HostBridge::sourceListEvent.toString() == "source.list", "source.list event id is stable");
    expect (lumascope::HostBridge::sourceStateEvent.toString() == "source.state", "source.state event id is stable");
}

void testPayloadEventFieldMatchesIdentifier()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    const auto listPayload = lumascope::HostBridge::makeSourceList (makeTestSourceList());
    expect (listPayload.getDynamicObject()->getProperty ("event").toString() == "source.list",
            "source.list payload has matching event field");

    const auto statePayload = lumascope::HostBridge::makeSourceStateSnapshot (makeTestSourceState());
    expect (statePayload.getDynamicObject()->getProperty ("event").toString() == "source.state",
            "source.state payload has matching event field");
}

void testEmptySourceList()
{
    int failures = 0;
    const auto expect = [&failures] (bool condition, const char* message)
    {
        if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    lumascope::SourceList empty;
    const auto payload = lumascope::HostBridge::makeSourceList (empty);
    const auto* obj = payload.getDynamicObject();

    expect (obj->getProperty ("inputDevices").isArray(), "empty source list has inputDevices array");
    expect (obj->getProperty ("inputDevices").size() == 0, "empty source list inputDevices is empty");
    expect (obj->getProperty ("systemOutputs").isArray(), "empty source list has systemOutputs array");
    expect (obj->getProperty ("systemOutputs").size() == 0, "empty source list systemOutputs is empty");
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
    const auto expectedSpectrumPayload = readFixture (LUMASCOPE_FIXTURE_SPECTRUM_SNAPSHOT);
    expect (lumascope::HostBridge::spectrumSnapshotEvent.toString() == "spectrum.snapshot", "spectrum event id is stable");
    const auto actualSpectrumJson = juce::JSON::toString (spectrumPayload, true);
    const auto expectedSpectrumJson = juce::JSON::toString (expectedSpectrumPayload, true);
    if (actualSpectrumJson != expectedSpectrumJson)
        std::cerr << "Actual spectrum JSON:\n" << actualSpectrumJson << "\nExpected spectrum JSON:\n"
                  << expectedSpectrumJson << '\n';
    expect (actualSpectrumJson == expectedSpectrumJson, "spectrum.snapshot matches fixture");

    // Source protocol tests
    testMakeSourceListPayload();
    testMakeSourceStatePayload();
    testMakeSourceStateStopped();
    testMakeSourceStateError();
    testMakeSourceStateSilent();
    testMakeSourceStateBoundedFields();
    testMakeSourceListBoundedFields();
    testEventIdentifiersStable();
    testPayloadEventFieldMatchesIdentifier();
    testEmptySourceList();

    return failures;
}
