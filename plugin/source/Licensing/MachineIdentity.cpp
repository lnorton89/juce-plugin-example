#include "LumaScope/Licensing/MachineIdentity.h"
#include <windows.h>
#include <bcrypt.h>
#include <sddl.h>
#include <array>
#include <cassert>
#include <mutex>
#include <vector>

namespace lumascope {
namespace {

std::string base64urlEncode(const unsigned char* data, size_t len)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve(44);
    for (size_t i = 0; i < len; i += 3)
    {
        uint32_t triple = (static_cast<uint32_t>(data[i]) << 16)
                        | (i + 1 < len ? static_cast<uint32_t>(data[i + 1]) << 8 : 0)
                        | (i + 2 < len ? static_cast<uint32_t>(data[i + 2]) : 0);
        result += alphabet[(triple >> 18) & 0x3F];
        result += alphabet[(triple >> 12) & 0x3F];
        if (i + 1 < len) result += alphabet[(triple >> 6) & 0x3F];
        if (i + 2 < len) result += alphabet[triple & 0x3F];
    }
    return result;
}

std::vector<unsigned char> sha256(const std::string& input)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        return {};

    DWORD hashLen = 0;
    DWORD resultLen = 0;
    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLen), sizeof(hashLen), &resultLen, 0);

    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0) != 0)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    std::vector<unsigned char> hash(hashLen);
    NTSTATUS status = BCryptHashData(hHash,
        const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(input.data())),
        static_cast<ULONG>(input.size()), 0);

    if (status == 0)
        status = BCryptFinishHash(hHash, hash.data(), static_cast<ULONG>(hash.size()), 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != 0)
        return {};

    return hash;
}

std::string deriveInternal()
{
    // 1. Get computer name for SID lookup
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD nameSize = static_cast<DWORD>(std::size(computerName));
    if (!GetComputerNameW(computerName, &nameSize))
        return {};

    // 2. Get machine SID
    char sidBuffer[SECURITY_MAX_SID_SIZE] = {};
    DWORD sidSize = sizeof(sidBuffer);
    wchar_t domainBuffer[256] = {};
    DWORD domainSize = static_cast<DWORD>(std::size(domainBuffer));
    SID_NAME_USE sidNameUse{};

    std::string sidString;
    if (LookupAccountNameW(nullptr, computerName, reinterpret_cast<PSID>(sidBuffer),
                            &sidSize, domainBuffer, &domainSize, &sidNameUse))
    {
        LPWSTR sidStr = nullptr;
        if (ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer), &sidStr))
        {
            int len = WideCharToMultiByte(CP_UTF8, 0, sidStr, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0)
            {
                sidString.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, sidStr, -1, sidString.data(), len, nullptr, nullptr);
            }
            LocalFree(sidStr);
        }
    }

    if (sidString.empty())
        return {};

    // 3. Get system drive volume serial
    wchar_t systemDir[MAX_PATH + 1] = {};
    if (!GetSystemDirectoryW(systemDir, MAX_PATH))
        return {};

    wchar_t rootPath[4] = { systemDir[0], L':', L'\\', L'\0' };

    DWORD volumeSerial = 0;
    if (!GetVolumeInformationW(rootPath, nullptr, 0, &volumeSerial,
                                nullptr, nullptr, nullptr, 0))
        return {};

    // 4. Composite: version + SID + volume serial
    const std::string composite = "v1:" + sidString + ":" + std::to_string(volumeSerial);

    // 5. SHA-256 hash via Windows CNG
    auto hash = sha256(composite);
    if (hash.empty())
        return {};

    // 6. Base64url encode
    return "mid_v1_" + base64urlEncode(hash.data(), hash.size());
}

std::string cachedResult;
std::once_flag cacheFlag;

} // anonymous namespace

std::string deriveMachineIdentifier()
{
    std::call_once(cacheFlag, [] { cachedResult = deriveInternal(); });
    return cachedResult;
}

MachineIdentityInfo deriveMachineIdentityInfo()
{
    MachineIdentityInfo info;
    info.identifier = deriveMachineIdentifier();
    info.version = "v1";
    info.hashAlgorithm = "SHA-256";
    return info;
}

} // namespace lumascope
