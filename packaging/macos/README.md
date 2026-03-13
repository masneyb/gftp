# macOS Packaging

This directory contains files and instructions for creating native macOS application bundles for gFTP.

## Prerequisites

- macOS 12.0 or later
- [AppBundleGenerator](https://github.com/stevenedwardss/AppBundleGenerator) installed
- gFTP built and installed via jhbuild or similar prefix

## Creating the Bundle

Use the AppBundleGenerator tool to create a self-contained .app bundle:

```bash
/path/to/AppBundleGenerator \
  --icon /Users/sedwards/source/gftp/icons/scalable/gftp.svg \
  --identifier org.gftp.gftp-gtk \
  --version 2.9.1b \
  --category public.app-category.utilities \
  --min-os 12.0 \
  --sign - \
  --hardened-runtime \
  --allow-dyld-vars \
  --stage-dependencies /path/to/jhbuild/install \
  'gFTP' ~/Desktop \
  /path/to/jhbuild/install/bin/gftp-gtk
```

This will create a `gFTP.app` bundle (~154 MB) with:
- All GTK3/GLib dependencies bundled
- Proper code signing (ad-hoc for development)
- Icon converted to .icns format
- Hardened runtime enabled

## Font Rendering on macOS

gFTP on macOS can use native CoreText font rendering for better quality and access to system fonts.

### Native Font Rendering (CoreText Backend)

The bundled app supports Pango's CoreText backend, which provides:
- Native macOS font rendering through CoreText and CoreGraphics
- Proper hinting optimized for macOS displays
- Native kerning and font metrics
- Full Retina/HiDPI support
- Access to all macOS system fonts (SF Pro, SF Mono, New York, etc.)

To enable CoreText backend, add to the launcher script:
```bash
export PANGOCAIRO_BACKEND=coretext
```

### Font Configuration

The `fontconfig/99-macos-rendering.conf` file provides optimized font rendering settings:
- Slight hinting for macOS-style appearance
- Subpixel antialiasing (RGB)
- LCD filtering
- System font fallbacks (SF Pro, Menlo, Monaco)

To install this configuration in a bundle:
```bash
cp packaging/macos/fontconfig/99-macos-rendering.conf \
   gFTP.app/Contents/Resources/etc/fonts/conf.d/
```

### Toggling Font Backends

A toggle script is provided in bundled apps at:
```
gFTP.app/Contents/Resources/bin/toggle-font-backend.sh
```

Usage:
```bash
# Show current backend
./toggle-font-backend.sh status

# Enable native CoreText (recommended for macOS)
./toggle-font-backend.sh coretext

# Enable cross-platform FreeType
./toggle-font-backend.sh freetype
```

## Font Rendering Architecture

```
gFTP GTK3 Application
    ↓
GTK3 Widget Rendering
    ↓
Pango Text Layout
    ├─→ CoreText Backend (macOS native)
    │       ↓
    │   macOS Font System (SF Pro, etc.)
    │       ↓
    │   CoreText APIs (shaping, kerning)
    │
    └─→ FreeType Backend (cross-platform)
            ↓
        FreeType Library
            ↓
        Fontconfig
    ↓
Cairo Graphics
    └─→ Quartz Backend (macOS native)
            ↓
        CoreGraphics (rendering, antialiasing)
```

## Bundle Size

Typical bundle size breakdown:
- **154 MB total**
  - 97 MB: Dynamic libraries (128 .dylib files)
  - 43 MB: Translations (64 languages)
  - 8 MB: GTK themes and icons
  - 6 MB: Other resources

## Distribution

For distribution outside personal use:
1. Sign with Developer ID certificate
2. Notarize with Apple (required for Gatekeeper)
3. Create DMG for distribution

Example:
```bash
# Sign with Developer ID
codesign --sign "Developer ID Application: Your Name" \
         --deep --force --options runtime \
         gFTP.app

# Notarize
xcrun notarytool submit gFTP.dmg \
      --apple-id your@email.com \
      --team-id TEAMID \
      --password @keychain:notarization
```

## Troubleshooting

### Font Issues

**Fonts look blurry or pixelated:**
- Enable CoreText backend: `export PANGOCAIRO_BACKEND=coretext`
- Check fontconfig is finding system fonts
- Verify Retina display settings in Info.plist

**Missing fonts:**
- Check fontconfig configuration in `Resources/etc/fonts/fonts.conf`
- Verify system font directories are listed
- Run: `fc-list` to see available fonts

**CoreText not working:**
- Verify Pango was built with CoreText support
- Check: `otool -L libpangocairo-1.0.0.dylib | grep CoreText`
- Ensure `PANGOCAIRO_BACKEND=coretext` is set before launch

### General Issues

**App won't launch:**
- Check code signature: `codesign -vv gFTP.app`
- Verify hardened runtime: `codesign -d --entitlements - gFTP.app`
- Try ad-hoc signing: `codesign --force --deep --sign - gFTP.app`

**Missing libraries:**
- Verify RPATH is correct: `otool -l gftp-gtk | grep RPATH`
- Check library references: `otool -L gftp-gtk`
- Ensure all dylibs are in `Contents/Resources/lib/`

## References

- [AppBundleGenerator Documentation](https://github.com/stevenedwardss/AppBundleGenerator)
- [Pango CoreText Backend](https://gitlab.gnome.org/GNOME/pango/-/tree/main/pango)
- [Cairo Quartz Backend](https://www.cairographics.org/manual/cairo-Quartz-Surfaces.html)
- [macOS Font Rendering](https://developer.apple.com/documentation/coretext)
