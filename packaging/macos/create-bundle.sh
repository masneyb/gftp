#!/bin/bash
#
# Create macOS Application Bundle for gFTP
# Requires: AppBundleGenerator, jhbuild installation
#

set -e

# Default values
JHBUILD_PREFIX="${JHBUILD_PREFIX:-$HOME/source/jhbuild/install}"
APP_BUNDLE_GENERATOR="${APP_BUNDLE_GENERATOR:-/usr/local/bin/AppBundleGenerator}"
DESTINATION="${DESTINATION:-$HOME/Desktop}"
GFTP_SOURCE="${GFTP_SOURCE:-$(cd "$(dirname "$0")/../.." && pwd)}"
VERSION="2.9.1b"
SIGN_IDENTITY="${SIGN_IDENTITY:--}"

# Check prerequisites
if [ ! -x "$APP_BUNDLE_GENERATOR" ]; then
    echo "Error: AppBundleGenerator not found at: $APP_BUNDLE_GENERATOR"
    echo "Set APP_BUNDLE_GENERATOR environment variable or install to /usr/local/bin"
    exit 1
fi

if [ ! -x "$JHBUILD_PREFIX/bin/gftp-gtk" ]; then
    echo "Error: gftp-gtk not found at: $JHBUILD_PREFIX/bin/gftp-gtk"
    echo "Set JHBUILD_PREFIX environment variable to your jhbuild install directory"
    exit 1
fi

if [ ! -f "$GFTP_SOURCE/icons/scalable/gftp.svg" ]; then
    echo "Error: gFTP source icon not found at: $GFTP_SOURCE/icons/scalable/gftp.svg"
    echo "Set GFTP_SOURCE environment variable to gFTP source directory"
    exit 1
fi

echo "Creating gFTP macOS Application Bundle"
echo "========================================"
echo "Source:      $GFTP_SOURCE"
echo "jhbuild:     $JHBUILD_PREFIX"
echo "Destination: $DESTINATION"
echo "Version:     $VERSION"
echo "Signing:     $SIGN_IDENTITY"
echo ""

# Create the bundle
"$APP_BUNDLE_GENERATOR" \
  --icon "$GFTP_SOURCE/icons/scalable/gftp.svg" \
  --identifier org.gftp.gftp-gtk \
  --version "$VERSION" \
  --category public.app-category.utilities \
  --min-os 12.0 \
  --sign "$SIGN_IDENTITY" \
  --hardened-runtime \
  --allow-dyld-vars \
  --stage-dependencies "$JHBUILD_PREFIX" \
  'gFTP' "$DESTINATION" \
  "$JHBUILD_PREFIX/bin/gftp-gtk"

BUNDLE="$DESTINATION/gFTP.app"

if [ ! -d "$BUNDLE" ]; then
    echo "Error: Bundle was not created"
    exit 1
fi

echo ""
echo "Installing font rendering configuration..."

# Install fontconfig for macOS rendering
mkdir -p "$BUNDLE/Contents/Resources/etc/fonts/conf.d"
if [ -f "$GFTP_SOURCE/packaging/macos/fontconfig/99-macos-rendering.conf" ]; then
    cp "$GFTP_SOURCE/packaging/macos/fontconfig/99-macos-rendering.conf" \
       "$BUNDLE/Contents/Resources/etc/fonts/conf.d/"
    echo "✓ Installed fontconfig rendering configuration"
else
    echo "⚠ Warning: Font rendering config not found, skipping"
fi

# Update launcher to enable CoreText by default
LAUNCHER="$BUNDLE/Contents/MacOS/gFTP"
if ! grep -q "^export PANGOCAIRO_BACKEND=coretext" "$LAUNCHER"; then
    echo "✓ Enabling CoreText backend for native macOS font rendering"
    # Add after XDG_DATA_DIRS line
    sed -i '' '/^export XDG_DATA_DIRS=/a\
\
# Enable native macOS font rendering via CoreText\
export PANGOCAIRO_BACKEND=coretext
' "$LAUNCHER"
fi

# Create the toggle script
echo "✓ Installing font backend toggle script"
cat > "$BUNDLE/Contents/Resources/bin/toggle-font-backend.sh" << 'TOGGLE_EOF'
#!/bin/bash
#
# Font Backend Toggle Script for gFTP
# Switches between CoreText (native macOS) and FreeType backends
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LAUNCHER="$SCRIPT_DIR/../../MacOS/gFTP"

