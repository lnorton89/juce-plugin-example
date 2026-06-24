#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <string>

namespace lumascope {

class LocalEntitlementStore {
public:
    static juce::File getEntitlementFilePath();

    bool write(const std::string& jsonContent);

    std::optional<std::string> read();

    bool clear();

    bool exists() const;

    void setBasePathForTesting(const juce::File& path);

private:
    juce::File basePath_;

    juce::File resolvePath() const;
};

} // namespace lumascope
