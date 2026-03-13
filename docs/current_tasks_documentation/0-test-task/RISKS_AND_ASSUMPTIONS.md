# RISKS AND ASSUMPTIONS

Purpose:  
Explicitly surface uncertainties that could affect correctness, scope,
timeline, or stability.

**Authoritative Location**: `docs/current_tasks_documentation/0-test-task/RISKS_AND_ASSUMPTIONS.md`

This document exists to:
- Prevent silent failure modes
- Make fragility visible early
- Enable informed go / no-go decisions

It does NOT define requirements or implementation plans.

---

## Risks

List concrete, credible risks.

A risk is:
- A future event or condition
- That may occur
- And would negatively affect the task if it does

For each risk, include:
- Description: What could go wrong
- Impact: Low / Medium / High
- Likelihood: Low / Medium / High
- Mitigation or monitoring plan: How the risk will be reduced, detected, or revisited

- **Risk**: GTK's icon theme system on macOS does not recognize or properly load icons from the chosen bundle location, even if assets are correctly placed.
    - **Impact**: High
    - **Likelihood**: Medium
    - **Mitigation or monitoring**: Extensive testing during implementation. If initial attempts fail, research GTK's icon lookup paths on macOS more deeply and experiment with alternative bundling structures or GTK environment variables. Potentially, a custom icon loading path might be required in gFTP's source.
- **Risk**: The chosen `.xpm` to `.png`/`.svg` conversion tool (e.g., `sips`, `ImageMagick`) introduces visual artifacts or quality degradation in the converted icons.
    - **Impact**: Medium
    - **Likelihood**: Low
    - **Mitigation or monitoring**: Visual inspection of converted icons for all sizes. If issues arise, explore alternative conversion tools or parameters.
- **Risk**: Modifications to build scripts (`build_and_bundle_gftp.sh`, `meson.build`) for icon handling break existing build processes for other platforms (e.g., Linux).
    - **Impact**: High
    - **Likelihood**: Medium
    - **Mitigation or monitoring**: Rigorous testing of the build process on both macOS and Linux. Use conditional logic in scripts/Meson files to apply macOS-specific icon handling only when building for macOS.
- **Risk**: The gFTP codebase contains hardcoded references to `.xpm` files or specific icon paths that are difficult to find and update, leading to incomplete icon display.
    - **Impact**: Medium
    - **Likelihood**: Medium
    - **Mitigation or monitoring**: Thorough code search (grep for `.xpm`, `gdk_pixbuf_new_from_file`, etc.) during discovery/implementation. Use of GTK's abstract icon naming (e.g., `gtk_image_new_from_icon_name`) where possible instead of direct file paths.
- **Risk**: The existing icon assets in `icons/` (e.g., `16x16/apps/`) are incomplete or do not cover all necessary file/folder types, even with successful conversion.
    - **Impact**: Low
    - **Likelihood**: Medium
    - **Mitigation or monitoring**: Visual inspection during testing will highlight missing icons. If identified, new icons may need to be sourced or created. This could expand scope.

---

## Assumptions

List assumptions that must hold true for this task to succeed.

If an assumption is false:
- Requirements may be invalid
- Scope may change
- Rework may be required

For each assumption, include:
- Assumption:
- Rationale: Why this is believed to be true
- Impact if incorrect:

Assumptions here must align with (but do not replace)
assumptions listed in REQUIREMENTS.

- **Assumption**: GTK's icon theme system on macOS is capable of locating and loading icons from standard bundle locations (e.g., `gFTP.app/Contents/Resources/share/icons/hicolor/`).
    - **Rationale**: This is a standard pattern for bundling resources in macOS applications that use cross-platform toolkits like GTK.
    - **Impact if incorrect**: Significant additional research and potentially a custom icon loading mechanism within gFTP would be required, or a non-standard bundling approach.
- **Assumption**: The `meson.build` system and existing shell scripts (`build_and_bundle_gftp.sh`, `packaging/macos/create_app_bundle.sh` or similar) provide sufficient extensibility to integrate icon conversion and staging steps without a complete rewrite of the build process.
    - **Rationale**: Modern build systems are generally designed for flexibility and extensibility.
    - **Impact if incorrect**: The effort to modify the build process could be substantially higher, impacting the timeline and complexity.
