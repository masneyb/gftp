# REQUIREMENTS

Purpose:  
Define **what must be true** for the task to be considered complete.

**Authoritative Location**: `{task_dir}/REQUIREMENTS.md`

This document is:
- Declarative
- Test-oriented
- Solution-agnostic

It describes **outcomes and constraints**, not implementation.
If *how* something is built is described, it does not belong here.

---

## Functional Requirements

List externally observable behaviors the system MUST exhibit.

Each requirement must:
- Be specific and unambiguous
- Be testable by observation or inspection
- Describe *what*, not *how*

Format:
- FR-1:
- FR-2:
- FR-3:

---

## Non-Functional Requirements

Constraints on system behavior or qualities.

These describe *how the system behaves*, not *how it is built*.

Common categories include:
- Performance
- Reliability
- Compatibility
- Security
- Accessibility
- Maintainability

Format:
- NFR-1:
- NFR-2:

---

## Acceptance Criteria Mapping

Define how each requirement will be verified.

This section links **requirements → verification**, not test implementation.

For each requirement:
- Requirement ID:
    - Verification method: Automated / Manual / Inspection
    - Success condition: What must be true for this to pass

---

## Explicit Non-Requirements

List behaviors, changes, or outcomes that are **explicitly out of scope**.

This section exists to prevent:
- Scope creep
- Assumed features
- “While we’re here” additions

Examples:
- Not doing:
- Not changing:
- Not supporting:

---

## Assumptions

List assumptions that must hold true for these requirements to be valid.

Each assumption should include:
- Assumption:
- Impact if false:

Assumptions are **not guarantees** and may require validation.

---

## Open Questions

Requirement-level questions that remain unresolved.

These must be answered **before or during implementation**.
If unanswered, they may block execution or acceptance.

For each question:
- Question:
- Owner:
- Needed by:


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
