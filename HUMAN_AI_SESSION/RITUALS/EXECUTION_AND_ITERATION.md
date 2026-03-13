# Ritual: Execution and Iteration

This ritual defines the core loop for executing development tasks and iterating based on feedback and testing. It is designed to be model-agnostic.

## Objective
To implement a defined task, verify its correctness, and refine it through iterative development.

## Core Loop

The execution process follows a cycle: Plan -> Implement -> Test -> Review -> Refine.

### 1. Implementation

*   **Action:** An AI agent (e.g., Gemini) uses the development plan to write or modify code.
*   **Constraints:** Adhere strictly to `PROJECT_CONTEXT.md` and conventions.
*   **Tools:** Utilize code generation, file manipulation, and potentially code completion assistants (like GitHub Copilot).

### 2. Testing

*   **Action:** Execute relevant tests to verify the implemented changes.
*   **Identification:** The system should identify appropriate test commands and frameworks based on project configuration (e.g., `package.json`, `build.gradle.kts`).
*   **Verification:** Tests must pass to proceed. Failures indicate a need for refinement.

### 3. Review

*   **Action:** Present the implemented changes and test results for review.
*   **Reviewers:** This can be a human user, or in some cases, another AI agent designated for review.
*   **Feedback:** Collect feedback, identify bugs, or note areas for improvement.

### 4. Refinement (Iteration)

*   **Action:** Based on test failures or review feedback, the implementation is revised.
*   **Process:** The cycle returns to Step 1 (Implementation) with the new feedback incorporated into the plan or directly applied as code modifications.
*   **New Implementation:** For significant changes, a new implementation cycle begins. For minor adjustments, direct modification and re-testing may suffice.

## Outputs
- Implemented code that meets task requirements.
- Passing test suites.
- Updated documentation.
- A stable, verified code change.

## Considerations
- The iteration loop should continue until the task is successfully completed and approved.
- Each iteration should be atomic and well-defined.
- Error handling and reporting are critical parts of this loop.
- The system should maintain a record of changes and iterations.
