#!/bin/bash

# This script installs the gftp-text executable to a writable directory in your PATH.

INSTALL_DIR="$HOME/.local/bin"
SOURCE_FILE="build/src/text/gftp-text"
DEST_FILE="$INSTALL_DIR/gftp-text"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: $SOURCE_FILE not found. Please build the project first."
    exit 1
fi

echo "Installing gftp-text to $DEST_FILE..."
cp "$SOURCE_FILE" "$DEST_FILE"
chmod +x "$DEST_FILE"

echo "gftp-text installed successfully."
echo "You can now run 'gftp-text' from your terminal."
