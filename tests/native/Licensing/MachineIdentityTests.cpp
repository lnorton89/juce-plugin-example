#include "LumaScope/Licensing/MachineIdentity.h"
#include <iostream>
#include <string>
#include <cctype>

namespace {

bool isBase64urlChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_';
}

} // anonymous namespace

int runMachineIdentityTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // testIdentityDerivation
    {
        const auto id = lumascope::deriveMachineIdentifier();
        expect(!id.empty(), "deriveMachineIdentifier() returns non-empty");
        expect(id.size() >= 48 && id.size() <= 60, "id length is 48-60 chars");
        expect(id.substr(0, 7) == "mid_v1_", "id starts with mid_v1_");
        for (size_t i = 7; i < id.size(); ++i)
            expect(isBase64urlChar(id[i]), "id contains only base64url chars after prefix");
        expect(id.find("S-1-") == std::string::npos, "no raw SID in output");
        expect(id.find("volume") == std::string::npos, "no 'volume' in output");
        expect(id.find("serial") == std::string::npos, "no 'serial' in output");
    }

    // testIdentityConsistency
    {
        const auto id1 = lumascope::deriveMachineIdentifier();
        const auto id2 = lumascope::deriveMachineIdentifier();
        expect(id1 == id2, "two calls return identical results");
    }

    // testIdentityVersionPrefix
    {
        const auto info = lumascope::deriveMachineIdentityInfo();
        expect(info.version == "v1", "info.version is v1");
        expect(info.hashAlgorithm == "SHA-256", "info.hashAlgorithm is SHA-256");
        expect(!info.identifier.empty(), "info.identifier is non-empty");
    }

    if (failures > 0)
        std::cerr << "runMachineIdentityTests: " << failures << " failure(s)\n";
    return failures;
}
