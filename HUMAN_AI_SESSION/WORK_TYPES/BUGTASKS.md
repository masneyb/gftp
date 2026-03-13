# BUGTASKS â€” Corrective Work Policy (Defects / Regressions)

## Purpose
BUGTASKS represent corrective work for defects discovered during or after implementation
that are attributable to a specific parent taskâ€™s changes.
BUGTASKS are a distinct task type and MUST be labeled as:
Task Type: Bugtask

BUGTASKS preserve traceability:
- Parent task = intent
- Bugtask = correction

BUGTASKS are a form of child work and are governed by `CHILD_WORK_RULES.md`.


---

## Prohibited Behavior
- BUGTASKS MUST NOT modify parent TASK_PACKET.md
- BUGTASKS MUST NOT expand scope beyond the defect fix
- BUGTASKS MUST NOT be created to â€œfinish planned workâ€ (use SUBTASKS for that)
- BUGTASKS MUST NOT introduce new features, enhancements, or refactors
  unrelated to the defect being corrected


---

## When a BUGTASK Is Mandatory (Post-Implementation Triggers)
â€œCode has been touchedâ€ means any committed, staged, or executed change
associated with the parent task.

A BUGTASK MUST be created when ANY of the following occur after code has been touched:

- A bug/regression is discovered in testing or review
- An acceptance test fails (or partially fails)
- Fix requires touching files not listed in the parent TASK_PACKET.md
- Fix requires changing behavior already marked complete in the parent task
- A documented assumption is invalidated by real results

---
## BugTasks â€” Comment Change Restrictions

BugTasks exist to **correct incorrect behavior**, not to improve presentation or consistency.

### Comment-Related Rules
- Comment-only changes are NOT valid BugTasks unless:
    - the comment is objectively wrong, misleading, or documents incorrect behavior
    - and the change restores the originally intended meaning

### Explicitly Not BugTasks
The following are NOT valid BugTasks:
- Comment normalization
- Comment formatting or consistency updates
- Adding missing headers or version fields
- Readability or style improvements

### Enforcement
- BugTasks MUST NOT be used to bypass parent task completion
- BugTasks MUST preserve historical intent and traceability
- If comment work is discovered post-completion and is non-corrective:
    - STOP
    - require a new parent task or explicit normalization authorization


---

## Folder Placement (Deterministic)

BUGTASKS live under the parent task directory:

docs/current_task_documentation/<parent-task>/bugtasks/

Naming:
- B1-short-description/
- B2-short-description/
- ...

Each BUGTASK directory MUST contain:
- HUMAN_IDEA_BRIEF.md (Required; bug-focused)
- DISCOVERY.md (required: bug-focused)
- REQUIREMENTS.md (required: bug-focused)
- RISKS_AND_ASSUMPTIONS.md (required: bug-focused)
- TASK_PACKET.md (scoped fix)
- TASK_PACKET_SUMMARY.md (execution record)

For Bugfix / Regression tasks, `HUMAN_IDEA_BRIEF.md` MUST include:
- Bug / Regression Details
- Bugfix Scope Guardrails
- Expected vs Actual behavior
- Reproduction steps (when applicable)

---

## Roles & Enforcement

### Gemini
- Creates BUGTASK directories ONLY when trigger conditions are met
- Produces a scoped BUGTASK TASK_PACKET.md
- References the parent task and the specific defect evidence (logs, repro steps)

## Lifecycle Control

- BUGTASK Lifecycle (Active / Paused / Abandoned) is human-controlled only
- Lifecycle changes MUST be recorded in TASK_PACKET_SUMMARY.md
- Execution state changes MUST NOT imply lifecycle changes


### Codex
- MUST refuse all corrective work unless a BUGTASK TASK_PACKET.md exists
- MUST NOT patch parent task directly
- MUST update the BUGTASK TASK_PACKET_SUMMARY.md

### Humans
- Provide repro steps, severity, and expected vs actual
- Approve converting a BUGTASK into a new parent task (rare)

---

## Relationship to SUBTASKS
- Use SUBTASKS for planned decomposition or incremental delivery
- Use BUGTASKS for defects/regressions caused or revealed by implementation
- If unsure: Gemini MUST pause and request human clarification

---

## Relationship to Task Execution State

- Creating a BUGTASK does NOT revert or alter the parent taskâ€™s execution state
- Parent execution state remains truthful to completed parent work
- BUGTASK execution state is tracked independently

## Misclassification Rule

If work is incorrectly implemented outside a BUGTASK when a trigger condition exists:
- The work is invalid
- A BUGTASK MUST be created retroactively
- The correction MUST be re-recorded under the BUGTASK
- The violation MUST be noted in the parent TASK_PACKET_SUMMARY.md


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>