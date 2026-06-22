---
phase: 1
slug: reproducible-product-shell
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-22
---

# Phase 1 — UI Design Contract

> Visual and interaction contract for the LumaScope product shell. Phase 1 establishes the durable frame that later analyzer, source, and activation experiences extend.

## Experience Intent

LumaScope should feel like a precise desktop audio instrument: dark, quiet, legible, and confident. The shell frames the future spectrum rather than competing with it. Avoid generic dashboard cards, marketing-page gradients, glassmorphism, faux hardware knobs, and stock Material UI demo styling.

Phase 1 is honest about its scope. It shows a polished analyzer-stage placeholder and bridge/runtime status, but it does not fake live spectrum data, source controls, analyzer settings, or activation controls.

## Design System

| Property | Value |
|----------|-------|
| Tool | Material UI theme (`ThemeProvider`, `CssBaseline`, CSS theme variables where supported) |
| Preset | Custom LumaScope dark theme; no stock preset |
| Component library | Material UI, imported through path-based package exports |
| Icon library | `@mui/icons-material`, path imports only; decorative icons hidden from assistive technology |
| Font | `"Segoe UI Variable", "Segoe UI", system-ui, sans-serif`; no network font dependency |
| Graph surface | Semantic placeholder in Phase 1; Canvas/WebGL renderer attaches here in Phase 2 |
| Styling | MUI `sx` and theme tokens; no scattered raw color/spacing literals in components |

## Layout Contract

### Window and Regions

- Default editor/app content size: **960 × 600 px**.
- Supported minimum content size: **560 × 360 px**. Below this, native code may enforce the minimum rather than allowing unusable compression.
- Root fills the WebView viewport and uses a three-row grid: `52px minmax(0, 1fr) 32px`.
- **Header (52 px):** LumaScope wordmark left; restrained product mode label (`VST3` or `Standalone`) right. No settings/source/license controls in Phase 1.
- **Analyzer stage:** one dominant panel with 16 px outer inset, full remaining height, and no nested card grid. It contains a subtle grid, spectral glow motif, status copy, and bridge/runtime state.
- **Status footer (32 px):** connection/build mode status at left and Signal Foundry Audio at right. Keep it visually subordinate.

### Responsive Behavior

- At widths below **720 px**, hide the company footer label before truncating status text.
- At widths below **640 px**, reduce outer inset from 16 px to 12 px and header horizontal padding from 20 px to 12 px.
- Never stack the header into two rows; use short mode labels and ellipsis for unexpected long diagnostic text.
- Height pressure reduces decorative stage copy and glow before shrinking required status text.
- Use CSS layout and MUI breakpoints/media queries; do not read window size on every render when CSS can express the behavior.

## Spacing Scale

All declared spacing is a multiple of 4.

| Token | Value | Usage |
|-------|-------|-------|
| `space-1` | 4px | Icon/label micro-gap, focus offset |
| `space-2` | 8px | Compact inline groups, status items |
| `space-3` | 12px | Compressed-window padding |
| `space-4` | 16px | Default stage inset and component padding |
| `space-5` | 20px | Header horizontal padding |
| `space-6` | 24px | Stage content grouping |
| `space-8` | 32px | Major empty-state separation |
| `space-12` | 48px | Large-screen stage breathing room |
| `space-16` | 64px | Maximum decorative separation only |

**Exceptions:** 1 px borders and grid lines; 2 px focus ring and bridge-status indicator stroke. These are line widths, not layout spacing.

## Shape and Elevation

| Token | Value | Usage |
|-------|-------|-------|
| `radius-sm` | 6px | Compact chips/status capsules only |
| `radius-md` | 10px | Analyzer stage |
| `border-subtle` | 1px solid `#24323A` | Stage and structural separators |
| `shadow-stage` | `0 16px 48px rgba(0,0,0,.32)` | Single dominant stage only |

Do not add shadows to the header/footer or layer multiple raised cards. The stage is the one elevated object.

