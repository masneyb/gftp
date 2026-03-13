# DISCOVERY

Purpose:  
Capture understanding, context, and unknowns **before** committing to
requirements or implementation.

**Authoritative Location**: `docs/current_tasks_documentation/0-test-task/DISCOVERY.md`

This document is **exploratory only**:
- It may contain uncertainty
- It may contain incorrect hypotheses
- It must not prescribe solutions or scope commitments

If certainty exists, it belongs in a later document.

---

## Problem Statement

The gFTP graphical user interface on macOS is failing to display icons for files, folders, and binaries in both local and remote file listings. This hinders user experience by obscuring visual cues for file types and directory navigation. The root cause appears to be the incompatibility of legacy `.xpm` icon files with macOS and GTK's rendering system, coupled with an inadequate mechanism for bundling and locating appropriate icon assets within the macOS application bundle.

---

## Current Behavior

- Icons for files, folders, and binaries are not displayed in gFTP on macOS.
- The `icons/legacy/` directory contains `.xpm` files, which are known to be incompatible with macOS/GTK.
- There are already converted `.png` and `.svg` icons in various size subdirectories within the `icons/` directory (e.g., `icons/16x16/apps/*.png`, `icons/scalable/*.svg`).
- The `INTENT_SPEC.md` (unapproved) hints at existing build scripts (`build_and_bundle_gftp.sh`, `packaging/macos/`) that are likely responsible for bundling assets.
- `gftp.icns` and `gftp.iconset` exist, suggesting an attempt to create a proper macOS application icon, but not necessarily addressing in-app file/folder icons.
- `gftp.icns` is referenced in the main directory which suggests a build step that generates the icns file from a collection of png files.
- `MACOS.md` and `docs/BUILDING-MACOS.md` likely contain relevant information about macOS-specific build and bundling processes.

---

## Desired Outcome

- Clarity on the exact process required to convert remaining `.xpm` icons (if any are still actively used by the application logic and not yet converted to `.png`/`.svg`).
- A defined strategy for integrating the converted and existing `.png`/`.svg` icon assets into the macOS application bundle (`gFTP.app/Contents/Resources/`).
- An understanding of how gFTP's GTK-based UI on macOS can correctly discover and load these bundled icon assets for display in both local and remote file listings.
- Identification of any necessary modifications to gFTP's source code (C/GTK) or build system (`meson.build`, shell scripts) to achieve proper icon display.
- Readiness to define precise technical requirements and implementation steps for fixing the missing icons.

---

## Scope Characterization

- [x] Localized (single component / feature) - Primarily focused on icon handling and display within the gFTP GUI on macOS.
- [x] Multi-component - Involves icon assets, build scripts, application bundle structure, and potentially GTK UI code.
- [ ] Systemic / cascading
- [ ] Unknown / unclear

---

## Suspected Root Causes (Optional)

- **Possible cause:** `.xpm` icon files are directly referenced or attempted to be loaded by GTK on macOS, which does not support the format.
    - **Supporting observations:** `HUMAN_IDEA_BRIEF.md` and `INTENT_SPEC.md` explicitly state `.xpm` incompatibility on macOS. `.xpm` files exist in `icons/legacy/`.
    - **Contradicting observations:** Converted `.png` and `.svg` files already exist for many icons, suggesting some conversion process might already be in place, but perhaps not fully integrated or correctly referenced.
    - **Confidence level:** High
- **Possible cause:** Icon assets are not correctly packaged or located within the `gFTP.app` bundle in a way that GTK's icon theme system can discover them.
    - **Supporting observations:** Icons are not displayed, implying a discovery issue. `MACOS.md` and `docs/BUILDING-MACOS.md` likely detail specific macOS bundling requirements.
    - **Contradicting observations:** None specific at this stage.
    - **Confidence level:** Medium
- **Possible cause:** gFTP's C/GTK code has incorrect paths or logic for loading icons when running on macOS.
    - **Supporting observations:** The problem is specific to macOS.
    - **Contradicting observations:** None specific at this stage.
    - **Confidence level:** Low (less likely than bundling/format issues but possible)

---

## Open Questions

- What is the exact build process for the macOS `gFTP.app` bundle, specifically regarding asset inclusion and icon theme paths? (Reference `build_and_bundle_gftp.sh`, `packaging/macos/`, `MACOS.md`, `docs/BUILDING-MACOS.md`).
- Are all `.xpm` icons in `icons/legacy/` actually used by the application, or are some vestigial? Which ones specifically need conversion?
- What tool is currently used (or is best suited) for converting `.xpm` to `.png` and `.svg` within the build pipeline? (e.g., `sips`, `ImageMagick`, `rsvg-convert`).
- Where should the converted `.png`/`.svg` icon files be placed within the `gFTP.app/Contents/Resources/` directory structure to be discoverable by GTK's icon theme engine on macOS? (e.g., `share/icons/hicolor/`).
- How does gFTP's GTK code (e.g., `src/gtk/`, `lib/`) request and load icons? Does it use `GtkIconTheme`, `gtk_image_new_from_icon_name`, or direct file paths?
- What are the implications of GTK3's icon theme caching on macOS, and how can we ensure new/converted icons are recognized?

---

## Notes / Artifacts

- `HUMAN_IDEA_BRIEF.md`: `docs/current_tasks_documentation/0-test-task/HUMAN_IDEA_BRIEF.md` (provides core problem statement and desired outcomes)
- `INTENT_SPEC.md`: `docs/current_tasks_documentation/0-test-task/INTENT_SPEC.md` (provides detailed technical translation and initial gap analysis, even if unapproved)
- Relevant files to investigate:
    - `build_and_bundle_gftp.sh`
    - `packaging/macos/` directory contents
    - `MACOS.md`
    - `docs/BUILDING-MACOS.md`
    - `src/gtk/` for icon loading logic
    - `lib/` for file type detection and icon association
    - `icons/` directory structure for existing icons
    - `meson.build` for build system configuration relevant to icons and bundling.


Time Created: 2026-02-27 00:00:00
Time Modified: 2026-02-27 00:00:00