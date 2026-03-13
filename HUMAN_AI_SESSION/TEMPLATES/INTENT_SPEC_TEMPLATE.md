# INTENT SPEC — <Task Name / ID>

> This document is a **technical translation** of the human-authored {task_dir}/HUMAN_IDEA_BRIEF.md.
> It is produced by the Claude Intent Translator and is **advisory until human-approved**.
>
> This document does NOT authorize planning or execution.
> It provides Gemini with a technically precise version of human intent.

---

## Source Brief

- Brief Location: `{task_dir}/HUMAN_IDEA_BRIEF.md`
- Brief Last Modified: <YYYY-MM-DD>
- Translation Date: <YYYY-MM-DD>
- Translator: Claude (Intent Translator)

---

## Human Approval

- Approved: ☐ Yes ☐ No
- Approved By: ____________
- Approval Date: <YYYY-MM-DD>

If not approved, Gemini MUST treat this document as **non-authoritative**
and fall back to `{task_dir}/HUMAN_IDEA_BRIEF.md`.

---

## 1) Translated Objective

Restate the Task Summary from the brief in precise technical language.

Rules:
- 1–3 sentences only
- Must reference correct architectural components (from PROJECT_CONTEXT.md)
- Must preserve the human's stated outcome exactly
- Must not introduce new objectives

**Human said:** (quote or paraphrase from brief)
**Technical translation:**

---

## 2) Translated Task Type

- [ ] Feature / Improvement
- [ ] Bugfix / Regression
- [ ] Follow-up / Cleanup

Must match the brief's classification. If Claude believes the classification
is incorrect, flag it in the Gaps section — do NOT change it here.

---

## 3) Translated Desired Outcomes

Map each human-stated outcome to technical language.

For each outcome:
- **Human said:** (from brief)
- **Technical translation:**
- **Affected components:** (from PROJECT_CONTEXT.md)
- **Traceability:** Brief section → Desired Outcome #N

---

## 4) Translated Constraints

Map each human-stated constraint to technical language.

For each constraint:
- **Human said:** (from brief)
- **Technical translation:**
- **Enforcement boundary:** (what must not change)
- **Traceability:** Brief section → Known Constraints #N

If the brief has no constraints section or it is empty:
- State: `No constraints specified in brief`
- Flag in Gaps section

---

## 5) Translated Acceptance Criteria

Map each human-stated acceptance criterion to testable technical language.

For each criterion:
- **Human said:** (from brief)
- **Technical translation:** (must be unambiguous and verifiable)
- **Verification method:** Automated / Manual / Inspection
- **Traceability:** Brief section → Acceptance Criteria #N

---

## 6) Bug / Regression Translation (If Applicable)

Only populate if Task Type is Bugfix / Regression.

- **Affected system boundary:** (Dart / Kotlin / Flutter / Native / Channel)
- **Translated reproduction steps:**
- **Translated expected vs actual:**
- **Translated fix boundary:**
- **Translated scope guardrails:**

If not applicable: `N/A — not a bugfix task`

---

## 7) Architectural Context (From PROJECT_CONTEXT.md)

List the specific components, layers, and boundaries relevant to this task.

This section helps Gemini scope its discovery accurately.

- **Relevant Dart components:**
- **Relevant Kotlin components:**
- **Relevant boundaries (channels, bridges, APIs):**
- **Relevant data flow segments:**

If uncertain: flag in Gaps section.

---

## 8) Translation Confidence

Rate confidence per section:

| Section | Confidence | Notes |
|---------|------------|-------|
| Objective | High / Medium / Low | |
| Task Type | High / Medium / Low | |
| Desired Outcomes | High / Medium / Low | |
| Constraints | High / Medium / Low | |
| Acceptance Criteria | High / Medium / Low | |
| Bug Translation | High / Medium / Low / N/A | |
| Architectural Context | High / Medium / Low | |

Low confidence sections MUST have an entry in the Gaps section.

---

## 9) Gaps and Ambiguities (MANDATORY)

List every gap, ambiguity, or potential conflict identified during translation.

For each gap:
- **Gap ID:** G-1, G-2, etc.
- **Section affected:**
- **Description:** What is missing, unclear, or potentially conflicting
- **Impact if unresolved:** What Gemini cannot plan without this
- **Suggested resolution:** (optional — must not introduce new scope)

If no gaps exist: `None identified`

---

## 10) Items NOT Translated (Transparency Record)

List any brief content that could not be mapped to a spec section.

For each item:
- **Brief content:** (quote or paraphrase)
- **Reason not translated:** (no matching spec section / too vague / requires human decision)

If everything was translated: `All brief content mapped successfully`


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
