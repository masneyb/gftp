# HUMAN IDEA BRIEF

> This brief is written by a human. Keep it plain-language and outcome-focused.
> If this is a bug/regression, fill out the **Bug / Regression Details** section.
> If this is a subtask, create this file inside the subtask folder and fill out **Subtask Linkage**.

---

## Task Summary (Required)
**One sentence:** Fix the icon loading issue in the gFTP macOS application by correcting the hardcoded icon paths.

---

## Task Type (Required)
- [x] Bugfix / Regression (existing behavior is wrong/broken)

---

## Subtask Linkage (Fill out if this is a subtask)
- **Parent task path:** 
- **Subtask folder (if already created):** 
- **Why this must be a subtask:**

---

## Why This Matters (Required)
The application is not displaying icons, which makes it look unprofessional and harder to use.

---

## Desired Outcome (Required)
- Icons should be displayed correctly in the application's list views.
- The "gftp: ... not found" and "Gtk-WARNING ... Could not find the icon" error messages should no longer appear.

---

## Rough Expectations (Optional)
I expect that there is a hardcoded path in the source code that needs to be changed to a relative path that correctly points to the icons within the application bundle.

---

## Bug / Regression Details (Fill out for Bugfix/Regression)
- **User-visible impact:** Icons are missing in the application.
- **Severity:** Medium
- **Where it happens:** In the file and directory listing views.
- **Steps to reproduce:**
    1. Launch the gFTP application on macOS.
    2. Observe the missing icons in the UI.
    3. Check the console output for error messages.
- **Expected vs Actual:**
    - Expected: Icons for files and directories are displayed.
    - Actual: No icons are displayed, and error messages are printed to the console.
- **Frequency:** Always
- **First noticed:** 
- **Workaround:** None

---

## Environment / Context (Helpful for bugs)
- **Platform(s):** macOS
- **Device(s):** 
- **OS version(s):** 
- **App version / build / branch:** 
- **Feature flags / configs:** 

---

## Known Constraints (Optional)
- The fix should not break the application on other platforms (e.g., Linux).

---

## External Research Needed?
- [ ] Yes (policies, SDK docs, platform rules, etc.)
- [x] No / Not sure

---

## Unknowns / Questions (Optional)
- I need to identify the exact location in the code where the icon paths are configured.

---

## Acceptance Criteria (Required)
- [ ] Icons are visible in the application's UI.
- [ ] No icon-related error messages are present in the console output.

---

## Verification Notes (Optional but recommended)
- Manual test steps:
    1. Build and run the application.
    2. Verify that icons are displayed for files and directories.
    3. Check the console for any icon-related errors.
- Automated tests expected: None
- Logs/metrics to review: Console output.

---

## Attachments / Evidence (Optional)
- Error message from the user:
  ```
  * gftp: /Users/sedwards/source/gftp/gFTP.app/Contents/Resources/share/gftp.png not found

  (gftp-gtk:45158): Gtk-WARNING **: 17:47:40.279: Could not find the icon 'network-server-ltr'. The 'hicolor' theme
  was not found either, perhaps you need to install it.
  You can get a copy from:
      http://icon-theme.freedesktop.org/releases
  ```
