# ROUTING_RULES.md

This document defines the **deterministic routing logic** used by AI sessions to decide:
- whether work is allowed,
- which agent may act,
- and what the next valid action is.

Routing MUST be derived ONLY from file-based state.
No routing may be inferred from chat history, tool memory, or assumptions.

Authoritative state definitions live in:
- `WORKFLOW_STATES.md`

Authoritative state transitions live in:
- `WORKFLOW_STATE_MACHINE.md`


---

## Source of Truth

Routing MUST read state from:
- `TASK_PACKET_SUMMARY.md`

If `TASK_PACKET_SUMMARY.md` is missing, routing MUST treat the task as:
- **not entered execution state**, and
- **not safe to resume automatically**.

---

## Required Inputs for Routing

A session MUST determine the following before any work:

1) Active task directory (by repository rule)
2) Presence of `TASK_PACKET_SUMMARY.md`
3) If present, read:
    - `LIFECYCLE STATE`
    - `EXECUTION STATE`

No other signals are allowed to influence routing.

---

## Hard Stop Gates (Non-Negotiable)

Routing MUST STOP work immediately if any of the following are true:

- Lifecycle state is not `Active`
- Execution state is `Completed`
- Required state fields are missing, malformed, or contain invalid values
- The agent attempting to act lacks authority for the state change it would perform
- The session attempts to modify parent lifecycle or execution state
  from within a subtask or bugtask context

---

## Execution Authority Routing (Gemini â†’ Codex Handoff)

Execution-state update authority is determined by whether **any non-documentation source file has been modified**:

- Before code is touched:
    - Gemini may update execution state
- After code is touched:
    - Codex becomes the exclusive authority for execution state updates

If there is uncertainty, routing MUST assume:
- code MAY have been touched,
- therefore Codex is the authority,
- and Gemini must not update execution state.

---

## Routing Outcomes (What the Session May Do)

A routing decision MUST resolve to exactly one of these outcomes:

1) **STOP** (no work allowed)
2) **AWAIT HUMAN** (work blocked pending human decision)
3) **GEMINI PLANNING** (planning/doc updates only)
4) **CODEX BUILD** (implementation only)
5) **REVIEW MODE**
    - Review and verification only
    - No agent may modify implementation files
    - Documentation or summaries may be updated only if they do not alter state

A session MUST NOT perform actions from multiple routing outcomes.



---

## Deterministic Routing Table

Routing is determined by `(Lifecycle, Execution)`:

### Lifecycle â‰  Active
- `Paused` â†’ **AWAIT HUMAN**
- `Abandoned` â†’ **STOP**

### Lifecycle = Active

#### Execution = Completed
- Outcome: **STOP**
- Rule: Completed is terminal. No further work may occur under this task.

#### Execution = Blocked
- Outcome: **AWAIT HUMAN**
- Rule: Explain blocker; await decision to resume or abandon.

#### Execution = Under Review
- Outcome: **REVIEW MODE**
- Rule: Review/validation may proceed. Implementation changes require explicit re-entry to `WIP`.

#### Execution = WIP
- Outcome depends on authority:
    - If code has NOT been touched â†’ **GEMINI PLANNING** (if planning/doc work is needed) OR **CODEX BUILD** (if packet is ready)
    - If code HAS been touched (or uncertain) â†’ **CODEX BUILD**

If the session cannot determine readiness, it MUST choose:
- **AWAIT HUMAN** (not guessing), OR
- **GEMINI PLANNING** (safe action), depending on agent type.

---

## Session Startup Routing (No Silent Continuation)

At session start, routing MUST:
1) Read state from `TASK_PACKET_SUMMARY.md` (if present)
2) Select routing outcome via the table above
3) If outcome is not STOP, the session MUST explicitly state:
    - Lifecycle state
    - Execution state
    - Authority (Gemini vs Codex)
    - Next allowed action category (one of the routing outcomes)

No work may begin until this declaration is made.

---

## Subtasks / Bugtasks Routing (Placeholder)

Subtasks and bugtasks MUST NOT modify parent lifecycle or execution state.

If additional work is requested while parent execution is `Completed`:
- Outcome MUST be **STOP**
- The only valid path is:
    - new task, or
    - explicit subtask/bugtask created under a new parent task

Detailed child-work creation rules are defined in:

- `CHILD_WORK_RULES.md` (when present)
- `SUBTASKS.md`
- `BUGTASKS.md`

Until those are present and frozen, routing MUST NOT invent child-work behavior.

---

## Planning Precedence â€” Hard Enforcement

All execution routing is gated by **artifact order**, not intent or best effort.

### Required Planning Order
Before a task, subtask, or bugtask may be routed to execution, the following artifacts MUST exist
and be explicitly referenced in the `TASK_PACKET.md`, in this exact order:

1. DISCOVERY
2. REQUIREMENTS
3. RISKS_AND_ASSUMPTIONS
4. TASK_PACKET

### Illegal Conditions
The system MUST NOT route execution if:
- Any required planning artifact is missing
- Artifacts exist but are out of order
- An artifact is referenced but incomplete
- An agent attempts to â€œproceed anywayâ€ or infer missing detail

### Failure Behavior
If a violation is detected:
- Routing MUST halt immediately
- The system MUST request the missing or corrected artifact
- No partial, provisional, or conditional execution is allowed

### Summary Synchronization
On every valid state transition:
- `TASK_PACKET_SUMMARY.md` MUST be updated
- The summary MUST reflect:
    - current execution state
    - artifact manifest
    - child work links (if any)
    - state transition log entry



---


## Prohibited Routing Behavior

Routing MUST NOT:
- infer â€œintentâ€ from chat
- resume automatically without reading disk state
- override terminal states
- change lifecycle state (human only)
- expand scope beyond the current taskâ€™s file-based contract

Any violation invalidates the session output.

Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>