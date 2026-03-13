# TASK PACKET — Fix Missing Icons in gFTP on macOS

Version: 1.0  
Last Updated: 2026-02-27  
Status: Draft

Supersedes: N/A

---

This document is the **execution contract** for this task.

Anything not explicitly authorized here is **out of scope**.
If ambiguity exists, execution must stop.

Subtask handling is governed by:
AI_SESSION/INSTRUCTIONS/SUBTASKS.md

---
## Human Authorization Gate (Required)

This task MUST NOT proceed to execution unless explicitly authorized by a human.

### Authorization Status
- Authorized for Build: [x] Yes ☐ No
- Authorized By: Stevem Edwards
- Authorization Date: <YYYY-MM-DD>

### Authorized Scope
(Check all that apply)
- [x] Implementation
- [ ] Refactor
- [ ] Comment normalization
- [ ] Other (explicitly defined):

### Enforcement Notes
- Planning completion does NOT imply authorization
- Presence of DISCOVERY / REQUIREMENTS / RISKS documents does NOT imply authorization
- COMMENT_STRATEGY.md does NOT authorize work
- If this section is incomplete or unchecked, the task MUST remain in `Draft`

---

# AUTHORIZATION GATE — READY FOR BUILD (HUMAN-OWNED)

> This section is a **hard gate**.  
> The Architect (Gemini) may draft it, but **may not mark it satisfied**.  
> The task packet may not be marked `Ready for Build` unless **every box is checked** and the **Human Approval** line is completed.

## Gate Checklist (Must be all ✅)

- [ ] **All “Needed by Implementation” items are resolved**
    - No open questions remain in REQUIREMENTS that would affect implementation choices. (Note: Open questions in RISKS_AND_ASSUMPTIONS.md are carried forward as part of the initial implementation steps for Builder to resolve)
- [ ] **DISCOVERY.md is complete**
    - Includes relevant context, constraints, and existing behavior notes.
- [ ] **REQUIREMENTS.md is complete**
    - Acceptance criteria are explicit and testable.
    - Validation rules are explicitly stated (no “implied” validation).
- [ ] **RISKS_AND_ASSUMPTIONS.md is complete**
    - Key risks are enumerated.
    - Assumptions are explicitly tagged as such.
- [ ] **Definitions & Decisions are locked**
    - Ambiguous terms/fields have authoritative definitions.
    - Any “edge cases” that could fork implementation are decided.
- [ ] **File touch list is explicit**
    - Exact files allowed to be modified/created are listed.
    - No “and related files” language.
- [ ] **Non-goals are explicit**
    - Clear exclusions to prevent scope creep.
- [ ] **HUMAN_IDEA_BRIEF provenance is valid**
    - Brief is human-authored **or** explicitly human-ratified below.

## Human Approval (Required)

Human Approval: ☐ Approved for Build   ☐ Not Approved  
Approved by (name/handle): ____________  
Date (YYYY-MM-DD): ____________

## Brief Ratification (Only if Architect drafted the brief text)

I confirm the HUMAN_IDEA_BRIEF content is correct and authorized for planning:
☐ Ratified by Human (name/handle): ____________  Date (YYYY-MM-DD): ____________

---



---

## 1) Objective

Address the deficiency in the gFTP graphical user interface (gftp-gtk) where file, folder, and binary icons are not displayed for both local and remote file listings on macOS, by ensuring proper conversion and integration of icon assets within the macOS application bundle.

---

## 2) Non-Goals

Explicit exclusions.

This section is authoritative.
If something is not allowed, it must be stated here.

- **Not doing**: Broad refactoring of gFTP's UI components beyond what is necessary for icon integration.
- **Not doing**: Performance tuning of the gFTP application unrelated to icon loading/rendering.
- **Not doing**: General UI polish or aesthetic changes not directly related to the missing icon issue.
- **Not supporting**: The use of `.xpm` icons for display within gFTP on macOS.

---

## 3) Constraints

Hard limits that must not be violated.

These are **non-negotiable** during execution.

- **Must preserve**: Compatibility with Linux operating environments for gFTP.
- **Must not change**: Core application logic unrelated to icon handling.
- **Must remain compatible with**: Existing `meson.build` build system and associated shell scripting patterns.
- **Must not violate**: macOS application bundle structure guidelines (`gFTP.app/Contents/Resources/`).

---

## 4) Definitions & Decisions (Lock Ambiguity)

All potentially ambiguous items must be resolved here.

This section overrides:
- defaults
- conventions
- prior assumptions

