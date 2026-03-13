# WORKFLOW_STATE_MACHINE.md

This document defines the **authoritative, file-based state machine**
governing all AI-assisted work.

State exists **only when written to disk**.
No state may be inferred from chat history, tool context, or assumptions.

This state machine applies globally across:
- Humans
- Architect
- Planner
- Specialist Engineers
- Tools

---

## Core Principle

**No state exists unless it is written to disk.**

The single source of truth for task state is:
- `{task_dir}/TASK_PACKET_SUMMARY.md`

---

## State Axes (Strict Separation)

The workflow operates on **two independent state axes**.
They MUST NOT be inferred from one another.

---

## 1) Lifecycle State (Authorization)

Represents human authorization and intent.

Allowed values:
- Active
- Paused
- Abandoned

Recorded in:
- `{task_dir}/TASK_PACKET_SUMMARY.md` â†’ **LIFECYCLE STATE**

Authority:
- **Human only**

Rules:
- Lifecycle changes are always explicit
- Lifecycle state never changes automatically
- Execution progress does NOT imply lifecycle changes

### Valid Lifecycle Transitions

Allowed:
- Active â†’ Paused
- Active â†’ Abandoned
- Paused â†’ Active
- Paused â†’ Abandoned

Disallowed:
- Abandoned â†’ Any
- Any implicit or inferred transition

**Abandoned** is a terminal lifecycle state.

---

## 2) Execution State (Progress)

Represents **observable implementation progress**.

Allowed values:
- WIP
- Blocked
- Under Review
- Completed

Recorded in:
- `{task_dir}/TASK_PACKET_SUMMARY.md` â†’ **EXECUTION STATE**

Authority:
- **Architect/Planner** â€” while no production source file has been modified
- **Specialist Engineer** â€” once any production source file has been modified

Authority transfer is determined by observable file changes,
not intent or phase.

Rules:
- Execution state reflects reality only
- Execution state MUST NOT modify lifecycle state

### Valid Execution Transitions

Allowed:
- WIP â†’ Blocked
- WIP â†’ Under Review
- Under Review â†’ Blocked
- Under Review â†’ Completed
- Blocked â†’ WIP

Disallowed:
- Completed â†’ Any
- Any execution transition implying a lifecycle change

**Completed** is a terminal execution state.

---

## Authorization Gate â€” Mandatory Transition Rule

### Rule
A task MUST NOT transition from `Draft` to `Ready for Build` unless the
**Human Authorization Gate** is explicitly satisfied.

### Definition: Authorization Gate (Human-Owned)
The Authorization Gate is satisfied ONLY when:
- A human has explicitly marked the task as authorized for build
- The authorization is recorded in the `TASK_PACKET.md`
- Authorization applies to the **specific scope** defined (implementation, refactor, normalization)

### Explicit Non-Equivalences
The following DO NOT satisfy the Authorization Gate:
- Completion of planning documents
- Presence of DISCOVERY / REQUIREMENTS / RISKS artifacts
- Review comments or chat confirmations
- Existence of COMMENT_STRATEGY.md

### Enforcement
- Any attempt to transition `Draft â†’ Ready for Build` without a satisfied Authorization Gate is **illegal**
- The system MUST halt and request human action
- No agent may â€œassume intentâ€ or proceed conditionally


---

## State Authority Matrix

| State Axis | Authority | Location |
|-----------|-----------|----------|
| Lifecycle | Human only | {task_dir}/TASK_PACKET_SUMMARY.md |
| Execution | Architect â†’ Engineer | {task_dir}/TASK_PACKET_SUMMARY.md |

Execution authority NEVER implies lifecycle authority.

---

## Terminal State Rules

Terminal states cannot be exited or resumed.

- Lifecycle terminal state: **Abandoned**
- Execution terminal state: **Completed**

Any additional work requires:
- A new task, or
- An explicit subtask or bugtask

---

## Subtask & Bugtask Isolation

- Subtasks do NOT modify parent lifecycle state
- Subtasks do NOT modify parent execution state
- Bugtasks do NOT modify parent lifecycle state
- Bugtasks do NOT modify parent execution state

Parent task state MUST reflect **parent task reality only**.

Child-work behavior is governed by:
- `AI_SESSION/CONTROL/CHILD_WORK_RULES.md`
- `AI_SESSION/CONTROL/SUBTASKS.md`
- `AI_SESSION/CONTROL/BUGTASKS.md`

---

## Prohibited Behavior

- Inferring state from chat history
- Resuming work without reading state files
- Modifying lifecycle state without human action
- Skipping state documentation
- Conflating lifecycle and execution state

Any such action invalidates the session.

---

## Enforcement Rule

Any work performed when:
- Lifecycle â‰  Active, or
- Required state is missing or ambiguous

is **invalid** and MUST be corrected
before work may proceed.

---

## Canonical Authority

This document is authoritative for:
- Allowed state values
- State meaning
- Allowed and disallowed transitions

All other instruction files MUST defer to this document
for state machine behavior.


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>