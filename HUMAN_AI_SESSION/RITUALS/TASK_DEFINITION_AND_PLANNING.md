# Ritual: Task Definition and Planning

This ritual describes the process of defining a development task and generating an execution plan, designed to be model-agnostic.

## Objective
To translate a human requirement into an actionable development plan, suitable for execution by an AI agent.

## Workflow

### Phase 1: Task Definition

This phase focuses on clearly articulating what needs to be done.

1.  **Input Source:**
    *   **Human Idea Brief (`HUMAN_IDEA_BRIEF.md`):** For high-level, initial ideas. This is a free-form description of the desired outcome.
    *   **Intent Specification (`INTENT_SPEC.md`):** For more detailed, structured requirements, often translated from a brief by an intent-translation agent.

2.  **Contextualization:**
    *   The AI system uses `PROJECT_CONTEXT.md` to understand the existing codebase, architecture, and conventions.

3.  **Clarification (if needed):**
    *   If the input is ambiguous, the system may prompt the human user for clarification.

### Phase 2: Planning

Once the task is understood, a plan is generated.

1.  **Plan Generation:**
    *   An AI agent (e.g., Gemini) analyzes the defined task and project context.
    *   It breaks down the task into smaller, manageable steps.
    *   It identifies necessary code changes, new files, and tests.
    *   It considers adherence to project conventions and best practices.

2.  **Plan Output:**
    *   The generated plan is typically documented internally or presented to the human for review. For complex tasks, it MUST be formalized in a `TASK_PACKET.md` (and related planning files) using the strict templates in `HUMAN_AI_SESSION/TEMPLATES/`. The agent MUST actually create these files on disk.

## Outputs
- A clear, documented task definition (either `HUMAN_IDEA_BRIEF.md` or `INTENT_SPEC.md`).
- An actionable development plan.

## Considerations
- The choice between `HUMAN_IDEA_BRIEF.md` and `INTENT_SPEC.md` as the primary input depends on the stage of the project and the availability of intent translation.
- The planning agent should prioritize extensibility, maintainability, and testability in its plans.
- Human review of the plan might be an optional step depending on the task's complexity and criticality.
