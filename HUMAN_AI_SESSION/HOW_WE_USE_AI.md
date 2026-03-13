# How We Use AI

This document defines the **human discipline** required to use AI tools safely and deterministically in this repository.

It does not describe agent internals or workflow mechanics.
Those are defined elsewhere and referenced here.

---

## Core Rule

AI tools assist only within **explicit, written contracts**.

If something is unclear, missing, or not written down:
**stop and fix the documentation first.**

---

## Before Any Implementation

Humans must ensure that:

- A `TASK_PACKET.md` exists for the work
- The packet clearly defines:
    - Scope
    - Files that may be touched
    - Non-goals
- The packet has been reviewed and approved by a human
- The task lifecycle state is **Active** (as defined in workflow state rules)

If any of these are not true, implementation must not begin.

---

## During AI-Assisted Work

Humans must:

- Provide AI tools with the relevant task documentation
- Reject outputs that rely on guessing or inferred requirements
- Stop the session if instructions become unclear or contradictory

Humans must not:
- Ask AI tools to “fill in the gaps”
- Allow scope expansion without a new or updated packet
- Treat chat output as authoritative

Files are the system. Chat is not.

---

## Session Discipline

Every AI session must begin with the appropriate session opening instructions.

If a session is started without them, the results should be treated as non-authoritative and reviewed carefully.

(Session entrypoint details live under `INSTRUCTIONS/`.)

---

## Child Work Discovery

When new work is discovered:

- Planned decomposition becomes a **Subtask**
- Corrective or defect-related work becomes a **BugTask**

Humans are responsible for ensuring follow-up work is classified correctly before execution continues.

Execution should not continue until the correct child work exists.

---

## When to Stop

Stop the session and update documentation if:

- Requirements are ambiguous
- Scope appears to change
- The AI proposes work outside the packet
- You are unsure whether something is a subtask or a bugtask

Stopping early is a feature, not a failure.

---

## Guiding Principle

AI accelerates thinking and execution.  
Humans remain responsible for correctness, scope, and approval.

For procedural steps, see `RUN_BOOK.md`.
