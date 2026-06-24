#pragma once
#include <string>

namespace lumascope {

inline const std::string builtinTestKeyRing = R"JSON(
[
  {
    "kid": "test-ed25519-2026-06",
    "publicKey": "MCowBQYDK2VwAyEAZgp6fEFiUBw78KnO_tKs0KLvDqCPtx9U5CwPT7sIrLM",
    "algorithm": "Ed25519"
  }
]
)JSON";

} // namespace lumascope
