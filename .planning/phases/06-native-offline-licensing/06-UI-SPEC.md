---
title: "Phase 06 — Native Offline Licensing: UI Design Contract"
status: draft
phase: 6
updated: 2026-06-23
design_system: Material UI (dark theme, custom lumaScopeTheme)
---

# UI Design Contract — Phase 6: Native Offline Licensing

## 1. Overview & Phase Context

**Goal:** Users can activate both products, work offline for seven days, understand their licensing state at a glance, and deactivate to transfer the license — all without displacing the analyzer-first instrument feel.

**Requirement:** UI-05 — UI exposes activation, activated, offline-grace, revalidation-required, deactivation, and actionable failure states.

**Product Tone:** The application is a spectrum analyzer first. Licensing is a compact status/control surface (D-08, D-09). Activation should feel like a capability unlock, not a dashboard login. Offline operation should feel normal while staying honest about state.

**Existing Layout Context:**

```
┌──────────────────────────────────────────────────────┐
│ BrandHeader (52px)                                    │
├──────────────────────────────────────────────────────┤
│ StandaloneSourceStrip (auto — VST3 hides this)        │
├──────────────────────────────────────────────────────┤
│ AnalyzerStage (minmax(0, 1fr) — primary content)      │
├──────────────────────────────────────────────────────┤
│ StatusFooter (32px)  ← extended with licensing Chip  │
└──────────────────────────────────────────────────────┘
```

**Key Decision from CONTEXT.md (D-09):** Use a compact footer-adjacent control/status affordance for activation state, with dialogs for license-key entry, activation progress/errors, and deactivation confirmation.

---

## 2. Design Principles

| # | Principle | Rationale |
|---|-----------|-----------|
| P1 | **Preserve analyzer primacy** | Licensing never replaces the analyzer stage, opens as a full-screen dashboard, or demands attention during normal use. All licensing UI is compact chip + dialog. |
| P2 | **Status at a glance** | The user can see their licensing state immediately in the footer without opening any dialog. Color, icon, and 2-3 word label encode the state. |
| P3 | **Actionable, not just informative** | Every non-activated state maps to a clear action (click to activate, view details, retry). No dead-end error states without recovery guidance. |
| P4 | **Fail safe, not silent** | Network errors, corruption, and revocation produce specific visible states with human-readable recovery copy. The app never silently falls back to unlicensed behavior. |
| P5 | **Offline feels normal** | During the 7-day grace window, the analyzer works fully. The only difference is a subtle indicator showing "Offline (X days)". No disruptive modals or nags until the revalidation-required boundary. |
| P6 | **Server-authoritative deactivation** | D-11: Clear local entitlement only after successful server deactivation. Failed deactivation preserves the current state and explains the failure. |

---

## 3. Layout & Composition

### 3.1 Grid Extension

Extend the existing `AppShell` grid from 4 rows to 4+ rows. The licensing status Chip lives in the **StatusFooter**. The licensing dialogs overlay the grid as modal dialogs (MUI `Dialog`).

```
gridTemplateRows: '52px auto minmax(0, 1fr) 32px'
                     │      │        │          │
                  header  source   analyzer   footer
                      strip                (+ licensing Chip)
```

**No new rows or full-height licensing panels.** The footer grows naturally by adding a Chip to the existing flex row. Dialogs mount at the `Box` component level (outside the grid) via MUI `Dialog` with `Portal`.

### 3.2 StatusFooter Extension

The StatusFooter (`ui/src/components/StatusFooter.tsx`) receives a new `licensing` prop:

```tsx
export interface StatusFooterProps {
  bridge: BridgeState;
  licensing?: LicenseStatusState;  // NEW
}
```

Layout within the footer (left-to-right):

```
[bridge status + diagnostics]    [LICENSING CHIP]    [Signal Foundry Audio]
   ◄— flex: 1 (left group) —►   ◄— compact —►      ◄— responsive hide —►
```

The licensing Chip always appears between the bridge status group (left) and the brand name (right). It has a fixed minimum width of 120px to ensure readability.

### 3.3 Dialog Mount Points

| Dialog | Trigger | Mount |
|--------|---------|-------|
| **ActivationDialog** | Click Chip when state is `not_activated` or `revalidation_required` or `revoked` or `corrupt` | MUI `Dialog` via state in `AppShell.tsx` |
| **DeactivationDialog** | Click Chip when state is `activated` or `offline_grace`, then click "Deactivate" | MUI `Dialog`, secondary action from ActivationDialog |

Both dialogs use `fullWidth` with `maxWidth="sm"` (600px cap). They are not full-screen and do not cover the analyzer stage entirely on wider viewports.

