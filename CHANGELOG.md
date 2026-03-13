# gFTP Changelog - Recent Changes

## 2025-01-09 - macOS Build System and Resource Handling Overhaul

### Added: Centralized macOS Resource Management in `$HOME/Library/gFTP`

**Problem**: gFTP was using the standard XDG base directories for configuration and resources on macOS, which is not the platform standard. This could lead to a confusing user experience and made it difficult to create a clean, self-contained app bundle.

**Solution**: The application now uses `$HOME/Library/gFTP` for all user-specific data on macOS:
- **Configuration:** `gftprc` and `bookmarks` are now stored in `~/Library/gFTP/`.
- **First Run:** On first launch, the application copies default resources (sample bookmarks, icons, etc.) from the app bundle's `Resources` directory to `~/Library/gFTP/`. This ensures the application is properly initialized.
- **C Code Changes:**
    - `lib/misc.c` was modified to detect macOS (`__APPLE__`) and set the base configuration directory accordingly.
    - `gftp_get_share_dir()` and `gftp_get_doc_dir()` were updated to point to the new location on macOS.
- **App Bundle Integration:**
    - A new function `gftp_gtk_first_run_check()` was added to `src/gtk/gftp-gtk.c` to handle the initial resource copy.

### Changed: Modernized Meson Build for macOS and App Bundles

**Problem**: The build system was not optimized for creating a relocatable macOS app bundle. Artifacts were installed to standard Unix-style locations, making it difficult to collect them into a `.app` structure.

**Solution**: The Meson build system has been updated to support a clean installation layout for app bundling:
- **`install` Directory:** All build artifacts (executables, libraries, documentation, icons, etc.) are now installed into a dedicated `install` directory within the build root.
- **`meson.build` Updates:**
    - A new `install_dir_base` option was added to the root `meson.build` file.
    - All `install_data`, `install_man`, and `executable` install targets in `src/`, `docs/`, and `icons/` were updated to use this base directory.
- **`create_app_bundle.sh`:** This script was updated to source all its contents from the new `install` directory, making the bundling process more reliable and self-contained.

### Fixed: Static Library Linking Issues on macOS

**Problem**: The build system was configured to create intermediate static libraries (`libgftp.a`, `libgftpui.a`), which caused `ld: multiple errors: archive member '//' not a mach-o file` linking errors on macOS.

**Solution**: The build system was refactored to build all source files directly into the final executables (`gftp-gtk`, `gftp-text`) instead of creating static libraries. This avoids the platform-specific linking issues with `.a` files.

**Files Modified**:
- `lib/misc.c`: Updated for macOS resource paths.
- `src/gtk/gftp-gtk.c`: Added first-run resource copy logic.
- `meson.build` (root and subdirectories): Updated for the new `install` directory layout and direct source compilation.
- `create_app_bundle.sh`: Updated to use the new `install` directory.
- `CHANGELOG.md`: Updated with these changes.
- `MACOS.md`: Updated with new build instructions.
- `README.AppBundle.md`: Updated with new bundle information.

## 2024-12-12 - macOS Improvements

### Fixed: SSH/SFTP Crash on macOS (Threading Issue)

**Problem**: When connecting to SSH/SFTP servers, gftp-gtk would crash with `EXC_CRASH (SIGABRT)` when trying to show host key verification dialogs. The crash occurred because GTK dialog windows were being created from background threads, which is not allowed on macOS.

**Solution**: Modified `gftpui_protocol_ask_yes_no()` to properly schedule dialog creation on the main thread using `g_idle_add()`:
- Created `YesNoDialogData` structure to pass data between threads
- Added `_create_yes_no_dialog_on_main_thread()` callback that runs on main thread
- Removed deprecated `GDK_THREADS_ENTER/LEAVE` macros
- Ensured all GTK GUI operations happen on the main thread

