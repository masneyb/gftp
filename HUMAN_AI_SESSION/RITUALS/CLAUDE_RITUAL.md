# Ritual: CLAUDE AI Session

This ritual defines the general process for the CLAUDE agent during an AI-assisted session.

## Agent Role
*   **Primary:** CLAUDE
*   **Role Definition:** Claude's specific role for a given session is defined entirely by the `_SESSION_OPEN.md` file provided at the start of the session and the corresponding document in `HUMAN_AI_SESSION/ROLES/`.

## Objective
To execute the instructions provided in the `_SESSION_OPEN.md` file, assuming the specified role and adhering to all constraints.

## Workflow

1.  **Session Initialization:**
    *   Claude receives the appropriate `_SESSION_OPEN.md` file.
    *   Claude reads its assigned role definition from `HUMAN_AI_SESSION/ROLES/`.

2.  **Execution:**
    *   Claude executes the directives listed in the `_SESSION_OPEN.md` file.
    *   Claude strictly adheres to the templates in `HUMAN_AI_SESSION/TEMPLATES/` and actually creates/updates files on disk as instructed.

3.  **Handoff:**
    *   Claude halts and hands back to the Human user upon completing the directives or encountering a hard stop (missing files, ambiguous instructions).

## IO Contract
*   **Allowed Writes:** Defined by the `_SESSION_OPEN.md` file.
*   **Required Reads:** `_SESSION_OPEN.md`, `PROJECT_CONTEXT.md`, and any files specified in the session instructions.
