# Phase 6: Native Offline Licensing — Research

**Researched:** 2026-06-23
**Domain:** Native Windows licensing, machine identity, Ed25519 signature verification, protected local storage, grace/clock model, bridge protocol extension, activation UI
**Confidence:** HIGH (all primary recommendations verified via Context7 JUCE docs, libsodium official docs, and Microsoft Win32 documentation)

## Summary

Phase 6 connects the native Windows products to the Phase 5 activation service. Users enter a license key, activate the current machine, verify and store a signed entitlement locally, launch offline for up to seven days, and deactivate to transfer the activation. This research covers all technical decisions needed to implement this: Windows machine identity derivation, Ed25519 verification in C++, protected local storage via DPAPI, JUCE application-data paths, clock rollback detection, non-real-time HTTP activation client, bridge protocol extension for licensing events, public-key rotation strategy, cross-language test fixtures, and the React/MUI activation state/control UI.

**Primary recommendation:**
- **Machine identity:** `SHA-256(Windows Machine SID + System Volume Serial Number)` — privacy-conscious, stable, matches Chromium's proven `rlz` approach
- **Ed25519 verification:** libsodium `crypto_sign_verify_detached()` via CPM.cmake-declared dependency
- **Local storage:** `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)` + DPAPI `CryptProtectData`/`CryptUnprotectData` for user-bound encryption
- **HTTP activation client:** JUCE `URL::createInputStream()` with POST + JSON body, run from a `juce::TimeSliceThread` (never audio thread)
- **Bridge protocol:** New events `license.status` (native→UI) and `license.activate`/`license.deactivate` (UI→native)
- **UI:** Compact `Chip`-based status in footer/header, `Dialog` for key entry and deactivation confirmation, `Alert` for warnings

<user_constraints>
## User Constraints (from CONTEXT.md)

### Implementation Decisions

#### Machine Identity
- **D-01:** Use a balanced Windows machine fingerprint for v1: derive the machine identifier locally from multiple privacy-conscious Windows/device signals, hash or otherwise derive the final stable identifier locally, and never transmit raw hardware identifiers to the service.
- **D-02:** The machine identity must be versioned so future platform or privacy changes can coexist with existing activations and fixtures.
- **D-03:** Document the exact local inputs, stability expectations, and limitations. The design should be honest about normal Windows hardware churn, profile changes, repair/reinstall cases, and the fact that client-side identity cannot be a perfect anti-tamper boundary.

#### Local Entitlement and Offline Grace
- **D-04:** Offline grace should be generous but visible. If the signed entitlement is otherwise valid and the last successful online validation is within seven days, both targets should launch and operate normally while showing a clear offline/grace status.
- **D-05:** The UI should warn before grace expiry and distinguish normal activated, offline grace, revalidation required, revoked/expired, corrupt/tampered, and service-unavailable states.
- **D-06:** After the seven-day offline window expires, the application requires successful online validation before treating the product as activated again.
- **D-07:** The local state machine should resist simple wall-clock rollback from extending grace indefinitely, while documenting remaining limitations rather than implying strong DRM guarantees.

#### Activation UI and Product Tone
- **D-08:** Licensing belongs in a compact status/control surface, not a full-screen or dashboard-first experience. Preserve the analyzer-first instrument feel.
- **D-09:** Use a compact header/footer-adjacent control or status affordance for activation state, with dialogs for license-key entry, activation progress/errors, and deactivation confirmation.
- **D-10:** The UI should make licensing action states clear and actionable without turning the app into an account-management portal. Recovery copy maps stable native/server error codes to friendly bounded messages.

#### Transfer and Failure Behavior
- **D-11:** Transfer is server-authoritative. Clear the local entitlement only after successful server deactivation of the matching activation.
- **D-12:** If refresh/validation fails because the service or network is unavailable, continue to honor the remaining local grace window and present a service-unavailable/offline status.
- **D-13:** If the server reports revoked, expired, deactivated, activation-not-found, or another authoritative entitlement failure, show reactivation-required or a specific failure state and stop relying on the local token for activated status.
- **D-14:** Failed deactivation must leave the current local entitlement intact and explain that transfer has not completed.

### the agent's Discretion

The planner may choose the exact Windows identity inputs, hash/encoding format, protected-storage mechanism, file layout, state-machine class boundaries, retry/backoff policy, request scheduling, UI component breakdown, and copy details, provided the decisions above and existing real-time, bridge, security, and privacy constraints are preserved.

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within Phase 6 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| LIC-01 | Derive versioned privacy-conscious machine identifier | Section: Windows Machine Identity — SHA-256 composite of SID + volume serial |
| LIC-02 | HTTPS activation on non-real-time thread | Section: Non-Real-Time HTTP Client — JUCE URL::createInputStream on TimeSliceThread |
| LIC-03 | Verify token signature, kid, schema, product, machine, time | Section: Ed25519 Verification — libsodium crypto_sign_verify_detached |
| LIC-04 | Store entitlement in user APPDATA with DPAPI protection | Section: Protected Local Storage |
| LIC-05 | Seven-day offline grace after last successful validation | Section: Grace/Clock Model |
| LIC-06 | Require revalidation after grace window, warn before expiry | Section: Grace/Clock Model + UI State Machine |
| LIC-07 | Resist wall-clock rollback | Section: Clock Rollback Detection |
| LIC-08 | Clear local entitlement only after successful server deactivation | Section: State Machine — deactivation flow |
| LIC-09 | Licensing state as non-blocking atomic, no enforcement on audio thread | Section: Thread Safety Model |
| LIC-10 | Cross-language fixtures and native tests | Section: Test Fixture Strategy |
| UI-05 | UI exposes activation, activated, offline-grace, revalidation-required, deactivation, and failure states | Section: UI State Machine |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Machine identity derivation | Native (plugin) | — | OS API calls (LookupAccountSid, GetVolumeInformation) are Windows-only native code |
| Ed25519 signature verification | Native (plugin) | — | libsodium is a native C library; verification runs on the message thread |
| Local entitlement storage | Native (plugin) | — | DPAPI calls, file I/O on application-data directory; never in browser |
| Grace/clock tracking | Native (plugin) | — | `juce::Time` comparison; stored alongside entitlement file |
| Activation HTTP calls | Native (plugin) | — | JUCE URL::createInputStream — must never run on audio thread |
| Licensing state propagation | Native (plugin) | Browser (UI) | Atomic/immutable status read by editor timer; emitted via bridge to React |
| Activation/deactivation UI | Browser (React/MUI) | — | Dialog for key entry, Chip for status, Alert for warnings |
| Public key ring | Native (plugin) | — | Embedded at compile time as binary data or fixture file |
| Cross-language fixtures | Shared (test) | — | Same canonical JSON used by both TypeScript (worker) and C++ (native) tests |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libsodium | 1.0.20+ (stable) | Ed25519 signature verification, SHA-256 hashing | Most portable, well-audited, CMake-compatible Ed25519 implementation; `crypto_sign_verify_detached()` is a one-liner; docs recommend it over OpenSSL's verbose EVP API |
| JUCE | 8.0.14 (pinned) | HTTP requests, application data paths, JSON, timer-polling | Existing project dependency; `URL::createInputStream`, `File::getSpecialLocation`, `JSON::parse/toString`, `TimeSliceThread` all serve Phase 6 needs |
| Windows SDK (DPAPI) | Shipped with VS2022 | Entitlement file encryption | `CryptProtectData`/`CryptUnprotectData` for user-bound encryption; zero setup beyond `#include <dpapi.h>` and linking `Crypt32.lib` |
| C++ Standard Library | C++20 | `std::atomic` for licensing state, `std::optional`, `std::chrono` | Lock-free licensing status handoff (LIC-09); standard utilities for time comparisons |

