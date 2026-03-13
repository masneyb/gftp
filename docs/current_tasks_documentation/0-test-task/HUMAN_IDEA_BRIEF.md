# HUMAN IDEA BRIEF

> This brief is written by a human. Keep it plain-language and outcome-focused.
> If this is a bug/regression, fill out the **Bug / Regression Details** section.
> If this is a subtask, create this file inside the subtask folder and fill out **Subtask Linkage**.

---

## 1 - Task Summary (Required)
**One sentence:** 
We need to fix he missing icons for files, folders and binaries, ensure they are included in the gFTP.app bundle and able to be visable


---

## 2 - Task Type (Required)
- [ ] Feature / Improvement (new behavior)
- [x] Bugfix / Regression (existing behavior is wrong/broken)
- [ ] Follow-up / Cleanup (non-breaking, scoped)

---

## 3 - Classification Gate (Required)

- [x] This is corrective work (bug/regression), not planned decomposition
- ☐ This is NOT a feature/improvement request in disguise
- ☐ If fix requires new behavior beyond restoring intended behavior, I will reclassify as Feature/Improvement


---

## 4 - Subtask Linkage (Fill out if this is a subtask)
- **Parent task path:** `docs/current_task_documentation/<parent-task>/`
- **Subtask folder (if already created):** `subtasks/<S#-short-description>/`
- **Why this must be a subtask:** (tie it to the parent task’s implementation)

---

## 5 - Why This Matters (Required)
Why do you want this? What pain does it solve or value does it add?

So users can know if they are downloading the correct type of file or need to change to a sub-diecory

---

## 6 - Desired Outcome (Required)
What should be true when this work is “done”? (bullet list)

 - Host side all files and folders icons are displayed
 - Client side all files and folders icons are displayed


---

## 7 - Rough Expectations (Optional)
What do you *think* should happen? (It’s okay if this is wrong or incomplete.)

When opening the application, when viewing the local and remote list of folder and files, weshould see icons of various types and the same when connecing to the remote host

---

## 8 - Bug / Regression Details (Fill out for Bugfix/Regression)
- **User-visible impact:** What breaks or degrades?
- **Severity:** Low / Medium / High / Critical
- **Where it happens:** screen/flow/module (if known)
- **Steps to reproduce:**
    1.
    2.
    3.
- **Expected vs Actual:**
    - Expected:
    - Actual:
- **Frequency:** Always / Often / Sometimes / Rare
- **First noticed:** date/time or “after task X” (if known)
- **Workaround:** (if any)

### 8.1 - Bugfix Scope Guardrails (Required for Bugfix/Regression)

- **Bug definition (one sentence):** (what exactly is wrong)
- **Fix boundary:** (what parts of the system may be changed)
- **Out of scope (explicit):**
    - (e.g., refactors, performance tuning, UI polish, new features)
- **Allowed collateral changes:** (tests, logging, docs) Yes/No + notes
- **Parent linkage (if known):** task/PR/commit that introduced it


---

## 9 - Environment / Context (Helpful for bugs)
- **Platform(s):** iOS / Android / Web / Backend / Other
   - macOS
- **Device(s):** (models if relevant)
- **OS version(s):**
- **App version / build / branch:**
- **Feature flags / configs:** (if any)

Note: This application is writen in C using GTK 3 libraies ported from linux.

---

## 10 - Known Constraints (Optional)
Anything you already know must be true:
- Platforms
   macOS and Linux
- Performance
    xpm files are not supported under macOS and macOS even with GTK2 or higher
- Business rules
- Deadlines

---

## 11 - External Research Needed?
- [x] Yes (policies, SDK docs, platform rules, etc.)
- [ ] No / Not sure

If yes, include links or keywords to search for.

In the past I've had to use tools lik sips and ImageMagick to converty legacy .xpm icons from X11 hertitage to make properly rendered *.png and *.svg files


