#pragma once
#include "EntitlementToken.h"
#include "PublicKeyRing.h"
#include <vector>
#include <string>

namespace lumascope {

struct VerificationResult {
    bool valid = false;
    std::string errorCode;
    std::string errorMessage;
};

class TokenVerifier {
public:
    explicit TokenVerifier(PublicKeyRing keyRing);

    VerificationResult verify(const SignedEntitlementToken& token) const;

    static bool verifyDetached(
        const std::vector<unsigned char>& message,
        const std::vector<unsigned char>& signature,
        const std::vector<unsigned char>& publicKeyRaw);

    const PublicKeyRing& keyRing() const noexcept { return keyRing_; }

private:
    PublicKeyRing keyRing_;

    VerificationResult verifyClaims(const EntitlementClaims& claims) const;
};

} // namespace lumascope
