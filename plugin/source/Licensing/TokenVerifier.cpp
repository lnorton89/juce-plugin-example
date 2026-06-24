#include "LumaScope/Licensing/TokenVerifier.h"
#include "LumaScope/Licensing/BuiltinPublicKeyRing.h"
#include "tweetnacl/tweetnacl.h"
#include <cstring>
#include <vector>

namespace lumascope {

TokenVerifier::TokenVerifier(PublicKeyRing keyRing)
    : keyRing_(std::move(keyRing))
{
}

VerificationResult TokenVerifier::verify(const SignedEntitlementToken& token) const
{
    VerificationResult result;

    auto* keyEntry = keyRing_.findByKid(token.kid);
    if (!keyEntry)
    {
        result.errorCode = "unknown_key_id";
        result.errorMessage = "No public key found for kid: " + token.kid;
        return result;
    }

    auto rawKey = PublicKeyRing::decodePublicKey(keyEntry->publicKey);
    if (rawKey.size() != 32)
    {
        result.errorCode = "invalid_key";
        result.errorMessage = "Decoded public key is not 32 bytes";
        return result;
    }

    auto rawSignature = PublicKeyRing::base64UrlDecode(token.signature);
    if (rawSignature.size() != 64)
    {
        result.errorCode = "invalid_signature";
        result.errorMessage = "Decoded signature is not 64 bytes";
        return result;
    }

    std::vector<unsigned char> msgBytes(token.canonical.begin(), token.canonical.end());

    if (!verifyDetached(msgBytes, rawSignature, rawKey))
    {
        result.errorCode = "signature_verification_failed";
        result.errorMessage = "Ed25519 signature does not match";
        return result;
    }

    auto claimsResult = verifyClaims(token.payload);
    if (!claimsResult.valid)
        return claimsResult;

    result.valid = true;
    return result;
}

bool TokenVerifier::verifyDetached(
    const std::vector<unsigned char>& message,
    const std::vector<unsigned char>& signature,
    const std::vector<unsigned char>& publicKeyRaw)
{
    if (signature.size() != 64)
        return false;
    if (publicKeyRaw.size() != 32)
        return false;

    return ed25519_verify_detached(
        signature.data(),
        message.data(),
        message.size(),
        publicKeyRaw.data()) == 0;
}

VerificationResult TokenVerifier::verifyClaims(const EntitlementClaims& claims) const
{
    VerificationResult result;

    if (claims.schemaVersion != 1)
    {
        result.errorCode = "unsupported_schema_version";
        result.errorMessage = "Expected schema version 1, got " + std::to_string(claims.schemaVersion);
        return result;
    }

    if (claims.status != "active")
    {
        result.errorCode = "inactive_status";
        result.errorMessage = "Entitlement status is not active";
        return result;
    }

    if (claims.machineId.empty())
    {
        result.errorCode = "missing_machine_id";
        result.errorMessage = "Entitlement claims missing machineId";
        return result;
    }

    if (claims.activationId.empty())
    {
        result.errorCode = "missing_activation_id";
        result.errorMessage = "Entitlement claims missing activationId";
        return result;
    }

    if (claims.productId.empty())
    {
        result.errorCode = "missing_product_id";
        result.errorMessage = "Entitlement claims missing productId";
        return result;
    }

    result.valid = true;
    return result;
}

} // namespace lumascope
