---
status: partial
phase: 06-native-offline-licensing
source:
  - 06-01-SUMMARY.md
  - 06-02-SUMMARY.md
  - 06-03-SUMMARY.md
  - 06-04-SUMMARY.md
started: 2026-06-24T00:00:00-07:00
updated: 2026-06-24T00:00:00-07:00
---

## Current Test

[testing paused - 6 items outstanding]

## Tests

### 1. Activate License In Product
expected: Open either LumaScope target, start activation from the licensing UI, enter a valid license key, and submit. The UI should move through a pending state and then show the product as activated without interrupting audio analysis.
result: [pending]

### 2. Launch Offline During Grace
expected: After a successful validation, disconnect from the activation service and relaunch. The product should continue to open as usable under offline grace and show the remaining grace status clearly.
result: [pending]

### 3. Understand License Status
expected: The status chip, warning alert, and footer should distinguish activated, offline grace, revalidation required, revoked or expired, corrupt, service unavailable, and deactivating states with clear user-facing text.
result: [pending]

### 4. Deactivate And Transfer
expected: Trigger deactivation from the UI. On server success, local entitlement should clear, the product should return to an unactivated state, and the license should be ready to activate on another machine.
result: [pending]

### 5. Recover From Corrupt Or Tampered Entitlement
expected: If the stored entitlement is corrupt or tampered, relaunching should not crash or silently accept it. The product should show a corrupt or revalidation-required status and guide the user back to activation.
result: [pending]

### 6. Preserve Audio Thread Safety During Licensing
expected: Activation, validation, deactivation, outages, and status updates should occur on non-real-time paths. Audio analysis should continue without dropouts caused by network, disk, or WebView work on the audio callback.
result: [pending]

## Summary

total: 6
passed: 0
issues: 0
pending: 6
skipped: 0
blocked: 0

## Gaps
