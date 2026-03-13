# Ritual: Session Initiation

This ritual defines the standard procedure for initiating a new AI-assisted development session.

## Objective
To establish a clear starting point for any new development task or feature request, ensuring all necessary context and initial inputs are gathered.

## Steps

1.  **Define the Project Context:** Ensure `PROJECT_CONTEXT.md` is up-to-date and accessible. This file provides foundational information about the project's architecture, existing technologies, and conventions.
2.  **Identify the Task:** Clearly articulate the goal of the session. This could be a new feature, a bug fix, refactoring, or exploration.
3.  **Prepare Initial Input:**
    *   If starting with a high-level idea, create or update `HUMAN_IDEA_BRIEF.md`.
    *   If starting with a more refined specification, ensure `INTENT_SPEC.md` is available and approved by a human.
4.  **Select the Primary Agent:** While the system aims for model agnosticism, an initial agent might be designated to interpret the first input (e.g., Claude for translating briefs to intent specs). The system will orchestrate agent handoffs as needed.
5. **Follow** HUMAN_AI_SESSION/CONTROL/ROUTING_RULES.md
5.  **Initiate the Workflow:** Begin the process by feeding the initial input to the designated agent or system entry point.

## Outputs
- A running AI session.
- Prepared initial input files (`HUMAN_IDEA_BRIEF.md` or `INTENT_SPEC.md`).

## Considerations
- Always ensure the project's temporary directory is clean or managed appropriately before starting a new session.
- Review `AGENT_REGISTRY.md` to understand the roles of available agents.
