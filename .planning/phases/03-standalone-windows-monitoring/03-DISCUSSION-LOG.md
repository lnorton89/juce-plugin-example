# Phase 3: Standalone Windows Monitoring - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-23
**Phase:** 3-Standalone Windows Monitoring
**Areas discussed:** Source model, recovery behavior, silence state, UI surface, persistence/defaults

---

## Source Model

| Option | Description | Selected |
|--------|-------------|----------|
| Two clear modes: `Input Device` and `System Output` | Keeps microphone/interface capture separate from WASAPI loopback; easier to explain and test. | ✓ |
| One combined source list | Simpler UI, but can blur recording input vs speaker/output capture. | |
| Advanced tabs/details | More explicit, but too heavy for this compact analyzer shell. | |

**User's choice:** 1 — two clear modes.
**Notes:** This locks a two-stage mental model: source mode first, concrete device/endpoint second.

---

## Recovery Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-fallback to default source with visible warning | Keeps the analyzer useful, but may unexpectedly capture a different source. | |
| Stop capture until user chooses again | Safer and explicit; avoids surprising source changes during Windows device churn. | ✓ |
| Retry same source silently for a short time, then show error | Less disruptive, but can feel mysterious. | |

**User's choice:** 2 — stop capture until user chooses again.
**Notes:** The implementation should not silently switch to another input/output device after failure.

---

## Silence State

| Option | Description | Selected |
|--------|-------------|----------|
| Keep analyzer running with subtle “No signal detected” status | Distinguishes silence from failure without alarming the user. | ✓ |
| Show the same empty analyzer with no extra message | Cleanest visually, but ambiguous. | |
| Use a stronger warning after a timeout | Clearer, but could be noisy when silence is intentional. | |

**User's choice:** 1 — keep capture active and show a subtle no-signal status.
**Notes:** Silence is valid source state, not a failure state.

---

## UI Surface

| Option | Description | Selected |
|--------|-------------|----------|
| Compact control strip above the analyzer stage | Obvious, close to the spectrum, but does not disturb the header/footer. | ✓ |
| Header button/dropdown | Saves vertical space, but source state is less discoverable. | |
| Settings dialog only | Cleanest main view, but too hidden for a monitoring app. | |

**User's choice:** 1 — compact control strip above the analyzer.
**Notes:** The strip should remain standalone-only and avoid becoming a full mixer or settings dashboard.

---

## Persistence and Defaults

| Option | Description | Selected |
|--------|-------------|----------|
| Restore last valid source if available; otherwise start stopped with “Choose source” state | Predictable and avoids surprise capture. | ✓ |
| Always start stopped and require a source selection | Most explicit, but repetitive. | |
| Auto-select default input/output based on last mode | Convenient, but may start capturing something unexpected. | |

**User's choice:** 1 — restore last valid source when available, otherwise start stopped.
**Notes:** Missing saved sources should not trigger automatic default-source capture.

---

## the agent's Discretion

- Exact native class boundaries for shared source lifecycle, JUCE input capture, and WASAPI loopback.
- Exact persisted source identifier format and matching strategy.
- Exact source-state wording/visual severity, as long as selected failure, stopped/no-source, and valid-but-silent are distinct.

## Deferred Ideas

None.
