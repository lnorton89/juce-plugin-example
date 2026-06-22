#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace lumascope
{
class WebResources
{
public:
    WebResources (const void* archiveData, std::size_t archiveSize);
    std::optional<juce::WebBrowserComponent::Resource> get (const juce::String& requestPath) const;

private:
    static std::optional<juce::String> normalisePath (const juce::String& requestPath);
    static std::optional<juce::String> mimeTypeFor (const juce::String& path);

    juce::MemoryBlock archiveBytes;
};
}