if [ ! -f "$LAUNCHER" ]; then
    echo "Error: Launcher script not found at $LAUNCHER"
    exit 1
fi

# Check current backend setting
if grep -q "^export PANGOCAIRO_BACKEND=coretext" "$LAUNCHER"; then
    CURRENT="coretext"
else
    CURRENT="freetype"
fi

# Determine action
if [ "$1" = "status" ]; then
    echo "Current font backend: $CURRENT"
    echo ""
    echo "CoreText backend:"
    echo "  - Native macOS font rendering"
    echo "  - Uses CoreText and CoreGraphics APIs"
    echo "  - Better kerning and hinting on macOS"
    echo "  - Access to all macOS system fonts"
    echo ""
    echo "FreeType backend:"
    echo "  - Cross-platform font rendering"
    echo "  - Uses FreeType library"
    echo "  - More predictable across platforms"
    echo "  - May look slightly different from native macOS apps"
    exit 0
fi

if [ "$1" = "coretext" ]; then
    NEW_BACKEND="coretext"
    NEW_DESC="native macOS CoreText"
elif [ "$1" = "freetype" ]; then
    NEW_BACKEND="freetype"
    NEW_DESC="cross-platform FreeType"
else
    echo "Usage: $0 {coretext|freetype|status}"
    echo ""
    echo "  coretext - Enable native macOS font rendering (recommended)"
    echo "  freetype - Enable cross-platform FreeType rendering"
    echo "  status   - Show current backend"
    echo ""
    echo "Current backend: $CURRENT"
    exit 1
fi

if [ "$CURRENT" = "$NEW_BACKEND" ]; then
    echo "Already using $NEW_BACKEND backend"
    exit 0
fi

# Make backup
cp "$LAUNCHER" "$LAUNCHER.bak"

# Toggle the backend
if [ "$NEW_BACKEND" = "coretext" ]; then
    # Enable CoreText
    if grep -q "^export PANGOCAIRO_BACKEND=" "$LAUNCHER"; then
        # Replace existing line
        sed -i '' 's/^export PANGOCAIRO_BACKEND=.*/export PANGOCAIRO_BACKEND=coretext/' "$LAUNCHER"
    else
        # Add after XDG_DATA_DIRS line
        sed -i '' '/^export XDG_DATA_DIRS=/a\
\
# Enable native macOS font rendering via CoreText\
export PANGOCAIRO_BACKEND=coretext
' "$LAUNCHER"
    fi
else
    # Disable CoreText (use FreeType)
    sed -i '' '/^# Enable native macOS font rendering/d' "$LAUNCHER"
    sed -i '' '/^export PANGOCAIRO_BACKEND=/d' "$LAUNCHER"
fi

echo "✓ Switched font backend from $CURRENT to $NEW_BACKEND ($NEW_DESC)"
echo ""
echo "Please restart gFTP for changes to take effect:"
echo "  killall gftp-gtk"
echo "  open \"$(dirname "$LAUNCHER")/../..\""
echo ""
echo "To compare rendering quality:"
echo "  1. Take a screenshot with current backend"
echo "  2. Run: $0 ${CURRENT}"
echo "  3. Restart gFTP and compare"
TOGGLE_EOF

chmod +x "$BUNDLE/Contents/Resources/bin/toggle-font-backend.sh"

# Re-sign if needed
if [ "$SIGN_IDENTITY" != "-" ]; then
    echo ""
    echo "Re-signing bundle after modifications..."
    codesign --force --deep --sign "$SIGN_IDENTITY" --options runtime "$BUNDLE"
fi

echo ""
echo "======================================"
echo "Bundle created successfully!"
echo "======================================"
echo "Location: $BUNDLE"
echo "Size: $(du -sh "$BUNDLE" | cut -f1)"
echo ""
echo "Font rendering: CoreText (native macOS)"
echo "Toggle script:  $BUNDLE/Contents/Resources/bin/toggle-font-backend.sh"
echo ""
echo "To launch:"
echo "  open \"$BUNDLE\""
echo ""
echo "To test font backend toggle:"
echo "  \"$BUNDLE/Contents/Resources/bin/toggle-font-backend.sh\" status"
