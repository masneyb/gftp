# Building gFTP for macOS

This guide explains how to build relocatable macOS application bundles for gFTP using jhbuild and AppBundleGenerator.

## Overview

The gFTP macOS build process creates a self-contained `.app` bundle that includes:
- gFTP GTK3 application
- All GTK3 and GLib dependencies (~150 MB of libraries)
- 64 language translations
- GTK themes, icons, and resources
- Font configuration
- Native macOS menu bar integration (gtk-mac-integration)

**Bundle Size**: ~150 MB
**Target**: macOS 12.0+ (Monterey and later)
**Architecture**: Native builds (Apple Silicon arm64 or Intel x86_64)

## Prerequisites

### Required Tools

1. **Xcode Command Line Tools**:
   ```bash
   xcode-select --install
   ```

2. **Homebrew** (for build tools only, NOT for dependencies):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **Build Tools via Homebrew**:
   ```bash
   brew install meson ninja cmake automake autoconf libtool pkg-config gettext intltool
   ```

### jhbuild Setup

jhbuild is used to build GTK3 and all dependencies natively without Homebrew libraries.

1. **Clone and Build jhbuild**:
   ```bash
   mkdir -p ~/source/jhbuild
   cd ~/source/jhbuild

   git clone https://gitlab.gnome.org/GNOME/jhbuild.git jhbuild-src
   cd jhbuild-src
   ./autogen.sh --simple-install --prefix=$HOME/source/jhbuild/install
   make && make install
   ```

2. **Download GTK-OSX Modulesets**:
   ```bash
   mkdir -p ~/source/jhbuild/modulesets
   cd ~/source/jhbuild/modulesets

   curl -O https://gitlab.gnome.org/GNOME/gtk-osx/-/raw/master/modulesets-stable/gtk-osx-bootstrap.modules
   curl -O https://gitlab.gnome.org/GNOME/gtk-osx/-/raw/master/modulesets-stable/gtk-osx.modules
   ```

3. **Create jhbuildrc Configuration**:
   ```bash
   cat > ~/source/jhbuild/jhbuildrc << 'EOF'
   import os

   prefix = os.path.expanduser('~/source/jhbuild/install')
   checkoutroot = os.path.expanduser('~/source/jhbuild/checkout')
   buildroot = os.path.expanduser('~/source/jhbuild/build')
   tarballdir = os.path.expanduser('~/source/jhbuild/pkgs')

   # Use system clang
   os.environ['CC'] = '/usr/bin/clang'
   os.environ['CXX'] = '/usr/bin/clang++'

   # macOS SDK
   _sdk = '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk'
   os.environ['CFLAGS'] = f'-isysroot {_sdk} -I{prefix}/include'
   os.environ['LDFLAGS'] = f'-isysroot {_sdk} -L{prefix}/lib'

   # Ensure only jhbuild libraries are found
   os.environ['PKG_CONFIG_LIBDIR'] = f'{prefix}/lib/pkgconfig:{prefix}/share/pkgconfig'

   moduleset = [
       'gtk-osx-bootstrap.modules',
       'gtk-osx.modules',
   ]

   modules = [
       'meta-gtk-osx-bootstrap',
       'meta-gtk-osx-gtk3',
       'gtk-mac-integration',
   ]

   makeargs = '-j4'  # Parallel builds (adjust for your CPU)
   skip = ['gtk-doc']  # Skip documentation
   EOF
   ```

4. **Add gFTP Moduleset**:
   ```bash
   cat > ~/source/jhbuild/modulesets/gftp.modules << 'EOF'
   <?xml version="1.0"?>
   <!DOCTYPE moduleset SYSTEM "moduleset.dtd">
   <?xml-stylesheet type="text/xsl" href="moduleset.xsl"?>
   <moduleset>
     <repository type="git" name="github" href="https://github.com/"/>

     <meson id="gftp" mesonargs="-Dgtk3=true -Dgtk2=false -Dssl=true">
       <branch repo="github" module="sedwards-ibowl/gftp.git" checkoutdir="gftp"/>
       <dependencies>
         <dep package="gtk+-3.0"/>
         <dep package="gtk-mac-integration"/>
         <dep package="glib"/>
         <dep package="openssl"/>
         <dep package="intltool"/>
         <dep package="gettext"/>
       </dependencies>
     </meson>
   </moduleset>
   EOF
   ```