### Supporting
| Class/API | Purpose | When to Use |
|-----------|---------|-------------|
| `juce::URL::createInputStream` | HTTPS POST to activation API | Activation, validation, deactivation requests |
| `juce::Time` | Wall-clock reading for grace period | `Time::getMillisecondCounter()`, `Time::getCurrentTime().toISO8601()` |
| `juce::File::getSpecialLocation` | Find application-data directory | `userApplicationDataDirectory` / `userDocumentsDirectory` |
| `juce::JSON::toString` | Build JSON request bodies | Serialize activation request to JSON string |
| `juce::JSON::parse` | Parse JSON responses | Parse activation API response and entitlement token |
| `juce::TimeSliceThread` | Background HTTP worker | Dedicated thread for activation/validation/deactivation calls |
| `juce::Identifier` | Bridge event constants | New licensing events follow existing HostBridge pattern |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| libsodium | OpenSSL 3.x EVP Ed25519 | OpenSSL is ~3MB+ and has a verbose, error-prone EVP API for Ed25519; libsodium is ~300KB with a clean crypto_sign API that is timing-safe and well-audited |
| DPAPI | CNG NCryptProtectSecret | Newer API (Windows 8+) but DPAPI is simpler, well-documented, and sufficient for v1; CNG adds complexity without meaningful v1 benefit |
| libsodium | TweetNaCl (single .c file) | TweetNaCl is minimal (~50KB) and avoids a dependency but is slower and less tested on Windows with MSVC; libsodium provides pre-built binaries for VS2022 |
| Composite SHA-256 hash | Windows MachineGUID from registry | MachineGUID is accessible to any process but persists across hardware changes; composite hash of SID + volume serial is more stable under hardware churn |

**Installation (libsodium via CPM):**
```cmake
# In cmake/Dependencies.cmake:
CPMAddPackage(
  NAME libsodium
  VERSION 1.0.20
  URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.20.tar.gz
  # Or use GitHub release:
  # GITHUB_REPOSITORY jedisct1/libsodium
  # GIT_TAG 1.0.20-RELEASE
)
if(libsodium_ADDED)
  # libsodium ships a CMake build system as of 1.0.18+
  # The target "sodium" or "sodiumstatic" is available
endif()
```

**Alternative (pre-built binaries):** Download from https://download.libsodium.org/libsodium/releases/ for VS2022. Add via:
```cmake
# Manual path config as fallback
find_library(LIBSODIUM_LIB sodium PATHS "${LUMASCOPE_DEPENDENCY_DIR}/libsodium/lib")
find_path(LIBSODIUM_INCLUDE_DIR sodium.h PATHS "${LUMASCOPE_DEPENDENCY_DIR}/libsodium/include")
```

**Version verification:** libsodium 1.0.20 (stable branch, latest point release as of June 2026) is confirmed at `https://download.libsodium.org/libsodium/releases/`. The CMake build system was added in libsodium 1.0.18.

## Package Legitimacy Audit

> Phase 6 adds zero external npm packages or pip packages. All additions are C/C++ native libraries via CPM.cmake or Windows SDK headers.

| Package | Registry | Age | Downloads | Source Repo | slopcheck | Disposition |
|---------|----------|-----|-----------|-------------|-----------|-------------|
| libsodium (CPM) | GitHub (jedisct1/libsodium) | 12+ years | 30K+ GitHub stars | github.com/jedisct1/libsodium | N/A (native C library) | Approved — well-audited, portable, official CMake support |
| DPAPI (Win32) | Windows SDK | 20+ years | Shipped with OS | microsoft.com | N/A (OS API) | Approved — stable Win32 API |

