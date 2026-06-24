#include "LumaScope/Standalone/SourcePreferenceStore.h"
#include "LumaScope/ProjectConfig.h"

namespace lumascope
{

SourcePreferenceStore::SourcePreferenceStore (const juce::File& appDataDir)
    : prefsFile (appDataDir.getChildFile (config::appdataDirName).getChildFile (config::sourcePreferenceFilename))
{
}

void SourcePreferenceStore::save (const SourceSelection& selection)
{
    if (selection.id.isEmpty())
        return;

    prefsFile.getParentDirectory().createDirectory();

    auto root = juce::var (new juce::DynamicObject());
    auto* object = root.getDynamicObject();
    object->setProperty ("schemaVersion", currentSchemaVersion);
    object->setProperty ("mode", toString (selection.mode));
    object->setProperty ("id", selection.id.substring (0, maxIdLength));
    object->setProperty ("displayName", selection.displayName.substring (0, maxDisplayNameLength));

    const auto json = juce::JSON::toString (root, true);
    prefsFile.replaceWithText (json);
}

std::optional<SourceSelection> SourcePreferenceStore::tryRestore()
{
    if (! prefsFile.existsAsFile())
        return std::nullopt;

    const auto root = juce::JSON::parse (prefsFile);
    if (root == juce::var() || ! root.isObject())
        return std::nullopt;

    const auto* object = root.getDynamicObject();
    if (object == nullptr)
        return std::nullopt;

    // Validate schema version
    const auto schemaVersion = object->getProperty ("schemaVersion");
    if (static_cast<int> (schemaVersion) != currentSchemaVersion)
        return std::nullopt;

    // Parse mode
    const auto modeStr = object->getProperty ("mode").toString();
    SourceMode mode;
    if (modeStr == "InputDevice")
        mode = SourceMode::inputDevice;
    else if (modeStr == "SystemOutput")
        mode = SourceMode::systemOutput;
    else
        return std::nullopt;

    // Parse ID — must be non-empty and bounded
    const auto id = object->getProperty ("id").toString();
    if (id.isEmpty() || id.length() > maxIdLength)
        return std::nullopt;

    // Parse display name — bounded
    const auto displayName = object->getProperty ("displayName").toString();
    if (displayName.length() > maxDisplayNameLength)
        return std::nullopt;

    return SourceSelection { mode, id, displayName };
}

void SourcePreferenceStore::clear()
{
    if (prefsFile.existsAsFile())
        prefsFile.deleteFile();
}

} // namespace lumascope