5. **Update jhbuildrc to Include gFTP**:
   ```bash
   # Add to moduleset list in jhbuildrc
   echo "moduleset.append('gftp.modules')" >> ~/source/jhbuild/jhbuildrc
   ```

6. **Build GTK3 Stack**:
   ```bash
   export PATH=~/source/jhbuild/install/bin:$PATH
   cd ~/source/jhbuild

   # Bootstrap (Python, build tools)
   jhbuild -f jhbuildrc build meta-gtk-osx-bootstrap

   # GTK3 and dependencies (~1-2 hours first time)
   jhbuild -f jhbuildrc build meta-gtk-osx-gtk3

   # macOS integration
   jhbuild -f jhbuildrc build gtk-mac-integration
   ```

### AppBundleGenerator Setup

1. **Clone and Build**:
   ```bash
   cd ~/source
   git clone https://github.com/sedwards-ibowl/AppBundleGenerator.git
   cd AppBundleGenerator
   make
   ```

2. **Verify**:
   ```bash
   ./AppBundleGenerator --help
   ```

## Building gFTP

### Method 1: Using build_gftp_app.sh (Recommended)

The automated build script handles everything:

```bash
cd ~/source/jhbuild/checkout/gftp
./build_gftp_app.sh
```

**What it does**:
1. Ensures gFTP is built and installed via jhbuild
2. Finds the best icon (PNG or SVG)
3. Runs AppBundleGenerator with correct flags
4. Stages all dependencies (libraries, resources, translations)
5. Creates launcher script with environment setup
6. Code signs the bundle (ad-hoc)
7. Validates the bundle structure

**Output**: `~/Desktop/gFTP.app`

**Configuration**: Edit variables at the top of `build_gftp_app.sh`:
```bash
JHBUILD_PREFIX="${JHBUILD_PREFIX:-$HOME/source/jhbuild/install}"
DEST_DIR="${DEST_DIR:-$HOME/Desktop}"
APP_NAME="gFTP"
BUNDLE_ID="org.gftp.gftp-gtk"
VERSION="2.9.1b"
```

### Method 2: Manual Build via jhbuild

```bash
cd ~/source/jhbuild

# Build gFTP only
jhbuild -f jhbuildrc buildone gftp

# Rebuild from scratch
jhbuild -f jhbuildrc cleanone gftp
jhbuild -f jhbuildrc buildone gftp
```

Then create bundle manually:
```bash
~/source/AppBundleGenerator/AppBundleGenerator \
    --icon ~/source/jhbuild/checkout/gftp/icons/scalable/gftp.svg \
    --identifier org.gftp.gftp-gtk \
    --version 2.9.1b \
    --category public.app-category.utilities \
    --min-os 12.0 \
    --sign - \
    --hardened-runtime \
    --allow-dyld-vars \
    --stage-dependencies ~/source/jhbuild/install \
    "gFTP" \
    ~/Desktop \
    ~/source/jhbuild/install/bin/gftp-gtk
```

## Build Customization

### Changing GTK Version

Edit `gftp.modules`:
```xml
<meson id="gftp" mesonargs="-Dgtk3=true -Dgtk2=false">
```

### Disabling SSL Support

Edit `gftp.modules`:
```xml
<meson id="gftp" mesonargs="-Dgtk3=true -Dssl=false">
```

### Custom Install Prefix

Edit `jhbuildrc`:
```python
prefix = '/opt/gftp'  # Custom location
```

### Build Options

Available meson options (see `meson_options.txt`):
- `gtk3` / `gtk2`: Choose GTK version
- `gtkport`: Enable GTK GUI (default: true)
- `textport`: Enable CLI (default: true)
- `ssl`: Enable OpenSSL/TLS (default: true)

## Validation

After building, validate the bundle:

```bash
./scripts/validate_bundle.sh ~/Desktop/gFTP.app
```

This checks:
- Bundle structure
- Info.plist validity
- Library relocatability
- All 64 translations present
- GSettings schemas compiled
- GTK resources included
- Code signature
- Bundle size < 250 MB

## Creating a DMG

Use the included script:

```bash
./create_dmg_for_app.sh ~/Desktop/gFTP.app
```

Or use `create-dmg` tool:

```bash
brew install create-dmg

create-dmg \
  --volname "gFTP" \
  --volicon icons/scalable/gftp.svg \
  --window-size 800 400 \
  --icon-size 100 \
  --app-drop-link 600 185 \
  gFTP-2.9.1b.dmg \
  ~/Desktop/gFTP.app
```

## Code Signing for Distribution

### Developer ID Signing

Requires Apple Developer account ($99/year).

1. **Get Certificate**:
   - Download "Developer ID Application" certificate from developer.apple.com
   - Install in Keychain

2. **Sign Bundle**:
   ```bash
   codesign --deep --force --sign "Developer ID Application: Your Name" \
     --options runtime \
     --entitlements macos/entitlements.plist \
     ~/Desktop/gFTP.app
   ```

3. **Verify**:
   ```bash
   codesign -dv --verbose=4 ~/Desktop/gFTP.app
   spctl -a -vv ~/Desktop/gFTP.app
   ```

### Notarization (Optional)

Required for Gatekeeper acceptance on macOS 10.15+:

```bash
# Create DMG first
./create_dmg_for_app.sh ~/Desktop/gFTP.app

# Submit for notarization
xcrun notarytool submit gFTP-2.9.1b.dmg \
  --apple-id "your@email.com" \
  --team-id "TEAMID" \
  --password "app-specific-password" \
  --wait

# Staple ticket
xcrun stapler staple gFTP-2.9.1b.dmg
```

## Troubleshooting

### Build Fails: "GTK not found"

**Solution**: Ensure PKG_CONFIG_PATH is set:
```bash
export PKG_CONFIG_PATH=~/source/jhbuild/install/lib/pkgconfig
```

### Bundle Launches But UI is Blank

**Check**: GSettings schemas compiled:
```bash
ls ~/Desktop/gFTP.app/Contents/Resources/share/glib-2.0/schemas/gschemas.compiled
```

**Fix**: Recompile schemas:
```bash
glib-compile-schemas ~/Desktop/gFTP.app/Contents/Resources/share/glib-2.0/schemas/
```

### Missing Translations

**Check**: Locale files present:
```bash
find ~/Desktop/gFTP.app -name "gftp.mo" | wc -l
# Should show 64
```

**Fix**: Ensure `--stage-dependencies` flag used with AppBundleGenerator

### "App is damaged" Error

**Solution**: Code sign the bundle:
```bash
codesign -s - --deep --force ~/Desktop/gFTP.app
```

### Libraries Still Reference jhbuild Paths

**Check**:
```bash
otool -L ~/Desktop/gFTP.app/Contents/Resources/bin/gftp-gtk | grep jhbuild
```

**Fix**: RPATH rewriting failed. This is a known issue being addressed in AppBundleGenerator.

## CI/CD Integration

See `.github/workflows/macos-bundle.yml` for automated builds via GitHub Actions.

The workflow:
- Caches jhbuild installation (saves 1-2 hours)
- Builds gFTP on every push
- Creates DMG for releases
- Uploads artifacts

## Directory Structure Reference

```
~/source/jhbuild/
├── jhbuild-src/          # jhbuild source code
├── jhbuildrc             # Configuration file
├── modulesets/           # XML module definitions
│   ├── gtk-osx-bootstrap.modules
│   ├── gtk-osx.modules
│   └── gftp.modules
├── install/              # Install prefix (all built libraries)
│   ├── bin/             # Executables (gftp-gtk, gftp-text)
│   ├── lib/             # Libraries (libgtk-3.0.dylib, etc.)
│   ├── share/           # Resources (translations, icons, schemas)
│   └── etc/             # Configuration (fonts, etc.)
├── checkout/             # Source code checkouts
│   └── gftp/            # gFTP source
├── build/                # Build artifacts
└── pkgs/                 # Downloaded tarballs (cached)
```

## Additional Resources

- [gFTP GitHub Repository](https://github.com/sedwards-ibowl/gftp)
- [GTK-OSX Project](https://gitlab.gnome.org/GNOME/gtk-osx)
- [jhbuild Documentation](https://gitlab.gnome.org/GNOME/jhbuild)
- [AppBundleGenerator](https://github.com/sedwards-ibowl/AppBundleGenerator)
- [macOS Code Signing Guide](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/)
