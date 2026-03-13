# TASK PACKET — <Task Name / ID>

Version: 1.0  
Last Updated: <YYYY-MM-DD>  
Status: Draft | Ready for Build | Blocked

Supersedes: N/A

---

This document is the **execution contract** for this task.

Anything not explicitly authorized here is **out of scope**.
If ambiguity exists, execution must stop.

Subtask handling is governed by:
AI_SESSION/INSTRUCTIONS/SUBTASKS.md

---
## Human Authorization Gate (Required)

This task MUST NOT proceed to execution unless explicitly authorized by a human.

### Authorization Status
- Authorized for Build: ☐ Yes ☐ No
- Authorized By: <Human name>
- Authorization Date: <YYYY-MM-DD>

### Authorized Scope
(Check all that apply)
- ☐ Implementation
- ☐ Refactor
- ☐ Comment normalization
- ☐ Other (explicitly defined):

### Enforcement Notes
- Planning completion does NOT imply authorization
- Presence of DISCOVERY / REQUIREMENTS / RISKS documents does NOT imply authorization
- COMMENT_STRATEGY.md does NOT authorize work
- If this section is incomplete or unchecked, the task MUST remain in `Draft`

---

# AUTHORIZATION GATE — READY FOR BUILD (HUMAN-OWNED)

> This section is a **hard gate**.  
> The Architect (Gemini) may draft it, but **may not mark it satisfied**.  
> The task packet may not be marked `Ready for Build` unless **every box is checked** and the **Human Approval** line is completed.

## Gate Checklist (Must be all ✅)

- [ ] **All “Needed by Implementation” items are resolved**
    - No open questions remain in REQUIREMENTS that would affect implementation choices.
- [ ] **DISCOVERY.md is complete**
    - Includes relevant context, constraints, and existing behavior notes.
- [ ] **REQUIREMENTS.md is complete**
    - Acceptance criteria are explicit and testable.
    - Validation rules are explicitly stated (no “implied” validation).
- [ ] **RISKS_AND_ASSUMPTIONS.md is complete**
    - Key risks are enumerated.
    - Assumptions are explicitly tagged as such.
- [ ] **Definitions & Decisions are locked**
    - Ambiguous terms/fields have authoritative definitions.
    - Any “edge cases” that could fork implementation are decided.
- [ ] **File touch list is explicit**
    - Exact files allowed to be modified/created are listed.
    - No “and related files” language.
- [ ] **Non-goals are explicit**
    - Clear exclusions to prevent scope creep.
- [ ] **HUMAN_IDEA_BRIEF provenance is valid**
    - Brief is human-authored **or** explicitly human-ratified below.

## Human Approval (Required)

Human Approval: ☐ Approved for Build   ☐ Not Approved  
Approved by (name/handle): ____________  
Date (YYYY-MM-DD): ____________

## Brief Ratification (Only if Architect drafted the brief text)

I confirm the HUMAN_IDEA_BRIEF content is correct and authorized for planning:
☐ Ratified by Human (name/handle): ____________  Date (YYYY-MM-DD): ____________

---



---

## 1) Objective

Describe exactly what this task accomplishes.

Rules:
- 1–2 sentences only
- Outcome-focused
- No implementation detail
- Must be achievable without inference

---

## 2) Non-Goals

Explicit exclusions.

This section is authoritative.
If something is not allowed, it must be stated here.

- Not doing:
- Not changing:
- Not refactoring:

---

## 3) Constraints

Hard limits that must not be violated.

These are **non-negotiable** during execution.

- Must not change:
- Must preserve:
- Must remain compatible with:

---

## 4) Definitions & Decisions (Lock Ambiguity)

All potentially ambiguous items must be resolved here.

This section overrides:
- defaults
- conventions
- prior assumptions

Include:
- Authoritative terms or field names
- Final decisions on edge cases
- Display or interpretation semantics
- Tie-breakers where multiple “reasonable” options exist

If something is ambiguous and not resolved here, execution must stop.

---

## 5) Exact File Touch List (Authoritative)

This section defines the **only files the Builder may interact with**.

### Create (or Modify if already present)
- `TASK_PACKET_SUMMARY.md`
    - Must be created in the same directory as this file
    - Must follow `TASK_PACKET_SUMMARY_TEMPLATE.md`

### Modify
- path/to/file.ext

### Do NOT Touch
- path/to/file.ext
- entire directories if applicable

Rules:
- Touching an unlisted file requires stopping execution
- Dependency or config files (e.g., pubspec, gradle, podspec) are forbidden unless explicitly listed
- “Incidental” changes are not permitted

---

## 6) Implementation Checklist (In Order)

Concrete, ordered steps the Builder must follow.

Rules:
- Steps must be explicit and verifiable
- No conditional logic
- No inferred sub-steps

- [ ] Step 1:
- [ ] Step 2:
- [ ] Step 3:

---

## 7) Acceptance Tests

These define **what success means**.

### Automated (Preferred)
- Command:
    - Expected result:

### Manual / Visual (When Necessary)
- Scenario:
    - Expected behavior:

### Non-Regression Checks (Required)

At least one check must validate that existing behavior was not broken.

- Check:
    - Expected result:

---

## 8) Open Risks / Blockers

Known issues that could invalidate or pause execution.

List only items that materially affect correctness or completion.

- Risk:
- Dependency:
- Open question:

If a blocker is encountered, execution must stop.

---

## 9) Builder Notes

Optional clarifications that:
- Explain intent
- Reduce misinterpretation

Rules:
- Must not introduce new requirements
- Must not expand scope
- Must not contradict earlier sections

---

Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
