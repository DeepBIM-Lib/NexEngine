# Project Truths & Invariants

This document defines the core architecture, immutable facts, and reasoning boundaries of NexEngine.
For coding styles and workflows, see `ENGINEERING.md` / `PROJECT_RULES.md`. For future plans, see `ROADMAP.md`.

---

## 1. Core Positioning & Architecture

* **Engine Identity**: Real-time rendering engine using Flecs ECS for scene graph management and Diligent Engine for graphics dispatch. Built for general software/game rendering and **BIM rendering**.
* **Target Platform**: Windows (D3D11, D3D12, Vulkan).
* **Backend Decoupling & Testing Boundary**:
    * **Development (Backend-Neutral)**: Public data models and core abstractions MUST remain strictly backend-agnostic. NEVER pollute public interfaces with single-API quirks or temporary smoke workarounds.
    * **Testing (D3D11 Anchor)**: Automated test suites and regression testing MUST focus strictly and exclusively on **D3D11**. Trust Diligent Engine for cross-backend abstraction; NEVER author parallel D3D12 or Vulkan tests.

---

## 2. Non-Negotiable Architectural Constraints

* **Dependency & API Rules**: Strictly inherit constraints from `PROJECT_RULES.md` (zero new external dependencies, Flecs C++ API only, `NexEngine Math` for public interfaces, `DirectXMath` restricted to internal performance hot paths, `DiligentMath` strictly prohibited).
* **Test Suite Purity**: Scratchpad test harnesses, temporary debug probes, and diagnostic tools MUST NOT be committed to the master branch. The `tests/` directory on `master` ONLY accepts plan-approved reusable regressions.

---

## 3. Evidence Hierarchy & Anti-Hallucination Boundaries

When reasoning about codebase state, existing APIs, or architectural intent, the AI MUST NOT treat the following as ground truth:

1. **Uncommitted Context**: Ephemeral chat discussions or agreements not persisted to disk.
2. **Roadmap Items**: Unimplemented concepts or tentative directions in `ROADMAP.md`.
3. **Historical Artifacts**: Behaviors observed in rolled-back branches or outdated builds.
4. **Commercial Engine Preconceptions (Design vs. Reality)**:
    * **Design & Ideation [ENCOURAGED]**: Actively study, reference, and borrow proven architectural patterns from mainstream engines (e.g., Unreal Engine, Unity) when brainstorming solutions.
    * **Code Inspection & Reasoning [STRICTLY FORBIDDEN]**: NEVER infer or assume NexEngine possesses APIs, reflection systems, lifecycle hooks, or components merely because they exist in Unreal or Unity. Base all code analysis strictly on the actual files in this repository.