# CHILD_WORK_RULES.md

This document defines the **authoritative policy** for creating and governing child work:
- **Subtasks**
- **Bugtasks**

It is **policy-only**:
- No CLI steps
- No runbook procedures
- No templates
- No routing logic (see `ROUTING_RULES.md`)

Child work exists to prevent scope creep, preserve determinism,
and keep task history truthful and auditable.

---

## Core Principles

1) **Parent task state must remain truthful**
- Child work MUST NOT alter the parentâ€™s lifecycle or execution state.
- Parent state reflects parent task progress only.

2) **No work happens outside a task container**
- Any additional work must be represented as either:
    - the parent task itself,
    - an explicit child work item (subtask or bugtask), or
    - a new task.

3) **Terminal states are enforced**
- If a parent execution state is `Completed`, no further work may occur *within* the parent task.
- Follow-on work is permitted only as child work or a new task, without modifying the parent.

---

## Definitions

### Subtask
A **Subtask** is child work created when:
- the parent task objective remains valid, and
- remaining work is logically separable into a smaller unit, and
- completing the parent requires additional scoped effort.

Subtasks represent **planned decomposition or scoped follow-on work**.

---

### Bugtask
A **Bugtask** is child work created when:
- a defect or regression is attributable to the parent taskâ€™s implementation, and
- the corrective work is scoped and specific, and
- the parent task must remain historically accurate and unchanged.

Bugtasks represent **corrective work** discovered during testing, review, or validation.

---

## Classification Rules (Deterministic)

When new work is identified, classify it using the **first matching rule**:

### 1) Bugtask
If the work is required because:
- behavior does not match the parent `TASK_PACKET.md`, or
- a regression or defect was introduced by the parent work, or
- previously satisfied acceptance criteria are failing.

---

### 2) Subtask
If the work is required because:
- the parent objective remains unchanged, and
- the remaining work is a distinct, bounded unit of effort,
- and the work is not corrective in nature.

---

### 3) New Task
If the work:
- changes the objective or scope materially,
- introduces a new feature direction,
- or would require revising the parent `TASK_PACKET.md` rather than executing it.

No agent may reinterpret scope to avoid creating a new task boundary.

---

## Terminal Gate (Hard Stop)

If a parent execution state is `Completed`:

- No modifications are permitted under the parent task directory as continuation work.
- The parent task remains `Completed` and unchanged.

Discovered issues must be represented as:
- a **Bugtask** (for defects attributable to the completed work), or
- a **New Task** (for new scope or feature work).

Bugtasks are permitted after completion **only because** they preserve parent history and state.

---

## Comment Normalization Work â€” Classification Rule

Comment normalization is **not** valid child work unless explicitly defined as such in a governed policy file.

### Rule
- Repo-wide or multi-file comment normalization MUST be handled as:
    1) an explicitly authorized parent task, OR
    2) the one-time normalization process defined in `COMMENT_NORMALIZATION.md`.

### Prohibited Misclassification
The following are NOT valid reasons to open a Subtask or Bugtask:
- â€œJust updating headersâ€
- â€œMinor comment cleanupâ€
- â€œFormatting-only changesâ€
- â€œMaking comments consistentâ€

If comment changes are needed but exceed the current task scope:
- STOP and require a new parent task or the one-time normalization procedure.

### Terminal Gate Reinforcement
If a parent task execution state is `Completed`:
- No comment normalization or â€œcleanupâ€ may be attached as child work
- Follow-on work MUST be a new task (or a bugtask only if it is corrective and restores intended behavior)


---

## Parent / Child State Isolation

- Child work MUST NOT modify:
    - parent lifecycle state
    - parent execution state
- Child work completion MUST NOT imply parent completion.
- A parent may reference a child item as a blocker or follow-up,
  but state ownership remains unchanged.

---

## Authority Rules

- Lifecycle state is always human-controlled.
- Execution state authority is defined in:
    - `WORKFLOW_STATE_MACHINE.md`

Child work creation authority:
- Humans may request child work at any time.
- Gemini may propose and document child work during planning or review.
- Codex MUST NOT invent child work and may only request it when blocked by scope or ambiguity.

---

## Required Linkage (Traceability)

Every child work item MUST explicitly reference:
- the parent task identifier or path
- the classification reason (Subtask or Bugtask)
- the acceptance criteria for the child work

Parent documentation MAY reference child work when it affects progress,
but MUST NOT alter parent state semantics.

---

## Prohibited Behavior

- Creating child work to bypass terminal state enforcement
- Folding bug fixes into the parent task after completion
- Changing parent scope instead of creating child work
- Treating child work as evidence of parent progress or state change
- Creating child work without explicit classification

---

## Canonical References

- State authority and terminal rules:
    - `WORKFLOW_STATE_MACHINE.md`

- Routing behavior:
    - `ROUTING_RULES.md`

- Child work operational details:
    - `SUBTASKS.md`
    - `BUGTASKS.md`


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>