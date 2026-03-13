# HUMAN IDEA BRIEF

> This brief is written by a human. Keep it plain-language and outcome-focused.
> If this is a bug/regression, fill out the **Bug / Regression Details** section.
> If this is a subtask, create this file inside the subtask folder and fill out **Subtask Linkage**.

---

## 1 - Task Summary (Required)
**One sentence:** What are we trying to achieve or fix?


---

## 2 - Task Type (Required)
- [ ] Feature / Improvement (new behavior)
- [ ] Bugfix / Regression (existing behavior is wrong/broken)
- [ ] Follow-up / Cleanup (non-breaking, scoped)

---

## 3 - Classification Gate (Required)

- ☐ This is corrective work (bug/regression), not planned decomposition
- ☐ This is NOT a feature/improvement request in disguise
- ☐ If fix requires new behavior beyond restoring intended behavior, I will reclassify as Feature/Improvement


---

## 4 - Subtask Linkage (Fill out if this is a subtask)
- **Parent task path:** `docs/current_task_documentation/<parent-task>/`
- **Subtask folder (if already created):** `subtasks/<S#-short-description>/`
- **Why this must be a subtask:** (tie it to the parent task’s implementation)

---

## 5 - Why This Matters (Required)
Why do you want this? What pain does it solve or value does it add?

---

## 6 - Desired Outcome (Required)
What should be true when this work is “done”? (bullet list)

---

## 7 - Rough Expectations (Optional)
What do you *think* should happen? (It’s okay if this is wrong or incomplete.)

---

## 8 - Bug / Regression Details (Fill out for Bugfix/Regression)
- **User-visible impact:** What breaks or degrades?
- **Severity:** Low / Medium / High / Critical
- **Where it happens:** screen/flow/module (if known)
- **Steps to reproduce:**
    1.
    2.
    3.
- **Expected vs Actual:**
    - Expected:
    - Actual:
- **Frequency:** Always / Often / Sometimes / Rare
- **First noticed:** date/time or “after task X” (if known)
- **Workaround:** (if any)

### 8.1 - Bugfix Scope Guardrails (Required for Bugfix/Regression)

- **Bug definition (one sentence):** (what exactly is wrong)
- **Fix boundary:** (what parts of the system may be changed)
- **Out of scope (explicit):**
    - (e.g., refactors, performance tuning, UI polish, new features)
- **Allowed collateral changes:** (tests, logging, docs) Yes/No + notes
- **Parent linkage (if known):** task/PR/commit that introduced it


---

## 9 - Environment / Context (Helpful for bugs)
- **Platform(s):** iOS / Android / Web / Backend / Other
- **Device(s):** (models if relevant)
- **OS version(s):**
- **App version / build / branch:**
- **Feature flags / configs:** (if any)

---

## 10 - Known Constraints (Optional)
Anything you already know must be true:
- Platforms
- Performance
- Business rules
- Deadlines

---

## 11 - External Research Needed?
- [ ] Yes (policies, SDK docs, platform rules, etc.)
- [ ] No / Not sure

If yes, include links or keywords to search for.

---

## 12 - Unknowns / Questions (Optional)
What are you unsure about? What needs investigation?

---

## 13 - Acceptance Criteria (Required)
List objective checks that confirm success.
- [ ]
- [ ]

---

## 14 - Verification Notes (Optional but recommended)
How will we test this?
- Manual test steps:
- Automated tests expected:
- Logs/metrics to review:

---

## 15 - Attachments / Evidence (Optional)
- Screenshots / screen recordings:
- Logs / crash reports:
- Links to related tasks, PRs, commits:


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
