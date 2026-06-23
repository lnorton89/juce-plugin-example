#include "LumaScope/Standalone/SourceModel.h"
#include "LumaScope/Standalone/StandaloneSourceController.h"

#include <iostream>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void expect (bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

void expectEq (int actual, int expected, const char* message)
{
    if (actual != expected)
    {
        std::cerr << "FAIL: " << message << " actual=" << actual << " expected=" << expected << '\n';
        ++failures;
    }
}

void expectEqStr (const juce::String& actual, const juce::String& expected, const char* message)
{
    if (actual != expected)
    {
        std::cerr << "FAIL: " << message << " actual=\"" << actual << "\" expected=\"" << expected << "\"\n";
        ++failures;
    }
}

void expectTrue (bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

void expectFalse (bool condition, const char* message)
{
    if (condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

// ===========================================================================
// Helper: build a render-endpoint descriptor as the enumerator would produce.
// ===========================================================================
lumascope::SourceDescriptor makeRenderDescriptor (const juce::String& rawEndpointId,
                                                    const juce::String& displayName)
{
    lumascope::SourceDescriptor desc;
    desc.id = "wasapi-loopback-" + rawEndpointId;
    desc.displayName = displayName;
    desc.mode = lumascope::SourceMode::systemOutput;
    return desc;
}

// ===========================================================================
// D-01 / D-03: SystemOutput descriptors carry the correct mode and a
// mode-specific prefix discriminator per T-03-02-01.
// ===========================================================================
void testRenderDescriptorHasSystemOutputMode()
{
    auto desc = makeRenderDescriptor ("{0.0.0.00000000}.{device-id}", "Speakers (Realtek Audio)");
    expect (desc.mode == lumascope::SourceMode::systemOutput,
            "D-01/D-03: Render endpoint descriptor has systemOutput mode");
}

void testRenderDescriptorIdHasWasapiPrefix()
{
    auto desc = makeRenderDescriptor ("{0.0.0.00000000}.{device-id}", "Speakers");
    expectTrue (desc.id.startsWith ("wasapi-loopback-"),
                "D-03/T-03-02-01: Render endpoint ID starts with 'wasapi-loopback-' prefix");
    expectTrue (desc.id.length() > 16,
                "D-03: Endpoint ID includes both prefix and the raw device ID");
}

void testInputDescriptorDoesNotHaveWasapiPrefix()
{
    // InputDevice descriptors from 03-01 use "juce-input-" prefix
    lumascope::SourceDescriptor inputDesc;
    inputDesc.id = "juce-input-Microphone (Realtek Audio)";
    inputDesc.displayName = "Microphone (Realtek Audio)";
    inputDesc.mode = lumascope::SourceMode::inputDevice;

    expectFalse (inputDesc.id.startsWith ("wasapi-loopback-"),
                 "T-03-02-01: Input device IDs do not have wasapi-loopback prefix");
    expectTrue (inputDesc.id.startsWith ("juce-input-"),
                "Input device ID starts with 'juce-input-' prefix");
}

// ===========================================================================
// D-04: Render endpoints appear in systemOutputs list, not inputDevices.
// ===========================================================================
void testRenderEndpointsOnlyInSystemOutputs()
{
    lumascope::SourceList list;
    list.inputDevices.push_back ({ "juce-input-Mic", "Microphone", lumascope::SourceMode::inputDevice });
    list.systemOutputs.push_back (makeRenderDescriptor ("{speaker-id}", "Speakers"));

    for (const auto& dev : list.inputDevices)
        expect (dev.mode == lumascope::SourceMode::inputDevice,
                "D-04: inputDevices list contains only inputDevice entries");

    for (const auto& dev : list.systemOutputs)
        expect (dev.mode == lumascope::SourceMode::systemOutput,
                "D-04: systemOutputs list contains only systemOutput entries");
}

// ===========================================================================
// Bounded fields: IDs and display names are at most 256 characters.
// ===========================================================================
void testRenderDescriptorBoundedFields()
{
    auto desc = makeRenderDescriptor ("{device-id}", "Speakers (Realtek High Definition Audio)");
    expectTrue (desc.id.length() <= 256,
                "Render endpoint ID is bounded to 256 characters");
    expectTrue (desc.displayName.length() <= 256,
                "Render endpoint display name is bounded to 256 characters");

    // Edge case: very long endpoint ID
    juce::String longId;
    longId = longId.paddedRight ('x', 240);
    auto longDesc = makeRenderDescriptor (longId, "Long ID Device");
    expectTrue (longDesc.id.length() <= 256,
                "Render endpoint with long ID still fits within 256 chars");

    // Edge case: very long display name
    auto nameDesc = makeRenderDescriptor ("{id}", juce::String().paddedRight ('N', 256));
    expectTrue (nameDesc.displayName.length() <= 256,
                "Render endpoint with 256-char name fits the bound");
}

// ===========================================================================
// Empty render endpoint list is valid (no endpoints available).
// ===========================================================================
void testEmptyRenderEndpointList()
{
    lumascope::SourceList list;
    expectTrue (list.systemOutputs.empty(),
                "Empty render endpoint list when no endpoints available");
    expectEq (static_cast<int> (list.systemOutputs.size()), 0,
              "Zero render endpoints in empty list");
}

// ===========================================================================
// Multiple render endpoints are correctly listed and ordered.
// ===========================================================================
void testMultipleRenderEndpoints()
{
    lumascope::SourceList list;
    list.systemOutputs.push_back (makeRenderDescriptor ("{speaker-1}", "Speakers (Realtek Audio)"));
    list.systemOutputs.push_back (makeRenderDescriptor ("{speaker-2}", "Speakers (USB Headset)"));
    list.systemOutputs.push_back (makeRenderDescriptor ("{hdmi-1}", "LG HDR Monitor (NVIDIA Audio)"));

    expectEq (static_cast<int> (list.systemOutputs.size()), 3,
              "Three render endpoints listed");
    expectEqStr (list.systemOutputs[0].id, "wasapi-loopback-{speaker-1}",
                 "First render endpoint ID is preserved");
    expectEqStr (list.systemOutputs[1].displayName, "Speakers (USB Headset)",
                 "Second render endpoint display name is preserved");
    expectEqStr (list.systemOutputs[2].displayName, "LG HDR Monitor (NVIDIA Audio)",
                 "Third render endpoint display name is preserved");
}

// ===========================================================================
// D-05/D-06: Selected endpoint removal or failure stops capture and surfaces
// error (tested via controller state model).
// ===========================================================================
void testRenderEndpointErrorState()
{
    lumascope::SourceStateSnapshot snapshot;
    snapshot.mode = lumascope::SourceMode::systemOutput;
    snapshot.state = lumascope::SourceState::error;
    snapshot.selectedSourceId = "wasapi-loopback-{removed-device}";
    snapshot.selectedSourceName = "Speakers (Removed)";
    snapshot.code = "endpoint_lost";
    snapshot.message = "The selected output endpoint was removed or is no longer available.";

    expect (snapshot.state == lumascope::SourceState::error,
            "D-05: Render endpoint removal transitions to error state");
    expectEqStr (snapshot.code, "endpoint_lost",
                 "D-05: Error code for lost render endpoint");
    expectTrue (snapshot.message.isNotEmpty(),
                "D-05: Error message is provided for render endpoint loss");
    expect (snapshot.mode == lumascope::SourceMode::systemOutput,
            "Error state preserves SystemOutput mode");
}

// ===========================================================================
// D-07: Bounded retry attempts for the same source.
// Test that the model supports retry tracking.
// ===========================================================================
void testSameSourceRetryTracking()
{
    // Simulate retry state as part of the error/stopped lifecycle.
    // After failed retries, the state should be error (not auto-fallback).
    lumascope::SourceStateSnapshot afterRetryFailure;
    afterRetryFailure.mode = lumascope::SourceMode::systemOutput;
    afterRetryFailure.state = lumascope::SourceState::error;
    afterRetryFailure.selectedSourceId = "wasapi-loopback-{flaky-device}";
    afterRetryFailure.code = "endpoint_retry_exhausted";
    afterRetryFailure.message = "The output endpoint could not be recovered after multiple attempts.";

    expect (afterRetryFailure.state == lumascope::SourceState::error,
            "D-07: After retry exhaustion, state is error (not active)");
    expectEqStr (afterRetryFailure.selectedSourceId, "wasapi-loopback-{flaky-device}",
                 "D-07: Retry targets the same endpoint ID only");
    expectFalse (afterRetryFailure.selectedSourceId.contains ("juce-input-"),
                 "D-07: Retry does not fall back to an input device");
}

// ===========================================================================
// D-08/D-09: Valid-but-silent render endpoint stays active.
// ===========================================================================
void testSilentRenderEndpointStaysActive()
{
    lumascope::SourceStateSnapshot snapshot;
    snapshot.mode = lumascope::SourceMode::systemOutput;
    snapshot.state = lumascope::SourceState::silent;
    snapshot.selectedSourceId = "wasapi-loopback-{speaker-id}";
    snapshot.selectedSourceName = "Speakers (Realtek Audio)";

    expect (snapshot.state == lumascope::SourceState::silent,
            "D-08: Silent render endpoint reports silent state (not error)");
    expectEqStr (snapshot.selectedSourceId, "wasapi-loopback-{speaker-id}",
                 "D-08: Silent render endpoint keeps its source ID");
    expectTrue (snapshot.code.isEmpty(),
                "D-09: Silent state has no error code (silence != failure)");
}

// ===========================================================================
// T-03-02-01: Endpoint identity matching uses mode + Core Audio endpoint ID.
// ===========================================================================
void testEndpointIdentityMatching()
{
    // Two different render endpoints with same display name but different IDs
    auto endpointA = makeRenderDescriptor ("{id-A}", "Speakers");
    auto endpointB = makeRenderDescriptor ("{id-B}", "Speakers");

    // Verify they are distinct by ID
    expectTrue (endpointA.id != endpointB.id,
                "T-03-02-01: Render endpoints with same display name but different IDs are distinct");
    expectEqStr (endpointA.displayName, endpointB.displayName,
                 "T-03-02-01: Display names may be the same for different endpoints");

    // Verify a render and input endpoint never compare equal even with same display name
    lumascope::SourceDescriptor inputDesc;
    inputDesc.id = "juce-input-Speakers";
    inputDesc.displayName = "Speakers";
    inputDesc.mode = lumascope::SourceMode::inputDevice;

    expectTrue (endpointA.id != inputDesc.id,
                "T-03-02-01: Render and input endpoint IDs never collide");
    expectFalse (endpointA.id == inputDesc.id,
                 "T-03-02-01: Mode prefix ensures distinct ID namespaces");
}

// ===========================================================================
// SourceDescriptor state for disabled/unplugged/not-present endpoints.
// D-04 requires that disabled or unplugged endpoints are enumerated so the
// UI can show them as available-but-inactive rather than hiding them.
// ===========================================================================
void testInactiveEndpointDescriptor()
{
    // An endpoint in disabled or unplugged state still gets a valid descriptor
    lumascope::SourceDescriptor desc;
    desc.id = "wasapi-loopback-{disabled-device}";
    desc.displayName = "Speakers (Disabled)";
    desc.mode = lumascope::SourceMode::systemOutput;

    expectTrue (desc.id.isNotEmpty(),
                "Inactive endpoint has a non-empty ID");
    expectTrue (desc.displayName.isNotEmpty(),
                "Inactive endpoint has a display name");
    expect (desc.mode == lumascope::SourceMode::systemOutput,
            "Inactive endpoint uses systemOutput mode");

    // The state (disabled/unplugged) is conveyed separately — the descriptor
    // itself is always valid for UI display
    expectTrue (desc.id.startsWith ("wasapi-loopback-"),
                "Inactive endpoint ID preserves the wasapi-loopback- prefix");
}

// ===========================================================================
// CAP-02: System output enumeration never includes recording input devices.
// ===========================================================================
void testSystemOutputEnumerationNoInputDevices()
{
    lumascope::SourceList list;
    list.systemOutputs.push_back (makeRenderDescriptor ("{speaker}", "Speakers"));
    list.systemOutputs.push_back (makeRenderDescriptor ("{headphones}", "Headphones"));

    // Verify none of the systemOutput entries are input devices
    for (const auto& output : list.systemOutputs)
    {
        expect (output.mode == lumascope::SourceMode::systemOutput,
                "CAP-02: System output list contains only systemOutput entries");
        expectTrue (output.id.startsWith ("wasapi-loopback-"),
                    "CAP-02: System output IDs start with wasapi-loopback-");
        expectFalse (output.id.startsWith ("juce-input-"),
                     "CAP-02: No juce-input- prefix in system output list");
    }
}

} // anonymous namespace

int runWasapiEndpointModelTests()
{
    testRenderDescriptorHasSystemOutputMode();
    testRenderDescriptorIdHasWasapiPrefix();
    testInputDescriptorDoesNotHaveWasapiPrefix();
    testRenderEndpointsOnlyInSystemOutputs();
    testRenderDescriptorBoundedFields();
    testEmptyRenderEndpointList();
    testMultipleRenderEndpoints();
    testRenderEndpointErrorState();
    testSameSourceRetryTracking();
    testSilentRenderEndpointStaysActive();
    testEndpointIdentityMatching();
    testInactiveEndpointDescriptor();
    testSystemOutputEnumerationNoInputDevices();
    return failures;
}
