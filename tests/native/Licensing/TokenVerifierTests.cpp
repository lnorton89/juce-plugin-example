#include "LumaScope/Licensing/EntitlementToken.h"
#include "LumaScope/Licensing/TokenVerifier.h"
#include "LumaScope/Licensing/PublicKeyRing.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <string>

int runTokenVerifierTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // Load fixtures
    juce::File fixtureFile(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
    auto fixtureVar = juce::JSON::parse(fixtureFile);
    auto* fixtureObj = fixtureVar.getDynamicObject();
    expect(fixtureObj != nullptr, "fixture parses to object");

    juce::File keyRingFile(LUMASCOPE_FIXTURE_KEY_RING);
    auto keyRingJson = juce::JSON::parse(keyRingFile);

    // Build token envelope from fixture data
    auto tokenObj = new juce::DynamicObject();
    tokenObj->setProperty("payload", fixtureObj->getProperties()["payload"]);
    tokenObj->setProperty("signature", fixtureObj->getProperties()["signature"]);
    tokenObj->setProperty("kid", juce::String("test-ed25519-2026-06"));
    tokenObj->setProperty("algorithm", juce::String("Ed25519"));
    auto tokenVar = juce::var(tokenObj);

    auto token = lumascope::parseSignedEntitlement(tokenVar);
    expect(token.has_value(), "fixture parses to SignedEntitlementToken");
    if (!token) return failures;

    auto keyRing = lumascope::PublicKeyRing::parse(keyRingJson);
    expect(keyRing.has_value(), "key ring parses");
    if (!keyRing) return failures;

    lumascope::TokenVerifier verifier(std::move(*keyRing));

    // testVerifyEntitlementFromFixture
    {
        auto result = verifier.verify(*token);
        expect(result.valid, "verify returns valid=true for fixture token");
        if (!result.valid)
            std::cerr << "  errorCode: " << result.errorCode
                      << ", message: " << result.errorMessage << '\n';
    }

    // testVerifyWithWrongPublicKey
    {
        auto badKeyRing = lumascope::PublicKeyRing::parse(
            std::string(R"JSON([
            { "kid": "bad-key", "publicKey": "MCowBQYDK2VwAyEAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB", "algorithm": "Ed25519" }
        ])JSON"));
        expect(badKeyRing.has_value(), "bad key ring parses");
        if (badKeyRing)
        {
            lumascope::TokenVerifier badVerifier(std::move(*badKeyRing));
            auto result = badVerifier.verify(*token);
            expect(!result.valid, "verify returns false with wrong public key");
        }
    }

    // testVerifyTamperedPayload
    {
        auto tamperedToken = *token;
        tamperedToken.payload.machineId = "different_machine";
        tamperedToken.canonical = lumascope::canonicalJson(
            lumascope::claimsToVar(tamperedToken.payload));
        // reuse the same verifier - kid lookup works but signature won't match
        // Since we moved keyRing into verifier, create a new one
        juce::File krFile(LUMASCOPE_FIXTURE_KEY_RING);
        auto krJson = juce::JSON::parse(krFile);
        auto kr = lumascope::PublicKeyRing::parse(krJson);
        lumascope::TokenVerifier tv(std::move(*kr));
        auto result = tv.verify(tamperedToken);
        expect(!result.valid, "verify returns false for tampered payload");
    }

    // testVerifyDetachedValid
    {
        juce::File f(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto json = juce::JSON::parse(f);
        auto* obj = json.getDynamicObject();
        auto& props = obj->getProperties();

        auto canonical = props["canonical"].toString().toStdString();
        auto signatureStr = props["signature"].toString().toStdString();
        auto pubKeyStr = props["publicKeySpki"].toString().toStdString();

        auto sig = lumascope::PublicKeyRing::base64UrlDecode(signatureStr);
        auto pk = lumascope::PublicKeyRing::decodePublicKey(pubKeyStr);

        std::vector<unsigned char> msg(canonical.begin(), canonical.end());
        auto valid = lumascope::TokenVerifier::verifyDetached(msg, sig, pk);
        expect(valid, "verifyDetached returns true for fixture");

        // tampered message
        msg[0] ^= 1;
        auto invalid = lumascope::TokenVerifier::verifyDetached(msg, sig, pk);
        expect(!invalid, "verifyDetached returns false for tampered message");
    }

    // testPublicKeyRingParseValid
    {
        juce::File f(LUMASCOPE_FIXTURE_KEY_RING);
        auto json = juce::JSON::parse(f);
        auto kr = lumascope::PublicKeyRing::parse(json);
        expect(kr.has_value(), "key ring parses from file");
        if (kr)
        {
            expect(kr->size() == 1, "key ring size is 1");
            auto* entry = kr->findByKid("test-ed25519-2026-06");
            expect(entry != nullptr, "findByKid finds test key");
            expect(entry->algorithm == "Ed25519", "key algorithm is Ed25519");
        }
    }

    // testPublicKeyRingFindByKidMissing
    {
        juce::File f(LUMASCOPE_FIXTURE_KEY_RING);
        auto json = juce::JSON::parse(f);
        auto kr = lumascope::PublicKeyRing::parse(json);
        if (kr)
        {
            auto* entry = kr->findByKid("nonexistent-kid");
            expect(entry == nullptr, "findByKid returns null for unknown kid");
        }
    }

    // testPublicKeyRingParseInvalid
    {
        auto kr = lumascope::PublicKeyRing::parse(std::string("not json"));
        expect(!kr.has_value(), "parse invalid string returns nullopt");

        kr = lumascope::PublicKeyRing::parse(std::string("{}"));
        expect(!kr.has_value(), "parse object returns nullopt");
    }

    // testDecodePublicKeyLength
    {
        auto pk = lumascope::PublicKeyRing::decodePublicKey(
            "MCowBQYDK2VwAyEAZgp6fEFiUBw78KnO_tKs0KLvDqCPtx9U5CwPT7sIrLM");
        expect(pk.size() == 32, "decoded public key is 32 bytes");
    }

    // testVerifyUnknownKid
    {
        auto unknownKid = *token;
        unknownKid.kid = "nonexistent-kid";
        unknownKid.payload.kid = "nonexistent-kid";
        unknownKid.canonical = lumascope::canonicalJson(
            lumascope::claimsToVar(unknownKid.payload));

        juce::File f(LUMASCOPE_FIXTURE_KEY_RING);
        auto json = juce::JSON::parse(f);
        auto kr = lumascope::PublicKeyRing::parse(json);
        lumascope::TokenVerifier tv(std::move(*kr));
        auto result = tv.verify(unknownKid);
        expect(!result.valid, "verify returns false for unknown kid");
        expect(result.errorCode == "unknown_key_id", "error code is unknown_key_id");
    }

    if (failures > 0)
        std::cerr << "runTokenVerifierTests: " << failures << " failure(s)\n";
    return failures;
}
