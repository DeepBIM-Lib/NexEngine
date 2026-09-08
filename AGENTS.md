# NexEngine Agent Protocol

Repository-wide agent rules. Nested instructions may extend but not weaken this document.

---

## 1. Authority & Reading Order

* **Ownership**: `Multica` owns workflow, squads, and agent interaction; this repository owns engineering facts and code invariants.
* **Document Hierarchy**:
    1. `docs/PROJECT.md` (Architectural truths and anti-hallucination boundaries)
    2. `docs/ENGINEERING.md` (Code style, API constraints, and implementation rules)
    3. `docs/ROADMAP.md` (Milestones and approved directions)

---

## 2. Repository Gates & Redlines

* **Hard Redline**: Agents MUST NEVER merge pull requests or cancel workflows.
* **Branch Isolation**: Work strictly on isolated branches named `agent/<ISSUE>/delivery`.
* **Re-planning Trigger**: Replan ONLY when scope, public API contract, dependencies, or architecture changes. Same-scope bug fixes stay within the current revision.
* **Validation Standards**:
    * Build and run validation from a clean directory under `build/` targeting **D3D11**.
    * Zero executed tests or missing infrastructure is an automatic failure.
* **PR Delivery**: Open a Draft PR only after all checks pass. PR head SHA, delivery SHA, and evidence SHA must match.

---

## 3. Storage & Test Hygiene

* **Build Artifacts**: All build outputs must reside strictly under the ignored `build/` directory. NEVER commit build artifacts.
* **Probe Purging**: Delete all temporary debug probes, diagnostic harnesses, and scratchpad tests before delivery. Production branches keep only plan-approved reusable regressions.