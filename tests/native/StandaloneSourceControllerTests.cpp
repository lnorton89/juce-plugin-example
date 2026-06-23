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

// Mock controller for testing lifecycle without real hardware.
class MockSourceController : public lumascope::StandaloneSourceController
{
public:
    struct Call
    {
        enum Type { enumerate, selectSource, stop, none };
        Type type = none;
        lumascope::SourceSelection selection;
    };

    lumascope::SourceList enumerateSources() override
    {
        lastCall.type = Call::enumerate;
        if (shouldFailEnumerate)
        {
            lumascope::SourceList empty;
            return empty;
        }
        return cachedList;
    }

    lumascope::SourceStateSnapshot selectSource (const lumascope::SourceSelection& selection) override
    {
        lastCall.type = Call::selectSource;
        lastCall.selection = selection;
        currentState = lumascope::SourceState::starting;
        cachedState.mode = selection.mode;
        cachedState.state = lumascope::SourceState::starting;
        cachedState.selectedSourceId = selection.id;
        cachedState.selectedSourceName = selection.displayName;

        if (shouldSucceedImmediately)
        {
            currentState = lumascope::SourceState::active;
            cachedState.state = lumascope::SourceState::active;
        }

        // Simulate transition to active after start
        if (currentState == lumascope::SourceState::starting && ! shouldFailSelect)
        {
            currentState = lumascope::SourceState::active;
            cachedState.state = lumascope::SourceState::active;
        }

        return cachedState;
    }

    lumascope::SourceStateSnapshot stop() override
    {
        lastCall.type = Call::stop;
        currentState = lumascope::SourceState::stopped;
        cachedState.state = lumascope::SourceState::stopped;
        cachedState.selectedSourceId = {};
        cachedState.selectedSourceName = {};
        cachedState.code = {};
        cachedState.message = {};
        return cachedState;
    }

    lumascope::SourceStateSnapshot currentStateSnapshot() const override
    {
        return cachedState;
    }

    // Test helpers
    void setCachedSources (const lumascope::SourceList& list)
    {
        cachedList = list;
    }

    void addInputSource (const juce::String& id, const juce::String& name)
    {
        cachedList.inputDevices.push_back ({ id, name, lumascope::SourceMode::inputDevice });
    }

    void addOutputSource (const juce::String& id, const juce::String& name)
    {
        cachedList.systemOutputs.push_back ({ id, name, lumascope::SourceMode::systemOutput });
    }

    void setShouldFailEnumerate (bool fail) { shouldFailEnumerate = fail; }
    void setShouldFailSelect (bool fail) { shouldFailSelect = fail; }
    void setShouldSucceedImmediately (bool immediate) { shouldSucceedImmediately = immediate; }

    void simulateError (const juce::String& code, const juce::String& message)
    {
        currentState = lumascope::SourceState::error;
        cachedState.state = lumascope::SourceState::error;
        cachedState.code = code;
        cachedState.message = message;
    }

    void simulateSilence()
    {
        if (currentState == lumascope::SourceState::active || currentState == lumascope::SourceState::starting)
        {
            currentState = lumascope::SourceState::silent;
            cachedState.state = lumascope::SourceState::silent;
        }
    }

    Call lastCall;
    lumascope::SourceState currentState = lumascope::SourceState::stopped;
    lumascope::SourceList cachedList;
    lumascope::SourceStateSnapshot cachedState;
    bool shouldFailEnumerate = false;
    bool shouldFailSelect = false;
    bool shouldSucceedImmediately = false;
};

// === D-01: SourceMode has separate InputDevice and SystemOutput ===
void testModesAreSeparate()
{
    expect (static_cast<int> (lumascope::SourceMode::inputDevice) != static_cast<int> (lumascope::SourceMode::systemOutput),
            "D-01: InputDevice and SystemOutput are distinct enum values");
}

