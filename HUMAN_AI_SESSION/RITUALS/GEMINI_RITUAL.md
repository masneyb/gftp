# Ritual: GEMINI AI Session

This ritual details the general workflow for the GEMINI agent during an AI-assisted session.

## Agent Role
*   **Primary:** GEMINI
*   **Role Definition:** Gemini's specific role for a given session is defined entirely by the `_SESSION_OPEN.md` file provided at the start of the session and the corresponding document in `HUMAN_AI_SESSION/ROLES/`.

## Objective
To execute the instructions provided in the `_SESSION_OPEN.md` file, assuming the specified role and adhering to all constraints.

## Workflow

1.  **Session Initialization:**
    *   Gemini receives the appropriate `_SESSION_OPEN.md` file.
    *   Gemini reads its assigned role definition from `HUMAN_AI_SESSION/ROLES/`.
    *   It loads `PROJECT_CONTEXT.md` to understand the environment.

2.  **Execution:**
    *   Gemini executes the directives listed in the `_SESSION_OPEN.md` file.
    *   Gemini strictly adheres to the templates in `HUMAN_AI_SESSION/TEMPLATES/` and actually creates/updates files on disk as instructed.

3.  **Handoff:**
    *   Gemini halts and hands back to the Human user upon completing the directives or encountering a hard stop (missing files, ambiguous instructions).

## IO Contract
*   **Allowed Writes:** Defined by the `_SESSION_OPEN.md` file.
*   **Required Reads:** `PROJECT_CONTEXT.md`, `_SESSION_OPEN.md`, and any files specified in the session instructions.