Include:
- Authoritative terms or field names
- Final decisions on edge cases
- Display or interpretation semantics
- Tie-breakers where multiple “reasonable” options exist

If something is ambiguous and not resolved here, execution must stop.

- **Icon Conversion Tool**: `sips` (macOS native) or `ImageMagick` (if available and preferred for robustness). Preference will be given to `sips` for simplicity and native integration if it meets all requirements. If `sips` proves insufficient, `ImageMagick` will be used.
- **Target Icon Formats**: `.png` and `.svg`. All `.xpm` icons deemed necessary for conversion must result in both `.png` and `.svg` variants where applicable (e.g., scalable for `.svg`).
- **macOS Application Bundle Icon Path**: Icons should be placed in `gFTP.app/Contents/Resources/share/icons/hicolor/` followed by standard size/format subdirectories (e.g., `16x16/apps/`, `scalable/apps/`). This is the conventional path for GTK applications.

---

## 5) Exact File Touch List (Authoritative)

This section defines the **only files the Builder may interact with**.

### Create (or Modify if already present)
- `TASK_PACKET_SUMMARY.md`
    - Must be created in the same directory as this file
    - Must follow `TASK_PACKET_SUMMARY_TEMPLATE.md`
- `docs/current_tasks_documentation/0-test-task/DISCOVERY.md`
- `docs/current_tasks_documentation/0-test-task/REQUIREMENTS.md`
- `docs/current_tasks_documentation/0-test-task/RISKS_AND_ASSUMPTIONS.md`
- `docs/current_tasks_documentation/0-test-task/TASK_PACKET.md` (this file)
- Any newly generated `.png` and `.svg` icon files within the `build/icons` (or similar staging) and `gFTP.app/Contents/Resources/share/icons/hicolor/` directories. These will be created during the build process.

### Modify
- `build_and_bundle_gftp.sh` (or `create_app_bundle.sh` if it exists and is the primary bundling script) - for integrating icon conversion and staging.
- `meson.build` files (specifically `icons/meson.build` and potentially `src/meson.build` or top-level `meson.build`) - to define icon conversion as a build step and manage icon installation paths.
- `src/gtk/*.c`, `src/uicommon/*.c`, `lib/*.c` (potentially) - to adjust icon loading logic if hardcoded paths are found or to ensure `GtkIconTheme` is used correctly.
- `gftp.icns` (if modifications are needed to the overall application icon bundle)
- `fix_rpaths.sh` - if changes to the application bundle structure require adjustments to `rpath` handling.
- `docs/current_tasks_documentation/0-test-task/HUMAN_IDEA_BRIEF.md`

### Do NOT Touch
- Files outside the scope of icon handling and macOS bundling.
- Any build system files for other platforms (e.g., Linux `meson.build` targets) unless explicitly conditional for macOS.
- Localisation `.po` files.

Rules:
- Touching an unlisted file requires stopping execution
- Dependency or config files (e.g., pubspec, gradle, podspec) are forbidden unless explicitly listed
- “Incidental” changes are not permitted

---

## 6) Implementation Checklist (In Order)

Concrete, ordered steps the Builder must follow.

Rules:
- Steps must be explicit and verifiable
- No conditional logic
- No inferred sub-steps

- [ ] **Step 1: Investigate Icon Usage and Build Process**
    - [ ] Determine the specific `.xpm` icons in `icons/legacy/` that are actively used by gFTP's UI. This may involve code search for references to `legacy/` or specific `.xpm` filenames.
    - [ ] Analyze `build_and_bundle_gftp.sh` and `packaging/macos/` scripts (e.g., `create_app_bundle.sh` if it exists) to understand the current macOS bundling process, especially how resources are added to `gFTP.app`.
    - [ ] Examine `meson.build` files (`icons/meson.build`, top-level `meson.build`) to understand how icon assets are currently built and installed.
    - [ ] Research GTK icon theme lookup paths and environment variables on macOS within an `.app` bundle context to confirm `gFTP.app/Contents/Resources/share/icons/hicolor/` is the correct target.
- [ ] **Step 2: Implement Icon Conversion and Staging**
    - [ ] Develop a mechanism (e.g., a new shell script or `meson` custom command) to convert identified `.xpm` icons to `.png` and `.svg` using `sips` or `ImageMagick`.
    - [ ] Integrate this conversion step into the `meson.build` system so it runs automatically during the macOS build.
    - [ ] Modify the build process to stage all necessary `.png` and `.svg` icons (both newly converted and existing ones) into a temporary directory structure that mirrors the target `gFTP.app/Contents/Resources/share/icons/hicolor/` layout.
