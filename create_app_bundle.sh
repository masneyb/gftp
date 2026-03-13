set -e

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-./gftp-bundle-content}"
BUNDLE_NAME="gFTP.app"
BUNDLE_ID="org.gftp.gftp-gtk"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEST_DIR="${1:-.}"

# --- Find AppBundleGenerator ---
# 1. Use environment variable if set
# 2. Otherwise, search in PATH
# 3. Check for ../AppBundleGenerator
# 4. If not found, provide instructions
APP_BUNDLE_GENERATOR=""
if [ -n "$APP_BUNDLE_GENERATOR_PATH" ]; then
    if [ -x "$APP_BUNDLE_GENERATOR_PATH" ]; then
        APP_BUNDLE_GENERATOR="$APP_BUNDLE_GENERATOR_PATH"
    fi
elif command -v AppBundleGenerator &>/dev/null; then
    APP_BUNDLE_GENERATOR=$(command -v AppBundleGenerator)
elif [ -x "../AppBundleGenerator/AppBundleGenerator" ]; then
    APP_BUNDLE_GENERATOR="../AppBundleGenerator/AppBundleGenerator"
fi
# --- End Find AppBundleGenerator ---

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
error() {
    echo -e "${RED}ERROR: $1${NC}" >&2
    exit 1
}

info() {
    echo -e "${GREEN}INFO: $1${NC}"
}

warn() {
    echo -e "${YELLOW}WARN: $1${NC}"
}

# --- Prerequisite Check ---
# Verify AppBundleGenerator is found
if [ -z "$APP_BUNDLE_GENERATOR" ]; then
    error "AppBundleGenerator not found. Please ensure the 'AppBundleGenerator' executable is in your PATH or set the APP_BUNDLE_GENERATOR_PATH environment variable. You can download it from: https://github.com/sedwards-ibowl/AppBundleGenerator"
fi
info "Found AppBundleGenerator at: $APP_BUNDLE_GENERATOR"
# --- End Prerequisite Check ---

# Verify install prefix exists
if [ ! -d "$INSTALL_PREFIX" ]; then
    error "Install prefix not found: $INSTALL_PREFIX. Please run ./build_gftp_homebrew.sh first."
fi

# Check for required binary
GFTP_GTK="$INSTALL_PREFIX/bin/gftp-gtk"
if [ ! -f "$GFTP_GTK" ]; then
    error "gftp-gtk not found at: $GFTP_GTK"
fi


info "Creating macOS application bundle: $BUNDLE_NAME"
info "Install prefix: $INSTALL_PREFIX"

# Remove existing bundle
if [ -d "$DEST_DIR/$BUNDLE_NAME" ]; then
    warn "Removing existing bundle: $DEST_DIR/$BUNDLE_NAME"
    rm -rf "$DEST_DIR/$BUNDLE_NAME"
fi

# Create a wrapper script that sets up the environment
WRAPPER_SCRIPT="/tmp/gftp-launcher.sh"
cat > "$WRAPPER_SCRIPT" << 'EOF'
#!/bin/bash
# gFTP launcher script
BUNDLE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
exec "$BUNDLE_DIR/Contents/MacOS/gftp-gtk" "$@"
EOF
chmod +x "$WRAPPER_SCRIPT"


# Get version from gftp-gtk
VERSION=$(strings "$GFTP_GTK" | grep -E '^2\.[0-9]+\.[0-9]+' | head -1 || echo "2.9.1b")

info "Clearing extended attributes from bundle contents..."
xattr -cr "$INSTALL_PREFIX"

# Use AppBundleGenerator to create the bundle
"$APP_BUNDLE_GENERATOR" \
  --icon "$SCRIPT_DIR/icons/scalable/gftp.svg" \
  --sign - \
  --hardened-runtime \
  --bundle-id "$BUNDLE_ID" \
  --category public.app-category.utilities \
  --version "$VERSION" \
  --min-os 12.0 \
  --executable "$GFTP_GTK" \
  --resource "$INSTALL_PREFIX/share/locale" \
  --resource "$INSTALL_PREFIX/share/icons" \
  --resource "$INSTALL_PREFIX/share/gftp" \
  --resource "$INSTALL_PREFIX/share/doc/gftp" \
  'gFTP' "$DEST_DIR" "$WRAPPER_SCRIPT"


echo ""
echo "✓ Bundle created successfully!"
echo "  Location: ${DEST_DIR}/${BUNDLE_NAME}"
echo ""
echo "To test the bundle:"
echo "  open ${DEST_DIR}/${BUNDLE_NAME}"
echo ""
echo "Or run directly:"
echo "  ${DEST_DIR}/${BUNDLE_NAME}/Contents/MacOS/gftp-gtk"
echo ""
# Create PkgInfo
echo "APPLGFTP" > "$DEST_DIR/$BUNDLE_NAME/Contents/PkgInfo"

# Verify bundle structure
info "Verifying bundle structure..."
if [ ! -f "$DEST_DIR/$BUNDLE_NAME/Contents/MacOS/gftp-gtk" ]; then
    error "Failed to create bundle: executable not found"
fi

if [ ! -f "$DEST_DIR/$BUNDLE_NAME/Contents/Info.plist" ]; then
    error "Failed to create bundle: Info.plist not found"
fi

# Print bundle info
info "Bundle created successfully!"
echo ""
echo "Bundle information:"
echo "  Location: $DEST_DIR/$BUNDLE_NAME"
echo "  Executable: $BUNDLE_NAME/Contents/MacOS/gftp-gtk"
echo "  Translations: $(find "$DEST_DIR/$BUNDLE_NAME/Contents/Resources/locale" -name "gftp.mo" 2>/dev/null | wc -l | tr -d ' ') locales"
echo "  Version: $VERSION"
echo ""
echo "To test the bundle:"
echo "  open $BUNDLE_NAME"
echo ""
echo "To create a DMG:"
echo "  ./create_dmg.sh"
