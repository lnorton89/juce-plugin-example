#include "LumaScope/Licensing/PublicKeyRing.h"
#include <cctype>
#include <algorithm>

namespace lumascope {

std::vector<unsigned char> PublicKeyRing::base64UrlDecode(const std::string& input)
{
    static const signed char decodeTable[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,62,-1,-1,
        52,53,54,55,56,57,58,59, 60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6,  7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22, 23,24,25,-1,-1,-1,-1,63,
        -1,26,27,28,29,30,31,32, 33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48, 49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    };

    std::vector<unsigned char> result;
    result.reserve((input.size() * 3) / 4 + 1);

    std::vector<int> values;
    values.reserve(input.size());
    for (auto ch : input)
    {
        if (ch == '=') break;
        int v = decodeTable[static_cast<unsigned char>(ch)];
        if (v >= 0) values.push_back(v);
    }

    for (size_t i = 0; i + 3 < values.size(); i += 4)
    {
        auto a = static_cast<unsigned int>(values[i]);
        auto b = static_cast<unsigned int>(values[i+1]);
        auto c = static_cast<unsigned int>(values[i+2]);
        auto d = static_cast<unsigned int>(values[i+3]);
        unsigned int triple = (a << 18) | (b << 12) | (c << 6) | d;
        result.push_back(static_cast<unsigned char>((triple >> 16) & 0xFF));
        result.push_back(static_cast<unsigned char>((triple >> 8) & 0xFF));
        result.push_back(static_cast<unsigned char>(triple & 0xFF));
    }

    auto totalInput = values.size();
    auto remaining = totalInput % 4;
    if (remaining == 2)
    {
        auto a = static_cast<unsigned int>(values[totalInput - 2]);
        auto b = static_cast<unsigned int>(values[totalInput - 1]);
        result.push_back(static_cast<unsigned char>(((a << 6) | b) >> 4));
    }
    else if (remaining == 3)
    {
        auto a = static_cast<unsigned int>(values[totalInput - 3]);
        auto b = static_cast<unsigned int>(values[totalInput - 2]);
        auto c = static_cast<unsigned int>(values[totalInput - 1]);
        unsigned int combined = static_cast<unsigned int>(
            (static_cast<unsigned long long>(a) << 12) |
            (static_cast<unsigned long long>(b) << 6) |
            c);
        result.push_back(static_cast<unsigned char>((combined >> 10) & 0xFF));
        result.push_back(static_cast<unsigned char>((combined >> 2) & 0xFF));
    }

    return result;
}

std::optional<PublicKeyRing> PublicKeyRing::parse(const std::string& json)
{
    auto parsed = juce::JSON::parse(json);
    return parse(parsed);
}

std::optional<PublicKeyRing> PublicKeyRing::parse(const juce::var& jsonVar)
{
    auto* arr = jsonVar.getArray();
    if (!arr) return std::nullopt;

    PublicKeyRing ring;
    for (auto& entry : *arr)
    {
        auto* obj = entry.getDynamicObject();
        if (!obj) return std::nullopt;

        auto& props = obj->getProperties();
        auto kid = props["kid"].toString().toStdString();
        auto publicKey = props["publicKey"].toString().toStdString();
        auto algorithm = props["algorithm"].toString().toStdString();

        if (kid.empty() || kid.size() > 64) return std::nullopt;
        if (publicKey.empty()) return std::nullopt;
        if (algorithm != "Ed25519") return std::nullopt;

        ring.entries.push_back({std::move(kid), std::move(publicKey), std::move(algorithm)});
    }

    return ring;
}

const PublicKeyEntry* PublicKeyRing::findByKid(const std::string& kid) const
{
    for (auto& entry : entries)
    {
        if (entry.kid == kid)
            return &entry;
    }
    return nullptr;
}

std::vector<unsigned char> PublicKeyRing::decodePublicKey(const std::string& base64urlKey)
{
    auto decoded = base64UrlDecode(base64urlKey);

    if (decoded.size() == 32)
        return decoded;

    if (decoded.size() > 32)
    {
        std::vector<unsigned char> raw(decoded.end() - 32, decoded.end());
        return raw;
    }

    return {};
}

} // namespace lumascope
