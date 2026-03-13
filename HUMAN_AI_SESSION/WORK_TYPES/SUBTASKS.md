# SUBTASKS

# SUBTASKS.md â€” Planned Decomposition Only

Subtasks are used **exclusively for planned decomposition** of an approved parent task.  
They are **not** used for bugs, defects, regressions, or corrective work.

Bug-related work is governed by `BUGTASKS.md`.

---

## Purpose

Subtasks exist to:
- Break a large, approved task into smaller, manageable units
- Enable sequencing or parallelization of **planned** work
- Preserve clarity without altering original intent

Subtasks are a form of child work and are governed by `CHILD_WORK_RULES.md`.


Subtasks do **not** correct mistakes.  
Subtasks do **not** change outcomes.  
Subtasks do **not** rewrite history.

---

## Prohibited Behavior

- Subtasks MUST NOT supersede parent task packets
- Parent `TASK_PACKET.md` files are immutable once implementation begins
- Subtasks MUST NOT modify parent objectives
- Subtasks MUST NOT be used for bugs, regressions, or failed tests
- Subtasks MUST NOT be created after discovering defects caused by implementation
- Subtasks MUST NOT be created to retroactively justify or explain work already performed


---

## When a Subtask Is Allowed

A subtask MAY be created **only if all conditions are true**:

- The work is explicitly listed in the parent TASK_PACKET.md
  OR is a direct, unavoidable decomposition of listed checklist items

- No defect, regression, or failure triggered the work
- The parent task intent remains valid and unchanged
- Decomposition improves clarity, sequencing, or delivery

If any defect or regression is involved â†’ **this is NOT a subtask**  
Refer to `BUGTASKS.md`.

---

## Folder Placement

Subtasks live under the parent task directory:

docs/current_task_documentation/<parent-task>/subtasks/

Naming:
- S1-short-description/
- S2-short-description/
- ...

Each subtask directory MUST contain:
- `TASK_PACKET.md`
- `TASK_PACKET_SUMMARY.md`

---

## Tool Responsibilities

### Gemini (Planner / Document Author)

Gemini MUST:

- Apply the **Cascading Issues** triage gate before creating a subtask
- Create a subtask only when criteria in this document are met
- Place the subtask in the correct parent directory
- Generate a scoped `TASK_PACKET.md` for the subtask
- Preserve the parent task packet unchanged

If uncertain whether work is planned or corrective:
- Gemini MUST assume it is **NOT** a subtask
- Gemini MUST consult `BUGTASKS.md`
- Gemini MUST ask for human confirmation if still ambiguous

---

## Canonical Subtask Initiation â€” Mandatory

Subtasks are valid ONLY when initiated through the canonical Subtask Worksheet.

### Valid Subtask Criteria
A Subtask is valid if and only if:
- It is created using the approved Subtask Worksheet template
- It is explicitly linked to a human-authorized parent task
- Its scope is planned decomposition, not corrective or cleanup work

### Invalid Subtask Uses
The following are NOT valid Subtasks:
- Comment-only changes
- Comment normalization or consistency work
- â€œQuick fixesâ€ discovered post-implementation
- Work intended to improve readability without changing behavior

### Enforcement
- Any Subtask not created via the canonical worksheet is invalid
- Invalid Subtasks MUST NOT be executed or approved
- If work does not qualify as a Subtask, it MUST be:
    - a Bugtask (corrective), or
    - a new parent task, or
    - explicitly rejected


---

### Codex (Builder / Executor)

Codex MUST:

- Implement subtasks only with an approved subtask `TASK_PACKET.md`
- Enforce subtask scope boundaries strictly

Codex MUST NOT:

- Modify parent `TASK_PACKET.md` files
- Implement corrective fixes under a subtask
- Expand scope beyond the subtask objective
- Act on chat instructions alone

---

## Human Responsibilities

Humans MUST:

- Use subtasks only for planned decomposition
- Use BUGTASKS for bugs or regressions tied to existing work
- Avoid reopening parent task packets unless explicitly required
- Approve any conversion of a subtask into a new parent task

Creating a subtask to fix a bug is a **process violation**.

---

## Immutability & Traceability

- Parent tasks represent **intent**
- Subtasks represent **planned execution slices**
- Bugtasks represent **correction**

History must remain linear and auditable.

Subtasks **add structure** â€” they do not repair mistakes.

---

## Relationship to Task Execution State

- Creating a subtask does NOT advance the parent taskâ€™s execution state
- Parent execution state reflects only work completed directly under the parent task
- Discovery of additional planned work MAY justify a subtask
- Discovery of defects MUST NOT justify a subtask

---

## Enforcement Rule

Any implementation, documentation, or execution that violates this specification
is considered **invalid** and must be corrected before work may proceed.

---

## Reference Policy

Other instruction files MUST reference this document rather than redefining
subtask behavior.

Recommended reference text:

Planned work decomposition follows `AI_SESSION/INSTRUCTIONS/SUBTASKS.md`
Corrective work follows `BUGTASKS.md`.â€

---

## Misclassification Rule

If work is incorrectly implemented as a subtask when it should have been a bugtask:
- The subtask is invalid
- The work must be rolled back or re-recorded
- A correct BUGTASK must be created
- The violation must be noted in the parent TASK_PACKET_SUMMARY.md



Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>