The newly converted files are located in the gftp folder in
.
├── 16x16
│   ├── apps
│   │   ├── deb.png
│   │   ├── diff.png
│   │   ├── dir.png
│   │   ├── doc.png
│   │   ├── dotdot.png
│   │   ├── exe.png
│   │   ├── gftp-logo.png
│   │   ├── img.png
│   │   ├── linkdir.png
│   │   ├── linkfile.png
│   │   ├── man.png
│   │   ├── open_dir.png
│   │   ├── rpm.png
│   │   ├── sound.png
│   │   ├── tar.png
│   │   ├── txt.png
│   │   └── world.png
│   └── gftp.png
├── 22x22
│   ├── apps
│   │   ├── deb.png
│   │   ├── diff.png
│   │   ├── dir.png
│   │   ├── doc.png
│   │   ├── dotdot.png
│   │   ├── exe.png
│   │   ├── gftp-logo.png
│   │   ├── img.png
│   │   ├── linkdir.png
│   │   ├── linkfile.png
│   │   ├── man.png
│   │   ├── open_dir.png
│   │   ├── rpm.png
│   │   ├── sound.png
│   │   ├── tar.png
│   │   ├── txt.png
│   │   └── world.png
│   └── gftp.png
├── 24x24
│   ├── apps
│   │   ├── deb.png
│   │   ├── diff.png
│   │   ├── dir.png
│   │   ├── doc.png
│   │   ├── dotdot.png
│   │   ├── exe.png
│   │   ├── gftp-logo.png
│   │   ├── img.png
│   │   ├── linkdir.png
│   │   ├── linkfile.png
│   │   ├── man.png
│   │   ├── open_dir.png
│   │   ├── rpm.png
│   │   ├── sound.png
│   │   ├── tar.png
│   │   ├── txt.png
│   │   └── world.png
│   └── gftp.png
├── 32x32
│   ├── apps
│   │   ├── deb.png
│   │   ├── diff.png
│   │   ├── dir.png
│   │   ├── doc.png
│   │   ├── dotdot.png
│   │   ├── exe.png
│   │   ├── gftp-logo.png
│   │   ├── img.png
│   │   ├── linkdir.png
│   │   ├── linkfile.png
│   │   ├── man.png
│   │   ├── open_dir.png
│   │   ├── rpm.png
│   │   ├── sound.png
│   │   ├── tar.png
│   │   ├── txt.png
│   │   └── world.png
│   └── gftp.png
├── 48x48
│   ├── apps
│   │   ├── deb.png
│   │   ├── diff.png
│   │   ├── dir.png
│   │   ├── doc.png
│   │   ├── dotdot.png
│   │   ├── exe.png
│   │   ├── gftp-logo.png
│   │   ├── img.png
│   │   ├── linkdir.png
│   │   ├── linkfile.png
│   │   ├── man.png
│   │   ├── open_dir.png
│   │   ├── rpm.png
│   │   ├── sound.png
│   │   ├── tar.png
│   │   ├── txt.png
│   │   └── world.png
│   └── gftp.png
├── fix-icom-gftp.sh
├── gftp.icns
├── gftp.iconset
│   ├── icon_1024x1024.png
│   ├── icon_1024x1024@2x.png
│   ├── icon_128x128.png
│   ├── icon_128x128@2x.png
│   ├── icon_16x16.png
│   ├── icon_16x16@2x.png
│   ├── icon_256x256.png
│   ├── icon_256x256@2x.png
│   ├── icon_32x32.png
│   ├── icon_32x32@2x.png
│   ├── icon_512x512.png
│   └── icon_512x512@2x.png
├── legacy
│   ├── deb.xpm
│   ├── diff.xpm
│   ├── dir.xpm
│   ├── doc.xpm
│   ├── dotdot.xpm
│   ├── exe.xpm
│   ├── gftp-logo.xpm
│   ├── img.xpm
│   ├── linkdir.xpm
│   ├── linkfile.xpm
│   ├── man.xpm
│   ├── open_dir.xpm
│   ├── rpm.xpm
│   ├── sound.xpm
│   ├── tar.xpm
│   ├── txt.xpm
│   └── world.xpm
├── meson.build
└── scalable
    └── gftp.svg



---

## 12 - Unknowns / Questions (Optional)
What are you unsure about? What needs investigation?

Ensure there are no legacy calls to *.xpm files
---

## 13 - Acceptance Criteria (Required)
List objective checks that confirm success.
- [x] if xpm files are still being called by any code, we need to ensure it is updated to use the correct png filee instead 
- [x] Ensure the application can find the correct icon in the app bundle

---

## 14 - Verification Notes (Optional but recommended)
How will we test this?
- Manual test steps:
- Automated tests expected:
- Logs/metrics to review:

---

## 15 - Attachments / Evidence (Optional)
- Screenshots / screen recordings:
- Logs / crash reports:
- Links to related tasks, PRs, commits:


Time Created: <YYYY-MM-DD HH:MM:SS>  
Time Modified: <YYYY-MM-DD HH:MM:SS>