## Typography

| Role | Size | Weight | Line Height | Letter Spacing | Usage |
|------|------|--------|-------------|----------------|-------|
| Body | 14px | 400 | 1.5 | 0 | Supporting and diagnostic copy |
| Label | 12px | 600 | 1.35 | 0.06em | Uppercase mode/status labels |
| Heading | 20px | 600 | 1.25 | -0.01em | Empty/ready state heading |
| Display | 28px | 650 | 1.15 | -0.025em | LumaScope wordmark text if rendered as type |
| Mono | 12px | 500 | 1.4 | 0 | Protocol/build identifiers only; `Cascadia Mono`, `Consolas`, monospace |

- Use sentence case for headings and actions; uppercase is reserved for short mode/status labels.
- Body copy should remain below roughly 70 characters per line inside the stage.
- Do not load fonts from Google Fonts or any external host; packaged UI must remain fully offline.

## Color

| Role | Value | Usage |
|------|-------|-------|
| Dominant canvas (60%) | `#0B0F12` | Viewport background |
| Secondary surface (30%) | `#121A20` | Analyzer stage and header/footer tonal separation |
| Raised surface | `#172129` | Status capsules and localized hover/focus surfaces |
| Structural border | `#24323A` | Panel edge and separators |
| Primary text | `#F2F7F8` | Headings and essential status |
| Secondary text | `#91A4AE` | Supporting copy and metadata |
| Cyan accent | `#53D8FB` | Wordmark detail, focus ring, connected state, sparse stage glow |
| Lime accent | `#B7F36B` | Ready/success indicator and a small complementary spectral highlight |
| Warning | `#FFC857` | Development-server/runtime warning only |
| Destructive | `#FF6B78` | Unrecoverable error state only |

**Accent is reserved for:** the LumaScope brand mark, keyboard focus ring, bridge connected/ready indicator, and sparse analyzer-stage spectral motif. It is not the default color for every label, border, or clickable surface.

- Primary and secondary text must meet WCAG AA contrast on their intended surfaces.
- Never communicate bridge/runtime state through color alone; pair color with icon and text.
- Do not use large-area cyan/lime gradients. A low-opacity localized radial glow is allowed inside the analyzer stage and must not reduce text contrast.

## Component Contract

### `AppShell`

- Owns viewport grid, theme, error boundary, and mode (`VST3`/`Standalone`) presentation.
- Must render a useful shell while bridge handshake is pending.
- Must not know future analyzer bins, audio devices, or license state schemas.

### `BrandHeader`

- Displays LumaScope and optional minimal prism/sweep mark.
- Mode label is text, not icon-only.
- No navigation drawer, overflow menu, or faux settings button in Phase 1.

### `AnalyzerStage`

- Uses a semantic region labeled “Spectrum display”.
- Provides one stable mount point for the Phase 2 renderer without changing page layout.
- Phase 1 visual is a non-animated subtle log-grid/spectral sweep motif plus honest ready-state copy.
- Decorative grid and glow are hidden from accessibility APIs.

### `BridgeStatus`

- Supports `connecting`, `ready`, `development-server-unavailable`, `protocol-mismatch`, and `runtime-error` presentation.
- Status text is stable and concise; detailed diagnostic IDs may use the mono style.
- Retry action appears only when retry can change the outcome.

### `StatusFooter`

- Shows bridge/build mode (`Embedded UI` or `Vite development`) and concise status.
- No version/update/license actions in Phase 1.

## State and Copywriting Contract

| State/Element | Copy |
|---------------|------|
| Product subtitle | `Spectrum analyzer` |
| Connecting label | `Connecting to audio engine…` |
| Ready heading | `Analyzer surface ready` |
| Ready body | `Live spectrum data will appear here when analysis is enabled.` |
| Embedded status | `Embedded UI · Bridge ready` |
| Development status | `Vite development · Bridge ready` |
| Development server error | `Development server unavailable. Start the UI dev task or switch to embedded assets.` |
| Protocol mismatch | `Interface version mismatch. Rebuild the native app and embedded UI from the same checkout.` |
| Runtime error | `The interface could not start. Check the WebView2 runtime and reopen LumaScope.` |
| Primary recovery CTA | `Retry connection` |
| Non-retry diagnostic CTA | `Copy diagnostics` |

