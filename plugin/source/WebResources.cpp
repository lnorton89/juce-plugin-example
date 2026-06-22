#include "LumaScope/WebResources.h"

namespace lumascope
{
WebResources::WebResources (const void* archiveData, std::size_t archiveSize)
    : archiveBytes (archiveData, archiveSize)
{
}

std::optional<juce::String> WebResources::normalisePath (const juce::String& requestPath)
{
    auto path = requestPath.upToFirstOccurrenceOf ("?", false, false)
                           .upToFirstOccurrenceOf ("#", false, false)
                           .replaceCharacter ('\\', '/');
    if (path == "/")
        return juce::String { "index.html" };
    if (path.startsWithChar ('/'))
        path = path.substring (1);
    if (path.isEmpty() || path.startsWithChar ('/') || path.contains ("../")
        || path.contains ("/..") || path == ".." || path.containsChar (':'))
        return std::nullopt;
    return path;
}

std::optional<juce::String> WebResources::mimeTypeFor (const juce::String& path)
{
    const auto extension = juce::File (path).getFileExtension().toLowerCase();
    if (extension == ".html") return "text/html";
    if (extension == ".js") return "text/javascript";
    if (extension == ".css") return "text/css";
    if (extension == ".json") return "application/json";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".png") return "image/png";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".woff2") return "font/woff2";
    return std::nullopt;
}

std::optional<juce::WebBrowserComponent::Resource> WebResources::get (const juce::String& requestPath) const
{
    const auto path = normalisePath (requestPath);
    if (! path)
        return std::nullopt;
    const auto mimeType = mimeTypeFor (*path);
    if (! mimeType)
        return std::nullopt;

    auto input = std::make_unique<juce::MemoryInputStream> (archiveBytes, false);
    juce::ZipFile archive (std::move (input));
    const auto index = archive.getIndexOfFileName (*path, false);
    if (index < 0)
        return std::nullopt;
    std::unique_ptr<juce::InputStream> stream (archive.createStreamForEntry (index));
    if (stream == nullptr)
        return std::nullopt;

    const auto size = stream->getTotalLength();
    if (size < 0 || size > 16 * 1024 * 1024)
        return std::nullopt;
    std::vector<std::byte> data (static_cast<std::size_t> (size));
    const auto bytesRead = stream->read (data.data(), static_cast<int> (data.size()));
    if (bytesRead != static_cast<int> (data.size()))
        return std::nullopt;
    return juce::WebBrowserComponent::Resource { std::move (data), *mimeType };
}
}
