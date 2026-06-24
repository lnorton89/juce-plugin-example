#pragma once
#include <string>

namespace lumascope {

// Versioned, privacy-conscious Windows machine identifier.
//
// v1 inputs: "mid_v1_" prefix + SHA-256 base64url("v1:" + Machine SID + ":" + volume serial number)
//
// Known limitations (per D-03):
// - Changes on OS reinstall, system drive reformat, domain join/leave
// - Not an anti-tamper boundary — client-side identity cannot be perfect
// - Raw SID and volume serial are never transmitted or logged (D-01)
struct MachineIdentityInfo {
    std::string identifier;
    std::string version;
    std::string hashAlgorithm;
};

// Derive the local machine identifier.
// Returns empty string on catastrophic failure.
// Safe to call multiple times — internally cached.
std::string deriveMachineIdentifier();

// Derive with metadata.
MachineIdentityInfo deriveMachineIdentityInfo();

} // namespace lumascope
