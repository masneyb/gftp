# AI Workflow Runbook (Human Steps)

This runbook describes the standard **human-driven procedure** for running an AI-assisted task.

It does not define agent behavior or workflow rules.
Those are defined under `AI_SESSION/INSTRUCTIONS/`.

---

## 0) Preconditions
- You are on a branch (recommended for any non-trivial task)
- `AI_SESSION/INSTRUCTIONS/` is up to date in your branch
- No other task is currently in the Active lifecycle state

---

## 1) Create a Task Directory
Create a new numbered task directory under:

docs/current_task_documentation/<NN-Task-Name>/

Example:
docs/current_task_documentation/22-Advertisement-Integration/

If early traceability is desired (recommended), commit directory creation.

If the task is large and requires planned decomposition, create **Subtasks** as defined in:
- `INSTRUCTIONS/SUBTASKS.md`

If defects are discovered after implementation begins, follow:
- `INSTRUCTIONS/BUGTASKS.md`

---

## 2) Create the Human Idea Brief
Copy the template into the task directory:

- `HUMAN_IDEA_BRIEF.md` (from `TEMPLATES/`)

Fill it out with the idea, constraints, and non-goals.

---

## 3) Gemini Phase (Architect)
1. Launch Gemini CLI
2. Paste `GEMINI_SESSION_OPEN.md`
3. Provide the task directory path and supporting docs
4. Gemini must not implement code or modify source files during this phase.

Expected outputs:
- Required planning documents
- A `TASK_PACKET.md` derived from those documents

Version control:
- Commit the packet in **Draft** status
- After human review, a human marks `Ready for Build` and commits again

---

## 4) Codex Phase (Builder)
1. Launch Codex CLI
2. Paste `CODEX_SESSION_OPEN.md`
3. Provide the task directory path

Expected behavior:
- Implementation follows `TASK_PACKET.md`
- `TASK_PACKET_SUMMARY.md` is updated using the standard template
- If Codex stops due to ambiguity or rule violations, do not override it—fix the documentation first

Verify that lifecycle state was not modified during execution.

---

## 4.5) Claude Review Phase (Non-Authoritative, Optional)

This phase provides an additional **contract compliance and risk review**
prior to Human Verification.

Claude operates strictly under:
- `AI_SESSION/INSTRUCTIONS/CLAUDE_REVIEW_AGENT.md`

When this phase is used:

1. Launch Claude
2. Load the Claude Review Agent instructions
3. Provide **only**:
    - TASK_PACKET.md
    - TASK_PACKET_SUMMARY.md (if present)
    - Git diff produced by Codex
    - Source files explicitly listed in the Exact File Touch List

Claude must not be provided:
- Discovery or planning documents
- Repository-wide context
- Files not explicitly authorized by the task packet

Expected outputs:
- Contract compliance findings
- Scope or file-boundary violations
- Correctness, lifecycle, or platform risk findings
- Acceptance test gaps or ambiguities

Claude is advisory only.

If Claude reports:
- Ambiguity in TASK_PACKET.md
- Violations of Constraints or Non-Goals
- Unauthorized file modifications

Then:
- Stop
- Do not proceed to Human Verification
- Correct documentation or classify follow-up work before continuing

Claude does not:
- Modify files
- Change lifecycle state
- Authorize corrective work

---

## 5) Human Verification
Run acceptance tests and/or manual checks defined in `TASK_PACKET.md`.

Record outcomes in TASK_PACKET_SUMMARY.md or supporting artifacts referenced from it.

---

## 6) Gemini Review & Follow-up
Provide Gemini with:
- Test results
- Failure evidence (if any)
- The execution summary

If corrective work is required, ensure it is classified and created according to:
- `INSTRUCTIONS/BUGTASKS.md`
- `INSTRUCTIONS/CHILD_WORK_RULES.md`

Do not modify the parent task packet unless explicitly permitted by those rules.

---

## 7) Lifecycle Control (Human Only)
Lifecycle state changes are performed by humans and recorded in `TASK_PACKET_SUMMARY.md`.

Execution status does not automatically change lifecycle state.
