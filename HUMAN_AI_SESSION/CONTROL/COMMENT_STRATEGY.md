# COMMENT_STRATEGY.md

Purpose: Define the canonical in-code commenting style for this codebase.
This standard exists to make human review fast, consistent, and durable.

Scope:
- Applies to all production source code.
- Tests and scripts should follow the same structure where it makes sense,
  but may be lighter.

Non-goals:
- Enforcing numeric comment density
- Replacing good naming and structure with comments
- Forcing language-specific doc tooling (unless explicitly adopted)

---

## 0) Style Guarantees (Must Hold)

- A human must be able to read a file top-to-bottom and understand:
    - what it does,
    - why it exists,
    - what it does NOT do,
    - any important assumptions / edge cases.
- Comments must reflect reality. If code changes, comments must be updated.

---

## 1) File Header Block (Required)

Every production file MUST begin with a header block using this exact shape.

Template:

/ ==========================================================================
// Project: <PROJECT_OR_REPO_NAME>
// File: <RELATIVE_PATH_FROM_REPO_ROOT>
// Purpose: <ONE_LINE_PURPOSE>
// Created: <MONTH YYYY>
// Updated: <MONTH YYYY | blank if none>
// Branch: <BRANCH_NAME | last_modified_branch | optional if unknown>
// ==========================================================================

Rules:
- Keep Purpose to a single line.
- Updated must be filled when meaningful behavior changes are made.

---

## 2) Imports Section (Required Convention)

After the file header, include the import list in a dedicated block.

Template:

<
List of Imports
>

Rules:
- Do not interleave imports with other code.
- Maintain existing repository ordering conventions (stdlib, third-party, internal),
  if they exist.

---

## 3) File-Level Context Doc Comment (Required)

Immediately after imports, add a file-level context comment.

This is the â€œreader orientationâ€ layer: it should explain intent, boundaries,
and high-level behavior.

Template:

/// <Short Title>
///
/// **PURPOSE**: <What this file is responsible for>
///
/// **SCOPE / BOUNDARIES**:
/// - In scope: <items>
/// - Out of scope: <items>
/// - Assumptions: <items>
/// - Invariants: <items>
///
/// **KEY BEHAVIORS**:
/// - <behavior 1>
/// - <behavior 2>
/// - <behavior 3>
///
/// **EDGE CASES**:
/// - <edge case 1>
/// - <edge case 2>
///
/// **NOTES**:
/// - <notes that reduce reviewer uncertainty>

Rules:
- Avoid marketing language and external brand references.
- Keep it factual and review-oriented.
- If â€œKEY BEHAVIORSâ€ or â€œEDGE CASESâ€ donâ€™t apply, include the heading
  with a single bullet like â€œ- None documentedâ€.

---

## 4) Section Dividers (Required)

Use explicit section banners to organize non-trivial files.

Template:

// ==========================================================================
// SECTION <LETTER>: <SECTION NAME>
// ==========================================================================

Rules:
- Use letters A, B, C... in top-to-bottom order.
- Keep section names descriptive (pattern + purpose), not vague.
- Group related members/functions together under a section.

---

## 5) Class / Module Commenting

For any public class/module, include:
- a short doc comment (what/why),
- any invariants,
- key responsibilities.

Template:

/// <What this class/module is>
///
/// Responsibilities:
/// - <responsibility 1>
/// - <responsibility 2>
///
/// Invariants:
/// - <invariant 1>

---

## 6) Function / Method Comments (Required When Non-Obvious)

A function/method MUST have a doc comment when any apply:
- non-trivial branching or multi-step flow
- side effects (IO, persistence, network, state mutation)
- subtle invariants or constraints
- error handling that matters for behavior
- performance tradeoffs or retries/backoff/queues/etc.

Template:

/// <Verb phrase summary: what it does>
/// <Optional: one sentence why it exists>
///
/// Behavior:
/// - <step/behavior 1>
/// - <step/behavior 2>
///
/// Inputs:
/// - <input notes only if not obvious from signature>
///
/// Outputs:
/// - <output notes only if not obvious>
///
/// Side effects:
/// - <side effects>
///
/// Failure modes:
/// - <expected failures and how handled>
///
/// Assumptions / invariants:
/// - <assumptions>

Rules:
- Prefer â€œwhyâ€ + â€œwhatâ€ over narrating syntax.
- If the function is tiny and obvious, a comment is optional.

---

## 7) Inline Comments (Use Sparingly, But For â€œWhyâ€)

Inline comments are encouraged for:
- tricky conditionals
- â€œmagic valuesâ€ (explain provenance)
- non-obvious ordering requirements
- workarounds and compatibility constraints

Avoid:
- restating the code
- narrating obvious lines

---

## 8) TODO / FIXME / NOTE Tags (Standardized)

Allowed tags:
- TODO: Deferred enhancement (include reason)
- FIXME: Known incorrect behavior (include risk)
- NOTE: Non-obvious constraint or rationale

Each tag MUST include:
- short reason
- reference to task/bug identifier if available (or â€œuntrackedâ€)

Examples:
- TODO(<id>): <reason>
- FIXME(<id>): <risk/impact>
- NOTE: <constraint + why>

---

## 9) Enforcement Notes

- This comment strategy is part of the execution contract.
- If code is implemented or modified without meeting this strategy,
  the work is considered incomplete and may be rejected for review.

Time Created: <YYYY-MM-DD HH:MM:SS>
Time Modified: <YYYY-MM-DD HH:MM:SS>