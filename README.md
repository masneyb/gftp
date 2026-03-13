# AI Session Instructions

This directory contains the **authoritative instructions** for using AI tools in this repository.

These files define **process, roles, and contracts**.
They are versioned via Git and apply to all contributors.

---

## Start Here

1. Read **HOW_WE_USE_AI.md** (one-page overview)
2. Choose the appropriate AI tool:
    - **Gemini CLI** → use `AI_SESSION_INSTRUCTIONS/GEMINI_SESSION_OPEN.md`
    - **Codex CLI** → use `AI_SESSION_INSTRUCTIONS/CODEX_SESSION_OPEN.md`
3. For each new task:
    - Use `AI_SESSION_INSTRUCTIONS/3TASK_PACKET_TEMPLATE.md` to create a **TASK_PACKET.md**
    - Place the TASK_PACKET.md inside the appropriate task directory under:
      ```
      docs/current_task_documentation/<task-path>/
      ```

---

## Workflow Rules (Non-Negotiable)

- **Gemini (Architect)**:
    - Reads project context and task documentation
    - Produces or updates TASK_PACKET.md using the template
    - Does NOT write implementation code

- **Human Review**:
    - Reviews TASK_PACKET.md for clarity and scope
    - Marks status as **Ready for Build** before implementation begins

- **Codex (Builder)**:
    - Implements **exactly** what is in TASK_PACKET.md
    - Touches only files listed in the packet
    - Stops immediately if instructions are unclear or incomplete

All subtask creation, scope, and enforcement rules are defined in AI_SESSION_INSTRUCTIONS/SUBTASKS.md.


---

## Source of Truth

- **TASK_PACKET.md** is the single source of truth for implementation
- If instructions conflict, **Codex must stop**
- No implementation begins without an approved TASK_PACKET.md

---

## Backup & Sync Note

Files in this directory are versioned via **Git** and are the authoritative source
for AI usage rules.

**Do not copy or duplicate these files into OneDrive.**
