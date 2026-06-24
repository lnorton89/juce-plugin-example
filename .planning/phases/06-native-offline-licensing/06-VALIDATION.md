---
phase: 06
slug: native-offline-licensing
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-23
---

# Phase 6 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native test runner (custom `main()` with expect macros) |
| **Config file** | `tests/native/CMakeLists.txt` |
| **Quick run command** | `cmake --build build --target LumaScopeNativeTests ; if ($?) { ctest --test-dir build -R LumaScopeNativeTests --output-on-failure }` |
| **Full suite command** | `cmake --build build --target LumaScopeNativeTests ; if ($?) { ctest --test-dir build --output-on-failure }` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build --target LumaScopeNativeTests && ctest --test-dir build -R LumaScopeNativeTests --output-on-failure`
- **After every plan wave:** Run `cmake --build build --target LumaScopeNativeTests && ctest --test-dir build --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | LIC-01 | — | Local machine identity derivation, never transmits raw HW IDs | unit | native test | ❌ W0 | ⬜ pending |
| 06-01-02 | 01 | 1 | LIC-03 | T-6-01 | Ed25519 signature verification via libsodium | unit | native test | ❌ W0 | ⬜ pending |
| 06-01-03 | 01 | 1 | LIC-03 | — | Canonical JSON matches fixture exactly | unit | native test | ❌ W0 | ⬜ pending |
| 06-02-01 | 02 | 2 | LIC-04 | T-6-02 | DPAPI encrypt/decrypt roundtrip with tamper detection | unit | native test | ❌ W0 | ⬜ pending |
| 06-02-02 | 02 | 2 | LIC-05, LIC-07 | T-6-03 | Grace period calculation and clock rollback detection | unit | native test | ❌ W0 | ⬜ pending |
| 06-02-03 | 02 | 2 | LIC-08 | T-6-04 | Deactivation clears only on successful server response | unit | native test | ❌ W0 | ⬜ pending |
| 06-03-01 | 03 | 3 | LIC-02 | — | Activation HTTP client never runs on audio thread | unit | native test | ❌ W0 | ⬜ pending |
| 06-03-02 | 03 | 3 | UI-05 | — | Licensing state rendering in React/MUI | integration | vitest component test | ❌ W0 | ⬜ pending |
| 06-04-01 | 04 | 4 | LIC-10 | — | Cross-language fixture verification | unit | native test + worker test | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/native/Licensing/MachineIdentityTests.cpp` — stubs for LIC-01
- [ ] `tests/native/Licensing/TokenVerifierTests.cpp` — stubs for LIC-03, LIC-10
- [ ] `tests/native/Licensing/EntitlementTokenTests.cpp` — stubs for canonical JSON
- [ ] `tests/native/Licensing/GraceModelTests.cpp` — stubs for LIC-05, LIC-07
- [ ] `tests/native/Licensing/LocalEntitlementStoreTests.cpp` — stubs for LIC-04, LIC-08
- [ ] `tests/native/Licensing/ActivationClientTests.cpp` — stubs for HTTP client
- [ ] `tests/native/Licensing/CMakeLists.txt` — test target registration

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Activation/deactivation end-to-end with live Worker | LIC-02, LIC-08 | Requires deployed Worker with real Cloudflare/D1 | Deploy Worker, activate with test license, validate, deactivate via native UI |
| Offline grace survival across app restart | LIC-05 | Requires system clock observation across process lifetime | Activate, disconnect network, restart app, confirm offline status persists up to 7 days |
| Deployed activation smoke | LIC-10 | Requires account-specific credentials | Follow `docs/activation-api.md` deployed smoke steps |

*If none: "All phase behaviors have automated verification."*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
