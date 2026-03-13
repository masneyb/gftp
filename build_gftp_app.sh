#!/bin/bash
#
# build_gftp_app.sh - Build gFTP and create macOS app bundle
#
# This script:
# 1. Builds gFTP with meson
# 2. Installs to local prefix
# 3. Creates relocatable app bundle with all dependencies
#
# Requirements:
# - Homebrew with GTK3 stack installed
# - AppBundleGenerator tool
# - meson, ninja
#

set -e # Exit on error

# Configuration
HOMEBREW_PREFIX=$(brew --prefix)
GFTP_SOURCE="$(cd "$(dirname "$0")" && pwd)"
DEST_DIR="${DEST_DIR:-$HOME/Desktop}"
APP_NAME="gFTP"
BUNDLE_ID="org.gftp.gftp-gtk"
VERSION="2.9.1b"
INSTALL_PREFIX="$GFTP_SOURCE/gftp-install"

# Function to clean up build artifacts
clean() {
    echo -e "${YELLOW}Cleaning up build artifacts...${NC}"
    rm -rf "$GFTP_SOURCE/build"
    rm -rf "$INSTALL_PREFIX"
    rm -rf "$DEST_DIR/$APP_NAME.app"
    echo -e "${GREEN}✓ Clean up complete.${NC}"
    exit 0
}

# Check for 'clean' argument
if [ "$1" == "clean" ]; then
    clean
