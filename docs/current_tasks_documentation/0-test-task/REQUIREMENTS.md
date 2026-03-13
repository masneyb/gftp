# REQUIREMENTS

Purpose:  
Define **what must be true** for the task to be considered complete.

**Authoritative Location**: `docs/current_tasks_documentation/0-test-task/REQUIREMENTS.md`

This document is:
- Declarative
- Test-oriented
- Solution-agnostic

It describes **outcomes and constraints**, not implementation.
If *how* something is built is built, it does not belong here.

---

## Functional Requirements

List externally observable behaviors the system MUST exhibit.

Each requirement must:
- Be specific and unambiguous
- Be testable by observation or inspection
- Describe *what*, not *how*

Format:
- FR-1:
- FR-2:
- FR-3:

- **FR-1**: The gFTP application on macOS MUST display appropriate graphical icons for all files and folders in the local file system view.
- **FR-2**: The gFTP application on macOS MUST display appropriate graphical icons for all files and folders in the remote (FTP/SFTP) file system view.
- **FR-3**: If `.xpm` icon files are encountered during the build or bundling process, the system MUST automatically convert them to `.png` and `.svg` formats.
- **FR-4**: The converted `.png` and `.svg` icon files MUST be correctly packaged and placed within the macOS application bundle (`gFTP.app/Contents/Resources/`) to be discoverable by the GTK icon theme system.
- **FR-5**: The gFTP application MUST successfully load and utilize the correctly bundled `.png` and `.svg` icons for display.

---

## Non-Functional Requirements

Constraints on system behavior or qualities.

These describe *how the system behaves*, not *how it is built*.

Common categories include:
- Performance
- Reliability
- Compatibility
- Security
- Accessibility
- Maintainability

Format:
- NFR-1:
- NFR-2:

- **NFR-1 (Compatibility)**: The solution for icon display MUST be compatible with both macOS and Linux operating environments for gFTP.
- **NFR-2 (Compatibility)**: The solution MUST NOT rely on `.xpm` files for icon rendering on macOS.
- **NFR-3 (Maintainability)**: The icon conversion and bundling process SHOULD be integrated into the existing build system (e.g., `meson.build`, shell scripts) to ensure automated and consistent application of the changes.

---

## Acceptance Criteria Mapping

Define how each requirement will be verified.

This section links **requirements → verification**, not test implementation.

For each requirement:
- Requirement ID:
    - Verification method: Automated / Manual / Inspection
    - Success condition: What must be true for this to pass

- **FR-1**:
    - Verification method: Manual / Visual Inspection
    - Success condition: Running gFTP on macOS, the local file list displays distinct and correct icons for various file and folder types (e.g., text file, image, executable, directory).
- **FR-2**:
    - Verification method: Manual / Visual Inspection
    - Success condition: Running gFTP on macOS, connecting to a remote server, the remote file list displays distinct and correct icons for various file and folder types.
- **FR-3**:
    - Verification method: Inspection (of build logs and file system)
    - Success condition: After a full build/bundle process, no `.xpm` files are directly used for display, and corresponding `.png` and `.svg` versions are generated and present in the build artifacts if `.xpm` sources were available.
- **FR-4**:
    - Verification method: Inspection (of application bundle contents)
    - Success condition: The `gFTP.app/Contents/Resources/` directory (or a standard subdirectory like `share/icons/hicolor/`) contains the necessary `.png` and `.svg` icon files in appropriate sizes.
- **FR-5**:
    - Verification method: Manual / Visual Inspection
    - Success condition: The icons displayed in the gFTP UI on macOS are rendered correctly and without visual artifacts.

---

## Explicit Non-Requirements

List behaviors, changes, or outcomes that are **explicitly out of scope**.

This section exists to prevent:
- Scope creep
- Assumed features
- “While we’re here” additions

Examples:
- Not doing:
- Not changing:
- Not supporting:

- **Not doing**: Broad refactoring of gFTP's UI components beyond what is necessary for icon integration.
- **Not doing**: Performance tuning of the gFTP application unrelated to icon loading/rendering.
- **Not doing**: General UI polish or aesthetic changes not directly related to the missing icon issue.
- **Not supporting**: The use of `.xpm` icons for display within gFTP on macOS.

---

## Assumptions

List assumptions that must hold true for these requirements to be valid.

Each assumption should include:
- Assumption:
- Impact if false:

Assumptions are **not guarantees** and may require validation.

- **Assumption**: The GTK icon theme system on macOS can successfully locate and load icons from standard locations within a macOS application bundle (e.g., `Contents/Resources/share/icons/hicolor/`).
    - **Impact if false**: Additional research and potentially significant changes to the bundling process or GTK configuration would be required.
- **Assumption**: Existing `meson.build` files and shell scripts provide sufficient hooks or flexibility to integrate new icon conversion and staging steps.
    - **Impact if false**: The build system may require more extensive modification, increasing complexity and risk.
- **Assumption**: `sips` or `ImageMagick` are available on the build system or can be easily integrated for `.xpm` to `.png`/`.svg` conversion.
    - **Impact if false**: An alternative conversion tool would need to be identified and integrated.

---

## Open Questions

Requirement-level questions that remain unresolved.

These must be answered **before or during implementation**.
If unanswered, they may block execution or acceptance.

For each question:
- Question:
- Owner:
- Needed by:

- **Question**: What is the definitive standard path for GTK icons within a macOS `.app` bundle, or what path does gFTP currently use for other assets that could be leveraged?
    - **Owner**: Architect/Builder
    - **Needed by**: Implementation
- **Question**: Are there any remaining `.xpm` icons in `icons/legacy/` that are still actively referenced by the gFTP codebase and require conversion, beyond those already converted to `.png` and `.svg` in the other `icons/` subdirectories?
    - **Owner**: Architect/Builder (requires codebase investigation)
    - **Needed by**: Implementation

Time Created: 2026-02-27 00:00:00
Time Modified: 2026-02-27 00:00:00