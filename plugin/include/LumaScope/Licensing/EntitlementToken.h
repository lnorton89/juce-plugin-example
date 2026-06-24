#pragma once
#include <juce_core/juce_core.h>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace lumascope {

struct EntitlementClaims {
    int schemaVersion = 1;
    std::string licenseKeyHash;
    std::string activationId;
    std::string machineId;
    std::string productId;
    std::string variantId;
    std::string status;
    std::string issuedAt;
    std::string refreshAfter;
    std::string expiresAt;
    std::string kid;
};

struct SignedEntitlementToken {
    EntitlementClaims payload;
    std::string signature;
    std::string kid;
    std::string algorithm;
    std::string canonical;
};

std::string canonicalJson(const juce::var& value);

std::optional<SignedEntitlementToken> parseSignedEntitlement(const juce::var& json);

SignedEntitlementToken parseSignedEntitlementOrThrow(const juce::var& json);

juce::var claimsToVar(const EntitlementClaims& claims);

} // namespace lumascope