**Files Modified**:
- `src/gtk/gtkui.c:526-587` - Rewrote `gftpui_protocol_ask_yes_no()` with proper thread safety

**Testing**:
```bash
# Test SFTP connection (will now show host key dialog properly)
./gFTP.app/Contents/MacOS/gftp-gtk
# Connect to: sftp://user@hostname
```

---

## 2024-12-12 - macOS Improvements

### Fixed: Application Hangs on Ctrl-C (SIGINT)

**Problem**: When pressing Ctrl-C or using "Force Quit" from macOS Finder, gftp-gtk would hang and not exit properly. The signal handler would print debug messages repeatedly but never terminate the application.

**Solution**: Added GTK-specific signal handler that properly quits the application:
- Created `_gftp_gtk_signal_handler()` in `src/gtk/gftp-gtk.c`
- Calls `_gftp_exit()` which performs proper cleanup (saves settings, disconnects)
- Overrides the common signal handler from `src/uicommon/gftpui.c`

**Files Modified**:
- `src/gtk/gftp-gtk.c:180-189` - Added signal handler
- `src/gtk/gftp-gtk.c:1498` - Installed signal handler in main()

**Testing**:
```bash
/Users/sedwards/source/jhbuild/install/bin/gftp-gtk
# Press Ctrl-C - application now exits cleanly
```

---

### Added: Relocatable Translations for macOS App Bundles

**Problem**: gFTP translations were hardcoded to install-time paths, making it difficult to create relocatable macOS application bundles. Users wanted translations to work when the app is moved anywhere on the system.

**Solution**: Implemented runtime detection of app bundles using CoreFoundation:

1. **CoreFoundation Integration**
   - Added framework dependency in `meson.build`
   - Defines `HAVE_COREFOUNDATION` when available on macOS

2. **Runtime Locale Detection**
   - Modified `gftp_locale_init()` to detect if running from app bundle
   - Uses `CFBundleGetMainBundle()` and `CFBundleCopyResourcesDirectoryURL()`
   - Automatically finds translations in `gFTP.app/Contents/Resources/locale/`
   - Falls back to traditional `LOCALE_DIR` if not in bundle

3. **App Bundle Packaging**
   - Created `create_app_bundle.sh` script
   - Packages gftp-gtk binary and all 64 translations
   - Generates proper `Info.plist`

**Files Modified**:
- `meson.build:40-47` - Added CoreFoundation framework
- `lib/misc.c:28-31` - Added CoreFoundation headers
- `lib/misc.c:965-1000` - Modified locale initialization

**Files Added**:
- `create_app_bundle.sh` - App bundle packaging script
- `README.AppBundle.md` - Complete documentation

**App Bundle Structure**:
```
gFTP.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── gftp-gtk
│   └── Resources/
│       └── locale/
│           ├── es/LC_MESSAGES/gftp.mo
│           ├── fr/LC_MESSAGES/gftp.mo
│           └── ... (64 locales)
```

**Usage**:
```bash
# Create app bundle
cd ~/source/jhbuild/checkout/gftp
./create_app_bundle.sh

# Test it
open gFTP.app

# Test with Spanish translations
LANG=es_ES.UTF-8 ./gFTP.app/Contents/MacOS/gftp-gtk
```

**Benefits**:
- ✓ Fully relocatable - bundle works anywhere
- ✓ Self-contained - all 64 translations included
- ✓ No wrapper scripts or environment variables needed
- ✓ Backward compatible with traditional Unix installs
- ✓ Minimal code changes (~30 lines)

---

## Summary

These changes make gFTP much more macOS-friendly:

1. **Better User Experience**: Application responds properly to quit signals
2. **Modern Packaging**: Supports standard macOS app bundle conventions
3. **Internationalization**: All 64 translations work out of the box
4. **Distribution Ready**: App bundle can be distributed as-is or packaged in DMG

Both features maintain full backward compatibility with traditional Unix installations and don't affect the text-mode client.