fi

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
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}  gFTP macOS App Bundle Builder${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo -e "${BLUE}Configuration:${NC}"
echo "  Homebrew prefix: $HOMEBREW_PREFIX"
echo "  Source directory: $GFTP_SOURCE"
echo "  AppBundleGenerator: $APP_BUNDLE_GENERATOR"
echo "  Destination: $DEST_DIR"
echo "  Bundle identifier: $BUNDLE_ID"
echo "  Version: $VERSION"
echo ""

# Verify prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

if [ -z "$APP_BUNDLE_GENERATOR" ]; then
    echo -e "${RED}Error: AppBundleGenerator not found. Please ensure the 'AppBundleGenerator' executable is in your PATH or set the APP_BUNDLE_GENERATOR_PATH environment variable. You can download it from: https://github.com/sedwards-ibowl/AppBundleGenerator"
    exit 1
fi
info "Found AppBundleGenerator at: $APP_BUNDLE_GENERATOR"

if ! command -v meson &>/dev/null; then
    echo -e "${RED}Error: meson not found in PATH${NC}"
    exit 1
fi

if ! command -v ninja &>/dev/null; then
    echo -e "${RED}Error: ninja not found in PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All prerequisites found${NC}"
echo ""

# Step 1: Ensure gFTP is built and installed
echo -e "${YELLOW}Step 1: Building and installing gFTP...${NC}"

cd "$GFTP_SOURCE"

# Set up Homebrew environment
export PKG_CONFIG_PATH="$HOMEBREW_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"

meson setup build --prefix="$INSTALL_PREFIX"
ninja -C build install

echo -e "${GREEN}✓ gFTP built and installed to $INSTALL_PREFIX${NC}"
echo ""

# Step 2: Create .icns file from SVG
echo -e "${YELLOW}Step 2: Creating .icns file from SVG...${NC}"
SVG_ICON="$GFTP_SOURCE/icons/scalable/gftp.svg"
ICNS_FILE="$GFTP_SOURCE/gftp.icns"

if [ -f "$SVG_ICON" ]; then
    echo "  Found SVG icon, converting to ICNS..."
    TEMP_ICONSET=$(mktemp -d)/gftp.iconset
    mkdir -p "$TEMP_ICONSET"

    # Convert SVG to PNG at high resolution using qlmanage
    qlmanage -t -s 1024 -o /tmp "$SVG_ICON" 2>/dev/null
    TEMP_PNG=$(ls /tmp/gftp.svg.png 2>/dev/null | head -1)

    if [ -f "$TEMP_PNG" ]; then
        # Generate all required icon sizes
        for size in 16 32 128 256 512; do
            sips -z $size $size "$TEMP_PNG" --out "$TEMP_ICONSET/icon_${size}x${size}.png" >/dev/null 2>&1
            sips -z $((size*2)) $((size*2)) "$TEMP_PNG" --out "$TEMP_ICONSET/icon_${size}x${size}@2x.png" >/dev/null 2>&1
        done

        # Create ICNS file
        iconutil -c icns "$TEMP_ICONSET" -o "$ICNS_FILE"
        if [ -f "$ICNS_FILE" ]; then
            echo "  Icon converted successfully to $ICNS_FILE"
        fi

        # Cleanup
        rm -rf "$(dirname "$TEMP_ICONSET")" "$TEMP_PNG"
    else
        echo "  ${YELLOW}Warning: Failed to convert SVG icon, proceeding without a high-res icon.${NC}"
        ICNS_FILE=""
    fi
else
    echo "  ${YELLOW}Warning: SVG icon not found, proceeding without a high-res icon.${NC}"
    ICNS_FILE=""
fi

echo ""

# Step 3: Create app bundle with AppBundleGenerator
echo -e "${YELLOW}Step 3: Creating macOS app bundle...${NC}"

BUNDLE_ARGS=()
BUNDLE_ARGS+=("--identifier" "$BUNDLE_ID")
BUNDLE_ARGS+=("--version" "$VERSION")
BUNDLE_ARGS+=("--category" "public.app-category.utilities")
BUNDLE_ARGS+=("--min-os" "12.0")
BUNDLE_ARGS+=("--sign" "-")
BUNDLE_ARGS+=("--hardened-runtime")
BUNDLE_ARGS+=("--allow-dyld-vars")
BUNDLE_ARGS+=("--stage-dependencies" "$INSTALL_PREFIX")

if [ -n "$ICNS_FILE" ] && [ -f "$ICNS_FILE" ]; then
    BUNDLE_ARGS+=("--icon" "$ICNS_FILE")
fi

echo "  Running AppBundleGenerator..."
echo "  Command: $APP_BUNDLE_GENERATOR ${BUNDLE_ARGS[*]} \"$APP_NAME\" \"$DEST_DIR\" \"$INSTALL_PREFIX/bin/gftp-gtk\""
echo ""

"$APP_BUNDLE_GENERATOR" \
    "${BUNDLE_ARGS[@]}" \
    "$APP_NAME" \
    "$DEST_DIR" \
    "$INSTALL_PREFIX/bin/gftp-gtk"

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: AppBundleGenerator failed${NC}"
    exit 1
fi

BUNDLE_PATH="$DEST_DIR/$APP_NAME.app"

echo ""
echo -e "${GREEN}✓ App bundle created successfully${NC}"
echo ""

# Step 4: Verify the bundle
echo -e "${YELLOW}Step 4: Verifying app bundle...${NC}"

if [ ! -d "$BUNDLE_PATH" ]; then
    echo -e "${RED}Error: Bundle not created at $BUNDLE_PATH${NC}"
    exit 1
fi

echo "  Checking bundle structure..."
for path in "Contents/Info.plist" \
            "Contents/MacOS" \
            "Contents/Resources/lib" \
            "Contents/Resources/share"; do
    if [ ! -e "$BUNDLE_PATH/$path" ]; then
        echo -e "${RED}  ✗ Missing: $path${NC}"
        exit 1
    fi
done

# Find the main executable (can be in MacOS or Resources/bin)
MAIN_BINARY=""
for candidate in "$BUNDLE_PATH/Contents/Resources/bin/gftp-gtk" \
                 "$BUNDLE_PATH/Contents/MacOS/gftp-gtk" \
                 "$BUNDLE_PATH/Contents/MacOS/gFTP"; do
    if [ -f "$candidate" ] && file "$candidate" 2>/dev/null | grep -q "Mach-O"; then
        MAIN_BINARY="$candidate"
        break
    fi
done

if [ -z "$MAIN_BINARY" ]; then
    echo -e "${RED}  ✗ No Mach-O executable found${NC}"
    exit 1
fi

echo "  Checking dylib dependencies..."

if [ -f "$MAIN_BINARY" ]; then
    BAD_DEPS=$(otool -L "$MAIN_BINARY" 2>/dev/null | grep -v "@" | grep -v "/usr/lib" | grep -v ":" | wc -l | tr -d ' ')
    if [ "$BAD_DEPS" -gt 0 ]; then
        echo -e "${YELLOW}  Warning: Found $BAD_DEPS non-relocatable dependencies${NC}"
        otool -L "$MAIN_BINARY" | grep -v "@" | grep -v "/usr/lib" | grep -v ":"
    else
        echo "  ✓ All dependencies are relocatable"
    fi
fi

echo "  Checking GSettings schemas..."
if [ -f "$BUNDLE_PATH/Contents/Resources/share/glib-2.0/schemas/gschemas.compiled" ]; then
    echo "  ✓ GSettings schemas compiled"
else
    echo -e "${YELLOW}  Warning: GSettings schemas not found${NC}"
fi

echo "  Checking translations..."
LOCALE_COUNT=$(find "$BUNDLE_PATH/Contents/Resources/share/locale" -name "gftp.mo" 2>/dev/null | wc -l | tr -d ' ')
if [ "$LOCALE_COUNT" -gt 0 ]; then
    echo "  ✓ Found $LOCALE_COUNT translation files"
else
    echo -e "${YELLOW}  Warning: No translation files found${NC}"
fi

echo "  Checking code signature..."
if codesign -dv "$BUNDLE_PATH" 2>&1 | grep -q "Signature="; then
    echo "  ✓ Bundle is code signed"
else
    echo -e "${YELLOW}  Warning: Bundle is not code signed${NC}"
fi

echo ""
echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}     Build Complete!${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo -e "${BLUE}App bundle location:${NC}"
echo "  $BUNDLE_PATH"
echo ""
echo -e "${BLUE}To test the application:${NC}"
echo "  open \"$BUNDLE_PATH\""
echo ""
echo -e "${BLUE}To create a DMG:${NC}"
echo "  cd $GFTP_SOURCE"
echo "  ./create_dmg_for_app.sh \"$BUNDLE_PATH\""
echo ""

INSTALL_DIR="/Users/sedwards/.local/bin"
SOURCE_FILE="build/src/text/gftp-text"
DEST_FILE="$INSTALL_DIR/gftp-text"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: $SOURCE_FILE not found. Please build the project first."
    exit 1
fi

echo "Installing gftp-text to $DEST_FILE..."
cp "$SOURCE_FILE" "$DEST_FILE"
chmod +x "$DEST_FILE"