- Error copy states the problem and the next action; never show “Something went wrong” alone.
- Do not call the Phase 1 stage “live” or show fabricated Hz/dB values.
- Destructive confirmation is not applicable in Phase 1.

## Interaction and Motion

- Keyboard focus uses a 2 px cyan ring with 2 px offset and remains visible against every surface.
- Interactive hit targets are at least 32 × 32 px in the compact desktop shell; recovery buttons use at least 36 px height.
- Normal shell transitions are 120–180 ms with standard ease-out. Only opacity/color/transform may animate.
- The spectral motif may use a very slow, low-amplitude 8–12 second drift only when motion is allowed; it must stop under `prefers-reduced-motion: reduce` and when the page is not visible.
- Never animate layout, rapidly pulse status indicators, or create continuous React state updates for decoration.
- Focus order: header (only if it contains a control), stage recovery action, footer diagnostic action. Decorative content is not focusable.

## Accessibility Contract

- Root language is English and the document has one meaningful page/app title.
- Use semantic landmarks/regions; do not recreate buttons with `div` elements.
- Mode, bridge status, and errors have visible text. Icons supplement rather than replace labels.
- Status changes use a polite live region; repeated connecting ticks must not spam announcements.
- Errors that require action move focus to the error heading only after a user-triggered retry/open, not during background startup.
- `Copy diagnostics` confirms success through visible text and a polite announcement.
- Test keyboard-only traversal at default and minimum window size plus Windows high-contrast/forced-colors behavior.

## Asset and Offline Contract

- All fonts, icons, SVG/Canvas assets, JavaScript, and CSS ship inside the embedded frontend bundle.
- No runtime requests to CDNs, analytics, Google Fonts, or remote image hosts.
- Development-server mode is opt-in and visibly labeled; production/embedded mode never silently falls back to a remote URL.
- MUI and icon imports use path-based imports to keep the embedded payload bounded.
- The native shell owns the last-resort WebView/runtime failure presentation when React cannot initialize; its copy should match this contract as closely as native controls allow.

## Design Tokens and Implementation Guardrails

- Define the palette, typography, spacing, radius, and elevation in one MUI theme module with CSS variables enabled where supported.
- Components consume semantic tokens (`background.default`, `background.paper`, `text.secondary`, custom status tokens) rather than hex values.
- Allow raw color literals only inside the theme/token definition and focused visual tests.
- Keep bridge state behind a typed adapter/provider so browser tests can supply deterministic states without JUCE.
- No third-party component registry or copied UI block is approved for this phase.

## Visual Verification Matrix

| Case | Expected Result |
|------|-----------------|
| 960 × 600 embedded | Full header, stage, footer; balanced negative space |
| 560 × 360 embedded | No overlap/scrollbars; company footer label may hide; essential status remains |
| 150% and 200% Windows scaling | Crisp text/icons, no clipped controls, stable stage geometry |
| Keyboard only | Visible focus, logical order, all recovery actions operable |
| Reduced motion | Decorative drift disabled; no loss of information |
| Bridge connecting/ready | Honest state copy; no fake data |
| Dev server unavailable | Actionable warning and route back to embedded assets |
| Protocol mismatch | Explicit rebuild guidance and diagnostic identifier |
| Forced colors/high contrast | Essential borders, text, focus, and statuses remain distinguishable |
| React startup failure | Native fallback explains WebView/runtime/assets next steps |

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | none | not applicable |
| Third-party registries | none | prohibited unless this UI-SPEC is revised and reviewed |

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-22