---

## 4. Component Inventory & Specifications

### 4.1 ActivationStatus Chip (NEW)

**File:** `ui/src/components/ActivationStatus.tsx`

**Purpose:** Compact, color-coded status indicator that lives in the StatusFooter. Clicking it opens the appropriate dialog.

**States:**

| State | Chip Color | MUI Palette Ref | Icon | Label | Click Action |
|-------|-----------|-----------------|------|-------|-------------|
| `not_activated` | default | `default` | `LockOpenOutlined` | "Activate" | Open ActivationDialog |
| `activating` | info (blue) | `info.main` (#53D8FB) | `HourglassEmpty` | "Activating…" | None (disabled) |
| `activated` | success (green) | `success.main` (#B7F36B) | `VerifiedOutlined` | "Activated" | Open info tooltip + deactivation option |
| `offline_grace` | warning (amber) | `warning.main` (#FFC857) | `WifiOffOutlined` | "Offline (Xd)" | Open details tooltip |
| `revalidation_required` | error (red) | `error.main` (#FF6B78) | `ErrorOutline` | "Re-activate" | Open ActivationDialog |
| `revoked` | error (red) | `error.main` (#FF6B78) | `BlockOutlined` | "License revoked" | Open info tooltip |
| `corrupt` | error (red) | `error.main` (#FF6B78) | `WarningAmberOutlined` | "License error" | Open info dialog |
| `service_unavailable` | warning (amber) | `warning.main` (#FFC857) | `CloudOffOutlined` | "Service offline" | Open tooltip |
| `deactivating` | info (blue) | `info.main` (#53D8FB) | `HourglassEmpty` | "Deactivating…" | None (disabled) |

**Specification:**

```tsx
// Minimal MUI Chip with custom color, icon, label
<Chip
  size="small"
  variant="outlined"
  color={muiColor}     // mapped from state
  icon={<StateIcon />}
  label={labelText}
  onClick={handleClick}
  disabled={isTransitionState}
  sx={{
    minWidth: 120,
    height: 24,
    '& .MuiChip-label': { fontSize: 12, fontWeight: 600, px: 1 },
    '& .MuiChip-icon': { fontSize: 16, ml: 0.5 },
  }}
/>
```

**Spacing:** 8px gap from adjacent footer items. 8px internal padding (MuiChip default).

**Accessibility:** `role="status"` with `aria-live="polite"` when state changes. The Chip is a `<button>` role by default (MUI Chip with onClick).

### 4.2 ActivationDialog (NEW)

**File:** `ui/src/components/ActivationDialog.tsx`

**Purpose:** Modal dialog for entering a license key and triggering activation.

**States:**

| State | UI Representation |
|-------|-------------------|
| **Initial (not_activated)** | Title: "Activate LumaScope", TextField for license key, "Activate" button, "Cancel" link |
| **Activating (pending)** | Title: "Activating…", LinearProgress (indeterminate), TextField disabled, buttons disabled |
| **Success** | Title: "Activation complete", green checkmark + message, "Continue" button (closes dialog) |
| **Error — invalid key** | Title: "Invalid license key", red error Alert with message, TextField editable for retry, "Try again" + "Cancel" |
| **Error — network** | Title: "Connection issue", amber warning Alert with message, "Retry" + "Cancel" |
| **Error — server** | Title: "Activation failed", red error Alert with server message, "Try again" + "Cancel" |
| **Error — already activated** | Title: "Already activated", warning Alert with message, "Manage activation" + "Cancel" |

**Layout:**

```
┌──────────────────────────────────────────────────┐
│ DialogTitle: "Activate LumaScope"                 │
├──────────────────────────────────────────────────┤
│ DialogContent:                                     │
│                                                   │
│  [key icon] License Key                           │
│  ┌──────────────────────────────────────────┐     │
│  │ XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX       │     │
│  └──────────────────────────────────────────┘     │
│                                                   │
│  [Alert for error/success — conditional]          │
│                                                   │
│  [LinearProgress — conditional, indeterminate]   │
│                                                   │
│  Typography: "Paste your license key from the     │
│  email you received after purchase."              │
│                                                   │
├──────────────────────────────────────────────────┤
│ DialogActions:                                     │
│  [Cancel]                    [Activate]            │
└──────────────────────────────────────────────────┘
```

**Specification:**

| Element | MUI Component | Props/SX |
|---------|--------------|----------|
| Container | `Dialog` | `fullWidth maxWidth="sm"` |
| Title | `DialogTitle` | `variant="h2"` — 20px, 600 weight, -0.01em tracking |
| Key input | `TextField` | `fullWidth variant="outlined" size="small" autoFocus` |
| Key input format | `InputProps` with `startAdornment` | `VpnKeyOutlined` icon |
| Error/success | `Alert` | `severity="error"` or `"warning"` or `"success"` |
| Progress | `LinearProgress` | Indeterminate, `sx={{ my: 1 }}` |
| Actions | `DialogActions` | `[Cancel Button] [Activate Button]` |

**Typographic specs:**
- Title: `h2` variant — 20px, 600 weight, 1.25 line-height
- Body copy: `body2` — 14px, 400 weight, 1.5 line-height, `text.secondary`
- Alert text: `body2` — 14px
- Button labels: `button` variant — 14px, 600 weight

**Spacing:**
- `DialogContent` padding: 24px (default MUI)
- Gap between TextField and Alert: 16px
- `DialogActions` padding: 16px 24px

**Keyboard behavior:**
- Enter key in TextField submits activation
- Escape closes dialog (MUI Dialog default)
- Tab order: TextField → [Cancel] → [Activate]
- autoFocus on TextField when dialog opens (MUI DialogContent contains autoFocus TextField)

**Error transitions:**
- On error: clear `activating` state, re-enable TextField, show error Alert above buttons
- User can edit the key and click "Try again" or "Cancel"
- "Try again" button is the same as Activate — re-sends the request
- "Cancel" closes dialog, returns to previous state

### 4.3 DeactivationDialog (NEW)

**File:** `ui/src/components/DeactivationDialog.tsx`

**Purpose:** Confirm the user wants to deactivate this machine, explaining that deactivation is permanent for this machine (the license transfers away).

**States:**

| State | UI Representation |
|-------|-------------------|
| **Confirm** | Title: "Deactivate this machine?", warning message, [Keep activated (primary)] [Deactivate (destructive)] |
| **Deactivating** | Title: "Deactivating…", LinearProgress, buttons disabled |
| **Success** | Title: "Deactivation complete", success message, "Close" |
| **Error** | Title: "Deactivation failed", error Alert, "Try again" + "Cancel" |

**Layout:**

```
┌──────────────────────────────────────────────────┐
│ DialogTitle: "Deactivate this machine?"           │
├──────────────────────────────────────────────────┤
│ DialogContent:                                     │
│                                                   │
│  [WarningAmberOutlined icon]                      │
│                                                   │
│  Typography: "This will free your activation so   │
│  you can activate LumaScope on another computer.  │
│  You can reactivate here at any time."            │
│                                                   │
│  [Alert for error — conditional]                  │
│  [LinearProgress — conditional]                   │
│                                                   │
├──────────────────────────────────────────────────┤
│ DialogActions:                                     │
│  [Keep activated]        [Deactivate — error]      │
└──────────────────────────────────────────────────┘
```

**Specification:**

| Element | MUI Component | Props/SX |
|---------|--------------|----------|
| Container | `Dialog` | `fullWidth maxWidth="sm"` |
| Title | `DialogTitle` | `variant="h2"` |
| Warning icon | `WarningAmberOutlined` | `color="warning"` (amber), centered block |
| Body | `Typography` | `variant="body2" color="text.secondary"` |
| Deactivate button | `Button` | `color="error" variant="contained"` — red to signal destructive action |
| Keep button | `Button` | `variant="outlined"` — neutral, primary action |

**D-11 enforcement:** The Deactivate button is only enabled after the server confirms deactivation. If the server request fails, the error Alert shows and the button remains enabled for retry. The local entitlement is cleared ONLY after server `200 OK` response from `/api/v1/deactivate`.

### 4.4 GraceWarnAlert (NEW)

**File:** `ui/src/components/GraceWarnAlert.tsx`

**Purpose:** A non-modal, auto-dismissable Alert that appears inside the AppShell when offline grace is about to expire. Appears between the source strip and the analyzer stage, or between header and stage when source strip is hidden.

**Trigger thresholds:**
- **Warning (≥3 days remaining):** No alert. The Chip shows "Offline (3d)" — sufficient notification.
- **Warning (3-2 days remaining):** Muted Alert appears: "Offline activation expires in X days. Connect to the internet to renew."
- **Urgent (1 day remaining, or <24h):** High-visibility Alert appears: "Activation expires tomorrow. Connect to the internet now to avoid interruption."
- **Critical (0 days, revalidation_required):** Error Alert appears (permanent until user acts): "Offline grace expired. Please activate to continue using LumaScope."

**Layout:**

```
┌──────────────────────────────────────────────────────┐
│  (header or source strip)                             │
├──────────────────────────────────────────────────────┤
│  [Alert severity="warning" onClose]                   │
│  "Offline activation expires in X days. Connect to    │
│   the internet to renew."                             │
│  [Dismiss]                                            │
├──────────────────────────────────────────────────────┤
│  AnalyzerStage                                        │
└──────────────────────────────────────────────────────┘
```

**Specification:**

| Element | MUI Component | Props/SX |
|---------|--------------|----------|
| Container | `Alert` | `severity={daysLeft <= 1 ? "error" : "warning"}` |
| Dismiss | `Alert` built-in `onClose` | Callback hides the alert (stores dismissed state in local state, resets on re-show) |
| Placement | `Box` between source strip and analyzer | `sx={{ mx: { xs: 3, sm: 5 }, my: 0 }}` |
| Animation | `Collapse` wrapper | Smooth enter/exit via MUI Collapse |

**Accessibility:** `role="alert"` with `aria-live="assertive"` for urgent/critical. For warning level: `role="status"` with `aria-live="polite"`.

**Dismiss behavior:** The Alert can be dismissed by the user. It reappears on next app launch if still in the warning window. No permanent suppression.

### 4.5 ActivationStateChip Tooltip

**Purpose:** Hovering the Chip in non-active states provides a brief explanation without opening a dialog.

| State | Tooltip text |
|-------|-------------|
| `activated` | "Licensed and activated. Click to deactivate." |
| `offline_grace` | "Operating offline. {X} days remaining before re-activation required." |
| `revoked` | "This license has been revoked by the issuer." |
| `corrupt` | "License file could not be verified. Please activate again." |
| `service_unavailable` | "Activation service is unreachable. Offline use continues." |
| `not_activated` | "No license activated. Click to enter your license key." |
| `revalidation_required` | "The offline grace period has expired. Connect to the internet to re-activate." |

**Specification:** MUI `Tooltip` wrapping the Chip with `placement="top"` and `enterDelay={500}`.

---

## 5. Bridge Protocol Extensions

### 5.1 New Event Definitions

Add to `protocol.ts`:

```typescript
// ===== Licensing Events (Phase 6) =====

export const licenseStatusEvent = 'license.status' as const;
export const licenseActivateEvent = 'license.activate' as const;
export const licenseDeactivateEvent = 'license.deactivate' as const;
export const licenseValidateEvent = 'license.validate' as const;

export type LicensingStateEnum =
  | 'not_activated'
  | 'activating'
  | 'activated'
  | 'offline_grace'
  | 'revalidation_required'
  | 'revoked'
  | 'corrupt'
  | 'service_unavailable'
  | 'deactivating';
```

### 5.2 Native-to-UI: `license.status`

**Direction:** Native → UI (emitted from editor timer, ~1-2 Hz)

**Purpose:** Deliver current licensing state and metadata to the frontend.

**Payload schema:**

```typescript
export interface LicenseStatusPayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseStatusEvent;
  state: LicensingStateEnum;
  activationId: string;            // bounded 64 chars
  lastVerifiedTime: string;        // ISO 8601, bounded 32 chars
  offlineGraceRemainingDays: number;  // -1 if not applicable
  code?: string;                   // error code, bounded 64 chars, omitted on success
  message?: string;                // error message, bounded 256 chars, omitted on success
}
```

**Parser:**

```typescript
export function parseLicenseStatus(value: unknown): LicenseStatusPayload | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (item.event !== licenseStatusEvent) return null;
  // Validate closed enum
  if (!isLicenseState(item.state)) return null;
  if (typeof item.activationId !== 'string' || item.activationId.length > 64) return null;
  if (typeof item.lastVerifiedTime !== 'string' || item.lastVerifiedTime.length > 32) return null;
  if (typeof item.offlineGraceRemainingDays !== 'number') return null;
  if (item.code !== undefined && (typeof item.code !== 'string' || item.code.length > 64)) return null;
  if (item.message !== undefined && (typeof item.message !== 'string' || item.message.length > 256)) return null;
  return item as unknown as LicenseStatusPayload;
}
```

### 5.3 UI-to-Native: `license.activate`

**Direction:** UI → Native (user clicks Activate in ActivationDialog)

**Purpose:** Send license key and trigger activation request on the native side.

**Payload schema:**

```typescript
export interface LicenseActivatePayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseActivateEvent;
  licenseKey: string;   // bounded 256 chars
}
```

### 5.4 UI-to-Native: `license.deactivate`

**Direction:** UI → Native (user confirms deactivation)

**Purpose:** Request deactivation of the current machine.

**Payload schema:**

```typescript
export interface LicenseDeactivatePayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseDeactivateEvent;
  // No additional fields — native side uses stored license info
}
```

### 5.5 UI-to-Native: `license.validate`

**Direction:** UI → Native (user clicks "Validate Now" or periodic background check)

**Purpose:** Trigger online validation of current activation.

**Payload schema:**

```typescript
export interface LicenseValidatePayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseValidateEvent;
  // No additional fields — native side uses stored license info
}
```

### 5.6 BridgeProvider Extension

Add to `BridgeProvider.tsx`:

```typescript
// New listener in useEffect:
const licenseToken = transport.addEventListener(licenseStatusEvent, (payload) => {
  const status = parseLicenseStatus(payload);
  if (status) setLicenseStatus(status);
});

// Return cleanup fn:
// ...existing cleanup...
transport.removeEventListener(licenseToken);
```

New hook:

```typescript
export function useLicenseStatus(): LicenseStatusPayload | null {
  return useContext(LicenseContext);
}

export function useLicenseRequest(): LicenseRequestHelpers | null {
  const transport = useMemo(() => { ... }, []);
  const sendActivate = useCallback((licenseKey: string) => { ... }, [transport]);
  const sendDeactivate = useCallback(() => { ... }, [transport]);
  const sendValidate = useCallback(() => { ... }, [transport]);
  if (!transport) return null;
  return { sendActivate, sendDeactivate, sendValidate };
}
```

### 5.7 BridgeStatus Type Extension

The `BridgeStatus` union type is not modified — licensing state is separate from bridge state. Components use `useLicenseStatus()` independently of `useBridgeStatus()`.

### 5.8 Native Side: HostBridge Extension

Add to `HostBridge.h` (pattern follows existing event identifiers):

```cpp
static const juce::Identifier licenseStatusEvent;
static const juce::Identifier licenseActivateEvent;
static const juce::Identifier licenseDeactivateEvent;
static const juce::Identifier licenseValidateEvent;

static juce::var makeLicenseStatusPayload (const LicensingState& state);
```

**Payload construction** mirrors the TypeScript schema with same bounded string limits.

---

## 6. Color & Typography

### 6.1 Color Contract

All colors reference the existing `lumaScopeTheme` palette. No new palette tokens are added — the existing `status`, `success`, `warning`, `error`, `info`, and `primary` tokens cover all licensing states.

| Token | Hex | Usage | Element |
|-------|-----|-------|---------|
| `success.main` | `#B7F36B` | Activated/ready | Chip border+text, success Alert |
| `warning.main` | `#FFC857` | Offline grace, service unavailable | Chip border+text, warning Alert |
| `error.main` | `#FF6B78` | Revoked, corrupt, revalidation required, destructive actions | Chip border+text, error Alert, Deactivate button |
| `info.main` | `#53D8FB` | Transition states (activating, deactivating) | Chip border+text, progress indicator |
| `default` (grey) | MUI default chip grey | Not activated | Chip border+text |
| `text.secondary` | `#91A4AE` | Dialog body copy, tooltip text | Typography |
| `text.primary` | `#F2F7F8` | Dialog titles, button labels, active chip label | Typography |
| `background.paper` | `#121A20` | Dialog surface, Chip background (variant="outlined") | Dialog, Chip |
| `background.default` | `#0B0F12` | Main app background | AppShell |
| `raised` | `#172129` | Chip hover background | Chip hover |
| `border` | `#24323A` | Dialog border, footer border | Border tokens |

**60/30/10 split:** Licensing does not alter the existing color distribution. The Chip consumes negligible area. Dialogs are transient overlays.

### 6.2 Typography Contract

All sizes reference existing theme tokens:

| Token | Size | Weight | Line-Height | Usage |
|-------|------|--------|-------------|-------|
| `h1` | 28px | 650 | 1.15 | (Not used in licensing — reserved for brand) |
| `h2` | 20px | 600 | 1.25 | Dialog titles |
| `body1` | 14px | 400 | 1.5 | (Not used in licensing) |
| `body2` | 14px | 400 | 1.5 | Dialog body copy, Alert text, footer copy |
| `subtitle2` | 12px | 600 | 1.35 | Chip label, tooltip text |
| `button` | 14px | 600 | — | Button labels (MUI default, inherited) |

**Monospace (error diagnostics):** `fontFamily: '"Cascadia Mono", Consolas, monospace', fontSize: 12` — used for diagnostic code display if needed (follows existing `BridgeStatus` pattern).

### 6.3 Spacing Contract

All spacing uses the existing 4px-base scale (`theme.spacing(4)` = 16px):

| Token | Value | Usage |
|-------|-------|-------|
| `spacing(1)` | 4px | Internal chip padding |
| `spacing(2)` | 8px | Gap between footer elements, icon margins |
| `spacing(4)` | 16px | Dialog content gap between elements |
| `spacing(6)` | 24px | Dialog content padding |
| `spacing(8)` | 32px | Alert horizontal margin |

**Touch targets:** All interactive elements (Chip, buttons) meet minimum 24px height. Dialog buttons are 36px min-height (existing MuiButton override).

---

## 7. Accessibility Requirements

### 7.1 Roles and Live Regions

| Element | Role | ARIA Attribute | Notes |
|---------|------|---------------|-------|
| ActivationStatus Chip | `status` | `aria-live="polite"` | Announces state changes |
| GraceWarnAlert (warning) | `status` | `aria-live="polite"` | Non-urgent warning |
| GraceWarnAlert (urgent/critical) | `alert` | `aria-live="assertive"` | Requires attention |
| ActivationDialog | `dialog` | `aria-modal="true"`, `aria-labelledby` (title ID) | MUI Dialog default |
| DeactivationDialog | `dialog` | `aria-modal="true"`, `aria-labelledby` | MUI Dialog default |
| LinearProgress | `progressbar` | `aria-label="Activating"` | Indeterminate |

### 7.2 Keyboard Navigation

| Action | Key | Scope |
|--------|-----|-------|
| Activate license | Enter (when TextField focused) | ActivationDialog |
| Cancel/close dialog | Escape | ActivationDialog, DeactivationDialog |
| Submit deactivation | Enter (when Deactivate button focused) | DeactivationDialog |
| Open dialog from Chip | Enter or Space (when Chip focused) | StatusFooter |
| Dismiss grace alert | Escape or click close button | GraceWarnAlert |
| Tab through dialog | Tab / Shift+Tab | Both dialogs — follows expected order |

### 7.3 Focus Management

- Dialog opens: Focus moves to first focusable element (TextField in ActivationDialog, "Deactivate" button in DeactivationDialog)
- Dialog closes: Focus returns to the Chip that triggered it
- Error state in dialog: Focus remains on the error Alert or moves to the "Try again" button
- Grace alert appears: Does NOT steal focus (uses `aria-live` not `assertive` for warning level)

### 7.4 Color Contrast

All existing palette tokens meet WCAG 2.1 AA contrast ratios in dark mode:
- `text.primary` (#F2F7F8) on `background.paper` (#121A20): >7:1
- `text.secondary` (#91A4AE) on `background.paper` (#121A20): >4.5:1
- `success.main` (#B7F36B) on `background.paper` (#121A20): >4.5:1
- `warning.main` (#FFC857) on `background.paper` (#121A20): >4.5:1
- `error.main` (#FF6B78) on `background.paper` (#121A20): >4.5:1

### 7.5 Screen Reader Announcements

| Event | Announcement |
|-------|-------------|
| State changes to `activated` | "Licensing status: activated" |
| State changes to `offline_grace` | "Licensing status: offline. {X} days remaining." |
| State changes to `revalidation_required` | "Licensing: reactivation required. Offline period has expired." |
| State changes to `revoked` | "Licensing: license has been revoked." |
| Activation succeeds | "Activation successful. LumaScope is now licensed." |
| Deactivation succeeds | "Deactivation successful. This machine is no longer activated." |
| Activation fails | "Activation failed. {reason}" |

### 7.6 Windows High Contrast Mode

The `forced-colors: active` media query already overrides focus outlines with `Highlight` color. Dialog borders and Chip outlines use the existing `border` token which respects forced-colors by default in MUI.

---

## 8. Responsive Behavior

### 8.1 Breakpoints

| Breakpoint | Width | Behavior |
|-----------|-------|----------|
| Mobile | < 639px | Reduced padding: `px: 3` on footer and dialogs. Chip label truncated if needed. Grace Alert margin reduced to 12px. |
| Tablet | 640-1023px | Standard padding: `px: 5`. Dialog `maxWidth="sm"` at 600px. |
| Desktop | ≥ 1024px | Full layout. Dialog remains `maxWidth="sm"` (does not grow). |

### 8.2 Footer Adaptation

At widths < 400px:
- The Chip may share a row with the brand name if bridge status text is long
- The brand name "Signal Foundry Audio" already hides at 719px `@media (max-width:719px)`
- At very narrow widths (<360px), the Chip label may truncate with ellipsis

### 8.3 Dialog Adaptation

Both ActivationDialog and DeactivationDialog use MUI `Dialog` with `fullWidth` and `maxWidth="sm"`. On narrow viewports:
- Dialog uses full viewport width with 16px side margins
- `DialogActions` stack vertically if buttons cannot fit side by side
- TextField and content maintain 14px font size (no scaling down)

### 8.4 Grace Alert Adaptation

- At <639px: Alert margin `mx: 1.5` (12px), Alert internal padding reduced
- Alert text wraps naturally within available width
- Dismiss button remains visible at all widths

---

## 9. Error Code to User Message Mapping

### 9.1 Server Error Codes (from Phase 5 activation API)

| Code | User Message | Severity |
|------|-------------|----------|
| `invalid_license_key` | "The license key you entered is not valid. Please check and try again." | error |
| `license_already_activated` | "This license is already activated on another machine. Deactivate that machine first, or contact support." | warning |
| `license_expired` | "This license has expired." | error |
| `license_revoked` | "This license has been revoked." | error |
| `activation_not_found` | "No activation found for this license. Please activate again." | error |
| `machine_count_exceeded` | "This license is at its activation limit. Deactivate another machine first." | warning |
| `rate_limited` | "Too many requests. Please wait a moment and try again." | warning |
| `internal_error` | "The activation service encountered an error. Please try again later." | error |
| `maintenance_mode` | "Activation service is undergoing maintenance. Please try again later." | warning |

### 9.2 Native Error Codes

| Code | User Message | Severity |
|------|-------------|----------|
| `network_error` | "Unable to reach the activation service. Check your internet connection and try again." | warning |
| `dns_lookup_failed` | "Activation service could not be found. Check your internet connection." | warning |
| `tls_error` | "Secure connection to the activation service failed. Your network may be blocking it." | warning |
| `timeout` | "The activation service did not respond in time. Please try again." | warning |
| `storage_error` | "Could not save license data. Try restarting the application." | error |
| `signature_verification_failed` | "The license file could not be verified. Please activate again." | error |
| `corrupt_entitlement` | "The license file on this machine is damaged. Please activate again." | error |
| `unknown_key_id` | "The license was signed with a key not recognized by this version. Please update." | error |
| `clock_rollback_detected` | "System clock change detected. Please connect to the internet to verify your license." | warning |
| `machine_mismatch` | "Activation data does not match this machine. Please activate again." | error |
| `activation_required` | "This license requires online activation." | error (revalidation_required) |
| `deactivation_failed` | "Deactivation failed. The activation service could not be reached. Your license remains active on this machine." | warning |
| `deactivation_server_error` | "Deactivation could not be completed due to a server error. Please try again later." | warning |

### 9.3 UI Display Rules

1. **Error Alerts** in ActivationDialog show the user message from the mapping above. The raw error `code` is never shown to the user except in the `copy diagnostics` pattern for support.

2. **The Chip Tooltip** for error states shows the user message (truncated to 120 chars).

3. **Support diagnostics:** For all error states, holding Shift+Click on the Chip copies the raw error code and message to clipboard (following the existing `StatusFooter` copy diagnostics pattern).

4. **Unknown errors:** If an error code is not in the mapping, display: "An unexpected error occurred. Code: {unknown_code}. Please contact support if this persists."

---

## 10. Success Criteria Checklist

This section defines the acceptance checks for the UI-SPEC design contract. Each item must be verifiable after implementation.

### Layout & Integration

- [ ] ActivationStatus Chip renders in StatusFooter between bridge status text and brand name
- [ ] Chip occupies no more than 200px width at any state
- [ ] Chip does not wrap to a new line in the footer at any supported viewport width
- [ ] Footer total height remains ≤ 36px with Chip present (existing footer is 32px — slight increase acceptable)
- [ ] Dialog opens as MUI Dialog overlay, does not modify grid layout
- [ ] GraceWarnAlert renders between source strip and analyzer stage (or header and stage when source strip hidden)
- [ ] GraceWarnAlert auto-dismisses after 10s or on user click

### States & Transitions

- [ ] All 9 licensing states render correct Chip color, icon, and label per Section 4.1 table
- [ ] State transition from `activating` to `activated` shows the Chip changing from blue/Hourglass to green/Verified with a smooth visual update
- [ ] State transition from `activated` to `offline_grace` shows Chip green→amber transition
- [ ] State transition from `offline_grace` to `revalidation_required` shows Chip amber→red transition
- [ ] `activating` and `deactivating` states have Chip disabled (no onClick)
- [ ] All non-transition states have Chip clickable (onClick triggers appropriate dialog or tooltip)

### Dialogs

- [ ] ActivationDialog auto-focuses TextField on open
- [ ] ActivationDialog Enter key submits activation
- [ ] ActivationDialog Escape key closes dialog
- [ ] ActivationDialog shows LinearProgress during activation
- [ ] ActivationDialog shows error Alert with user-friendly message on failure, with Retry/Cancel
- [ ] ActivationDialog shows success Alert with Continue on success
- [ ] DeactivationDialog shows warning message and two clear actions
- [ ] DeactivationDialog confirms both buttons are present: "Keep activated" (outlined) and "Deactivate" (error/contained)
- [ ] DeactivationDialog error state preserves local entitlement (no clearing until server confirms)
- [ ] Both dialogs restore focus to Chip on close

### Grace Warnings

- [ ] No Alert shown when ≥3 days remaining (Chip-only notification)
- [ ] Warning Alert shown when 1-2 days remaining (severity="warning")
- [ ] Urgent Alert shown when <1 day remaining (severity="error")
- [ ] Alert is dismissible via close button
- [ ] Alert reappears on next app launch if still in warning window
- [ ] Alert has correct `role` and `aria-live` per Section 7.1

### Bridge Protocol

- [ ] `license.status` event parsed by `parseLicenseStatus()` correctly
- [ ] `parseLicenseStatus()` rejects malformed payloads with wrong protocolVersion
- [ ] `parseLicenseStatus()` rejects invalid state enum values
- [ ] `parseLicenseStatus()` rejects oversized bounded strings
- [ ] `license.activate` event emitted with correct payload on dialog submission
- [ ] `license.deactivate` event emitted with correct payload on deactivation confirmation
- [ ] `license.validate` event emitted with correct payload on manual validate request
- [ ] BridgeProvider cleans up license event listener on unmount
- [ ] `useLicenseStatus()` returns correct state from context

### Accessibility

- [ ] Chip has `role="status"` with `aria-live="polite"` for state changes
- [ ] GraceWarnAlert has correct role (`status` for warning, `alert` for urgent/critical)
- [ ] Dialogs have `aria-modal="true"` and `aria-labelledby` pointing to title
- [ ] All licensing UI navigable via keyboard alone (Tab, Enter, Escape)
- [ ] Focus returns to Chip after dialog closes
- [ ] Screen reader announces state changes appropriately (Section 7.5)
- [ ] Color contrast meets WCAG 2.1 AA minimum for all text/background combinations
- [ ] High Contrast Mode respected via existing forced-colors media query

### Error Handling

- [ ] Error messages from mapping in Section 9 are displayed — never raw error codes to end users
- [ ] Unknown error codes show generic fallback message with code reference
- [ ] Network errors during activation show appropriate warning (not error) severity
- [ ] Deactivation errors preserve local entitlement (no premature clearing)
- [ ] Shift+Click on Chip copies diagnostic code info to clipboard

### Responsive

- [ ] Chip visible and functional at all viewport widths ≥ 320px
- [ ] Dialogs use `fullWidth` with `maxWidth="sm"` at all viewports
- [ ] Dialog buttons stack vertically at narrow widths (<400px) if needed
- [ ] Grace Alert margins and padding adapt to viewport width (Section 8.4)
- [ ] Brand name hides responsively at 719px (existing behavior preserved)

---

## Appendix: File Modification Summary

| File | Action | Description |
|------|--------|-------------|
| `ui/src/bridge/protocol.ts` | EXTEND | Add licensing event constants, `LicensingStateEnum`, payload interfaces, `parseLicenseStatus()` |
| `ui/src/bridge/BridgeProvider.tsx` | EXTEND | Add `license.status` listener, `LicenseContext`, `useLicenseStatus()` hook, `useLicenseRequest()` hook |
| `ui/src/components/ActivationStatus.tsx` | CREATE | Chip component with all 9 state renderings |
| `ui/src/components/ActivationDialog.tsx` | CREATE | License key entry dialog with all states |
| `ui/src/components/DeactivationDialog.tsx` | CREATE | Deactivation confirmation dialog |
| `ui/src/components/GraceWarnAlert.tsx` | CREATE | Grace expiry warning Alert |
| `ui/src/components/StatusFooter.tsx` | EXTEND | Add `licensing` prop, render ActivationStatus Chip |
| `ui/src/app/AppShell.tsx` | EXTEND | Add licensing state management, GraceWarnAlert, dialog mount points |
| `ui/src/hooks/useLicensing.ts` | CREATE (or inline in AppShell) | Licensing state mapping and action handlers |
| `ui/src/app/theme.ts` | NO CHANGE | All needed palette tokens exist |
| `plugin/include/LumaScope/HostBridge.h` | EXTEND | Add license event IDs and `makeLicenseStatusPayload()` |
| `plugin/include/LumaScope/HostBridge.cpp` | EXTEND | Add event ID definitions and payload construction |
| `docs/bridge-protocol.md` | EXTEND | Document licensing events (separate task) |
