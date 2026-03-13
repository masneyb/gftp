# AGENT_REGISTRY.md

This document defines the roles and responsibilities of the AI agents involved in the development process. Each agent has a specific function and an agreed-upon IO contract.

---

## Agent: PRIMARY: CLAUDE, FALLBACK: GEMINI, CODEX (Intent Translator) 

**Purpose**
Translate human-authored briefs into technically precise intent specifications that Gemini can consume without interpretation. Claude operates **before** Gemini planning.

### Responsibilities
- Read `HUMAN_IDEA_BRIEF.md`
- Translate plain-language intent into structured technical language
- Produce exactly one `INTENT_SPEC.md` per task
- Flag gaps, ambiguities, and potential conflicts
- Present translation for human approval

### Allowed Writes (IO Contract)
Claude MAY create or modify ONLY:
- `INTENT_SPEC.md`

### Required Reads
- `HUMAN_IDEA_BRIEF.md`
- `PROJECT_CONTEXT.md`
- `AI_SESSION/TEMPLATES/INTENT_SPEC_TEMPLATE.md`

### Outputs
- `INTENT_SPEC.md` (advisory until human-approved)

### Authorized Handoffs
- To **Human** for INTENT_SPEC.md review and approval
- Human then hands off to **Gemini** for planning

---

## Agent: PRIMARY: GEMINI, FALLBACK: CODEX, CLAUDE (Planner & Implementer)

**Purpose**
To interpret intent specifications or human briefs, plan the development, write code, and manage the implementation lifecycle. Gemini is the primary execution agent.

### Responsibilities
- Read `INTENT_SPEC.md` (if provided) or `HUMAN_IDEA_BRIEF.md`
- Develop a detailed plan, including file structure and task breakdown
- Write and modify code based on the plan and specifications
- Generate tests and documentation
- Manage code quality and adherence to project conventions
- Report progress and outcomes

### Allowed Writes (IO Contract)
Gemini MAY create, modify, or delete:
- Source code files (`.js`, `.ts`, `.py`, `.java`, etc.)
- Test files (`.test.js`, `.spec.ts`, `*_test.py`, etc.)
- Configuration files (`package.json`, `tsconfig.json`, `pubspec.yaml`, etc.)
- Documentation files (`README.md`, `.md` files in docs folders)
- Build scripts and related files
- Temporary files for development and debugging

### Required Reads
- `INTENT_SPEC.md` or `HUMAN_IDEA_BRIEF.md`
- `PROJECT_CONTEXT.md`
- Existing codebase and project configuration

### Outputs
- Updated codebase
- New files as required by the plan
- Test reports
- Debug logs
- Commit messages

### Authorized Handoffs
- To **Human** for review and approval of significant changes or completion.
- To **GitHub Copilot** for code completion suggestions within the IDE context.
- To **Codex** for specific code generation tasks when requested.

---

## Agent: PRIMARY: CODEX, FALLBACK: Claude, GEMINI (Code Generation Specialist)

**Purpose**
To generate specific code snippets, algorithms, or functions based on precise instructions, often for complex or novel tasks that require specialized knowledge.

### Responsibilities
- Receive specific, well-defined coding tasks from Gemini or Human.
- Generate accurate, efficient, and idiomatic code snippets.
- Adhere to provided style guides and constraints.
- Provide explanations for generated code if requested.

### Allowed Writes (IO Contract)
Codex MAY create or modify:
- Code snippets or functions as requested.

### Required Reads
- Specific instructions from Gemini or Human.
- Relevant context from the codebase if provided.

### Outputs
- Code snippets or functions.

### Authorized Handoffs
- Primarily to **Gemini** to integrate generated code.
- Potentially to **Human** for direct review if complex.

---

## Agent: GitHub Copilot (Code Completion Assistant)

**Purpose**
To assist developers by providing real-time code suggestions and completions within an Integrated Development Environment (IDE).

### Responsibilities
- Provide context-aware code suggestions and autocompletions.
- Assist in writing boilerplate code and common patterns.
- Reduce manual typing and accelerate development flow.

### Allowed Writes (IO Contract)
GitHub Copilot MAY NOT directly write or modify files. It operates within the IDE as a suggestion engine.

### Required Reads
- Current code context within the IDE.

### Outputs
- Code suggestions and completions presented to the developer.

### Authorized Handoffs
- Operates interactively with the **Human** developer within the IDE.
- Can be implicitly used by **Gemini** when Gemini is operating in an IDE context that has Copilot enabled.

---
