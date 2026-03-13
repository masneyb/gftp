# TASK PACKET — <Task Name / ID>

Version: 1.0  
Last Updated: <YYYY-MM-DD>  
Status: Draft | Ready for Build | Blocked

Supersedes: N/A

---

Subtask handling is governed by AI_SESSION_INSTRUCTIONS/SUBTASKS.md.

---

## 1) Objective
(1–2 sentences describing exactly what this task achieves.)

---

## 2) Non-Goals
(Explicit exclusions. If it’s not here, it’s not allowed.)

- Not doing:
- Not changing:
- Not refactoring:

---

## 3) Constraints
(Technical, architectural, or product limits.)

- Must not change:
- Must preserve:
- Must remain compatible with:

---

## 4) Definitions & Decisions (Lock Ambiguity)
(Anything that could be interpreted multiple ways gets locked here.)

- Terms/fields to treat as authoritative (e.g., `rangeMin/rangeMax` vs `optimalMin/optimalMax`):
- Final decisions on “edge” items (e.g., metrics that MUST remain score-based):
- Display semantics (e.g., raw value vs score selection rules):

---

## 5) Exact File Touch List (Authoritative)

### Create
- `TASK_PACKET_SUMMARY.md` create in same path as TASK_PACKET.md for the task being completed based on `TASK_PACKET_SUMMARY_TEMPLATE.md`
### Modify
- path/to/file.ext

### Do NOT Touch
- path/to/file.ext
- entire directories if applicable

⚠️ If an unlisted file must be touched, the Builder MUST STOP.
Builder may not modify dependency files (pubspec, gradle, podspec, etc.) unless explicitly listed.

---

## 6) Implementation Checklist (In Order)
(Each step must be concrete and verifiable.)

- [ ] Step 1:
- [ ] Step 2:
- [ ] Step 3:

---

## 7) Acceptance Tests (Executable Where Possible)

### Automated (Preferred)
- Command:
    - Expected Result:

### Manual / Visual (When Necessary)
- Scenario:
    - Expected Behavior:

### Non-Regression Checks (Required)
(At least one “did we break anything?” check.)

- Check:
    - Expected Result:

---

## 8) Open Risks / Blockers
(Unknowns that could invalidate the task.)

- Risk:
- Dependency:
- Open question:

---

## 9) Builder Notes
(Optional clarifications; NOT new requirements.)
