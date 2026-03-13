# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for building, testing, and releasing gFTP.

## Workflows

### CI (ci.yml)
Runs on every push and pull request to ensure code quality.

**Jobs:**
- **check-format** - Checks for trailing whitespace and tabs
- **build-minimal** - Tests minimal build (text-only, no SSL)
- **build-debug** - Tests debug build configuration
- **build-warnings** - Builds with extra warnings enabled
- **test-install** - Verifies installation process

**When it runs:** On push to master/main/develop, and on all pull requests

### Build (build.yml)
Builds gFTP for multiple platforms and configurations.

**Jobs:**
- **build-linux** - Builds for Linux with GTK2 and GTK3
- **build-macos** - Builds macOS app bundle and creates DMG
- **build-macos-text** - Builds text-only version for macOS

**Artifacts:**
- Linux builds (GTK2 and GTK3)
- macOS DMG (30 day retention)
- macOS text-only binary

**When it runs:** On push to master/main/develop, pull requests, and manual trigger

### Release (release.yml)
Creates releases with binaries for all platforms.

**Jobs:**
- **create-release** - Creates GitHub release
- **build-macos-release** - Builds and uploads macOS DMG
- **build-linux-release** - Builds and uploads Linux binaries
- **build-source-release** - Creates and uploads source tarball

**Artifacts:**
- `gFTP-{version}-{git-rev}-macOS.dmg` - macOS disk image
- `gftp-linux-gtk3-{arch}.tar.gz` - Linux GTK3 build
- `gftp-linux-gtk2-{arch}.tar.gz` - Linux GTK2 build
- `gftp-{version}-source.tar.gz` - Source tarball

**When it runs:**
- Automatically on version tags (e.g., `v2.9.1b`)
- Manual trigger via workflow_dispatch

## Creating a Release

### Method 1: Tag-based (Recommended)

```bash
# Create and push a version tag
git tag -a v2.9.1b -m "Release version 2.9.1b"
git push origin v2.9.1b
```

The release workflow will automatically:
1. Create a GitHub release
2. Build binaries for all platforms
3. Upload artifacts to the release
4. Generate release notes

### Method 2: Manual Trigger

1. Go to the "Actions" tab on GitHub
2. Select "Release" workflow
3. Click "Run workflow"
4. Enter the tag name (e.g., `v2.9.1b`)
5. Click "Run workflow"

## Build Scripts

### create_app_bundle.sh
Creates a macOS application bundle from built binaries.

**Usage:**
```bash
# Using default install prefix
./create_app_bundle.sh

# Using custom install prefix
INSTALL_PREFIX=/custom/path ./create_app_bundle.sh
```

**Output:** `gFTP.app` - macOS application bundle

### create_dmg.sh
Creates a distributable DMG from the app bundle.

**Usage:**
```bash
# Must run create_app_bundle.sh first
./create_dmg.sh
```

**Output:** `gFTP-{version}-{git-rev}-macOS.dmg`

## Testing Locally

### Build for Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install meson ninja-build pkg-config \
  libglib2.0-dev libgtk-3-dev libssl-dev gettext

# Configure and build
meson setup build -Dgtk2=false -Dgtk3=true
ninja -C build

# Test installation
DESTDIR=/tmp/gftp-test ninja -C build install
```

### Build for macOS

```bash
# Install dependencies
brew install meson ninja pkg-config gtk+3 openssl@3

# Configure and build
export PKG_CONFIG_PATH="/usr/local/opt/openssl@3/lib/pkgconfig:$PKG_CONFIG_PATH"
meson setup build -Dgtk2=false -Dgtk3=true
ninja -C build

# Create app bundle and DMG
DESTDIR="$(pwd)/install-root" ninja -C build install
INSTALL_PREFIX="$(pwd)/install-root/usr/local" ./create_app_bundle.sh
./create_dmg.sh
```

## Artifacts Retention

- **CI builds:** 7 days
- **Regular builds:** 7 days for binaries, 30 days for DMG
- **Release builds:** Permanent (attached to GitHub release)

## Code Signing (macOS)

The release workflow includes optional code signing for macOS builds. To enable:

1. Add a signing certificate to your macOS runner
2. The workflow will automatically detect and use it
3. If no certificate is found, the build continues unsigned

For distribution outside the Mac App Store, you'll need:
- Developer ID Application certificate
- Notarization (for macOS 10.15+)

## Troubleshooting

### Build fails on macOS
- Ensure Homebrew dependencies are up to date
- Check that OpenSSL 3.x is installed and in PKG_CONFIG_PATH

### DMG creation fails
- Verify `gFTP.app` bundle exists (run `create_app_bundle.sh` first)
- Check that you have write permissions in the current directory

### Release workflow doesn't trigger
- Ensure tag follows version format: `v*` (e.g., `v2.9.1b`)
- Check that tag is pushed to remote: `git push origin v2.9.1b`

### Artifacts not uploading
- Check GitHub Actions quota (storage and minutes)
- Verify artifact paths exist after build
- Check workflow permissions in repository settings

## Dependencies

### Linux
- meson >= 0.50.0
- ninja
- pkg-config
- glib-2.0 >= 2.32
- gtk+-2.0 >= 2.14 OR gtk+-3.0
- openssl >= 3.0 (optional, for FTPS)
- gettext (for translations)

### macOS
- Same as Linux, but installed via Homebrew
- Xcode Command Line Tools

## Workflow Permissions

These workflows require:
- `contents: write` - For creating releases
- `actions: read` - For checking workflow status

These are typically granted by default, but can be restricted in repository settings.
