#pragma once

#include "LumaScope/Standalone/SourceModel.h"

#include <juce_core/juce_core.h>

#include <optional>

namespace lumascope
{

// Non-secret source preference persistence for CAP-05.
// Stores the last valid user-selected source (mode, ID, display name)
// in the user's application data directory. Uses simple JSON format
// with a schema version for forward compatibility.
//
// This store intentionally does NOT store raw hardware identifiers,
// account credentials, or any sensitive data (T-03-03-02).
//
// Design constraints:
// - D-14: Missing/unavailable saved source returns empty (no auto-fallback)
// - D-15: Never auto-selects a different source
// - Persist only after successful user-selected activation
class SourcePreferenceStore
{
public:
    explicit SourcePreferenceStore (const juce::File& appDataDir);

    // Save the current source selection to preferences.
    // Only called after successful user-selected source activation.
    void save (const SourceSelection& selection);

    // Try to restore the last saved source selection.
    // Returns empty if no preference file exists, data is corrupt,
    // schema version is unknown, or values are malformed.
    std::optional<SourceSelection> tryRestore();

    // Clear saved preferences (e.g., on deactivation or factory reset).
    void clear();

    // Exposed for testing — get the backing file path.
    juce::File getPreferenceFile() const noexcept { return prefsFile; }

private:
    static constexpr int currentSchemaVersion = 1;
    static constexpr int maxIdLength = 256;
    static constexpr int maxDisplayNameLength = 256;

    juce::File prefsFile;
};

} // namespace lumascope