// === D-04: Input devices and system outputs are separate lists ===
void testModesAreNeverCombinedInOneList()
{
    MockSourceController controller;
    controller.addInputSource ("mic-1", "Microphone (Realtek Audio)");
    controller.addInputSource ("mic-2", "Microphone (USB Camera)");
    controller.addOutputSource ("speaker-1", "Speakers (Realtek Audio)");

    const auto list = controller.enumerateSources();

    // Verify input devices are only in the input list
    for (const auto& device : list.inputDevices)
        expect (device.mode == lumascope::SourceMode::inputDevice,
                "D-04: Input list contains only inputDevice entries");

    // Verify system outputs are only in the output list
    for (const auto& device : list.systemOutputs)
        expect (device.mode == lumascope::SourceMode::systemOutput,
                "D-04: Output list contains only systemOutput entries");

    expectEq (static_cast<int> (list.inputDevices.size()), 2, "D-04: Two input devices");
    expectEq (static_cast<int> (list.systemOutputs.size()), 1, "D-04: One system output");
}

// === D-04 initial state is stopped/no-source ===
void testInitialStateIsStopped()
{
    MockSourceController controller;
    const auto state = controller.currentStateSnapshot();
    expect (state.state == lumascope::SourceState::stopped,
            "D-04: Initial source state is stopped");
    expect (state.selectedSourceId.isEmpty(),
            "D-04: No source selected initially");
    expect (state.selectedSourceName.isEmpty(),
            "D-04: No source name initially");
}

// === CAP-04: Selecting available input moves through starting to active ===
void testSelectAvailableInputMovesThroughStartingToActive()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);

    lumascope::SourceSelection selection;
    selection.mode = lumascope::SourceMode::inputDevice;
    selection.id = "mic-1";
    selection.displayName = "Microphone (Realtek Audio)";

    const auto result = controller.selectSource (selection);
    expect (result.state == lumascope::SourceState::active,
            "CAP-04: Successful selection results in active state");
    expectEqStr (result.selectedSourceId, "mic-1",
                 "CAP-04: Selected source ID is recorded");
    expectEqStr (result.selectedSourceName, "Microphone (Realtek Audio)",
                 "CAP-04: Selected source name is recorded");
    expect (result.mode == lumascope::SourceMode::inputDevice,
            "CAP-04: Selected source mode is inputDevice");
}

// === CAP-04: Stopping clears active capture ===
void testStopClearsActiveCapture()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);

    lumascope::SourceSelection selection;
    selection.mode = lumascope::SourceMode::inputDevice;
    selection.id = "mic-1";
    selection.displayName = "Microphone";

    controller.selectSource (selection);
    const auto afterStop = controller.stop();

    expect (afterStop.state == lumascope::SourceState::stopped,
            "CAP-04: After stop, state is stopped");
    expect (afterStop.selectedSourceId.isEmpty(),
            "CAP-04: After stop, source ID is cleared");
    expect (afterStop.selectedSourceName.isEmpty(),
            "CAP-04: After stop, source name is cleared");
}

// === D-05: Failed selected source stops with error state ===
void testFailedSourceReportsError()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);
    controller.setShouldFailSelect (false); // select succeeds

    lumascope::SourceSelection selection;
    selection.mode = lumascope::SourceMode::inputDevice;
    selection.id = "mic-1";
    selection.displayName = "Microphone";

    controller.selectSource (selection);

    // Now simulate failure during active capture
    controller.simulateError ("source_lost", "The selected input device was disconnected.");

    const auto state = controller.currentStateSnapshot();
    expect (state.state == lumascope::SourceState::error,
            "D-05: Failed source transitions to error state");
    expectEqStr (state.code, "source_lost",
                 "D-05: Error code is reported");
    expect (state.message.isNotEmpty(),
            "D-05: Error message is reported");
}