- [ ] **Step 3: Update Bundling Script**
    - [ ] Modify `build_and_bundle_gftp.sh` (or `create_app_bundle.sh`) to copy the staged icon files into the final `gFTP.app/Contents/Resources/share/icons/hicolor/` directory within the application bundle.
    - [ ] Ensure that the `gFTP.app` bundle has correct permissions and structure for the new icon files.
    - [ ] Check if `fix_rpaths.sh` needs adjustment due to changes in bundle structure.
- [ ] **Step 4: Adjust gFTP Icon Loading Logic (if necessary)**
    - [ ] Search gFTP's C/GTK source (`src/gtk/`, `lib/`) for direct references to `.xpm` files or hardcoded icon paths.
    - [ ] If found, update the code to use abstract icon names (e.g., `gtk_image_new_from_icon_name`) or to correctly reference the new `.png`/`.svg` assets via GTK's icon theme.
    - [ ] Ensure GTK is initialized with proper icon theme paths for macOS within the application context.
- [ ] **Step 5: Verification and Testing**
    - [ ] Perform a full clean build of `gFTP.app` on macOS.
    - [ ] Manually verify the contents of `gFTP.app/Contents/Resources/share/icons/hicolor/` to ensure all expected `.png` and `.svg` icons are present and correctly organized.
    - [ ] Launch `gFTP.app` on macOS and visually inspect both local and remote file listings to confirm that all files, folders, and binaries display correct and distinct icons.
    - [ ] Run the build on Linux to ensure no regressions were introduced.

---

## 7) Acceptance Tests

These define **what success means**.

### Automated (Preferred)
- Command: `test -d gFTP.app/Contents/Resources/share/icons/hicolor/16x16/apps`
    - Expected result: Directory exists.
- Command: `test -f gFTP.app/Contents/Resources/share/icons/hicolor/16x16/apps/dir.png`
    - Expected result: File exists.
- Command: `find gFTP.app -name "*.xpm"`
    - Expected result: No `.xpm` files found within the bundled `gFTP.app` that are actively used by the UI (a visual check of the running app is still needed for this to confirm the "actively used" part).

### Manual / Visual (When Necessary)
- Scenario: Launch gFTP.app on macOS and navigate local file system.
    - Expected behavior: Icons for files, folders, and binaries are displayed correctly and consistently across the local file view.
- Scenario: Connect to a remote FTP/SFTP server with gFTP.app on macOS and navigate remote file system.
    - Expected behavior: Icons for files, folders, and binaries are displayed correctly and consistently across the remote file view.

### Non-Regression Checks (Required)

At least one check must validate that existing behavior was not broken.

- Check: Build and run gFTP on Linux.
    - Expected result: gFTP builds successfully, runs, and displays icons correctly on Linux, ensuring macOS-specific changes did not introduce regressions.

---

## 8) Open Risks / Blockers

Known issues that could invalidate or pause execution.

List only items that materially affect correctness or completion.

- **Risk**: GTK's icon theme system on macOS does not recognize or properly load icons from `gFTP.app/Contents/Resources/share/icons/hicolor/`. This would require further investigation into GTK's macOS integration or a custom icon loading solution within gFTP.
- **Open Question**: Exact build system modifications (`meson.build` and shell scripts) needed to conditionally apply icon conversion and bundling steps only for macOS. This needs to be determined during Step 1 of implementation.
- **Open Question**: Full list of gFTP C/GTK source files requiring modification to adjust icon loading logic. This needs to be determined during Step 1 and Step 4 of implementation.

If a blocker is encountered, execution must stop.

---

## 9) Builder Notes

Optional clarifications that:
- Explain intent
- Reduce misinterpretation

Rules:
- Must not introduce new requirements
- Must not expand scope
- Must not contradict earlier sections

- The Builder should prioritize using existing build system mechanisms (Meson custom commands, `install_data`) rather than entirely new, standalone scripts for icon conversion and staging if possible, to maintain consistency.
- When searching for `.xpm` references in code, also look for `gdk_pixbuf_new_from_file`, `gtk_image_new_from_file`, or similar functions that might load images directly by path rather than through the theme.
- Pay close attention to macOS path conventions (case-insensitivity, bundle structure) when configuring icon installation paths.

---

Time Created: 2026-02-27 00:00:00
Time Modified: 2026-02-27 00:00:00
