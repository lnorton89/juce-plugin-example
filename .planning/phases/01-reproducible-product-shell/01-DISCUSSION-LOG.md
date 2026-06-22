# Phase 1: Reproducible Product Shell - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-22
**Phase:** 1-Reproducible Product Shell
**Areas discussed:** Product identity, Windows baseline, Visual shell, Developer workflow

---

## Product Identity

| Option | Description | Selected |
|--------|-------------|----------|
| User-supplied identity | User provides final product, company, and JUCE identifiers | |
| Agent-created identity | Create a coherent fictitious example brand and valid four-character codes | ✓ |

**User's choice:** “Make up one.”
**Notes:** Selected LumaScope by Signal Foundry Audio with codes `SgFd` and `LmSc`.

---

## Windows Baseline

| Option | Description | Selected |
|--------|-------------|----------|
| Match current machine | Use the detected Windows/toolchain/runtime as the first verified baseline | ✓ |
| Choose a broader baseline | Select a different minimum independently of the development machine | |

**User's choice:** “Whatever this machine has.”
**Notes:** Detected Windows 10 Pro 22H2 x64 build 19045, Visual Studio 2019 16.11, and Evergreen WebView2 runtime 149.0.4022.80.

---

## Visual Shell

| Option | Description | Selected |
|--------|-------------|----------|
| User-directed theme | User specifies visual references and layout | |
| Agent best judgment | Agent chooses a coherent production-minded audio-tool direction | ✓ |

**User's choice:** “Use best judgement.”
**Notes:** Locked a dark instrument-panel theme with restrained cyan/lime accents, compact header, central stage, and quiet footer.

---

## Developer Workflow

| Option | Description | Selected |
|--------|-------------|----------|
| User-directed workflow | User specifies package manager, scripts, presets, and hot-reload behavior | |
| Agent best judgment | Agent chooses a reproducible workflow suited to the detected machine | ✓ |

**User's choice:** “Use best judgement.”
**Notes:** npm lockfile, explicit CMake presets, a PowerShell environment check/bootstrap, embedded assets by default, and opt-in Vite hot reload.

## Agent's Discretion

- Detailed visual tokens and placeholder artwork.
- Script, package-command, preset, and folder names.
- Dependency-fetch and typed bridge implementation details.
- Exact diagnostic copy and fallback presentation.

## Deferred Ideas

None.
