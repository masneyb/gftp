# AI Workflow Runbook (Human Steps)

This runbook describes the standard human-driven process for running an AI-assisted task.

## 0) Preconditions
- You are on a branch (recommended for any non-trivial task)
- AI_SESSION_INSTRUCTIONS/ is up to date in your branch

---

## 1) Create a Task Directory
Create a new numbered task directory under:

docs/current_task_documentation/<NN-Task-Name>/

Example:
docs/current_task_documentation/22-Advertisement-Integration/

Commit directory creation if you want early traceability (optional).

Bug fixes discovered during testing MUST be implemented via subtasks as defined in AI_SESSION_INSTRUCTIONS/SUBTASKS.md.


---

## 2) Create the Human Idea Brief (Optional but Recommended)
Copy the template into the task directory:

- HUMAN_IDEA_BRIEF.md (from AI_SESSION_INSTRUCTIONS/)

Fill it out with the idea/problem and constraints.

---

## 3) Gemini Phase (Architect)
1) Launch Gemini CLI
2) Paste `AI_SESSION_INSTRUCTIONS/GEMINI_SESSION_OPEN.md`
3) Provide the task directory path (and any brief/docs)

Gemini produces:
- Required planning docs (minimum set)
- TASK_PACKET.md (derived from those docs)

### Version Control
- Commit TASK_PACKET.md immediately in Draft status
- After human review, mark “Ready for Build” and commit again

---

## 4) Codex Phase (Builder)
1) Launch Codex CLI
2) Paste `AI_SESSION_INSTRUCTIONS/CODEX_SESSION_OPEN.md`
3) Provide the task identifier/path

Codex:
- Locates TASK_PACKET.md
- Implements exactly what is specified
- Updates TASK_PACKET_SUMMARY.md using the summary template
- Sets task State: WIP / Completed / Blocked

---

## 5) Human Verification
Run the acceptance tests and/or manual checks defined in TASK_PACKET.md.

Record outcomes:
- If all pass → proceed to review
- If issues found → capture details for Gemini

---

## 6) Gemini Review & Follow-up
Paste back to Gemini:
- Test output
- Any failures / logs / screenshots
- Codex summary (TASK_PACKET_SUMMARY.md)

Gemini:
- Reviews implementation against TASK_PACKET.md
- Updates documentation as needed
  If bugs or defects are discovered:

- Gemini MUST create a subtask under the original task directory:
  docs/current_task_documentation/<parent-task>/subtasks/

- Gemini creates:
    - A new subtask directory (S1, S2, ...)
    - A dedicated TASK_PACKET.md for the fix

- The original task directory is NOT modified
- The original TASK_PACKET.md scope remains frozen

Only create a new top-level task if the issue is:
- Unrelated to the original task, or
- Explicitly approved by a human

