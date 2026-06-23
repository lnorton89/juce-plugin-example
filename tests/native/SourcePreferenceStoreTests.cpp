#include "LumaScope/Standalone/SourcePreferenceStore.h"
#include "LumaScope/Standalone/SourceModel.h"

#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect (bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

void expectEqStr (const juce::String& actual, const juce::String& expected, const char* message)
{
    if (actual != expected)
    {
        std::cerr << "FAIL: " << message << " actual=\"" << actual << "\" expected=\"" << expected << "\"\n";
        ++failures;
    }
}

// Use a temp directory for test persistence
juce::File getTestPrefsDir()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("LumaScopePrefsTest_" + juce::String (juce::Random::getSystemRandom().nextInt (99999)));
}

// === CAP-05: Save and restore a valid source selection ===
void testSaveAndRestore()
{
    lumascope::SourcePreferenceStore store (getTestPrefsDir());
    store.save ({ lumascope::SourceMode::inputDevice, "juce-input-mic-1", "Microphone (Realtek Audio)" });

    const auto restored = store.tryRestore();
    expect (restored.has_value(), "CAP-05: Restore returns a value after save");
    expect (restored->mode == lumascope::SourceMode::inputDevice,
            "CAP-05: Restored mode matches saved");
    expectEqStr (restored->id, "juce-input-mic-1",
                 "CAP-05: Restored ID matches saved");
    expectEqStr (restored->displayName, "Microphone (Realtek Audio)",
                 "CAP-05: Restored display name matches saved");
}

// === D-14: Missing saved source file returns no value ===
void testMissingFileReturnsEmpty()
{
    lumascope::SourcePreferenceStore store (
        juce::File::getSpecialLocation (juce::File::tempDirectory)
            .getChildFile ("nonexistent_dir_" + juce::String (juce::Random::getSystemRandom().nextInt (99999))));

    const auto restored = store.tryRestore();
    expect (! restored.has_value(),
            "D-14: Missing preference file returns empty");
}

// === D-14: SystemOutput mode is saved and restored correctly ===
void testSystemOutputSaveAndRestore()
{
    lumascope::SourcePreferenceStore store (getTestPrefsDir());
    store.save ({ lumascope::SourceMode::systemOutput, "wasapi-speaker-1", "Speakers (Realtek Audio)" });

    const auto restored = store.tryRestore();
    expect (restored.has_value(), "SystemOutput: Restore returns a value after save");
    expect (restored->mode == lumascope::SourceMode::systemOutput,
            "SystemOutput: Restored mode matches saved");
    expectEqStr (restored->id, "wasapi-speaker-1",
                 "SystemOutput: Restored ID matches saved");
    expectEqStr (restored->displayName, "Speakers (Realtek Audio)",
                 "SystemOutput: Restored display name matches saved");
}

// === CAP-05: Empty store after clear ===
void testClearStore()
{
    lumascope::SourcePreferenceStore store (getTestPrefsDir());
    store.save ({ lumascope::SourceMode::inputDevice, "mic-1", "Microphone" });
    store.clear();

    const auto restored = store.tryRestore();
    expect (! restored.has_value(),
            "CAP-05: After clear, restore returns empty");
}

// === Preference store handles invalid/corrupt data gracefully ===
void testCorruptData()
{
    lumascope::SourcePreferenceStore store (getTestPrefsDir());

    // Write corrupt data
    const auto file = store.getPreferenceFile();
    file.replaceWithText ("{ invalid json }");

    const auto restored = store.tryRestore();
    expect (! restored.has_value(),
            "Corrupt preference data returns empty");
}

// === Multiple saves overwrite previous values ===
void testOverwriteOnRepeatedSave()
{
    lumascope::SourcePreferenceStore store (getTestPrefsDir());
    store.save ({ lumascope::SourceMode::inputDevice, "mic-1", "Microphone" });
    store.save ({ lumascope::SourceMode::systemOutput, "speaker-1", "Speakers" });

    const auto restored = store.tryRestore();
    expect (restored.has_value(), "Overwrite: Restore returns a value");
    expect (restored->mode == lumascope::SourceMode::systemOutput,
            "Overwrite: Last saved mode wins");
    expectEqStr (restored->id, "speaker-1",
                 "Overwrite: Last saved ID wins");
}
}

int runSourcePreferenceStoreTests()
{
    testSaveAndRestore();
    testMissingFileReturnsEmpty();
    testSystemOutputSaveAndRestore();
    testClearStore();
    testCorruptData();
    testOverwriteOnRepeatedSave();
    return failures;
}
