# Engineering Rules & AI Instructions

You MUST strictly comply with the following rules when generating or modifying code in this workspace.

---

## 1. Workflow & Gatekeeping

* **Context Audit**: Always inspect adjacent code, corresponding headers, and dependency chains before making changes. Never design in isolation.
* **Planning**: For non-trivial modifications, explicitly define risk level, design slices, and test matrices first.
* **Public Interface Gate**: Any additions, deletions, or modifications to `public API`, `struct`, or `Component` MUST be explicitly declared and reviewed before code generation.
* **Merge Policy**: Merges require all checks to pass and maintainer approval.

---

## 2. API, ECS & Dependencies

* **Flecs ECS**:
    * Use the C++ API exclusively.
    * Register components, observers, and systems via **chained calls** in one statement; NEVER store raw entities to append metadata later.
    * C API is strictly prohibited unless the C++ API cannot satisfy requirements, the boundary is isolated, and approval is granted.
    * **Atomicity**: Each Tag/Component must express exactly one fact. NEVER create mirrored "god components" with high sync overhead.
* **Third-Party Dependencies**: DO NOT introduce new libraries. Strictly reuse dependencies locked in `externals/`.
* **Math Libraries**:
    * NEVER include or use `DiligentMath`.
    * Public interfaces and general math types MUST use `NexEngine Math`.
    * `DirectXMath` is restricted to internal performance-critical implementations (e.g., matrix cascades, batching) and MUST NOT leak into public headers.
* **Resource Architecture**:
    * The resource subsystem may hold render device/context handles.
    * **Cross-references between resource managers are strictly prohibited.** Pass required dependencies explicitly via function parameters.

---

## 3. Code Style & Formatting

* **Naming Conventions**: Use `snake_case` for variables. Private member variables MUST have a trailing underscore (e.g., `member_`).
* **Struct Layout**:
    * `public`: Data members FIRST, member functions SECOND.
    * `private`: Member functions FIRST, data members SECOND.
  ```cpp
  struct surface_descriptor {
  public:
      uint32_t width = 0;
      uint32_t height = 0;

      bool is_valid() const;

  private:
      void sanitize_internal();

      uint32_t internal_flags_ = 0;
  };
  ```
* **Types & Lifecycle**:
    * DO NOT nest `struct` declarations inside another struct/class. Place them at header root scope and isolate conflicts using namespaces.
    * Prefer POD / Aggregate Initialization for pure data types. Avoid non-trivial hidden lifecycles.
* **Formatting & Comments**:
    * Multi-line function parameters MUST use the compact layout (first argument immediately after `(`, trailing arguments indented, closing `)` on the final line):
  ```cpp
  void execute_pass(render_context& ctx,
      const render_target& target );
  ```
    * Group `#include` directives and struct members by semantics; maintain stable ordering without arbitrary reshuffling.
    * Comments MUST only explain: ownership transfer (move/borrow), invariants, and counter-intuitive performance trade-offs. NEVER restate obvious syntax.