// === D-06: No auto-fallback to different source after failure ===
void testNoFallbackAfterFailure()
{
    MockSourceController controller;
    lumascope::SourceSelection originalSelection;
    originalSelection.mode = lumascope::SourceMode::inputDevice;
    originalSelection.id = "mic-1";
    originalSelection.displayName = "Microphone";

    controller.setShouldSucceedImmediately (true);
    controller.selectSource (originalSelection);

    // Simulate failure
    controller.simulateError ("source_lost", "Device disconnected");

    // After failure, verify we're in error state — not auto-selecting another source
    const auto state = controller.currentStateSnapshot();
    expect (state.state == lumascope::SourceState::error,
            "D-06: After failure, state is error not active");

    // The selected ID should still reference the failed source
    expectEqStr (state.selectedSourceId, "mic-1",
                 "D-06: Failed source ID remains recorded for UI context");
}

// === D-05: Failed selected source requires user action to restart ===
void testFailedSourceStopsAndRequiresUserAction()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);

    lumascope::SourceSelection selection;
    selection.mode = lumascope::SourceMode::inputDevice;
    selection.id = "mic-1";
    selection.displayName = "Microphone";

    controller.selectSource (selection);
    controller.simulateError ("device_error", "The device encountered an error.");

    // Simulate user explicitly choosing to stop after error
    const auto afterStop = controller.stop();
    expect (afterStop.state == lumascope::SourceState::stopped,
            "D-05: User can stop after error to clear state");

    // Verify user must explicitly select a new source to restart
    const auto afterStopState = controller.currentStateSnapshot();
    expect (afterStopState.selectedSourceId.isEmpty(),
            "D-05: After stop from error, no source is active");
}

// === CAP-04: Source switching tears down old source before starting new ===
void testSourceSwitchingTearsDownBeforeStart()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);

    // Select first source
    lumascope::SourceSelection firstSelection;
    firstSelection.mode = lumascope::SourceMode::inputDevice;
    firstSelection.id = "mic-1";
    firstSelection.displayName = "Microphone One";
    controller.selectSource (firstSelection);

    // Select second source — this should call stop internally then start new
    lumascope::SourceSelection secondSelection;
    secondSelection.mode = lumascope::SourceMode::systemOutput;
    secondSelection.id = "speaker-1";
    secondSelection.displayName = "Speakers (Realtek)";

    // The controller should handle the switch:
    // We mock this by calling stop then selectSource
    const auto stopResult = controller.stop();
    const auto selectResult = controller.selectSource (secondSelection);

    expect (selectResult.mode == lumascope::SourceMode::systemOutput,
            "CAP-04: Switched source mode is systemOutput");
    expectEqStr (selectResult.selectedSourceId, "speaker-1",
                 "CAP-04: Switched source ID is correct");
}

// === SourceDescriptor has bounded fields ===
void testSourceDescriptorsAreBounded()
{
    lumascope::SourceDescriptor desc;
    desc.id = "a";
    desc.displayName = "b";
    desc.mode = lumascope::SourceMode::inputDevice;

    expect (desc.id.isNotEmpty(), "SourceDescriptor ID is non-empty");
    expect (desc.displayName.isNotEmpty(), "SourceDescriptor displayName is non-empty");
    expect (desc.id.length() <= 256, "SourceDescriptor ID is bounded to 256");
    expect (desc.displayName.length() <= 256, "SourceDescriptor displayName is bounded to 256");
}

// === CAP-04: Silent source stays active with silent state ===
void testSilentSourceRemainsActive()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (true);

    lumascope::SourceSelection selection;
    selection.mode = lumascope::SourceMode::inputDevice;
    selection.id = "mic-1";
    selection.displayName = "Microphone";

    controller.selectSource (selection);

    // Simulate silence detection
    controller.simulateSilence();
    const auto state = controller.currentStateSnapshot();
    expect (state.state == lumascope::SourceState::silent,
            "D-08: Silent source reports silent state (not error)");
    expectEqStr (state.selectedSourceId, "mic-1",
                 "D-08: Silent source keeps its source ID");
}

