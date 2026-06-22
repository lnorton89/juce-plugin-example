---
phase: 1
slug: reproducible-product-shell
status: approved
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-22
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest for native/build contracts; Vitest + React Testing Library for frontend/bridge |
| **Config files** | `CMakePresets.json`, `ui/vitest.config.ts`; Wave 1 creates them |
| **Quick run command** | `pwsh -NoProfile -File scripts/test-quick.ps1` |
| **Full suite command** | `pwsh -NoProfile -File scripts/test-all.ps1` |
| **Estimated runtime** | Quick < 60 seconds after initial build; full < 5 minutes excluding first dependency download |

## Sampling Rate

- **After every task commit:** Run `pwsh -NoProfile -File scripts/test-quick.ps1` or the narrower command named by the task when infrastructure is still being created.
- **After every plan wave:** Run `pwsh -NoProfile -File scripts/test-all.ps1`.
- **Before `$gsd-verify-work`:** Full suite must be green and both native artifacts must exist.
- **Max feedback latency:** 60 seconds for quick checks after dependencies/build cache are warm.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 1 | BUILD-01, BUILD-02 | T-01-01 | Dependency sources and dev URL are explicit | preflight/build | `pwsh -NoProfile -File scripts/check-environment.ps1 -Json` plus VS2019 configure/build probe | ❌ W0 | ⬜ pending |
| 01-01-02 | 01 | 1 | BUILD-02, BUILD-03, UI-01 | T-01-02 | Embedded archive serves only normalized known paths | unit/build | `npm --prefix ui test -- --run && cmake --build --preset vs2019-debug --target LumaScope_All` | ❌ W0 | ⬜ pending |
| 01-01-03 | 01 | 1 | UI-03 | T-01-03 | Handshake rejects malformed/unsupported protocol versions | unit/integration | `pwsh -NoProfile -File scripts/test-quick.ps1` | ❌ W0 | ⬜ pending |
| 01-02-01 | 02 | 2 | UI-01 | — | UI uses local bundled assets and semantic controls | component | `npm --prefix ui test -- --run` | ❌ W0 | ⬜ pending |
| 01-02-02 | 02 | 2 | BUILD-03, UI-01 | T-01-02 | Production bundle contains no remote font/CDN dependency | build/source | `npm --prefix ui run build && npm --prefix ui run check:bundle` | ❌ W0 | ⬜ pending |
| 01-02-03 | 02 | 2 | UI-01, UI-03 | T-01-03 | Error/status states are typed and keyboard accessible | component/a11y | `npm --prefix ui test -- --run` | ❌ W0 | ⬜ pending |
| 01-03-01 | 03 | 3 | BUILD-04, UI-03 | T-01-04 | Native integration is enabled only for trusted embedded or fixed loopback origin | native/integration | `pwsh -NoProfile -File scripts/test-web-modes.ps1` | ❌ W0 | ⬜ pending |
| 01-03-02 | 03 | 3 | BUILD-01, BUILD-02, BUILD-04 | — | Setup/build scripts fail early and do not hide toolchain substitution | script/build | `pwsh -NoProfile -File scripts/check-environment.ps1 -Json; pwsh -NoProfile -File scripts/test-all.ps1` | ❌ W0 | ⬜ pending |
| 01-03-03 | 03 | 3 | BUILD-05, UI-03 | T-01-01 | Documentation contains no secrets or unrestricted remote origins | docs/source | `pwsh -NoProfile -File scripts/verify-project.ps1` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

## Threat References

| ID | Threat | Required Control |
|----|--------|------------------|
| T-01-01 | Unpinned or spoofed dependency/config source | Exact versions/commits, HTTPS sources, lockfiles, no committed credentials |
| T-01-02 | Resource-provider path traversal or unsafe content type | Normalize/reject `..`, query/fragment stripping, allowlisted MIME types, nullopt on unknown/missing resources |
| T-01-03 | Malformed or incompatible bridge payload | Protocol version, schema validation, bounded payload shape, typed error state |
| T-01-04 | Untrusted page gains native integration | Embedded provider root or fixed `http://127.0.0.1:<port>` only; never arbitrary URL/origin |

## Wave 0 Requirements

- [ ] `ui/vitest.config.ts` and `ui/src/test/setup.ts` — Vitest/RTL environment.
- [ ] `tests/native/CMakeLists.txt` — CTest target for protocol/resource helpers.
- [ ] `tests/fixtures/bridge/*.json` — protocol v1 fixture corpus.
- [ ] `tests/fixtures/web/` — embedded resource and MIME fixtures.
- [ ] `scripts/test-quick.ps1` — non-watch frontend/native fast checks.
- [ ] `scripts/test-all.ps1` — full UI build/tests plus native configure/build/CTest.
- [ ] `scripts/test-web-modes.ps1` — embedded/dev-mode smoke orchestration.
- [ ] `scripts/verify-project.ps1` — artifact, documentation, and forbidden-string checks.

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Embedded standalone renders approved shell and reports bridge ready | BUILD-03, UI-01, UI-03 | Requires real WebView2 window | Build `vs2019-debug`, launch standalone with dev option empty, verify LumaScope shell, `Embedded UI · Bridge ready`, no blank window |
| Vite hot reload displays same handshake contract | BUILD-04, UI-03 | Requires live Vite/native coordination | Start documented Vite task, build/launch dev-mode standalone, edit visible subtitle, verify reload and `Vite development · Bridge ready` |
| Minimum size and 150%/200% DPI | UI-01 | Visual/system scaling behavior | Inspect 960×600 and 560×360 at Windows scaling values; verify no overlap or essential-status clipping |
| WebView2/runtime failure fallback | UI-01 | React cannot prove native fallback | Invoke documented diagnostic/test hook and verify native fallback explains runtime/assets recovery |

## Validation Sign-Off

- [x] All anticipated tasks have automated verification or Wave 0 dependencies.
- [x] Sampling continuity: no three consecutive tasks without automated verification.
- [x] Wave 0 covers all missing test infrastructure.
- [x] No watch-mode flags are used in verification commands.
- [x] Feedback latency target is below 60 seconds after warm-up.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-06-22
