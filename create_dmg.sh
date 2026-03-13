#!/bin/bash
# create_dmg.sh - Create a distributable DMG for gFTP
# This script creates a macOS disk image (DMG) from the gFTP.app bundle

set -e

# Configuration
BUNDLE_NAME="gFTP.app"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION=$(defaults read "$SCRIPT_DIR/$BUNDLE_NAME/Contents/Info.plist" CFBundleShortVersionString 2>/dev/null || echo "2.9.1b")
GIT_REV=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
DMG_NAME="gFTP-${VERSION}-${GIT_REV}-macOS"
VOLUME_NAME="gFTP $VERSION"

# Include source code by default (can be disabled with INCLUDE_SOURCE=0)
INCLUDE_SOURCE="${INCLUDE_SOURCE:-1}"

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

# Verify bundle exists
if [ ! -d "$BUNDLE_NAME" ]; then
    error "Bundle not found: $BUNDLE_NAME. Run ./create_app_bundle.sh first."
fi

info "Creating DMG for gFTP $VERSION (git: $GIT_REV)"

# Remove old DMG if it exists
if [ -f "${DMG_NAME}.dmg" ]; then
    warn "Removing existing DMG: ${DMG_NAME}.dmg"
    rm -f "${DMG_NAME}.dmg"
fi

# Create temporary directory for DMG contents
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

info "Preparing DMG contents..."
cp -R "$BUNDLE_NAME" "$TEMP_DIR/"

# Include source code if requested
if [ "$INCLUDE_SOURCE" = "1" ]; then
    info "Including source code..."
    mkdir -p "$TEMP_DIR/Source"

    # Copy source files (excluding build artifacts)
    rsync -av \
        --exclude=build \
        --exclude=gFTP.app \
        --exclude=.git \
        --exclude='*.o' \
        --exclude='*.a' \
        --exclude='*.dylib' \
        --exclude='*.dmg' \
        "$SCRIPT_DIR/" \
        "$TEMP_DIR/Source/gftp/"
fi

# Create README for DMG
cat > "$TEMP_DIR/README.txt" << EOF
gFTP - GTK+ FTP Client
Version: $VERSION

gFTP is a multi-threaded file transfer client supporting:
  - FTP (File Transfer Protocol)
  - FTPS (FTP over TLS/SSL)
  - SFTP (SSH File Transfer Protocol)
  - HTTP/HTTPS
  - Local file browsing

Installation:
  Drag gFTP.app to your Applications folder

Usage:
  Double-click gFTP.app to launch

Configuration:
  Settings are stored in ~/.config/gftp/

Documentation:
  Visit: https://github.com/sedwards-ibowl/gftp
  or: https://www.gftp.org

Source Code:
  Included in the Source/gftp directory

License:
  MIT License - See LICENSE file

Requirements:
  macOS 10.13 or later
EOF

# Copy license and documentation if available
if [ -f "LICENSE" ]; then
    cp LICENSE "$TEMP_DIR/"
fi

if [ -f "README.md" ]; then
    cp README.md "$TEMP_DIR/"
fi

# Create symbolic link to Applications folder for easy installation
info "Creating Applications symlink..."
ln -s /Applications "$TEMP_DIR/Applications"

# Create DMG
info "Creating DMG: ${DMG_NAME}.dmg"
hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$TEMP_DIR" \
    -ov \
    -format UDZO \
    -imagekey zlib-level=9 \
    "${DMG_NAME}.dmg"

# Verify DMG was created
if [ ! -f "${DMG_NAME}.dmg" ]; then
    error "Failed to create DMG"
fi

# Get DMG size
DMG_SIZE=$(du -h "${DMG_NAME}.dmg" | cut -f1)

info "DMG created successfully!"
echo ""
echo "DMG information:"
echo "  File: $SCRIPT_DIR/${DMG_NAME}.dmg"
echo "  Size: $DMG_SIZE"
echo "  Version: $VERSION"
echo "  Git revision: $GIT_REV"
echo ""
echo "To test the DMG:"
echo "  open ${DMG_NAME}.dmg"
echo ""
echo "To verify the bundle:"
echo "  hdiutil verify ${DMG_NAME}.dmg"
echo ""
echo "Note: Source code is included by default"
echo "To exclude source code from the DMG:"
echo "  INCLUDE_SOURCE=0 ./create_dmg.sh"
