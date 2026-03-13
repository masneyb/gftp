# INTENT SPEC — 0-test-task

> This document is a **technical translation** of the human-authored docs/current_tasks_documentation/0-test-task/HUMAN_IDEA_BRIEF.md.
> It is produced by the Claude (Intent Translator) and is **advisory until human-approved**.
>
> This document does NOT authorize planning or execution.
> It provides Gemini with a technically precise version of human intent.

---

## Source Brief

- Brief Location: `docs/current_tasks_documentation/0-test-task/HUMAN_IDEA_BRIEF.md`
- Brief Last Modified: 2026-02-26
- Translation Date: 2026-02-26
- Translator: Claude (Intent Translator)

---

## Human Approval

- Approved: ☐ Yes ☐ No
- Approved By: ____________
- Approval Date: <YYYY-MM-DD>

If not approved, Gemini MUST treat this document as **non-authoritative**
and fall back to `docs/current_tasks_documentation/0-test-task/HUMAN_IDEA_BRIEF.md`.

---

## 1) Translated Objective

**Human said:** We need to fix the missing icons for files, folders and binaries
**Technical translation:** Address the deficiency in the gFTP graphical user interface (gftp-gtk) where file, folder, and binary icons are not displayed for both local and remote file listings, by ensuring proper conversion and integration of icon assets within the macOS application bundle.

---

## 2) Translated Task Type

- [ ] Feature / Improvement
- [x] Bugfix / Regression
- [ ] Follow-up / Cleanup

---

## 3) Translated Desired Outcomes

- **Human said:** Legacy icons in icons/legacy are converted to proper *.png and *.svg images
- **Technical translation:** The legacy `.xpm` icon files located in `icons/legacy/` shall be programmatically converted into modern `.png` and `.svg` formats, suitable for rendering on macOS.
- **Affected components:** `icons/`, `build_and_bundle_gftp.sh` (or a similar build script), `packaging/macos/`
- **Traceability:** Brief section → Desired Outcome #1

- **Human said:** Host side all files and folders icons are displayed
- **Technical translation:** The gftp-gtk graphical user interface shall correctly display appropriate icons for files and folders within the local file system view on macOS.
- **Affected components:** `src/gtk/`, `src/uicommon/`, `lib/`, macOS application bundle structure.
- **Traceability:** Brief section → Desired Outcome #2

- **Human said:** Client side all files and folders icons are displayed
- **Technical translation:** The gftp-gtk graphical user interface shall correctly display appropriate icons for files and folders retrieved from remote FTP/SFTP servers within the remote file system view on macOS.
- **Affected components:** `src/gtk/`, `src/uicommon/`, `lib/`, macOS application bundle structure.
- **Traceability:** Brief section → Desired Outcome #3

---

## 4) Translated Constraints

- **Human said:** macOS and Linux
- **Technical translation:** The implemented solution must be compatible with both macOS and Linux operating environments for gFTP.
- **Enforcement boundary:** Build system, runtime environment.
- **Traceability:** Brief section → Known Constraints #1

- **Human said:** xpm files are not supported under macOS and macOS even with GTK2 or higher
- **Technical translation:** The current `.xpm` icon format is fundamentally incompatible with standard GTK rendering on macOS, irrespective of GTK version.
- **Enforcement boundary:** GTK rendering pipeline on macOS.
- **Traceability:** Brief section → Known Constraints #2

---

## 5) Translated Acceptance Criteria

- **Human said:** if xpm files are found, convert to png and svg files
- **Technical translation:** Upon encountering `.xpm` icon files during the build or bundling process, the system shall automatically generate corresponding `.png` and `.svg` files for each, placing them in an accessible location within the application bundle.
- **Verification method:** Inspection (of build logs and bundle contents).
- **Traceability:** Brief section → Acceptance Criteria #1

- **Human said:** Ensure the application can find the correct icon in the app bundle
- **Technical translation:** The gftp-gtk application, when run on macOS, must successfully resolve and display the intended icons for various file and folder types in both local and remote views.
- **Verification method:** Manual / Visual Inspection (running the application).
- **Traceability:** Brief section → Acceptance Criteria #2

---

