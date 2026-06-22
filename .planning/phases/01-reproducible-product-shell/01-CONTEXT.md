# Phase 1: Reproducible Product Shell - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 1 delivers a reproducible Windows build shell for VST3 and standalone targets, with the same packaged React/TypeScript/Material UI interface hosted through JUCE WebView2. It establishes dependency pinning, CMake presets, frontend development and embedded-release workflows, a typed/versioned native-web bridge handshake, and project Context7 guidance. Spectrum DSP, standalone audio capture, and licensing behavior remain in later phases.

</domain>

<decisions>
## Implementation Decisions

### Product Identity
- **D-01:** The product name is **LumaScope**.
- **D-02:** The fictitious company name is **Signal Foundry Audio**.
- **D-03:** Use `LumaScope` as the primary CMake/project target identity, `SgFd` as the four-character JUCE manufacturer code, and `LmSc` as the four-character plug-in code.
- **D-04:** Treat this identity as a coherent example brand throughout source, UI, artifacts, and documentation; do not leave tutorial names or generic placeholders in user-facing output.

### Windows and Toolchain Baseline
- **D-05:** Match the current development machine as the initial verified baseline: Windows 10 Pro 22H2 x64, build 19045.
- **D-06:** Support the installed Visual Studio Community 2019 16.11 C++ toolchain in Phase 1; do not require Visual Studio 2022 merely because it is newer.
- **D-07:** Use the Evergreen WebView2 Runtime model. The machine currently has WebView2 `149.0.4022.80`; build and diagnostics must not hard-code that exact runtime version.
- **D-08:** The Phase 1 setup check should report the detected Windows, compiler, CMake, Node/npm, Ninja, and WebView2 environment and give actionable messages for missing prerequisites.

### Visual Shell
- **D-09:** Use a polished dark instrument-panel theme rather than a stock Material UI demo appearance.
- **D-10:** Use restrained cyan and lime spectral accents against near-black/charcoal surfaces, with high contrast and a professional audio-tool feel.
- **D-11:** The initial shell should have a compact branded header, a large central analyzer-stage placeholder, and a quiet status/footer region suitable for later source and activation state.
- **D-12:** Phase 1 should look intentional and production-minded, but it must not invent spectrum controls, source controls, or licensing interactions assigned to later phases.

### Developer Workflow
- **D-13:** Provide one documented PowerShell bootstrap/environment-check entry point plus explicit package and CMake preset commands; the script should explain actions and remain suitable for handoff.
- **D-14:** Use npm with a committed lockfile because Node 22.18.0 and npm 10.9.3 are available on the baseline machine.
- **D-15:** Embedded compiled frontend assets are the default and authoritative behavior for normal Debug/Release native builds.
- **D-16:** Provide an opt-in Vite development-server mode for hot reload. The mode must be explicit, display a useful failure when the server is unavailable, and allow returning to embedded assets without code edits.
- **D-17:** Provide presets compatible with Visual Studio 2019 and the installed Ninja 1.13.1 tooling. Planner may choose exact preset names and whether Ninja requires a developer-shell bootstrap.

### Agent's Discretion
- Exact MUI theme tokens, fonts, spacing, shadows, border radii, and placeholder illustration details, provided they follow the locked visual direction and accessibility requirement.
- Exact PowerShell script names, package-script names, CMake preset names, repository folder layout, and dependency-fetch mechanism.
- Exact typed bridge serialization/code-generation strategy and handshake message names, provided the protocol is versioned, validated, and testable.
- Exact fallback copy and diagnostic presentation for missing WebView2, asset loading, or development-server failures.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Scope and Phase Contract
- `.planning/PROJECT.md` — Product scope, constraints, requested Context7 sources, and key architectural decisions.
- `.planning/REQUIREMENTS.md` — Phase 1 requirements `BUILD-01` through `BUILD-05`, `UI-01`, and `UI-03`.
- `.planning/ROADMAP.md` — Phase 1 goal, success criteria, MVP mode, and planned work breakdown.
- `.planning/STATE.md` — Current project position and baseline concerns.

### Technical Research
- `.planning/research/STACK.md` — Recommended pinned stack, WebView2/CMake setup, tool choices, and version policy.
- `.planning/research/ARCHITECTURE.md` — Recommended repository structure, bridge boundary, embedded assets, and system integration points.
- `.planning/research/PITFALLS.md` — WebView bridge, dependency, asset-packaging, and portability failure modes Phase 1 must prevent.
- `.planning/research/SUMMARY.md` — Roadmap implications and confidence/gap summary.

### Project Guidance
- `AGENTS.md` — GSD workflow rules and generated project guidance.

No phase-specific SPEC or ADR exists; the documents above plus the decisions in this file are canonical.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No implementation source exists yet. The workspace contains only planning artifacts and `AGENTS.md`; Phase 1 is a true greenfield scaffold.
- Research artifacts already contain the recommended high-level repository split and source references; use them as guidance rather than creating a competing structure.

### Established Patterns
- Planning has locked modern CMake, pinned dependencies, JUCE 8, React/TypeScript, Material UI, WebView2, and one Context7 MCP endpoint with explicit library IDs.
- Release assets are embedded and self-contained; hot reload is an explicit developer-only variation.
- Native/web communication is a versioned contract, not ad hoc JavaScript evaluation.

### Integration Points
- Phase 1 creates the root native build, shared VST3/standalone shell, `ui/` frontend, embedded asset target, bridge contract, setup checks, and Context7 project configuration that all later phases extend.
- Phase 2 will attach analyzer snapshots to the bridge and central stage; Phase 3 will add standalone source controls; Phase 6 will add activation states. Leave deliberate extension points without implementing those capabilities now.

</code_context>

<specifics>
## Specific Ideas

- Brand concept: **LumaScope** should evoke illuminated spectral inspection without feeling like a tutorial toy.
- Baseline machine observations: Windows build 19045 x64, WebView2 149.0.4022.80, CMake 4.0.3, Node 22.18.0, npm 10.9.3, Git 2.51.1, Ninja 1.13.1, and Visual Studio 2019 16.11 C++ workload.
- Context7 is one MCP server. Required library IDs remain `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 1-Reproducible Product Shell*
*Context gathered: 2026-06-22*
