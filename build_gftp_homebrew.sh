#!/bin/bash
#
# build_gftp_homebrew.sh - Build gFTP using Homebrew GTK3 and create macOS app bundle
#
# This script:
# 1. Builds gFTP with meson using Homebrew's GTK3
# 2. Installs to a local prefix
# 3. Creates relocatable app bundle with all dependencies
#
# Requirements:
# - Homebrew with gtk+3 installed (brew install gtk+3)
# - AppBundleGenerator tool
# - meson, ninja
#

set -e  # Exit on error

# Configuration
HOMEBREW_PREFIX="$(brew --prefix)"
GFTP_PREFIX="$HOME/source/gftp/gftp-install"
GFTP_SOURCE="$(cd "$(dirname "$0")" && pwd)"
DEST_DIR="${DEST_DIR:-.}"
APP_NAME="gFTP"
BUNDLE_ID="org.gftp.gftp-gtk"
VERSION="2.9.1b"

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

if [ "$1" == "clean" ]; then
    info "Cleaning up..."
    if [ -d "build" ]; then
        rm -rf build
    fi
    if [ -d "gFTP.app" ]; then
        rm -rf gFTP.app
    fi
    if [ -d "gftp-install" ]; then
        rm -rf gftp-install
    fi
    info "Cleanup complete."
    exit 0
fi

echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}  gFTP macOS Builder (Homebrew)${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo -e "${BLUE}Configuration:${NC}"
echo "  Homebrew prefix: $HOMEBREW_PREFIX"
echo "  gFTP install prefix: $GFTP_PREFIX"
echo "  Source directory: $GFTP_SOURCE"
echo "  AppBundleGenerator: $APP_BUNDLE_GENERATOR"
echo "  Destination: $DEST_DIR"
echo "  Bundle identifier: $BUNDLE_ID"
echo "  Version: $VERSION"
echo ""

# Step 0: Check prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

# Check for GTK3
if ! pkg-config --exists gtk+-3.0; then
    echo -e "${RED}Error: GTK3 not found. Please install with:${NC}"
    echo "  brew install gtk+3"
    exit 1
fi

GTK_VERSION=$(pkg-config --modversion gtk+-3.0)
echo "  ✓ GTK3 version: $GTK_VERSION"

# Check for meson
if ! command -v meson &> /dev/null; then
    echo -e "${RED}Error: meson not found. Installing...${NC}"
    brew install meson
fi

# Check for ninja
if ! command -v ninja &> /dev/null; then
    echo -e "${RED}Error: ninja not found. Installing...${NC}"
    brew install ninja
fi

# Check for AppBundleGenerator
if [ -z "$APP_BUNDLE_GENERATOR" ]; then
    error "AppBundleGenerator not found. Please ensure the 'AppBundleGenerator' executable is in your PATH or set the APP_BUNDLE_GENERATOR_PATH environment variable. You can download it from: https://github.com/sedwards-ibowl/AppBundleGenerator"
fi
info "Found AppBundleGenerator at: $APP_BUNDLE_GENERATOR"

echo -e "${GREEN}✓ All required prerequisites found${NC}"
echo ""

# Step 1: Set up environment for Homebrew GTK
echo -e "${YELLOW}Step 1: Setting up build environment...${NC}"

export PKG_CONFIG_PATH="$HOMEBREW_PREFIX/lib/pkgconfig:$HOMEBREW_PREFIX/opt/libffi/lib/pkgconfig"
export PATH="$HOMEBREW_PREFIX/bin:$PATH"
export CFLAGS="-I$HOMEBREW_PREFIX/include"
export LDFLAGS="-L$HOMEBREW_PREFIX/lib"

# Create install prefix
mkdir -p "$GFTP_PREFIX"

echo "  PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
echo "  PREFIX: $GFTP_PREFIX"
echo ""

# Step 2: Build gFTP
echo -e "${YELLOW}Step 2: Building gFTP...${NC}"

cd "$GFTP_SOURCE"

# Clean previous build
if [ -d "build" ]; then
    echo "  Cleaning previous build..."
    rm -rf build
fi

# Configure with meson
echo "  Configuring with meson..."
meson setup build \
    --prefix="$GFTP_PREFIX" \
    --buildtype=release \
    -Dgtk2=false -Dgtk3=true

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: meson configuration failed${NC}"
    exit 1
fi

# Build
echo "  Building..."
meson compile -C build

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: build failed${NC}"
    exit 1
fi

# Install
echo "  Installing to $GFTP_PREFIX..."
meson install -C build

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: installation failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ gFTP structue built and installed${NC}"
echo ""

# Explicity app glib-2.0 scemes for AooBundler
echo -e "${GREEM}Please ignore the following glib-2.0 templates to bundle error, it is resolved later in the build...${NC}"

# Step 4: Creating .icns file from SVG
echo -e "${YELLOW}Step 4: Creating .icns file from SVG...${NC}"
SVG_ICON="$GFTP_SOURCE/icons/scalable/gftp.svg"
ICNS_FILE="$GFTP_SOURCE/gftp.icns"

if [ -f "$SVG_ICON" ]; then
    echo "  Found SVG icon, converting to ICNS..."
    TEMP_ICONSET=$(mktemp -d)/gftp.iconset
    mkdir -p "$TEMP_ICONSET"

    # Convert SVG to PNG at high resolution
    # Try multiple methods
    TEMP_PNG=""
    
    # Method 1: Try rsvg-convert if available
    if command -v rsvg-convert &> /dev/null; then
        TEMP_PNG="/tmp/gftp_icon.png"
        rsvg-convert -w 1024 -h 1024 "$SVG_ICON" -o "$TEMP_PNG" 2>/dev/null
    # Method 2: Try qlmanage
    elif command -v qlmanage &> /dev/null; then
        qlmanage -t -s 1024 -o /tmp "$SVG_ICON" 2>/dev/null
        TEMP_PNG=$(ls /tmp/gftp.svg.png 2>/dev/null | head -1)
    fi

    if [ -f "$TEMP_PNG" ]; then
        # Generate all required icon sizes
        for size in 16 32 128 256 512; do
            sips -z $size $size "$TEMP_PNG" --out "$TEMP_ICONSET/icon_${size}x${size}.png" >/dev/null 2>&1
            sips -z $((size*2)) $((size*2)) "$TEMP_PNG" --out "$TEMP_ICONSET/icon_${size}x${size}@2x.png" >/dev/null 2>&1
        done

        # Create ICNS file
        iconutil -c icns "$TEMP_ICONSET" -o "$ICNS_FILE"
        if [ -f "$ICNS_FILE" ]; then
            echo -e "${GREEN}  ✓ Icon converted successfully${NC}"
        fi

        # Cleanup
        rm -rf "$(dirname "$TEMP_ICONSET")" "$TEMP_PNG"
    else
        echo -e "${YELLOW}  Warning: Failed to convert SVG icon${NC}"
        ICNS_FILE=""
    fi
else
    echo -e "${YELLOW}  Warning: SVG icon not found${NC}"
    ICNS_FILE=""
fi
echo ""

# Step 5: Create app bundle
echo -e "${YELLOW}Step 5: Creating macOS app bundle...${NC}"

# Remove existing bundle
if [ -d "$DEST_DIR/$APP_NAME.app" ]; then
    warn "Removing existing bundle: $DEST_DIR/$APP_NAME.app"
    rm -rf "$DEST_DIR/$APP_NAME.app"
fi

# Create a wrapper script that sets up the environment
WRAPPER_SCRIPT="/tmp/gftp-launcher.sh"
cat > "$WRAPPER_SCRIPT" << 'EOF'
#!/bin/bash
# gFTP launcher script
BUNDLE_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
export GTK_PATH="$BUNDLE_DIR/Contents/Resources"
export GDK_PIXBUF_MODULE_FILE="$BUNDLE_DIR/Contents/Resources/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
export XDG_DATA_DIRS="$BUNDLE_DIR/Contents/Resources/share:$XDG_DATA_DIRS"
export GFTP_SHARE_DIR="$BUNDLE_DIR/Contents/Resources/share"
export GFTP_SHARE_DIR="$BUNDLE_DIR/Contents/Resources/share"

# Set GFTP_CONFIG_DIR to point to bundled resources for default config lookups
# This will be used by lib/misc.c on macOS if GFTP_CONFIG_DIR is set.
export GFTP_CONFIG_DIR="$BUNDLE_DIR/Contents/Resources/share/gftp"

# Define the user's config directory (where gFTP expects writable config)
# This mimics the C code's default if GFTP_CONFIG_DIR is not set or ignored.
USER_HOME=$(eval echo "~") # Safely get home directory
USER_CONFIG_DIR="$USER_HOME/Library/gFTP"
USER_GFTPRC="$USER_CONFIG_DIR/gftprc"
BUNDLED_GFTPRC="$BUNDLE_DIR/Contents/Resources/share/gftp/gftprc"
BUNDLED_BOOKMARKS="$BUNDLE_DIR/Contents/Resources/share/gftp/bookmarks"
USER_BOOKMARKS="$USER_CONFIG_DIR/bookmarks"

# Ensure the user's config directory exists
mkdir -p "$USER_CONFIG_DIR"

# Copy default gftprc and bookmarks if they don't exist in the user's config
if [ ! -f "$USER_GFTPRC" ]; then
    echo "Initializing user's gftprc from bundle defaults..."
    if [ -f "$BUNDLED_GFTPRC" ]; then
        cp "$BUNDLED_GFTPRC" "$USER_GFTPRC"
    else
        echo "Warning: Bundled gftprc not found at $BUNDLED_GFTPRC"
    fi
fi

if [ ! -f "$USER_BOOKMARKS" ]; then
    echo "Initializing user's bookmarks from bundle defaults..."
    if [ -f "$BUNDLED_BOOKMARKS" ]; then
        cp "$BUNDLED_BOOKMARKS" "$USER_BOOKMARKS"
    else
        echo "Warning: Bundled bookmarks not found at $BUNDLED_BOOKMARKS"
    fi
fi

exec "$BUNDLE_DIR/Contents/Resources/bin/gftp-gtk" "$@"
EOF
chmod +x "$WRAPPER_SCRIPT"

echo "  Running AppBundleGenerator..."

"$APP_BUNDLE_GENERATOR" \
    --gtk \
    --icon "$ICNS_FILE" \
    --sign - \
    --hardened-runtime \
    --category public.app-category.utilities \
    --version "$VERSION" \
    --min-os 12.0 \
    --stage-dependencies "$GFTP_PREFIX" \
    --stage-dependencies "$HOMEBREW_PREFIX" \
    "$APP_NAME" \
    "$DEST_DIR" \
    "$WRAPPER_SCRIPT"

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: AppBundleGenerator failed${NC}"
    exit 1
fi

BUNDLE_PATH="$DEST_DIR/$APP_NAME.app"
echo -e "${GREEN}✓ App bundle created${NC}"
echo ""

# Explicitly copy gftp-gtk into the bundle's Resources/bin
echo -e "${YELLOW}Copying gftp-gtk to bundle...${NC}"
mkdir -p "$BUNDLE_PATH/Contents/Resources/bin"
cp "$GFTP_PREFIX/bin/gftp-gtk" "$BUNDLE_PATH/Contents/Resources/bin/gftp-gtk"
echo -e "${GREEN}✓ gftp-gtk copied to bundle${NC}"
echo ""

# Explicitly copy default config files (gftprc and bookmarks) into the bundle
echo -e "${YELLOW}Copying default config files to bundle...${NC}"
mkdir -p "$BUNDLE_PATH/Contents/Resources/share/gftp"
cp "$GFTP_PREFIX/share/gftp/gftprc" "$BUNDLE_PATH/Contents/Resources/share/gftp/gftprc"
cp "$GFTP_PREFIX/share/gftp/bookmarks" "$BUNDLE_PATH/Contents/Resources/share/gftp/bookmarks"
echo -e "${GREEN}✓ Default config files copied to bundle${NC}"
echo ""

# Explicity app glib-2.0 scemes for AooBundker to cimouke
echo -e "${YELLOW}Copying glib-2.0 templates to bundle...${NC}"
mkdir -p "$BUNDLE_PATH/Contents/Resources/share/glib-2.0/"
cp -R "$GFTP_SOURCE/third_party/glib-2.0-schemas" "$BUNDLE_PATH/Contents/Resources/share/glib-2.0/schemas"

# Explicitly copy images and icons into the bundle
echo -e "${YELLOW}Copying images and icons to bundle...${NC}"

# Copy gftp.png from share/gftp
mkdir -p "$BUNDLE_PATH/Contents/Resources/share/gftp"
cp "$GFTP_PREFIX/share/gftp/gftp.png" "$BUNDLE_PATH/Contents/Resources/share/gftp/gftp.png"

# Copy hicolor icons from the main build
if [ -d "$GFTP_PREFIX/share/icons/hicolor" ]; then
    cp -R "$GFTP_PREFIX/share/icons/hicolor" "$BUNDLE_PATH/Contents/Resources/share/icons/"
fi

echo -e "${GREEN}✓ Images and icons copied to bundle${NC}"
echo ""

# Remove unwanted Qt dependencies
echo -e "${YELLOW}Cleaning unwanted Qt and Node.js dependencies...${NC}"
find "$BUNDLE_PATH/Contents/Resources/lib/" -depth 1 -name "Qt*.framework" -exec rm -rf {} +
find "$BUNDLE_PATH/Contents/Resources/lib/" -depth 1 -name "libqhull_r*.dylib" -exec rm -rf {} +
rm -rf "$BUNDLE_PATH/Contents/Resources/share/qt"

# Remove unwanted Node.js dependencies
rm -rf "$BUNDLE_PATH/Contents/Resources/lib/node_modules"
find "$BUNDLE_PATH/Contents/Resources/lib/" -depth 1 -name "libnode*.dylib" -exec rm -rf {} +
rm -rf "$BUNDLE_PATH/Contents/Resources/etc/bash_completion.d/npm"
echo -e "${GREEN}✓ Unwanted dependencies cleaned${NC}"
echo ""

echo -e "${YELLOW}Step 6: Verifying app bundle...${NC}"
find "$BUNDLE_PATH/Contents/Resources/lib/" -depth 1 -name "python3.1*"  -exec rm -rf {} +
find "$BUNDLE_PATH/Contents/Resources/lib/" -depth 1 -name "libpython*.dylib" -exec rm -rf {} +
gtk-update-icon-cache -f -t gFTP.app/Contents/Resources/share/icons/hicolor
echo ""


# Step 6: Verify the bundle
echo -e "${YELLOW}Step 6: Verifying app bundle...${NC}"

if [ ! -d "$BUNDLE_PATH" ]; then
    echo -e "${RED}Error: Bundle not created at $BUNDLE_PATH${NC}"
    exit 1
fi

echo "  Checking bundle structure..."
if [ -d "$BUNDLE_PATH/Contents/MacOS" ]; then
    echo "  ✓ MacOS directory exists"
fi

if [ -f "$BUNDLE_PATH/Contents/Info.plist" ]; then
    echo "  ✓ Info.plist exists"
fi

# Find the main executable
MAIN_BINARY=""
for candidate in "$BUNDLE_PATH/Contents/MacOS/gftp-gtk" \
                 "$BUNDLE_PATH/Contents/MacOS/gFTP" \
                 "$BUNDLE_PATH/Contents/MacOS/gftp-launcher.sh"; do
    if [ -f "$candidate" ] && file "$candidate" 2>/dev/null | grep -q "Mach-O"; then
        MAIN_BINARY="$candidate"
        echo "  ✓ Found executable: $(basename $(dirname $candidate))/$(basename $candidate)"
        break
    fi
done

if [ -n "$MAIN_BINARY" ]; then
    echo "  Checking dylib dependencies..."
    BAD_DEPS=$(otool -L "$MAIN_BINARY" 2>/dev/null | grep -v "@" | grep -v "/usr/lib" | grep -v ":" | wc -l | tr -d ' ')
    if [ "$BAD_DEPS" -gt 0 ]; then
        echo -e "${YELLOW}  Warning: Found $BAD_DEPS non-relocatable dependencies${NC}"
        otool -L "$MAIN_BINARY" | grep -v "@" | grep -v "/usr/lib" | grep -v ":"
    else
        echo "  ✓ All dependencies are relocatable"
    fi
fi

echo ""

# Summary
echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}     Build Complete!${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo -e "${BLUE}Installed files:${NC}"
echo "  gFTP GTK: $GFTP_PREFIX/bin/gftp-gtk"
echo "  gFTP Text: $GFTP_PREFIX/bin/gftp-text (included as resource)"
echo ""

if [ -d "$BUNDLE_PATH" ]; then
    echo -e "${BLUE}App bundle:${NC}"
    echo "  $BUNDLE_PATH"
    echo ""
    echo -e "${BLUE}To test the application:${NC}"
    echo "  open \"$BUNDLE_PATH\""
    echo ""
else
    echo -e "${BLUE}To run gFTP:${NC}"
    echo "  $GFTP_PREFIX/bin/gftp-gtk"
    echo ""
fi

echo -e "${BLUE}To build an app bundle manually:${NC}"
echo "  1. Install AppBundleGenerator from: https://github.com/yourusername/AppBundleGenerator"
echo "  2. Set APP_BUNDLE_GENERATOR environment variable"
echo "  3. Re-run this script"
echo ""

echo -e "${BLUE}Build information:${NC}"
echo "  GTK version: $(pkg-config --modversion gtk+-3.0)"
echo "  GLib version: $(pkg-config --modversion glib-2.0)"
echo "  Build directory: $GFTP_SOURCE/build"
echo "  Install prefix: $GFTP_PREFIX"
echo ""
