#pragma once
#include <juce_core/juce_core.h>
#include <string>
#include <vector>
#include <optional>

namespace lumascope {

struct PublicKeyEntry {
    std::string kid;
    std::string publicKey;
    std::string algorithm;
};

class PublicKeyRing {
public:
    static std::optional<PublicKeyRing> parse(const std::string& json);
    static std::optional<PublicKeyRing> parse(const juce::var& jsonVar);

    const PublicKeyEntry* findByKid(const std::string& kid) const;
    bool isEmpty() const noexcept { return entries.empty(); }
    size_t size() const noexcept { return entries.size(); }

    static std::vector<unsigned char> decodePublicKey(const std::string& base64urlKey);
    static std::vector<unsigned char> base64UrlDecode(const std::string& input);

private:
    std::vector<PublicKeyEntry> entries;
};

} // namespace lumascope