- **Assumption**: `sips` (native macOS command-line tool) and/or `ImageMagick` (commonly available via Homebrew) are suitable and accessible tools for `.xpm` to `.png`/`.svg` conversion in the macOS build environment.
    - **Rationale**: These tools are mentioned in the `HUMAN_IDEA_BRIEF.md` as previously used and are standard for image manipulation.
    - **Impact if incorrect**: An alternative image conversion utility would need to be identified and integrated into the build pipeline.
- **Assumption**: The existing gFTP codebase predominantly uses abstract icon names or a consistent pattern for icon lookup that can be mapped to a standard GTK icon theme.
    - **Rationale**: This is typical for well-structured GTK applications to allow for theme changes.
    - **Impact if incorrect**: If icon paths are widely hardcoded, identifying and updating all references could be a large, manual effort.

---

## Open Questions

Questions that, if answered, would:
- Reduce risk, or
- Validate or invalidate assumptions

For each question:
- Question:
- Owner:
- Blocking? Yes / No
- Needed by:

- **Question**: What is the most robust and idiomatic path for GTK icons within a macOS `.app` bundle that ensures discoverability by the GTK icon theme engine?
    - **Owner**: Architect
    - **Blocking?**: Yes (for final bundling strategy)
    - **Needed by**: Prior to `TASK_PACKET.md` creation
- **Question**: Are there any specific GTK environment variables or configuration files that need to be set within the macOS app bundle to correctly point GTK to the icon theme directories?
    - **Owner**: Architect
    - **Blocking?**: Yes (for correct runtime behavior)
    - **Needed by**: Prior to `TASK_PACKET.md` creation
- **Question**: What is the minimal set of `.xpm` icons that *must* be converted because they are actively used and not yet covered by existing `.png`/`.svg` assets? This requires a code search.
    - **Owner**: Architect/Builder
    - **Blocking?**: No (can be determined during implementation, but better to know upfront for completeness)
    - **Needed by**: Early implementation phase

---

## Change Triggers

Events or conditions that should force re-evaluation of this task.

Triggers exist to prevent:
- Continuing work under invalid assumptions
- Ignoring external changes
- Retrofitting explanations after failure

For each trigger:
- Trigger:
- Expected response: Pause, re-scope, create subtask, create bugtask, or abandon

- **Trigger**: Discovery that GTK on macOS fundamentally cannot load icons from within the `.app` bundle via standard icon theme mechanisms (e.g., requiring icons to be installed system-wide).
    - **Expected response**: Pause, re-scope (potentially requiring a different approach for macOS icons, or abandoning the GTK icon theme approach for macOS).
- **Trigger**: Significant, unexpected changes to the gFTP build system or macOS packaging scripts occur before or during implementation.
    - **Expected response**: Pause, re-scope, re-evaluate assumptions about build system extensibility.
- **Trigger**: Identification of a large number of hardcoded `.xpm` references in the codebase, indicating a more extensive refactoring is needed beyond simple path updates or theme integration.
    - **Expected response**: Pause, re-scope (potentially creating a subtask for code refactoring).

---

## Subtask Gate (MANDATORY — FINAL)

This section determines whether the task can safely proceed as a single
bounded TASK_PACKET, or whether decomposition is required.

### Evaluation
- subtasks_likely: NO
- rationale: The task, while involving multiple components (icon conversion, bundling, code check), appears to be tightly coupled around the single objective of fixing icon display. The open questions can likely be resolved during the initial phase of implementation or through focused research, rather than requiring separate, independent subtasks. The scope is well-defined and constrained to icon handling.

### Human Alert Requirement
If `subtasks_likely = YES`:
- Gemini MUST alert the human
- Gemini MUST explain the rationale
- Gemini MUST STOP after writing this document


Time Created: 2026-02-27 00:00:00
Time Modified: 2026-02-27 00:00:00