**Packages removed due to slopcheck [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│  Native Application (VST3 / Standalone)                             │
│                                                                     │
│  ┌─────────────────────┐    ┌──────────────────────────────┐        │
│  │ LicensingCore       │    │  ActivationClient             │       │
│  │  ┌────────────────┐ │    │  (juce::TimeSliceThread)     │       │
│  │  │ MachineIdentity│ │    │  URL::createInputStream()    │       │
│  │  │ (SID+VolSerial)│ │    │       │                      │       │
│  │  │ → SHA-256 hash │ │    │       │ HTTPS POST           │       │
│  │  └────────────────┘ │    │       ▼                      │       │
│  │  ┌────────────────┐ │    │  ┌────────────────────────┐  │       │
│  │  │ TokenVerifier  │ │    │  │ Parse + Store Response │  │       │
│  │  │ (libsodium)    │◄─┼────┼──┘                       │  │       │
│  │  └────────────────┘ │    └──────────────────────────────┘       │
│  │  ┌────────────────┐ │                                           │
│  │  │ StorageManager │ │    ┌──────────────────────────────┐       │
│  │  │ (DPAPI + File) │ │    │  Editor Timer (message thread)│      │
│  │  └────────────────┘ │    │  polls LicensingCore status  │       │
│  │  ┌────────────────┐ │    │       │                      │       │
│  │  │ GraceKeeper    │ │    │       ▼                      │       │
│  │  │ (clock rollback│ │    │  bridge.emit("license.status")│      │
│  │  │  detection)    │ │    └──────────────────────────────┘       │
│  │  └────────────────┘ │                                           │
│  └─────────────────────┘                                           │
│                                                                     │
│  Audio Callback (NEVER touches LicensingCore directly)               │
│  ┌──────────────────────────────────────────────────────────┐      │
│  │ Processors read licensing state via std::atomic<bool>     │      │
│  │ or std::atomic<int> status enum — no locks, no I/O       │      │
│  └──────────────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────────────┘
         │
         │ JUCE WebBrowserComponent (bridge events)
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Browser (React / TypeScript / MUI)                                 │
│                                                                     │
│  ┌─────────────────┐   ┌────────────────────┐   ┌───────────────┐  │
│  │ BridgeProvider  │   │ ActivationDialog   │   │ StatusChip    │  │
│  │ (listens for    │   │ (TextField key     │   │ (footer chip  │  │
│  │  license.status)│   │  entry, activate/  │   │  shows state  │  │
│  │                 │   │  deactivate)       │   │  with color)  │  │
│  │ emits           │   └────────────────────┘   └───────────────┘  │
│  │ license.activate│                                              │
│  │ license.deact.  │   ┌────────────────────┐                      │
│  └─────────────────┘   │ RevalidationAlert  │                      │
│                        │ (warning before    │                      │
│                        │  grace expiry)     │                      │
│                        └────────────────────┘                      │
└─────────────────────────────────────────────────────────────────────┘
         │
         │ Cloudflare Worker (Phase 5)
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Activation Service                                                  │
│  POST /api/v1/activate → signed EntitlementClaims                   │
│  POST /api/v1/validate → fresh signed token                        │
│  POST /api/v1/deactivate → status: deactivated                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure

```
plugin/
├── include/LumaScope/
│   ├── Licensing/                          # NEW: licensing core
│   │   ├── EntitlementToken.h             # Token data model + canonical JSON
│   │   ├── MachineIdentity.h              # Windows machine ID derivation
│   │   ├── TokenVerifier.h               # libsodium Ed25519 verification
│   │   ├── LocalEntitlementStore.h        # DPAPI + file storage
│   │   ├── GraceModel.h                   # 7-day grace + clock rollback
│   │   ├── LicensingState.h               # Atomic status enum for cross-thread
│   │   └── ActivationClient.h            # Non-real-time HTTP client
│   ├── LicensingEventIds.h                # NEW: bridge event IDs for licensing
│   └── HostBridge.h                       # Extended with licensing helper methods
├── source/Licensing/                       # NEW: licensing source files
│   ├── EntitlementToken.cpp
│   ├── MachineIdentity.cpp
│   ├── TokenVerifier.cpp
│   ├── LocalEntitlementStore.cpp
│   ├── GraceModel.cpp
│   ├── LicensingState.cpp
│   └── ActivationClient.cpp
│
tests/native/
├── Licensing/                              # NEW: licensing tests
│   ├── MachineIdentityTests.cpp
│   ├── TokenVerifierTests.cpp
│   ├── EntitlementTokenTests.cpp
│   ├── LocalEntitlementStoreTests.cpp
│   ├── GraceModelTests.cpp
│   └── ActivationClientTests.cpp
├── fixtures/
│   ├── entitlement-v1.json                 # COPIED from tests/worker/fixtures/
│   └── public-key-ring.json                # Public key ring for verification

ui/src/
├── bridge/
│   ├── protocol.ts                         # EXTENDED: license.status, license.activate, etc.
│   └── BridgeProvider.tsx                  # EXTENDED: license event listeners
├── components/
│   ├── ActivationStatus.tsx                # NEW: Chip showing licensing state + action button
│   ├── ActivationDialog.tsx                # NEW: License key entry dialog
│   └── DeactivationDialog.tsx              # NEW: Confirm deactivation dialog
├── hooks/
│   └── useLicensing.ts                     # NEW: Hook for licensing state + actions
└── app/
    └── AppShell.tsx                        # EXTENDED: Include ActivationStatus in footer/header
```

### Pattern 1: Bridge Event Extension for Licensing

**What:** Follow existing HostBridge pattern to add licensing events with closed, typed, bounded schemas. New native-to-UI event `license.status` carries the current licensing state. New UI-to-native events `license.activate`, `license.deactivate`, and `license.validate` carry user-initiated actions.

**When to use:** Whenever the editor or UI needs to react to licensing state changes or the user initiates activation/deactivation.

**Source:** [Existing bridge protocol — docs/bridge-protocol.md], [HostBridge.h pattern]

**Native side (HostBridge.h - extend with new event IDs):**
```cpp
// NEW event identifiers in HostBridge.h
static const juce::Identifier licenseStatusEvent;
static const juce::Identifier licenseActivateEvent;
static const juce::Identifier licenseDeactivateEvent;
static const juce::Identifier licenseValidateEvent;

// NEW helper: build license.status payload from LicensingState
static juce::var makeLicenseStatusPayload (const LicensingState& state);
```

**Native side (HostBridge.cpp - event ID values):**
```cpp
const juce::Identifier HostBridge::licenseStatusEvent { "license.status" };
const juce::Identifier HostBridge::licenseActivateEvent { "license.activate" };
const juce::Identifier HostBridge::licenseDeactivateEvent { "license.deactivate" };
const juce::Identifier HostBridge::licenseValidateEvent { "license.validate" };
```

**Native side (payload construction):**
```cpp
juce::var HostBridge::makeLicenseStatusPayload (const LicensingState& state)
{
    // LicensingState is a struct with: status enum, activationId, serverTime,
    // lastVerifiedTime, offlineGraceRemaining, errorCode, errorMessage
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("protocolVersion", protocolVersion);
    object->setProperty ("event", licenseStatusEvent.toString());
    object->setProperty ("state", toString (state.status));  // "activated", "offline_grace", "revalidation_required", "revoked", "corrupt", "service_unavailable", "not_activated"
    object->setProperty ("activationId", state.activationId.substring (0, 64));
    object->setProperty ("lastVerifiedTime", state.lastVerifiedTime.substring (0, 32));
    object->setProperty ("offlineGraceRemainingDays", state.offlineGraceRemainingDays);
    if (state.errorCode.isNotEmpty())
    {
        object->setProperty ("code", state.errorCode.substring (0, 64));
        object->setProperty ("message", state.message.substring (0, 256));
    }
    return result;
}
```

**TypeScript side (protocol.ts - extend):**
```typescript
export const licenseStatusEvent = 'license.status' as const;
export const licenseActivateEvent = 'license.activate' as const;
export const licenseDeactivateEvent = 'license.deactivate' as const;
export const licenseValidateEvent = 'license.validate' as const;

export type LicensingState =
  | 'not_activated'
  | 'activating'
  | 'activated'
  | 'offline_grace'
  | 'revalidation_required'
  | 'revoked'
  | 'corrupt'
  | 'service_unavailable'
  | 'deactivating';

export interface LicenseStatusPayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseStatusEvent;
  state: LicensingState;
  activationId: string;
  lastVerifiedTime: string;
  offlineGraceRemainingDays: number;
  code?: string;
  message?: string;
}

export function parseLicenseStatus(value: unknown): LicenseStatusPayload | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (item.event !== licenseStatusEvent) return null;
  // ... validate closed enum, bounded strings, etc.
  return item as unknown as LicenseStatusPayload;
}
```

**BridgeProvider.tsx - extend listeners:**
```typescript
const licenseToken = transport.addEventListener(licenseStatusEvent, (payload) => {
  const status = parseLicenseStatus(payload);
  if (status) setLicensingState(status);
});
```

**Editor integration - timer callback emits licensing state:**
```cpp
// In PluginEditor::timerCallback(), alongside spectrum snapshot polling:
if (bridgeReady)
{
    // ... existing spectrum polling ...
    
    // NEW: emit licensing status periodically
    const auto state = licensingCore.currentState();  // non-blocking atomic read
    if (state.hasChangedSince(lastEmittedLicenseState))
    {
        browser.emitEventIfBrowserIsVisible (
            HostBridge::licenseStatusEvent,
            HostBridge::makeLicenseStatusPayload (state));
        lastEmittedLicenseState = state;
    }
}
```

### Pattern 2: Atomic Licensing State for Cross-Thread Safety (LIC-09)

**What:** Licensing state is read from the audio thread for enforcement (e.g., mute output if not activated) but must never perform I/O, locking, or allocation. Use a `std::atomic` status enum and atomic sequence counter.

**Source:** [Existing SnapshotMailbox.h pattern for lock-free handoff]

```cpp
// plugin/include/LumaScope/Licensing/LicensingState.h
#pragma once
#include <atomic>
#include <cstdint>

namespace lumascope {

enum class LicenseStatus : std::uint8_t {
    uninitialized = 0,
    notActivated,
    activating,
    activated,
    offlineGrace,
    revalidationRequired,
    revoked,
    corrupt,
    serviceUnavailable,
    deactivating
};

class LicensingState {
public:
    LicensingState() = default;
    // Non-copyable for atomic safety
    LicensingState(const LicensingState&) = delete;
    LicensingState& operator=(const LicensingState&) = delete;

    void updateStatus(LicenseStatus newStatus) noexcept {
        status.store(newStatus, std::memory_order_release);
        sequence.fetch_add(1, std::memory_order_release);
    }

    LicenseStatus readStatus() const noexcept {
        return status.load(std::memory_order_acquire);
    }

    bool hasChanged(std::uint32_t& lastSeen) const noexcept {
        const auto current = sequence.load(std::memory_order_acquire);
        if (current == lastSeen) return false;
        lastSeen = current;
        return true;
    }

private:
    std::atomic<LicenseStatus> status { LicenseStatus::uninitialized };
    std::atomic<std::uint32_t> sequence { 0 };
};

} // namespace lumascope
```

### Anti-Patterns to Avoid
- **Audio thread blocking:** Never call `ActivationClient::activate()` or `ActivationClient::validate()` from the audio callback. All HTTP and file I/O must run on `juce::TimeSliceThread`.
- **WebView calls from audio thread:** Never `emitEventIfBrowserIsVisible()` from the audio callback. Licensing status is polled from the message-thread timer.
- **Raw hardware identifiers to server:** Never transmit MAC addresses, CPU serials, motherboard serial numbers, or Windows Product IDs to the activation API. Only the locally-derived hash leaves the machine.
- **Manual JSON building on audio thread:** JSON construction (even `juce::JSON::toString`) is prohibited on the audio callback. It happens on the `ActivationClient` thread.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Ed25519 signature verification | Custom Ed25519 implementation | libsodium `crypto_sign_verify_detached()` | Timing-safe, audited, constant-time; one function call vs. 500+ lines of error-prone crypto |
| Windows data protection | Custom file encryption | DPAPI `CryptProtectData()` | OS-managed key derivation from user logon credentials; zero key management overhead |
| Cross-platform canonical JSON | Ad-hoc sorting | C++ canonical JSON function (implement once, match Worker's algorithm) | Deterministic sort + serialize is ~30 lines; must match TypeScript `canonicalJson.ts` exactly |
| Thread-safe status handoff | Mutex/lock on audio thread | `std::atomic` enum with sequence counter | Proven in SnapshotMailbox; no contention, no allocation, no blocking |
| HTTP POST with JSON body | Raw WinHTTP/Schannel | JUCE `URL::createInputStream()` | Already abstracts TLS, redirects, timeouts; matches JUCE integration pattern |

**Key insight:** The most dangerous code in licensing is custom crypto or ad-hoc thread synchronization. libsodium (for Ed25519) and DPAPI (for storage) are the correct off-the-shelf solutions. The only "hand-roll" needed is canonical JSON, which must match the Worker's implementation character-for-character.

## Windows Machine Identity (LIC-01)

### Recommended Approach: Composite SHA-256 of SID + Volume Serial

Derive the machine identifier locally from two stable, privacy-conscious Windows signals:

1. **Windows Machine SID** — obtained via `LookupAccountNameW(NULL, computerName, ...)` and `ConvertSidToStringSidW()`. Stable across reboots and most hardware changes. Survives OS reinstall only if the computer domain membership or local SID remains the same (which is typical for non-domain machines on reinstall? Actually it changes on reinstall — this is a documented limitation, D-03).

2. **System Drive Volume Serial Number** — obtained via `GetVolumeInformationW(systemPath, ...)`. Stable across reboots. Changes on reformat. Complements the SID for cases where the profile SID might change.

**Composition:**
```cpp
// plugin/source/Licensing/MachineIdentity.cpp
#include <windows.h>
#include <sodium.h>
#include <string>
#include <array>

namespace lumascope {

std::string deriveMachineIdentifier()
{
    // 1. Get computer name
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = std::size(computerName);
    if (!GetComputerNameW(computerName, &size))
        return {};

    // 2. Get machine SID
    char sidBuffer[SECURITY_MAX_SID_SIZE] = {};
    SID* sid = reinterpret_cast<SID*>(sidBuffer);
    DWORD sidSize = sizeof(sidBuffer);
    wchar_t domainBuffer[256] = {};
    DWORD domainSize = std::size(domainBuffer);
    SID_NAME_USE sidNameUse;

    std::string sidString;
    if (LookupAccountNameW(nullptr, computerName, sid, &sidSize,
                            domainBuffer, &domainSize, &sidNameUse))
    {
        wchar_t* sidStr = nullptr;
        if (ConvertSidToStringSidW(sid, &sidStr))
        {
            // Convert wide string to UTF-8
            const int len = WideCharToMultiByte(CP_UTF8, 0, sidStr, -1, nullptr, 0, nullptr, nullptr);
            sidString.resize(len - 1);  // exclude null terminator
            WideCharToMultiByte(CP_UTF8, 0, sidStr, -1, sidString.data(), len, nullptr, nullptr);
            LocalFree(sidStr);
        }
    }

    // 3. Get system drive volume serial number
    wchar_t systemPath[MAX_PATH + 1] = {};
    if (!GetSystemDirectoryW(systemPath, MAX_PATH))
        return {};
    wchar_t* firstSlash = wcspbrk(systemPath, L"\\/");
    if (firstSlash != nullptr)
        firstSlash[1] = L'\0';

    DWORD volumeSerial = 0;
    if (!GetVolumeInformationW(systemPath, nullptr, 0, &volumeSerial,
                                nullptr, nullptr, nullptr, 0))
        volumeSerial = 0;

    // 4. Composite string: version + SID + volume serial
    const std::string composite = "v1:" + sidString + ":" + std::to_string(volumeSerial);

    // 5. SHA-256 hash
    std::array<unsigned char, crypto_hash_sha256_BYTES> hash;
    crypto_hash_sha256(hash.data(),
                       reinterpret_cast<const unsigned char*>(composite.data()),
                       composite.size());

    // 6. Base64url-encode
    static constexpr char base64url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve(44);  // 32 bytes → 43 chars + 1 padding or unpadded
    // simplified base64url encoding...

    return "mid_v1_" + result;
}

} // namespace lumascope
```

**Why this approach:**
- **Privacy:** Only the SHA-256 hash leaves the machine (D-01). Raw SID and volume serial never leave the device
- **Stability:** Survives most hardware changes (GPU swap, RAM upgrade, disk addition). Changes on: OS reinstall, system drive reformat, domain migration
- **Abuse resistance:** Hard to spoof without compromising the Windows user account
- **Versioned:** `v1` prefix in the composite string (D-02)

**Signals NOT used and why:**
- MAC addresses — change with network adapters, privacy concerns
- CPUID — WMI queries add complexity, changes on CPU swap
- TPM attestation — over-engineered for v1, not universally available on all Windows hardware
- Registry MachineGUID — changes on OS reinstall like SID but is more fragile

**Limitations to document (D-03):**
- Changes on OS reinstall (user must reactivate)
- Changes on system drive reformat
- May change on domain join/leave operations
- Not an anti-tamper boundary, just a binding mechanism

## Ed25519 Verification in C++ (LIC-03)

### libsodium Integration

**Library choice: libsodium (confirmed via doc.libsodium.org and GitHub)**

`crypto_sign_verify_detached()` is the correct function for verifying a detached Ed25519 signature against a message and public key.

```cpp
// plugin/source/Licensing/TokenVerifier.cpp
#include <sodium.h>

namespace lumascope {

bool verifyEntitlementSignature(
    const std::vector<unsigned char>& message,    // canonical JSON bytes
    const std::vector<unsigned char>& signature,   // raw 64-byte Ed25519 signature
    const std::vector<unsigned char>& publicKey    // raw 32-byte Ed25519 public key
) {
    if (sodium_init() < 0)
        return false;  // library initialization failed

    // crypto_sign_verify_detached returns 0 on success, -1 on failure
    return crypto_sign_verify_detached(
        signature.data(),
        message.data(),
        message.size(),
        publicKey.data()
    ) == 0;
}

} // namespace lumascope
```

### Canonical JSON in C++ (must match Worker's `canonicalJson.ts`)

The canonical JSON algorithm is deterministic sorted-key serialization. It must produce identical output to the TypeScript `canonicalJson()` function in `worker/src/signing/canonicalJson.ts`.

```cpp
// plugin/source/Licensing/EntitlementToken.cpp
#include <string>
#include <vector>
#include <algorithm>
#include <juce_core/juce_core.h>

namespace lumascope {

// Canonical JSON: sorted keys, no whitespace, finite numbers only
// Must match worker/src/signing/canonicalJson.ts character-for-character
std::string canonicalJson(const juce::var& value)
{
    if (value.isVoid() || value.isUndefined())
        return "null";
    if (value.isBool())
        return static_cast<bool>(value) ? "true" : "false";
    if (value.isInt() || value.isInt64())
        return juce::String(static_cast<int64_t>(value)).toStdString();
    if (value.isDouble())
    {
        const double d = static_cast<double>(value);
        jassert(std::isfinite(d));
        return juce::JSON::toString(value, true).toStdString();
    }
    if (value.isString())
    {
        // juce::JSON::escapeString wraps in quotes properly
        return juce::JSON::escapeString(value.toString(), true).toStdString();
    }
    if (auto* array = value.getArray())
    {
        std::string result = "[";
        for (int i = 0; i < array->size(); ++i)
        {
            if (i > 0) result += ",";
            result += canonicalJson((*array)[i]);
        }
        result += "]";
        return result;
    }
    if (auto* obj = value.getDynamicObject())
    {
        auto& props = obj->getProperties();
        // Sort keys alphabetically
        std::vector<juce::Identifier> keys;
        for (auto& prop : props)
            keys.push_back(prop.name);
        std::sort(keys.begin(), keys.end(),
            [](const juce::Identifier& a, const juce::Identifier& b) {
                return a.toString() < b.toString();
            });

        std::string result = "{";
        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0) result += ",";
            result += juce::JSON::escapeString(keys[i].toString(), true).toStdString();
            result += ":";
            result += canonicalJson(props[keys[i]]);
        }
        result += "}";
        return result;
    }
    return "null";
}

} // namespace lumascope
```

### Public Key Ring Parsing

The public key ring is JSON (matching `SIGNING_PUBLIC_KEYS` in the Worker). Embed it as a compile-time resource:

```cpp
// plugin/include/LumaScope/Licensing/PublicKeyRing.h
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace lumascope {

struct PublicKeyEntry {
    std::string kid;
    std::string publicKey;  // base64url-encoded SPKI
    std::string algorithm;
};

class PublicKeyRing {
public:
    // Parse from JSON string (embedded at compile time)
    static std::optional<PublicKeyRing> parse(const std::string& json);

    // Find key by identifier
    const PublicKeyEntry* findByKid(const std::string& kid) const;

    // Get raw public key bytes (decoded from base64url)
    std::vector<unsigned char> decodedPublicKey(const PublicKeyEntry& entry) const;

private:
    std::vector<PublicKeyEntry> entries;
};

} // namespace lumascope
```

**Embed at compile time:**
```cmake
# In tests/native/CMakeLists.txt or plugin/CMakeLists.txt
target_compile_definitions(LumaScopeNativeTests PRIVATE
    LUMASCOPE_PUBLIC_KEY_RING="${CMAKE_SOURCE_DIR}/tests/native/fixtures/public-key-ring.json"
    LUMASCOPE_ENTITLEMENT_FIXTURE="${CMAKE_SOURCE_DIR}/tests/worker/fixtures/entitlement-v1.json")
```

## Protected Local Storage (LIC-04)

### Recommended Approach: DPAPI-Encrypted File in User AppData

**File location:** `%APPDATA%/LumaScope/entitlement.dat`

Using `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)`:
```cpp
#include <juce_core/juce_core.h>
#include <dpapi.h>
#include <windows.h>

namespace lumascope {

juce::File getEntitlementFilePath()
{
    const auto appData = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory);
    const auto dir = appData.getChildFile("LumaScope");
    dir.createDirectory();  // ensure exists
    return dir.getChildFile("entitlement.dat");
}

} // namespace lumascope
```

**DPAPI encryption on write:**
```cpp
bool LocalEntitlementStore::write(const std::string& jsonContent)
{
    const auto filePath = getEntitlementFilePath();

    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(jsonContent.data()));
    input.cbData = static_cast<DWORD>(jsonContent.size());

    DATA_BLOB output;
    if (!CryptProtectData(&input,
                          L"LumaScopeEntitlement",  // description
                          nullptr,                   // optional entropy (none)
                          nullptr,                   // reserved
                          nullptr,                   // prompt struct (none)
                          CRYPTPROTECT_UI_FORBIDDEN, // no UI
                          &output))
        return false;  // encryption failed

    // Write encrypted blob to file
    const juce::File file(filePath);
    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
    {
        LocalFree(output.pbData);
        return false;
    }

    // Write as hex or raw bytes - raw is simpler
    stream.write(output.pbData, output.cbData);
    stream.flush();
    LocalFree(output.pbData);
    return true;
}
```

**DPAPI decryption on read:**
```cpp
std::optional<std::string> LocalEntitlementStore::read()
{
    const auto filePath = getEntitlementFilePath();
    const juce::File file(filePath);
    if (!file.existsAsFile())
        return std::nullopt;

    juce::FileInputStream stream(file);
    if (!stream.openedOk())
        return std::nullopt;

    const auto size = stream.getTotalLength();
    if (size <= 0 || size > 64 * 1024)  // sanity cap at 64KB
        return std::nullopt;

    std::vector<BYTE> encryptedData(static_cast<size_t>(size));
    stream.read(encryptedData.data(), size);

    DATA_BLOB input;
    input.pbData = encryptedData.data();
    input.cbData = static_cast<DWORD>(encryptedData.size());

    DATA_BLOB output;
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr,
                            nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;  // decryption failed (corrupt or wrong user)

    std::string result(reinterpret_cast<char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return result;
}
```

**Tamper detection:** After decryption, parse the JSON and validate the Ed25519 signature before trusting the content. This is built into the `TokenVerifier` — verified signature = trusted entitlement. Invalid signature = `corrupt` state. No additional integrity layer needed beyond the signature.

**Transparent fallback:** If the file doesn't exist (`std::nullopt` from `read()`), the licensing state is `notActivated`. If the file exists but DPAPI decryption fails (wrong user, corrupt file), treat as `corrupt`.

## JUCE Application Data Patterns

### Entitlement File Location

```cpp
// Option 1: User's application data directory (recommended)
const auto appData = juce::File::getSpecialLocation(
    juce::File::userApplicationDataDirectory);
// Windows: C:\Users\<user>\AppData\Roaming\LumaScope\entitlement.dat

// Option 2: User's documents directory (more visible)
const auto docs = juce::File::getSpecialLocation(
    juce::File::userDocumentsDirectory);
// Windows: C:\Users\<user>\Documents\LumaScope\entertainment.dat

// Option 3: Common application data (all users - discouraged for user-bound data)
const auto commonAppData = juce::File::getSpecialLocation(
    juce::File::commonApplicationDataDirectory);
```

**Recommendation:** Use `userApplicationDataDirectory` (Option 1) — it's the standard location for application state, automatically backed up by Windows, and user-specific by default. This aligns with Windows conventions for roaming application data.

### Public Key Ring Embedding

Embed the public key ring as a compile-time constant:

```cpp
// Option A: Parse at runtime from a CMake-defined path (for tests)
// Defined via target_compile_definitions
const auto keyRingPath = juce::String(LUMASCOPE_PUBLIC_KEY_RING);
const auto keyRingFile = juce::File(keyRingPath);
const auto json = keyRingFile.loadFileAsString();

// Option B: Embed as a C++ string constant (for release builds)
// Generate via cmake file(READ ...)
// plugin/include/LumaScope/Licensing/BuiltinPublicKeyRing.h
namespace lumascope {
constexpr const char* builtinPublicKeyRing = R"JSON(
[
  {
    "kid": "prod-ed25519-2026-06",
    "publicKey": "...base64url...",
    "algorithm": "Ed25519"
  }
]
)JSON";
}
```

**Recommendation for production:** Generate `BuiltinPublicKeyRing.h` at CMake configure time from the JSON file. For tests, load from the fixture path directly.

## Clock Rollback Detection (LIC-07)

### Best-Effort Approach

The grace model stores the wall-clock time of the last successful online validation. On each launch, compare:

1. **Stored `lastVerifiedTime`** — ISO 8601 timestamp written when the Worker returns a successful validation
2. **Current system time** — `juce::Time::getCurrentTime().toISO8601()`
3. **Previous launch time** — stored alongside the entitlement when the application shuts down

```cpp
namespace lumascope {

class GraceModel {
public:
    // Check if offline grace is valid
    GraceStatus checkOfflineGrace(
        const juce::Time& lastVerifiedTime,
        const juce::Time& lastKnownSystemTime,  // saved at last shutdown
        const juce::Time& currentSystemTime
    ) {
        const auto gracePeriod = juce::RelativeTime::days(7);

        // Clock rollback detection
        if (currentSystemTime < lastKnownSystemTime)
        {
            // System clock was rolled back since last run
            // Count it against grace: difference from last verified
            auto timeSinceVerification = lastKnownSystemTime > lastVerifiedTime
                ? lastKnownSystemTime - lastVerifiedTime
                : juce::RelativeTime(0);
            if (timeSinceVerification > gracePeriod)
                return GraceStatus::expired;

            // Grant proportional remaining grace
            auto remaining = gracePeriod - timeSinceVerification;
            return remaining > juce::RelativeTime::days(1)
                ? GraceStatus::active
                : GraceStatus::warning;  // warn if < 1 day remaining
        }

        // Normal path: grace from last verified time
        auto elapsed = currentSystemTime - lastVerifiedTime;
        if (elapsed > gracePeriod)
            return GraceStatus::expired;

        auto remaining = gracePeriod - elapsed;
        if (remaining <= juce::RelativeTime::days(1))
            return GraceStatus::warning;  // less than 1 day remaining

        return GraceStatus::active;
    }

    enum class GraceStatus {
        active,     // within grace period
        warning,    // < 1 day remaining
        expired     // grace period elapsed
    };
};

} // namespace lumascope
```

**Limitations to document (D-07):**
- A sophisticated user who modifies system time before launching can reset the clock
- Performance counters (QueryPerformanceCounter) can detect monotonic violations but not all rollback scenarios
- This is best-effort abuse resistance, not a DRM boundary
- Server-side validation on every activation/validation call is the authoritative enforcement

**Enhancement (v2):** Store an encrypted monotonic counter that increments on each successful validation, making rollback detectable across system resets.

## Non-Real-Time HTTP Client (LIC-02)

### Recommended: JUCE URL::createInputStream on TimeSliceThread

```cpp
// plugin/source/Licensing/ActivationClient.h
#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include "EntitlementToken.h"

namespace lumascope {

class ActivationClient : private juce::TimeSliceClient {
public:
    using Callback = std::function<void(ActivationResult)>;

    ActivationClient();

    // Queue activation requests - runs on background thread
    void activate(const juce::String& licenseKey,
                  const juce::String& machineId,
                  Callback callback);

    void validate(const juce::String& licenseKey,
                  const juce::String& machineId,
                  Callback callback);

    void deactivate(const juce::String& licenseKey,
                    const juce::String& machineId,
                    Callback callback);

    // Cancel pending requests
    void cancelAll();

private:
    struct PendingRequest {
        juce::String endpoint;  // e.g., "/api/v1/activate"
        juce::String body;      // JSON string
        Callback callback;
    };

    int useTimeSlice() override;  // juce::TimeSliceClient

    juce::TimeSliceThread thread;  // background thread
    juce::CriticalSection queueLock;
    std::vector<PendingRequest> queue;

    void performRequest(const PendingRequest& request);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivationClient)
};

} // namespace lumascope
```

```cpp
// plugin/source/Licensing/ActivationClient.cpp
#include "ActivationClient.h"
#include <juce_core/juce_core.h>

namespace lumascope {

ActivationClient::ActivationClient()
    : thread("LumaScope Activation Thread")
{
    thread.startThread(2);  // normal priority
    thread.addTimeSliceClient(this, 50);  // check queue every 50ms
}

void ActivationClient::activate(const juce::String& licenseKey,
                                 const juce::String& machineId,
                                 Callback callback)
{
    // Build request body matching Phase 5 contract
    auto body = juce::var(new juce::DynamicObject());
    auto* obj = body.getDynamicObject();
    obj->setProperty("licenseKey", licenseKey);
    obj->setProperty("machineId", machineId);
    obj->setProperty("requestId", "req_" + juce::Uuid().toString());
    obj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601());
    obj->setProperty("appVersion", JucePlugin_VersionString);

    const juce::ScopedLock lock(queueLock);
    queue.push_back({ "/api/v1/activate", juce::JSON::toString(body), callback });
}

int ActivationClient::useTimeSlice()
{
    PendingRequest request;
    {
        const juce::ScopedLock lock(queueLock);
        if (queue.empty())
            return 500;  // check again in 500ms
        request = queue.front();
        queue.erase(queue.begin());
    }

    // Perform HTTP request on background thread
    performRequest(request);
    return 5;  // check for more work soon
}

void ActivationClient::performRequest(const PendingRequest& request)
{
    // Build URL (base from settings or compile-time default)
    const juce::URL base("https://api.lumascope.example.com");
    const auto url = base.getChildURL(request.endpoint);

    juce::StringPairArray responseHeaders;
    auto inStream = url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withHttpRequestCmd("POST")
            .withExtraHeaders("Content-Type: application/json")
            .withResponseHeaders(&responseHeaders)
            .withConnectionTimeoutMs(10000)
            .withNumRedirectsToFollow(3));

    if (inStream == nullptr)
    {
        // Network error
        ActivationResult result;
        result.errorCode = "network_error";
        result.errorMessage = "Unable to reach activation service.";
        if (request.callback) request.callback(result);
        return;
    }

    // Read response
    const auto responseStr = inStream->readEntireStreamAsString();
    const auto statusCode = responseHeaders["Status-Code"].getIntValue();

    // Parse response
    juce::var parsed;
    const auto parseOk = juce::JSON::parse(responseStr, parsed);

    // Build result and invoke callback
    ActivationResult result;
    result.httpStatusCode = statusCode;
    result.responseBody = parsed;
    // Parse entitlement token from response if successful...

    juce::MessageManager::callAsync([callback = request.callback, result]() {
        // Deliver result on message thread
        if (callback) callback(result);
    });
}

} // namespace lumascope
```

**Key threading rules:**
1. `ActivationClient` is created/owned by the `LumaScopeAudioProcessor` (which is the permanent object, not the editor)
2. The audio callback never calls `ActivationClient` methods — the editor timer or the React UI (via bridge→editor) triggers activation
3. `performRequest()` runs on the `TimeSliceThread`, not the message thread
4. Callbacks are delivered via `juce::MessageManager::callAsync()` to run on the message thread
5. Licensing state is updated on the message thread, which is also where the timer polls it for bridge emission

## Grace/Clock Model (LIC-05, LIC-06)

### Entitlement State Machine

```
                     ┌──────────────────────────────┐
                     │        not_activated          │
                     │  (no local entitlement file)  │
                     └──────────────┬───────────────┘
                                    │ User activates
                                    ▼
                     ┌──────────────────────────────┐
                     │         activated             │
                     │  (valid signed entitlement,   │
                     │   online validation succeeded)│
                     └──────────────┬───────────────┘
                                    │
                      ┌─────────────┴─────────────┐
                      │                           │
                      ▼                           ▼
          ┌──────────────────────┐    ┌──────────────────────┐
          │    offline_grace     │    │   revalidation_      │
          │  (within 7 days of   │    │   required            │
          │   last online OK)    │    │  (>7 days since      │
          └──────────────────────┘    │  last online OK)     │
                      │               └──────────────────────┘
                      │                           │
                      │     User tries validation  │
                      ▼                           ▼
          ┌──────────────────────────────────────────────┐
          │        Validate online                        │
          │   Success → back to activated                 │
          │   Failure (network) → stay in grace/revalidation│
          │   Failure (revoked) → revoked state           │
          └──────────────────────────────────────────────┘
                      │
                      ▼
          ┌──────────────────────┐
          │       revoked        │
          │  or corrupt          │
          │  or service_unavail. │
          └──────────────────────┘
```

### Grace Tracking Data

Stored alongside (or inside) the DPAPI-protected entitlement:

```json
{
  "entitlementToken": { /* full signed entitlement from server */ },
  "verifiedAt": "2026-06-23T12:00:00.000Z",
  "lastKnownSystemTime": "2026-06-23T12:00:00.000Z",
  "lastOfflineCheckAt": "2026-06-24T08:00:00.000Z"
}
```

The `verifiedAt` and `lastKnownSystemTime` are signed with the same Ed25519 key? No — they're stored alongside and protected by DPAPI. Tampering with these values would require compromising DPAPI, which is the same protection boundary as the entitlement itself.

**Grace check logic (at startup and periodically):**

```cpp
void LicensingCore::checkGrace()
{
    const auto currentTime = juce::Time::getCurrentTime();
    const auto storedData = store.read();

    if (!storedData.has_value())
    {
        state.updateStatus(LicenseStatus::notActivated);
        return;
    }

    // Verify Ed25519 signature first
    if (!verifier.verify(storedData->entitlementToken))
    {
        state.updateStatus(LicenseStatus::corrupt);
        return;
    }

    const auto lastVerified = juce::Time::fromISO8601(storedData->verifiedAt);
    const auto lastKnown = juce::Time::fromISO8601(storedData->lastKnownSystemTime);

    // Check server-side expiry first
    const auto expiresAt = juce::Time::fromISO8601(
        storedData->entitlementToken.payload.expiresAt);
    if (currentTime > expiresAt)
    {
        state.updateStatus(LicenseStatus::revoked);
        return;
    }

    // Check grace period
    const auto graceResult = graceModel.checkOfflineGrace(lastVerified, lastKnown, currentTime);
    switch (graceResult)
    {
        case GraceModel::GraceStatus::active:
            state.updateStatus(LicenseStatus::offlineGrace);
            break;
        case GraceModel::GraceStatus::warning:
            state.updateStatus(LicenseStatus::offlineGrace);
            // UI shows "X days remaining" warning
            break;
        case GraceModel::GraceStatus::expired:
            state.updateStatus(LicenseStatus::revalidationRequired);
            break;
    }
}
```

### Periodic Revalidation Strategy

- **On launch:** Check grace status immediately
- **Periodically (every 6 hours):** Try online validation in background (if network available)
- **On user action:** Activation dialog triggers explicit validation
- **Before grace expiry:** Show warning 1 day before expiry
- **After grace expiry:** Show revalidation-required state; require successful online validation to restore activated status

## Public-Key Rotation Strategy

### Recommended Approach: Embed Key Ring, Support Multiple Kids

The native code embeds the entire public key ring (list of `{kid, publicKey, algorithm}` entries). Each signed entitlement carries a `kid` field. Verification:
1. Look up the `kid` from the ring
2. Decode the base64url-encoded public key
3. Use libsodium to verify the Ed25519 signature

**Rotation workflow:**
1. Deploy new Worker with new `SIGNING_PRIVATE_KEY` + updated `SIGNING_PUBLIC_KEYS` (both old and new keys)
2. Release native app update that embeds the new key ring
3. New entitlements use the new key
4. Old entitlements remain verifiable because the old key is still in the ring
5. After sufficient migration time, remove old key from ring in a future update

**Graceful degradation on unknown kid:** If a token's `kid` is not found in the embedded ring, treat it as `corrupt` (don't crash, don't assume valid). The UI will prompt the user to revalidate online, which returns a token with a known kid.

```cpp
bool TokenVerifier::verify(const SignedEntitlementToken& token)
{
    // Look up kid
    const auto* entry = keyRing.findByKid(token.payload.kid);
    if (entry == nullptr)
        return false;  // unknown key → corrupt

    // Decode public key from base64url SPKI
    const auto publicKeyBytes = decodeSpkiBase64Url(entry->publicKey);

    // Decode signature from base64url
    const auto signatureBytes = base64UrlDecode(token.signature);

    // Compute canonical JSON bytes
    const auto canonicalBytes = canonicalJsonBytes(token.payload);

    // Verify with libsodium
    return verifyEntitlementSignature(canonicalBytes, signatureBytes, publicKeyBytes);
}
```

## Test Fixture Strategy (LIC-10)

### Cross-Language Fixture Alignment

The canonical fixture `tests/worker/fixtures/entitlement-v1.json` is the source of truth. The native C++ tests consume the same fixture file.

**Native test pattern (following `HostBridgeTests.cpp` pattern):**

```cpp
// tests/native/Licensing/TokenVerifierTests.cpp
#include "LumaScope/Licensing/TokenVerifier.h"
#include "LumaScope/Licensing/EntitlementToken.h"
#include "LumaScope/Licensing/PublicKeyRing.h"
#include <juce_core/juce_core.h>
#include <iostream>

namespace {

juce::var readFixture(const char* path)
{
    return juce::JSON::parse(juce::File(path));
}

void testVerifyValidSignature()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    const auto fixture = readFixture(LUMASCOPE_ENTITLEMENT_FIXTURE);
    const auto* obj = fixture.getDynamicObject();
    expect(obj != nullptr, "fixture parsed");

    const auto canonical = obj->getProperty("canonical").toString();
    const auto signature = obj->getProperty("signature").toString();
    const auto publicKeySpki = obj->getProperty("publicKeySpki").toString();

    // Parse and verify
    lumascope::TokenVerifier verifier;
    const auto valid = verifier.verifyFromFixture(
        canonical.toStdString(),
        signature.toStdString(),
        publicKeySpki.toStdString());

    expect(valid, "valid signature from fixture");
}

void testRejectWrongSignature()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // Tamper with canonical JSON
    const auto fixture = readFixture(LUMASCOPE_ENTITLEMENT_FIXTURE);
    const auto* obj = fixture.getDynamicObject();

    std::string tamperedCanonical = obj->getProperty("canonical").toString().toStdString();
    tamperedCanonical += "x";  // invalidate

    const auto signature = obj->getProperty("signature").toString();
    const auto publicKeySpki = obj->getProperty("publicKeySpki").toString();

    lumascope::TokenVerifier verifier;
    const auto valid = verifier.verifyFromFixture(
        tamperedCanonical, signature.toStdString(), publicKeySpki.toStdString());

    expect(!valid, "tampered canonical rejected");
}

} // namespace

int runTokenVerifierTests()
{
    testVerifyValidSignature();
    testRejectWrongSignature();
    testVerifyWrongMachine();  // token.machineId != local machine
    testVerifyExpired();       // token.expiresAt in the past
    testVerifyUnknownKid();    // token.kid not in key ring
    return 0;
}
```

### Fixture Copy Pattern

```cmake
# In tests/native/CMakeLists.txt:
# Copy the worker fixture so native tests can find it
configure_file(
  "${CMAKE_SOURCE_DIR}/tests/worker/fixtures/entitlement-v1.json"
  "${CMAKE_CURRENT_BINARY_DIR}/fixtures/entitlement-v1.json"
  COPYONLY)

target_compile_definitions(LumaScopeNativeTests PRIVATE
  LUMASCOPE_ENTITLEMENT_FIXTURE="${CMAKE_CURRENT_BINARY_DIR}/fixtures/entitlement-v1.json"
  LUMASCOPE_PUBLIC_KEY_RING="${CMAKE_SOURCE_DIR}/tests/native/fixtures/public-key-ring.json")
```

## UI State Machine (UI-05)

### Recommended MUI Components

| Component | Usage | Why |
|-----------|-------|-----|
| `Chip` | Compact status indicator in footer/header | Small footprint, color-coded (success/warning/error/info), clickable for action |
| `Dialog` | License key entry, deactivation confirmation | Modal interaction requires user attention; standard MUI pattern |
| `Alert` | Grace-expiry warning banner | Temporarily appears in the shell, auto-dismissable |
| `TextField` | License key input in dialog | Standard text entry with paste support |
| `Button` | Activate/Deactivate actions | Clear call-to-action |
| `LinearProgress` | Activation/deactivation in-progress state | Indeterminate for network operations |
| `Tooltip` | Hover explanation of licensing state | Concise context without clutter |

### Layout Integration

Following D-08 (compact, not full-screen) and D-09 (footer/header control):

**Option A (recommended): Add activation status Chip to StatusFooter**
```
┌───────────────────────────────────────────────────────────────┐
│ [Activated Chip ✅] · Embedded UI · Bridge ready               │ Signal Foundry Audio  │
└───────────────────────────────────────────────────────────────┘
```

**Option B: Add activation status Chip to BrandHeader**
```
┌───────────────────────────────────────────────┬──────────────┐
│ LumaScope                                    │ [Activated]  │
│                                              │ [Deactivate] │
│ Standalone · 0.1.0                           │              │
└───────────────────────────────────────────────┴──────────────┘
```

**Recommended: Option A in footer** — preserves the analyzer-first instrument feel while keeping licensing accessible. The Chip is clickable to open the activation/deactivation dialog.

### Status-to-Color Mapping

| State | Chip Color | Icon | Label |
|-------|-----------|------|-------|
| `not_activated` | default (gray) | LockOpen | "Not activated — click to activate" |
| `activating` | info (blue) | HourglassEmpty | "Activating…" |
| `activated` | success (green) | Verified | "Activated" |
| `offline_grace` | warning (orange) | WifiOff | "Offline (X days remaining)" |
| `revalidation_required` | error (red) | Error | "Reactivation required" |
| `revoked` | error (red) | Block | "License revoked" |
| `corrupt` | error (red) | Warning | "License file corrupt" |
| `service_unavailable` | warning (orange) | CloudOff | "Service unavailable" |
| `deactivating` | info (blue) | HourglassEmpty | "Deactivating…" |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| JWT for entitlement tokens | Canonical JSON + detached Ed25519 signature | Phase 5 decision | Simpler cross-language verification; no JWT library needed |
| Projucer project files | CMake-only build | Phase 1 decision | libsodium integration via CPM.cmake follows existing dependency pattern |
| Always-online enforcement | 7-day offline grace with best-effort rollback detection | Project requirement | Users with intermittent connectivity can use the product; documented limitations |

**Deprecated/outdated:**
- **Windows MachineGUID-only identity:** The `HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid` registry value is accessible to any process on the machine but persists across hardware changes. However, using it as the sole identity source is discouraged because it's also used by Windows for other purposes and may change in-place.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | yes | Ed25519 signature verification via libsodium; embedded public key ring with kid rotation |
| V5 Input Validation | yes | Canonical JSON parsing in C++; bounded string lengths; closed protocol schemas |
| V6 Cryptography | yes | libsodium Ed25519 verification (constant-time); DPAPI for storage |
| V8 Data Protection | yes | DPAPI user-bound encryption; no raw hardware IDs transmitted |
| V9 Communication Security | yes | HTTPS via JUCE URL::createInputStream (OS-managed TLS) |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Entitlement file tampering | Tampering | DPAPI encryption prevents modification without user credentials |
| Signature forgery | Spoofing | Ed25519 verification with embedded public key ring |
| Clock rollback to extend grace | Elevation of Privilege | Best-effort monotonic time comparison; documented limitations |
| Machine identity spoofing | Spoofing | Composite hash of SID + volume serial; hard to forge both |
| Network MITM on activation | Spoofing | HTTPS via OS TLS stack; JUCE URL handles TLS natively |
| Replay activation request | Tampering | requestId + timestamp + replay protection on server (Phase 5) |

## Common Pitfalls

### Pitfall 1: Canonical JSON Mismatch
**What goes wrong:** The C++ canonical JSON implementation produces a different string than the TypeScript version, causing all signatures to fail verification.
**Why it happens:** Subtle differences in number formatting, escape sequences, or key ordering.
**How to avoid:** Share the exact canonical JSON string from the fixture in both TypeScript and C++ tests. The fixture `entitlement-v1.json` contains the exact `"canonical"` string. Verify that `canonicalJson()` in C++ produces EXACTLY this string for the same input.
**Warning signs:** Valid signatures from the Worker fail to verify in native tests; the canonical strings differ when compared byte-by-byte.

### Pitfall 2: Audio Thread Violation
**What goes wrong:** A developer accidentally calls `ActivationClient::validate()` or writes to the entitlement file from the audio callback.
**Why it happens:** The processor object owns both the analyzer and the licensing core, making it tempting to inline checks.
**How to avoid:** The `LicensingCore` object is completely separate from the `SpectrumAnalyzer`. Its methods that perform I/O are only callable via the `ActivationClient` (which asserts it's not on the audio thread). Use `juce::MessageManager::callAsync()` or the `TimeSliceThread` queue for all activation operations.
**Warning signs:** Dropouts, crackling, or host "audio device reset" errors when activation dialog is open.

### Pitfall 3: DPAPI Entropy Mismatch
**What goes wrong:** After an OS reinstall or logging in as a different user, the DPAPI blob cannot be decrypted.
**Why it happens:** DPAPI keys are tied to the user's logon credential; they change on password reset (with backup key), fresh install, or different user.
**How to avoid:** Handle `CryptUnprotectData` failure gracefully by treating the file as corrupt. The user reactivates. This is expected behavior documented in D-03.
**Warning signs:** Frozen UI at startup because the app tried to read the entitlement and got stuck.

### Pitfall 4: Overly Aggressive Validation Polling
**What goes wrong:** The editor timer calls `ActivationClient::validate()` every 100ms, flooding the server and draining battery.
**Why it happens:** The timer callback pattern from `EditorSnapshotPoller` is applied to licensing without considering that licensing doesn't need 45Hz updates.
**How to avoid:** Poll licensing state for bridge emission at 1-2 Hz (every 500-1000ms), not 45-60 Hz. Only trigger online validation at startup, every 6 hours during use, and on explicit user action.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | libsodium 1.0.20 provides a CMake build that works with CPM.cmake | Standard Stack | Fall back to pre-built binaries + FindSodium.cmake |
| A2 | The JUCE `userApplicationDataDirectory` path resolves to `%APPDATA%` on Windows | Application Data | Verify at runtime with `juce::Logger::writeToLog()` |
| A3 | The `CryptProtectData`/`CryptUnprotectData` API is available on all supported Windows versions | Protected Storage | Windows 2000+ — confirmed by Microsoft docs; no risk for Windows-only v1 |
| A4 | The canonical JSON C++ implementation matches TypeScript exactly | Ed25519 Verification | The fixture `entitlement-v1.json` `"canonical"` field is the gold standard — test against it |
| A5 | Multiple `sodium_init()` calls are safe | TokenVerifier | Confirmed by libsodium docs: "safe to call more than once and from different threads" |

## Open Questions

1. **Activation API base URL for development vs. production**
   - What we know: Phase 5 deployed endpoints exist at a configurable Worker URL
   - What's unclear: Should the base URL be a compile-time constant, an environment variable, or configurable via bridge event?
   - Recommendation: Use a compile-time constant with a `LUMASCOPE_ACTIVATION_BASE_URL` CMake variable, defaulting to `https://api.lumascope.example.com` for CI/release. For development, set it to the local Wrangler dev URL.

2. **Request retry/backoff policy**
   - What we know: Network errors during activation must not block the UI; we should retry transient failures
   - What's unclear: What backoff strategy (exponential? fixed?) and maximum retries?
   - Recommendation: 3 retries with 1s/2s/4s backoff; exponential backoff with jitter is standard but overengineered for v1.

3. **Smoke test integration for cross-language verification**
   - What we know: The existing `scripts/verify-project.ps1` runs native tests
   - What's unclear: How to add a "canonical JSON matches" verifier that compares C++ output to TypeScript output
   - Recommendation: Generate the canonical JSON from both sides and diff in CI; add a verifier step to `verify-project.ps1`

## Validation Architecture

> workflow.nyquist_validation is not explicitly false in config.json — treating as enabled.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Native test runner (custom `main()` with expect macros) |
| Config file | `tests/native/CMakeLists.txt` |
| Quick run command | `cmake --build build --target LumaScopeNativeTests && ctest --test-dir build -R LumaScopeNativeTests --output-on-failure` |
| Full suite command | `cmake --build build --target LumaScopeNativeTests && ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| LIC-01 | Machine identity derivation | unit | native test | ❌ Wave 0 |
| LIC-03 | Ed25519 signature verification | unit | native test | ❌ Wave 0 |
| LIC-03 | Canonical JSON correctness | unit | native test + compare to fixture | ❌ Wave 0 |
| LIC-04 | DPAPI encrypt/decrypt roundtrip | unit | native test | ❌ Wave 0 |
| LIC-05 | Grace period calculation | unit | native test | ❌ Wave 0 |
| LIC-07 | Clock rollback detection | unit | native test | ❌ Wave 0 |
| LIC-08 | Deactivation clears only on success | unit | native test | ❌ Wave 0 |
| UI-05 | Licensing state rendering in UI | integration | vitest component test | ❌ Wave 0 |
| LIC-10 | Cross-language fixture verification | unit | native test + worker test | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build -R LumaScopeNativeTests --output-on-failure`
- **Per wave merge:** Full build + full test suite
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `tests/native/Licensing/MachineIdentityTests.cpp` — covers LIC-01
- [ ] `tests/native/Licensing/TokenVerifierTests.cpp` — covers LIC-03, LIC-10
- [ ] `tests/native/Licensing/EntitlementTokenTests.cpp` — covers canonical JSON
- [ ] `tests/native/Licensing/GraceModelTests.cpp` — covers LIC-05, LIC-07
- [ ] `tests/native/Licensing/LocalEntitlementStoreTests.cpp` — covers LIC-04, LIC-08
- [ ] `tests/native/Licensing/ActivationClientTests.cpp` — covers HTTP client (mocked)
- [ ] `tests/native/Licensing/CMakeLists.txt` — test target registration

## Sources

### Primary (HIGH confidence)
- [Context7: /websites/juce_master] — `File::getSpecialLocation`, `URL::createInputStream`, `JSON::parse/toString`, `TimeSliceThread`, `WebBrowserComponent::withEventListener`
- [libsodium docs: doc.libsodium.org] — Ed25519 detached signatures, `crypto_sign_verify_detached()`, SHA-256, base64url encoding, CMake integration
- [Microsoft Win32 docs: learn.microsoft.com] — `CryptProtectData`, `CryptUnprotectData`, DPAPI; `LookupAccountNameW`, `ConvertSidToStringSidW`, `GetVolumeInformationW`, `GetComputerNameW`; `GetSystemDirectoryW`

### Secondary (MEDIUM confidence)
- [WebSearch: Chromium rlz machine_id_win.cc] — Proven approach: Machine SID + System Volume Serial Number for stable Windows ID
- [WebSearch: NebulaID-Open] — Educational Windows HWID generation patterns (WMI-based, but validates SID+volume serial approach)
- [WebSearch: HappyIn.space licensing C++ implementation] — Confirms libsodium as preferred Ed25519 library for C++ licensing; size/audit comparison table

### Tertiary (LOW confidence)
- None — all key claims are backed by official documentation (JUCE, libsodium, Microsoft)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — JUCE 8.0.14 patterns confirmed via Context7; libsodium via official docs; DPAPI via Microsoft docs
- Architecture: HIGH — bridge extension follows existing HostBridge pattern exactly; state machine is standard licensing design
- Pitfalls: HIGH — canonical JSON mismatch and audio thread violations are known failure patterns from Phase 2/3 experience

**Research date:** 2026-06-23
**Valid until:** 2026-07-23 (30 days — stable dependencies)
