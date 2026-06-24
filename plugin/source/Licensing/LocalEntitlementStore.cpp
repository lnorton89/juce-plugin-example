#include "LumaScope/Licensing/LocalEntitlementStore.h"
#include <windows.h>
#include <dpapi.h>
#include <vector>

namespace lumascope {

juce::File LocalEntitlementStore::getEntitlementFilePath()
{
    const auto appData = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory);
    const auto dir = appData.getChildFile("LumaScope");
    dir.createDirectory();
    return dir.getChildFile("entitlement.dat");
}

juce::File LocalEntitlementStore::resolvePath() const
{
    if (basePath_ != juce::File{})
        return basePath_.getChildFile("entitlement.dat");
    return getEntitlementFilePath();
}

bool LocalEntitlementStore::write(const std::string& jsonContent)
{
    if (jsonContent.empty() || jsonContent.size() > 64 * 1024)
        return false;

    const auto file = resolvePath();
    file.getParentDirectory().createDirectory();

    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(jsonContent.data()));
    input.cbData = static_cast<DWORD>(jsonContent.size());

    DATA_BLOB output;
    if (!CryptProtectData(&input, L"LumaScopeEntitlement", nullptr, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return false;

    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
    {
        LocalFree(output.pbData);
        return false;
    }

    if (!stream.write(output.pbData, output.cbData))
    {
        LocalFree(output.pbData);
        return false;
    }
    stream.flush();
    LocalFree(output.pbData);
    return true;
}

std::optional<std::string> LocalEntitlementStore::read()
{
    const auto file = resolvePath();
    if (!file.existsAsFile())
        return std::nullopt;

    juce::FileInputStream stream(file);
    if (!stream.openedOk())
        return std::nullopt;

    const auto size = stream.getTotalLength();
    if (size <= 0 || size > 64 * 1024)
        return std::nullopt;

    std::vector<BYTE> encryptedData(static_cast<size_t>(size));
    if (stream.read(encryptedData.data(), static_cast<int>(size)) != size)
        return std::nullopt;

    DATA_BLOB input;
    input.pbData = encryptedData.data();
    input.cbData = static_cast<DWORD>(encryptedData.size());

    DATA_BLOB output;
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr,
                            nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;

    std::string result(reinterpret_cast<char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return result;
}

bool LocalEntitlementStore::clear()
{
    const auto file = resolvePath();
    if (!file.existsAsFile())
        return true;
    return file.deleteFile();
}

bool LocalEntitlementStore::exists() const
{
    return resolvePath().existsAsFile();
}

void LocalEntitlementStore::setBasePathForTesting(const juce::File& path)
{
    basePath_ = path;
}

} // namespace lumascope
