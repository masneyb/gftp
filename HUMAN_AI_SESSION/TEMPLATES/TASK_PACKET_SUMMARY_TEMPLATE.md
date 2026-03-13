# TASK PACKET SUMMARY

## IDENTIFIERS

- Task ID: (Required)
- Task Name: (Required)
- Task Type: Parent | Subtask | Bugtask (Required)

- Parent Task Path: (Required if Subtask or Bugtask)
- Related TASK_PACKET.md Version: (Required)
- Task Directory Path: (Required)

- Summary Author (Agent/Human):
- Summary Date: YYYY-MM-DD

---



---

## AUTHORIZATION SNAPSHOT (Human-Controlled)

> This section reflects authorization state only.  
> It MUST match the authoritative TASK_PACKET.md.

- Authorized for Build: ☐ Yes ☐ No (Required)
- Authorized Scope:
    - ☐ Implementation
    - ☐ Refactor
    - ☐ Comment normalization
- Authorized By: (Required if Authorized)
- Authorization Date: YYYY-MM-DD (Required if Authorized)
- Authorization Reference: (Path/link to TASK_PACKET.md)

**Notes**
- Planning completion ≠ authorization
- COMMENT_STRATEGY.md does NOT authorize work
- If not authorized, execution state MUST NOT advance

---

## LIFECYCLE STATE (Human-Controlled)

<!-- Valid Lifecycle values (from WORKFLOW_STATES.md):
Active | Paused | Blocked | Completed | Abandoned
-->

- Lifecycle: (Required — must be a valid value from `WORKFLOW_STATES.md`)
- Set On: YYYY-MM-DD (Required)
- Set By: (Required)
- Reason: (Required if Lifecycle is Paused or Abandoned)

- Lifecycle Notes:

---

## EXECUTION STATE (Task State System)

<!-- Valid Execution State values (from WORKFLOW_STATES.md):
WIP | Under Review | Blocked | Completed
-->

- Execution State: (Required — must be one of: WIP | Under Review | Blocked | Completed)  
  Invalid values (e.g. "Active") MUST NOT be used.

- Last Updated: YYYY-MM-DD (Required)
- Updated By: (Required)

- State Notes:

- State Machine Reference: `WORKFLOW_STATE_MACHINE.md` (informational pointer)

---

## PLANNING ARTIFACT MANIFEST (Order Matters)

> These artifacts MUST exist and MUST have been used in this order.

- DISCOVERY: (Path / link)
- REQUIREMENTS: (Path / link)
- RISKS_AND_ASSUMPTIONS: (Path / link)
- TASK_PACKET: (Path / link)

Missing or out-of-order artifacts invalidate execution.

---

## IMPLEMENTATION OVERVIEW

(High-level summary of what was implemented. No design discussion.)

- Summary:

---

## FILE TOUCH MANIFEST

### Created
- path/to/file.ext
- (none)

### Modified
- path/to/file.ext
- (none)

### Deleted
- path/to/file.ext
- (none)

**Out-of-Scope Files Touched:** ☐ No ☐ Yes (If Yes → STOP; requires explanation)

---

## TASK CHECKLIST RESULTS (from TASK_PACKET.md)

Map each checklist item from TASK_PACKET.md:

- [ ] Item 1 — Completed / Not Completed — Notes:
- [ ] Item 2 — Completed / Not Completed — Notes:
- [ ] Item 3 — Completed / Not Completed — Notes:

---

## CHILD WORK LINKS

### Subtasks
- Subtask ID / Path:
- Worksheet Reference:
- Status:

### Bugtasks
- Bugtask ID / Path:
- Originating Task:
- Status:

---

## COMMANDS RUN (WITH OUTPUT)

Include full command + full output (or note truncation + where saved).

- Command:
    - Output:

---

## TESTS & VERIFICATION

### Automated Tests
- Command(s) run:
- Result: Pass / Fail / Not Run
- Notes:

### Manual / Visual Verification
- Steps performed:
- Expected result:
- Actual result:

---

## NON-REGRESSION CHECKS

- Check performed:
- Outcome: Pass / Fail / Not Run
- Notes:

---

## DEVIATIONS

List any deviation from TASK_PACKET.md.  
If none: **None**.

---

## KNOWN ISSUES / FOLLOW-UPS

- Issue:
- Impact:
- Suggested next action:
- Requires new TASK_PACKET? Yes / No
- If Yes: Bugtask or Subtask?
- Justification:

---

## REVIEW NOTES

What a reviewer should focus on, risk areas, or anything non-obvious.

---

## CHANGESET NOTES

- Proposed commit message:
- Related PR (if any):
- Key diffs / areas to review:

---

## STATE TRANSITION LOG (Append-Only)

- YYYY-MM-DD — <From State> → <To State> — Reason:
- YYYY-MM-DD — <From State> → <To State> — Authorized by:

---

## STATE RECONCILIATION (Must Match Execution State)

- Confirmed Execution State: (Must exactly match “Execution State” above)
- Confirmation Reason:
- Confirmed By:
- Confirmed On: YYYY-MM-DD

---

## REFERENCES

- Subtask rules: `AI_SESSION/INSTRUCTIONS/SUBTASKS.md`
- Bugtask rules: `AI_SESSION/INSTRUCTIONS/BUGTASKS.md`
- Workflow states (authoritative): `WORKFLOW_STATES.md`
- Workflow machine (authoritative): `WORKFLOW_STATE_MACHINE.md`

Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
