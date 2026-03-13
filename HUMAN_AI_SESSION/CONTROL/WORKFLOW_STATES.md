# WORKFLOW STATES (Authoritative)

This document defines the **only valid state values** for task governance and routing.

If a value is not listed here, it is **invalid** and must not be used.
All task state recorded in `{task_dir}/TASK_PACKET_SUMMARY.md` MUST conform to this file.

---

## LIFECYCLE STATES (Human-Controlled)

Lifecycle states describe whether a task is **allowed to proceed at all**.
They are set **only by humans** and change infrequently.

### Active
- Meaning: The task is approved and allowed to proceed.
- Who sets: Human
- Notes: Most work occurs while lifecycle is Active.

### Paused
- Meaning: The task is intentionally paused.
- Who sets: Human
- Required: Reason must be provided.
- Notes: Execution must not continue while paused.

### Blocked
- Meaning: The task cannot proceed due to an external dependency.
- Who sets: Human
- Required: Blocking reason must be provided.

### Completed
- Meaning: The task is fully complete and closed.
- Who sets: Human
- Notes: No further work is permitted under this task.

### Abandoned
- Meaning: The task has been intentionally discarded.
- Who sets: Human
- Required: Reason must be provided.
- Notes: No further work is permitted.

---

## EXECUTION STATES (Task State System)

Execution states describe **where the task is in the workflow**.
They are used by routing logic and recorded in `{task_dir}/TASK_PACKET_SUMMARY.md`.

The following values are the **only valid execution states**.

### WIP
- Meaning: Implementation or active work is in progress.
- Typical use: A specialized engineer is implementing or changes are underway.
- Routing: Eligible for execution.

### Under Review
- Meaning: Implementation is complete and awaiting human review.
- Typical use: Tests run, summary updated, awaiting approval.
- Routing: Not eligible for further execution.

### Blocked
- Meaning: Execution cannot proceed due to missing information or dependency.
- Typical use: Awaiting clarification, decision, or external input.
- Routing: Not eligible for execution.

### Completed
- Meaning: All execution work is finished.
- Typical use: Task ready for lifecycle completion.
- Routing: Terminal execution state.

---

## STATE INVARIANTS (Non-Negotiable)

- Lifecycle and Execution State are independent but must not contradict.
- `Lifecycle: Paused` or `Lifecycle: Blocked` implies `Execution State` MUST be `Blocked`.
- `Lifecycle: Completed` implies `Execution State: Completed`.
- Execution State MUST be recorded in `{task_dir}/TASK_PACKET_SUMMARY.md`.
- Only values defined in this file are valid.

---

## REFERENCES

- Routing rules: `AI_SESSION/CONTROL/ROUTING_RULES.md`
- State transitions: `AI_SESSION/CONTROL/WORKFLOW_STATE_MACHINE.md`
- Execution record: `{task_dir}/TASK_PACKET_SUMMARY.md`