# macOS App Bundle Testing Checklist

This document provides a comprehensive testing checklist for gFTP macOS app bundles.

## Automated Validation

Before manual testing, always run the validation script:

```bash
./scripts/validate_bundle.sh /path/to/gFTP.app
```

This checks:
- Bundle structure completeness
- Info.plist validity
- Library relocatability
- Resource presence (translations, schemas, icons)
- Code signature validity
- Bundle size

## Pre-Release Testing

### Basic Functionality

- [ ] **App Launch**: Double-click gFTP.app launches without errors
- [ ] **Main Window**: Main window appears with dual-pane interface
- [ ] **Menu Bar**: All menus accessible (File, Edit, Bookmarks, Transfer, Log, Tools, Help)
  - If gtk-mac-integration enabled: menus should appear in macOS menu bar, not in window
- [ ] **Toolbar**: Connection toolbar visible and functional

### Protocol Connections

Test each supported protocol:

- [ ] **FTP**: Connect to public FTP server (e.g., ftp://ftp.gnu.org/)
  - Browse directories
  - Download a file
  - Verify file integrity
- [ ] **FTPS (Explicit TLS)**: Connect to FTPS server on port 21
  - Verify TLS negotiation
  - Test file transfer
- [ ] **FTPS (Implicit TLS)**: Connect to FTPS server on port 990
- [ ] **SFTP (SSH2)**: Connect via SSH (requires SSH key or password)
  - Test authentication
  - Browse remote directories
  - Transfer files
- [ ] **Local Filesystem**: Browse local directories in both panes

### File Operations

- [ ] **Download Files**:
  - Single file download
  - Multiple file selection and download
  - Directory download (recursive)
- [ ] **Upload Files**:
  - Single file upload
  - Multiple file upload
  - Directory upload
- [ ] **File Management**:
  - Rename file
  - Delete file
  - Create directory
  - Change permissions (chmod)
  - View file properties

### Bookmarks

- [ ] **Add Bookmark**: Save a connection as bookmark
- [ ] **Edit Bookmark**: Modify saved bookmark
- [ ] **Connect via Bookmark**: Use bookmark to connect
- [ ] **Delete Bookmark**: Remove bookmark

### Configuration

- [ ] **Preferences Dialog**: Open and navigate preferences
- [ ] **Save Settings**: Modify and save settings
- [ ] **Settings Persistence**: Quit and relaunch, verify settings saved

### Internationalization

- [ ] **System Language**: Test with macOS system language set to non-English
  - Go to System Settings → General → Language & Region
  - Add a language (French, German, Spanish, Japanese, etc.)
  - Relaunch gFTP
  - Verify UI text appears in selected language

- [ ] **Translation Coverage**: Verify menus, dialogs, and messages are translated

### macOS Integration

- [ ] **Menu Bar Integration** (if gtk-mac-integration enabled):
  - Application menu shows "gFTP" in menu bar
  - About menu item in application menu
  - File/Edit/etc. menus in system menu bar (not in window)

- [ ] **Dock Integration**:
  - App icon appears in Dock when running
  - Right-click Dock icon shows "Quit" option
  - Dock icon updates if app is transferred to Applications folder

- [ ] **Window Management**:
  - Minimize to Dock (Cmd+M)
  - Maximize/zoom window (green button)
  - Close window (Cmd+W)
  - Quit application (Cmd+Q)

- [ ] **Native UI Elements**:
  - File dialogs use macOS native style
  - Fonts render correctly (no missing glyphs)
  - Retina display support (sharp rendering on high-DPI screens)

### Resource Loading

- [ ] **File Type Icons**:
  - Folders show folder icon
  - Files show appropriate icons (archive, document, image, etc.)
  - Symbolic links indicated with link icon

- [ ] **Application Icon**:
  - Icon appears correctly in Finder
  - Icon appears correctly in Dock
  - Icon appears correctly in Cmd+Tab switcher

- [ ] **Log Window**:
  - View log output
  - No "failed to load resource" errors in log

### Performance

- [ ] **Launch Time**: App launches within 5 seconds
- [ ] **Responsiveness**: UI remains responsive during file transfers
- [ ] **Memory Usage**: Check Activity Monitor - no excessive memory leaks
- [ ] **Transfer Speed**: File transfers complete at reasonable speeds

## Relocatability Testing

### Test 1: Move to Applications

```bash
# Move app to Applications
cp -R ~/Desktop/gFTP.app /Applications/

# Launch from Applications
open /Applications/gFTP.app
```

- [ ] App launches successfully from new location
- [ ] All functionality works (connect, transfer, etc.)

### Test 2: Run from Different Location

```bash
# Move to arbitrary location
mkdir -p ~/TestApps
cp -R ~/Desktop/gFTP.app ~/TestApps/

# Launch
open ~/TestApps/gFTP.app
```

- [ ] App launches from custom location
- [ ] All resources load correctly

### Test 3: Verify No External Dependencies

```bash
# Check library dependencies
otool -L /path/to/gFTP.app/Contents/Resources/bin/gftp-gtk | grep -v "@rpath" | grep -v "/usr/lib" | grep -v "/System"
```

- [ ] No dependencies outside bundle (except system libs)
- [ ] All @rpath references resolve to bundled libraries

## Clean System Testing

### Prerequisites

Test on a system WITHOUT:
- Homebrew installed
- jhbuild installed
- GTK installed via any package manager

### Test Procedure

1. [ ] Copy gFTP.app to clean test Mac
2. [ ] Double-click to launch
3. [ ] Verify no missing library errors
4. [ ] Test basic FTP connection
5. [ ] Test file transfer

**Recommended**: Use a macOS virtual machine (VMware Fusion, Parallels) for clean testing

## Code Signing & Gatekeeper

### Ad-hoc Signed Bundle (Development)

```bash
# Verify signature
codesign -dv --verbose=4 ~/Desktop/gFTP.app
```

- [ ] Reports "Signature=adhoc"
- [ ] Shows "Sealed Resources" count
- [ ] No verification errors

- [ ] **First Launch**: Right-click → Open (bypasses Gatekeeper warning)
- [ ] **Subsequent Launches**: Double-click launches without warning

### Developer ID Signed Bundle (Distribution)

```bash
# Verify signature
codesign -dv --verbose=4 ~/Desktop/gFTP.app
spctl -a -vv ~/Desktop/gFTP.app
```

- [ ] Reports Developer ID certificate
- [ ] Hardened runtime enabled
- [ ] Gatekeeper acceptance: "source=Developer ID"

- [ ] **First Launch**: No Gatekeeper warning, launches immediately
- [ ] **Downloaded Bundle**: Test with bundle downloaded from internet (quarantine attribute)

## Troubleshooting Common Issues

### Issue: "App is damaged and can't be opened"

**Solution**: Bundle must be code signed (even ad-hoc)
```bash
codesign -s - --deep --force ~/Desktop/gFTP.app
```

### Issue: Missing translations

**Check**: Verify locale files present
```bash
find gFTP.app -name "gftp.mo" | wc -l  # Should show 64
```

### Issue: Blank window or UI not rendering

**Check**: GTK resources and schemas
```bash
ls gFTP.app/Contents/Resources/share/glib-2.0/schemas/gschemas.compiled
ls gFTP.app/Contents/Resources/share/gtk-3.0/
```

### Issue: "Failed to load resource" errors

**Check Console.app** for detailed error messages:
1. Open Console.app
2. Filter for "gftp" or "gFTP"
3. Launch app and observe errors

Common causes:
- Missing GSettings schemas
- Missing icon theme cache
- Incorrect environment variables

### Issue: SSH/SFTP connections fail

**Check**: SSH binary accessible
```bash
which ssh  # Should show /usr/bin/ssh (system)
```

gFTP uses system OpenSSH, which is always present on macOS.

## Regression Testing

When making changes to the bundle or build process, re-test:

1. [ ] All basic functionality tests
2. [ ] At least one protocol connection (FTP or SFTP)
3. [ ] File transfer (upload and download)
4. [ ] App relocatability (move and relaunch)
5. [ ] Resource loading (icons, translations)

## Test Matrix

Minimum testing should cover:

| macOS Version | Architecture | Result | Notes |
|---------------|--------------|--------|-------|
| macOS 14 (Sonoma) | Apple Silicon | ☐ | Primary target |
| macOS 13 (Ventura) | Apple Silicon | ☐ | |
| macOS 12 (Monterey) | Apple Silicon | ☐ | Minimum supported version |
| macOS 12 (Monterey) | Intel x86_64 | ☐ | If universal binary |

## Reporting Issues

When reporting bugs, include:

1. **Environment**:
   - macOS version (`sw_vers`)
   - Architecture (`uname -m`)
   - App location (Desktop, Applications, etc.)

2. **Bundle Information**:
   ```bash
   plutil -p gFTP.app/Contents/Info.plist | grep CFBundleVersion
   codesign -dv gFTP.app
   du -sh gFTP.app
   ```

3. **Error Messages**:
   - Screenshot of error dialog
   - Console.app logs
   - Terminal output if launched via `open`

4. **Steps to Reproduce**:
   - Exact steps that trigger the issue
   - Expected vs. actual behavior

## Success Criteria

A bundle is ready for release when:

- ✅ All automated validation tests pass
- ✅ All basic functionality tests pass
- ✅ Tested on at least 2 macOS versions
- ✅ Tested on clean system (no jhbuild/Homebrew)
- ✅ Code signature valid
- ✅ Bundle size < 250 MB
- ✅ No critical or high-priority bugs