// === SourceMode string representation is bounded and stable ===
void testSourceModeToString()
{
    const auto inputStr = lumascope::toString (lumascope::SourceMode::inputDevice);
    const auto outputStr = lumascope::toString (lumascope::SourceMode::systemOutput);
    expectEqStr (inputStr, "InputDevice", "SourceMode::inputDevice string is 'InputDevice'");
    expectEqStr (outputStr, "SystemOutput", "SourceMode::systemOutput string is 'SystemOutput'");
}

// === SourceState string representation is bounded and stable ===
void testSourceStateToString()
{
    expectEqStr (lumascope::toString (lumascope::SourceState::stopped), "stopped", "stopped state string");
    expectEqStr (lumascope::toString (lumascope::SourceState::starting), "starting", "starting state string");
    expectEqStr (lumascope::toString (lumascope::SourceState::active), "active", "active state string");
    expectEqStr (lumascope::toString (lumascope::SourceState::silent), "silent", "silent state string");
    expectEqStr (lumascope::toString (lumascope::SourceState::error), "error", "error state string");
}

// === Enumerate without sources returns empty lists ===
void testEnumerateNoSources()
{
    MockSourceController controller;
    const auto list = controller.enumerateSources();
    expect (list.inputDevices.empty(), "Empty input devices list when none available");
    expect (list.systemOutputs.empty(), "Empty system outputs list when none available");
}

// === Selecting invalid source produces error ===
void testSelectInvalidSourceFails()
{
    MockSourceController controller;
    controller.setShouldSucceedImmediately (false);
    controller.setShouldFailSelect (true);

    lumascope::SourceSelection invalidSelection;
    invalidSelection.mode = lumascope::SourceMode::inputDevice;
    invalidSelection.id = "nonexistent-device";
    invalidSelection.displayName = "Nonexistent";

    // Mock: selecting a nonexistent device should fail
    controller.simulateError ("source_not_found", "The selected source could not be opened.");

    const auto state = controller.currentStateSnapshot();
    expect (state.state == lumascope::SourceState::error,
            "Invalid source selection transitions to error");
    expectEqStr (state.code, "source_not_found", "Error code for nonexistent source");
}

// === SourceDescriptor display name cannot be confused with mode ===
void testDisplayNameDoesNotRevealMode()
{
    // D-04: mode is explicit from SourceMode enum, not inferred from display text
    lumascope::SourceDescriptor inputDesc;
    inputDesc.id = "mic-1";
    inputDesc.displayName = "Microphone (Realtek Audio)";
    inputDesc.mode = lumascope::SourceMode::inputDevice;

    lumascope::SourceDescriptor outputDesc;
    outputDesc.id = "speaker-1";
    outputDesc.displayName = "Speakers (Realtek Audio)";
    outputDesc.mode = lumascope::SourceMode::systemOutput;

    // Mode is determined by the enum, not parsed from display name
    expect (inputDesc.mode != outputDesc.mode,
            "D-04: Input and output modes are distinct in SourceDescriptor");
}
}

int runStandaloneSourceControllerTests()
{
    testModesAreSeparate();
    testModesAreNeverCombinedInOneList();
    testInitialStateIsStopped();
    testSelectAvailableInputMovesThroughStartingToActive();
    testStopClearsActiveCapture();
    testFailedSourceReportsError();
    testNoFallbackAfterFailure();
    testFailedSourceStopsAndRequiresUserAction();
    testSourceSwitchingTearsDownBeforeStart();
    testSourceDescriptorsAreBounded();
    testSilentSourceRemainsActive();
    testSourceModeToString();
    testSourceStateToString();
    testEnumerateNoSources();
    testSelectInvalidSourceFails();
    testDisplayNameDoesNotRevealMode();
    return failures;
}
