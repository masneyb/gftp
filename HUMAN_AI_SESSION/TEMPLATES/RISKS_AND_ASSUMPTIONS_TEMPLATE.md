# RISKS AND ASSUMPTIONS

Purpose:  
Explicitly surface uncertainties that could affect correctness, scope,
timeline, or stability.

**Authoritative Location**: `{task_dir}/RISKS_AND_ASSUMPTIONS.md`

This document exists to:
- Prevent silent failure modes
- Make fragility visible early
- Enable informed go / no-go decisions

It does NOT define requirements or implementation plans.

---

## Risks

List concrete, credible risks.

A risk is:
- A future event or condition
- That may occur
- And would negatively affect the task if it does

For each risk, include:
- Description: What could go wrong
- Impact: Low / Medium / High
- Likelihood: Low / Medium / High
- Mitigation or monitoring plan: How the risk will be reduced, detected, or revisited

Format:
- Risk:
    - Impact:
    - Likelihood:
    - Mitigation or monitoring:

---

## Assumptions

List assumptions that must hold true for this task to succeed.

If an assumption is false:
- Requirements may be invalid
- Scope may change
- Rework may be required

For each assumption, include:
- Assumption:
- Rationale: Why this is believed to be true
- Impact if incorrect:

Assumptions here must align with (but do not replace)
assumptions listed in REQUIREMENTS.

---

## Open Questions

Questions that, if answered, would:
- Reduce risk, or
- Validate or invalidate assumptions

For each question:
- Question:
- Owner:
- Blocking? Yes / No
- Needed by:

Unanswered blocking questions may prevent execution.

---

## Change Triggers

Events or conditions that should force re-evaluation of this task.

Triggers exist to prevent:
- Continuing work under invalid assumptions
- Ignoring external changes
- Retrofitting explanations after failure

For each trigger:
- Trigger:
- Expected response: Pause, re-scope, create subtask, create bugtask, or abandon


---

## Subtask Gate (MANDATORY — FINAL)

This section determines whether the task can safely proceed as a single
bounded TASK_PACKET, or whether decomposition is required.

### Evaluation
- subtasks_likely: YES | NO
- rationale:
    - (independent phases, verification sequencing, platform divergence,
      high-risk components, context overload)

### Human Alert Requirement
If `subtasks_likely = YES`:
- Gemini MUST alert the human
- Gemini MUST explain the rationale
- Gemini MUST STOP after writing this document


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
