# Ritual: Copilot Assist

This ritual defines how GitHub Copilot functions as an AI assistant within the development environment.

## Agent Role
*   **Assistant:** GitHub Copilot (Operates within IDE, not a direct participant in file-based rituals like others).

## Objective
To accelerate development by providing real-time, context-aware code suggestions and autocompletions directly within the Integrated Development Environment (IDE).

## Workflow

1.  **IDE Integration:**
    *   Copilot operates as a plugin or feature within the developer's IDE.
    *   It analyzes the code currently being written or edited.

2.  **Suggestion Generation:**
    *   Based on the context (code, comments, surrounding files), Copilot generates suggestions for code snippets, lines, or entire functions.

3.  **Developer Interaction:**
    *   **Human Developer:** The Human developer can accept, reject, or modify Copilot's suggestions as they type.
    *   **Gemini (as IDE user):** When Gemini is operating within an IDE environment that has Copilot enabled, it can leverage Copilot's suggestions as part of its implementation process.

## IO Contract
*   **Allowed Writes:** None directly. Copilot provides suggestions, which are then accepted or rejected by the user.
*   **Required Reads:** Current code context within the IDE.

## Role in Fallback/Handoffs
*   Copilot is not a primary agent with complex fallback chains. It is an **enhancement** to the primary implementation agent (Gemini) or the Human developer.
*   Its "fallback" is essentially the developer's ability to ignore suggestions and write code manually or request different suggestions.
*   It facilitates the **Gemini Execution Ritual** by speeding up the implementation phase.
