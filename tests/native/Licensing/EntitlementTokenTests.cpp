#include "LumaScope/Licensing/EntitlementToken.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <string>

int runEntitlementTokenTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // testCanonicalJsonEmptyObject
    {
        auto s = lumascope::canonicalJson(juce::var(new juce::DynamicObject()));
        expect(s == "{}", "canonicalJson({}) returns '{}'");
    }

    // testCanonicalJsonSortedKeys
    {
        auto obj = new juce::DynamicObject();
        obj->setProperty("b", 1);
        obj->setProperty("a", 2);
        auto s = lumascope::canonicalJson(juce::var(obj));
        expect(s == R"({"a":2,"b":1})", "canonicalJson sorts keys alphabetically");
    }

    // testCanonicalJsonPrimitives
    {
        expect(lumascope::canonicalJson(juce::var(true)) == "true", "bool true");
        expect(lumascope::canonicalJson(juce::var(false)) == "false", "bool false");
        expect(lumascope::canonicalJson(juce::var()) == "null", "void becomes null");
        expect(lumascope::canonicalJson(juce::var(42)) == "42", "integer");
        expect(lumascope::canonicalJson(juce::var(3.14)) == "3.1400000000000001", "double"); // juce stores as binary
        expect(lumascope::canonicalJson(juce::var("hello")) == "\"hello\"", "string");
    }

    // testCanonicalJsonNested
    {
        auto inner = new juce::DynamicObject();
        inner->setProperty("b", 2);
        inner->setProperty("c", 3);
        auto outer = new juce::DynamicObject();
        outer->setProperty("a", juce::var(inner));
        juce::Array<juce::var> arr;
        arr.add(1); arr.add(2);
        outer->setProperty("d", juce::var(arr));
        auto s = lumascope::canonicalJson(juce::var(outer));
        expect(s == R"({"a":{"b":2,"c":3},"d":[1,2]})", "nested object and array");
    }

    // testCanonicalJsonFixtureMatch
    {
        juce::File fixtureFile(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto json = juce::JSON::parse(fixtureFile);
        auto* obj = json.getDynamicObject();
        expect(obj != nullptr, "fixture parses as object");

        auto& props = obj->getProperties();
        auto* payloadObj = props["payload"].getDynamicObject();
        expect(payloadObj != nullptr, "fixture has payload object");

        auto canonical = lumascope::canonicalJson(juce::var(payloadObj));
        auto expected = props["canonical"].toString().toStdString();
        expect(canonical == expected, "canonicalJson matches worker fixture exactly");
        if (canonical != expected)
            std::cerr << "  got:      " << canonical << "\n  expected: " << expected << '\n';
    }

    // testParseSignedEntitlementValid
    {
        juce::File fixtureFile(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto fixtureVar = juce::JSON::parse(fixtureFile);
        auto* fixtureObj = fixtureVar.getDynamicObject();
        expect(fixtureObj != nullptr, "fixture parses");

        // Build a proper token envelope from fixture data
        auto tokenObj = new juce::DynamicObject();
        tokenObj->setProperty("payload", fixtureObj->getProperties()["payload"]);
        tokenObj->setProperty("signature", fixtureObj->getProperties()["signature"]);
        tokenObj->setProperty("kid", juce::String("test-ed25519-2026-06"));
        tokenObj->setProperty("algorithm", juce::String("Ed25519"));

        auto token = lumascope::parseSignedEntitlement(juce::var(tokenObj));
        expect(token.has_value(), "parseSignedEntitlement succeeds on valid fixture");
        if (token)
        {
            expect(token->algorithm == "Ed25519", "algorithm is Ed25519");
            expect(!token->kid.empty(), "kid is non-empty");
            expect(!token->signature.empty(), "signature is non-empty");
            expect(token->payload.schemaVersion == 1, "schemaVersion is 1");
            expect(token->payload.status == "active", "status is active");
            expect(!token->payload.activationId.empty(), "activationId is non-empty");
            expect(!token->payload.machineId.empty(), "machineId is non-empty");
        }
    }

    // testParseSignedEntitlementMalformed
    {
        auto result = lumascope::parseSignedEntitlement(juce::var());
        expect(!result.has_value(), "parseSignedEntitlement on void returns nullopt");

        result = lumascope::parseSignedEntitlement(juce::var(42));
        expect(!result.has_value(), "parseSignedEntitlement on number returns nullopt");

        auto emptyObj = new juce::DynamicObject();
        result = lumascope::parseSignedEntitlement(juce::var(emptyObj));
        expect(!result.has_value(), "parseSignedEntitlement on empty obj returns nullopt");
    }

    if (failures > 0)
        std::cerr << "runEntitlementTokenTests: " << failures << " failure(s)\n";
    return failures;
}
