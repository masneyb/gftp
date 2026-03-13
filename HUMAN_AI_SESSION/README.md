# AI_SESSION

This directory is the **authoritative control surface** for how AI tools are used in this repository.

All AI-assisted work is governed by the files in this directory.
No AI behavior, workflow, or task state is authoritative unless it is defined here.

---

## What This Directory Is

`AI_SESSION/` contains:

- Human-authored rules for using AI tools
- Agent instruction documents (by role)
- Workflow state definitions
- Templates and contracts required before execution

These files are:
- Versioned via Git
- Tool-agnostic by design
- The single source of truth for AI usage rules

---

## What This Directory Is Not

- It does **not** contain implementation code
- It does **not** contain task-specific work
- It does **not** store execution state
- It does **not** rely on chat or AI memory

If something is not written to a file, it does not exist.

---

## How to Navigate

- **Human operating rules**  
  → `HOW_WE_USE_AI.md`

- **Step-by-step human procedures**  
  → `RUN_BOOK.md`

- **Agent instructions and workflow rules**  
  → `INSTRUCTIONS/`

- **Reusable templates (TASK_PACKET, worksheets, etc.)**  
  → `TEMPLATES/`

---

## Authority Model (Summary)

- Humans decide scope, priority, and approval
- AI tools assist only within written contracts
- Execution never begins without an approved `TASK_PACKET.md`
- If instructions are unclear or conflicting, AI-assisted work must stop until clarified in files

Details are defined in the files above; this README is navigational only.

---

## Change Discipline

Changes to files in `AI_SESSION/` affect how **all AI work** in this repository operates.

Treat edits here as governance changes, not convenience tweaks.

A small change here can invalidate prior assumptions across multiple tasks.
