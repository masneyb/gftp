# SUBTASK_WORKSHEET.md

This worksheet captures **planned decomposition details** for a subtask.
It MUST comply with `AI_SESSION/INSTRUCTIONS/SUBTASKS.md`

This document does NOT authorize corrective work.

---

## Role of This Worksheet

This worksheet:
- Justifies classification as a Subtask
- Documents planned decomposition intent
- Does NOT authorize execution

Execution authorization requires an approved subtask `TASK_PACKET.md`.


---

## Parent Task Reference (Required)

- Parent Task ID:
- Parent Task Path:
- Parent TASK_PACKET.md Hash (or last modified timestamp):

---

## Subtask Identity

- Subtask ID:
- Subtask Directory Name (S#-short-description):

---

## Planned Decomposition Justification (Required)

Describe how this work:
- Maps directly to specific checklist items in the parent TASK_PACKET.md
- Represents decomposition, not correction or discovery
- Was known or implied before implementation began


If this justification cannot be stated clearly,  
**this work is NOT a subtask**.

---

## Scope Definition

### In Scope
(List only work that directly decomposes the parent objective)

### Explicitly Out of Scope
(Must include *any* corrective, bug-related, or exploratory work)


Work not explicitly listed as “In Scope” is prohibited.

---

## Dependency & Sequencing Notes

- Depends On (other subtasks, if any):
- May Run In Parallel With:
- Ordering Constraints (if applicable):

---

## Acceptance Criteria

- AC-1:
- AC-2:
- AC-3:

Acceptance criteria MUST:
- Align with parent task intent
- Introduce no new outcomes
- Acceptance criteria MUST be satisfiable without modifying parent task success criteria.


---

## Invariants (Machine-Enforced)

By completing this worksheet, the following are asserted:

- ☐ This subtask does NOT fix a bug or regression
- ☐ This subtask does NOT modify the parent TASK_PACKET.md
- ☐ This subtask does NOT change parent objectives
- ☐ This subtask represents planned decomposition only
- ☐ This subtask does NOT alter parent lifecycle or execution state


If any box cannot be checked,  
**this work must be classified as a BUGTASK or new task**.

---

## Classification Authorization

- Created By:
- Reviewed By (Human):
- Approved to Create Subtask TASK_PACKET.md: ☐ Yes ☐ No
- Date Approved:

---

## Notes (Optional, Non-Authoritative)

(Clarifying information only. No scope expansion.)



Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
