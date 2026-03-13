
### Prohibited behavior

- Subtasks MUST NOT supersede parent task packets
- Parent `TASK_PACKET.md` files are immutable once implementation begins
- Subtasks MUST NOT modify parent objectives

---

## Tool Responsibilities

### Gemini (Planner / Document Author)

Gemini MUST:

- Apply the **Cascading Issues** triage gate before creating subtasks
- Create a subtask only when criteria in this document are met
- Place the subtask in the correct parent directory
- Generate a scoped `TASK_PACKET.md` for the subtask
- Preserve the parent task packet unchanged

If uncertain whether an issue belongs to a parent task:
- Gemini MUST assume it **is a subtask**
- Gemini MUST ask for confirmation before creating a new parent task

---

### Codex (Builder / Executor)

Codex MUST:

- Refuse to implement bug fixes without a subtask `TASK_PACKET.md`
- Stop execution if asked to modify a parent task directly for a bug fix
- Enforce subtask scope boundaries strictly

Codex MUST NOT:

- Modify parent `TASK_PACKET.md` files
- Implement fixes based on chat instructions alone
- Expand scope beyond the subtask objective

---

## Human Responsibilities

Humans MUST:

- Use subtasks for bug fixes tied to existing work
- Avoid reopening parent task packets unless explicitly required
- Approve any conversion of a subtask into a new parent task

Creating a new parent task for a bug fix **without approval** is a process violation.

---

## Immutability & Traceability

- Parent tasks represent **intent**
- Subtasks represent **correction**
- History must remain linear and auditable

Subtasks **add clarity** — they do not rewrite history.

---

## Enforcement Rule

Any implementation, documentation, or execution that violates this specification
is considered **invalid** and must be corrected before work may proceed.

---

## Reference Policy

Other instruction files MUST reference this document rather than redefining
subtask behavior.

Recommended reference text:

