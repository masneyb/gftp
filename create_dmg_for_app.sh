#!/bin/bash

#
# This script creates a DMG installer for gFTP
# Usage: ./create_dmg_for_app.sh [path-to-gftp.app]
#
# If no path is provided, it will search in common locations:
# - Desktop
# - Current directory
# - Build output directory
#

APP_NAME="gFTP"
APP_BUNDLE="$APP_NAME.app"
DMG_NAME="$APP_NAME.dmg"
VOL_NAME="$APP_NAME Installer"
SRC_FOLDER="$(cd "$(dirname "$0")" && pwd)"
TEMP_DIR="$APP_NAME-temp"  # Temporary directory for DMG contents

# If user provided a path to the app, use it
if [ -n "$1" ]; then
    APP_PATH="$1"
elif [ -d "$HOME/Desktop/$APP_BUNDLE" ]; then
    APP_PATH="$HOME/Desktop/$APP_BUNDLE"
elif [ -d "$SRC_FOLDER/$APP_BUNDLE" ]; then
    APP_PATH="$SRC_FOLDER/$APP_BUNDLE"
elif [ -d "$SRC_FOLDER/build/$APP_BUNDLE" ]; then
    APP_PATH="$SRC_FOLDER/build/$APP_BUNDLE"
else
    echo "Error: Could not find $APP_BUNDLE"
    echo "Please specify the path to the .app bundle:"
    echo "  $0 /path/to/$APP_BUNDLE"
    echo ""
    echo "Or create it in one of these locations:"
    echo "  - $HOME/Desktop/$APP_BUNDLE"
    echo "  - $SRC_FOLDER/$APP_BUNDLE"
    echo "  - $SRC_FOLDER/build/$APP_BUNDLE"
    exit 1
fi

# Get the directory where the app is located
APP_DIR="$(dirname "$APP_PATH")"

echo "App Name:     $APP_NAME"
echo "App Path:     $APP_PATH"
echo "DMG Name:     $DMG_NAME"
echo "Volume Name:  $VOL_NAME"
echo "Source Dir:   $SRC_FOLDER"
echo "Temp Dir:     $TEMP_DIR"
echo "Working Dir:  $APP_DIR"
echo ""

# Change to directory where app is located
cd "$APP_DIR"

# Remove any existing temporary directory
rm -rf "$TEMP_DIR"

# Create new directory for temporary files
mkdir -p "$TEMP_DIR/source"
cp create_dmg_for_app.sh $TEMP_DIR/source/

# Copy Application bundle
echo "Copying $APP_BUNDLE to temporary directory..."
rsync -av --links "$APP_BUNDLE" "$TEMP_DIR/"

# Copy the source code
echo "Copying source code..."
rsync -av --links "$SRC_FOLDER" "$TEMP_DIR/source" \
    --exclude='.git' \
    --exclude='build' \
    --exclude='*.o' \
    --exclude='*.dmg' \
    --exclude="$TEMP_DIR"

# Create the DMG file
echo ""
echo "Creating DMG installer..."
hdiutil create -volname "$VOL_NAME" -srcfolder "$TEMP_DIR" -ov -format UDZO "$DMG_NAME"

# Clean up the temporary directory
echo ""
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo ""
echo "✓ Successfully created: $APP_DIR/$DMG_NAME"
echo ""
