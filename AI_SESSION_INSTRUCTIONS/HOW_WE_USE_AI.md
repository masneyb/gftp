# How We Use AI on This Project

This repository uses AI tools as **assistants**, not autonomous agents.

There is **no single orchestrator** and no AI tool works independently.
A human is always responsible for coordination, review, and approval.

This document explains **how AI is used safely and consistently** by the team.

---

## AI Roles (Strict Separation)

### 🧠 Gemini CLI — Architect
Gemini is used for **thinking, planning, and documentation**.

Gemini is responsible for:
- Understanding the system and task context
- Performing discovery and analysis
- Writing or updating planning documents
- Creating `TASK_PACKET.md` for each task

Gemini must:
- Read `PROJECT_CONTEXT.md`
- Follow `AI_SESSION_INSTRUCTIONS/GEMINI.md`
- Use `TASK_PACKET_TEMPLATE.md` when creating packets

Gemini must NOT:
- Write implementation code
- Modify source files
- Expand scope without documenting it

---

### 🛠️ Codex CLI — Builder
Codex is used for **implementation only**.

Codex is responsible for:
- Implementing exactly what is specified in `TASK_PACKET.md`

Codex must:
- Follow `AI_SESSION_INSTRUCTIONS/CODEX.md`
- Locate the correct `TASK_PACKET.md` under:
  ```
  docs/current_task_documentation/**
  ```
- Touch ONLY files listed in the packet
- Stop immediately if anything is unclear or incomplete

Codex must NOT:
- Infer requirements from other documentation
- Refactor beyond the Task Packet
- Change metric logic unless explicitly instructed

---

## The TASK_PACKET Contract

Every task MUST have a `TASK_PACKET.md`.

- Created by **Gemini**
- Reviewed and approved by a **human**
- Executed by **Codex**

Implementation must not begin until:
- A `TASK_PACKET.md` exists
- Its Status is marked **Ready for Build**

`TASK_PACKET.md` is the **single source of truth** for implementation.

---

## Standard Workflow

1. Gemini analyzes the problem and existing docs
2. Gemini creates or updates `TASK_PACKET.md`
3. A human reviews and approves the packet
4. Codex implements exactly what the packet specifies
5. Gemini reviews results and issues patch lists if needed


Bug fixes, regressions, and follow-up work MUST follow the subtask process defined in AI_SESSION_INSTRUCTIONS/SUBTASKS.md.

Skipping steps leads to unreliable results.

---

## Session Start Rule (Mandatory)

Every AI session MUST begin by pasting the appropriate session opening prompt:

- **Gemini CLI:** `AI_SESSION_INSTRUCTIONS/GEMINI_SESSION_OPEN.md`
- **Codex CLI:** `AI_SESSION_INSTRUCTIONS/CODEX_SESSION_OPEN.md`

If this step is skipped, the output is considered non-authoritative.

---

## Source of Truth Hierarchy

If instructions conflict:

1. `TASK_PACKET.md` (for implementation)
2. `GEMINI.md` / `CODEX.md` (role behavior)
3. `PROJECT_CONTEXT.md` (system facts)
4. `AGENTS.md` (workflow rules)

When in doubt, the Builder must stop.

---

## Guiding Principle

> AI assists decision-making and execution.  
> Humans retain responsibility for correctness, scope, and quality.
> For day-to-day usage steps, see AI_SESSION_INSTRUCTIONS/RUNBOOK.md