## 6) Bug / Regression Translation (If Applicable)

- **Affected system boundary:** gFTP GTK+ user interface (`gftp-gtk`) on macOS.
- **Translated reproduction steps:**
    1. Launch the `gftp-gtk` application on macOS.
    2. Observe the local file system view.
    3. Connect to a remote FTP/SFTP server and observe the remote file system view.
    Expected: Icons for files, folders, and binaries are displayed correctly.
    Actual: Icons are missing or not displayed.
- **Translated expected vs actual:**
    - Expected: Visual representation of file, folder, and binary types through corresponding icons in gftp-gtk UI.
    - Actual: Absence of icons for file, folder, and binary types in gftp-gtk UI on macOS.
- **Translated fix boundary:** Icon asset management (conversion, packaging, deployment within the `.app` bundle), gFTP's icon loading mechanism, macOS application bundle structure.
- **Translated scope guardrails:**
    - Out of scope (explicit): Broad refactoring of UI components beyond icon integration, performance tuning unrelated to icon loading, general UI polish.
    - Allowed collateral changes: Yes (tests, logging, documentation related to icon handling and macOS bundling).

---

## 7) Architectural Context (From PROJECT_CONTEXT.md)

- **Relevant components:**
    - `src/gtk/`: GTK+ user interface, responsible for rendering UI elements including file/folder listings.
    - `lib/`: Core library, likely contains logic for file type detection and associating types with icons.
    - `icons/`: Directory containing various icon assets, including the legacy `.xpm` files.
    - `build_and_bundle_gftp.sh`: Existing script involved in building and bundling the application, which will need modification to handle icon conversion and staging.
    - `meson.build`: Build system configuration, which might need updates for new icon formats or build steps.
    - `packaging/macos/`: Directory containing macOS-specific packaging scripts and configurations, specifically `create_app_bundle.sh` (or `AppBundleGenerator` usage described in `MACOS.md`/`docs/BUILDING-MACOS.md`).
- **Relevant boundaries:**
    - macOS Application Bundle structure (`gFTP.app/Contents/Resources/`).
    - Interaction between GTK's icon theme system and the application's bundled resources.
    - Integration with macOS native services for font rendering (Pango CoreText backend, as mentioned in packaging docs).

---

## 8) Translation Confidence

| Section | Confidence | Notes |
|---------|------------|-------|
| Objective | High | Clear and direct translation. |
| Task Type | High | Clearly a bugfix. |
| Desired Outcomes | High | Outcomes are specific and translatable. |
| Constraints | High | Constraints are well-defined. |
| Acceptance Criteria | High | Criteria are specific and testable. |
| Bug Translation | High | Comprehensive details provided in the brief. |
| Architectural Context | High | Components are identifiable from PROJECT_CONTEXT.md and other documentation. |

---

## 9) Gaps and Ambiguities (MANDATORY)

- **Gap ID:** G-1
- **Section affected:** Translated Acceptance Criteria, Translated Bugfix Scope Guardrails
- **Description:** The brief mentions converting `.xpm` to `.png` and `.svg`, but it doesn't specify *which* tool to use for conversion, *how* these conversions should be integrated into the build process, or *where* the new `.png`/`.svg` files should be placed within the `gFTP.app` bundle to be discoverable by GTK. The brief in Section 11 hints at `sips` and `ImageMagick`.
- **Impact if unresolved:** Gemini cannot create a concrete plan for the icon conversion and integration without this detail. The Builder will not know how to proceed.
- **Suggested resolution:** During the planning phase, research the best practices for integrating converted icons into a GTK macOS bundle and select a suitable conversion tool. Define the exact build steps and target directories for the converted icons.

---

## 10) Items NOT Translated (Transparency Record)

- **Brief content:** "In the past I've had to use tools lik sips and ImageMagick to converty legacy .xpm icons from X11 hertitage to make properly rendered *.png and *.svg files"
- **Reason not translated:** This is context/historical information about potential tools, rather than a direct requirement or constraint for the current task. It will inform the planning phase but doesn't map directly to a section in the `INTENT_SPEC`.

Time Created: 2026-02-26 00:00:00
Time Modified: 2026-02-26 00:00:00
