#!/bin/bash
set -e

# This script fixes the rpaths of a gFTP.app bundle.
# Usage: ./fix_rpaths.sh gFTP.app

APP_BUNDLE="$1"
if [ -z "$APP_BUNDLE" ]; then
    echo "Usage: $0 <AppName.app>"
    exit 1
fi

LIB_DIR="$APP_BUNDLE/Contents/Resources/lib"
EXECUTABLE="$APP_BUNDLE/Contents/MacOS/gftp-gtk"

# Add rpath to the executable
install_name_tool -add_rpath "@loader_path/../Resources/lib" "$EXECUTABLE"

# Fix rpaths for libraries
for lib in "$LIB_DIR"/*.dylib; do
    if [ -f "$lib" ]; then
        lib_name=$(basename "$lib")
        
        # Set the library's ID
        install_name_tool -id "@rpath/$lib_name" "$lib"
        
        # Change the library's dependencies to use @rpath
        dependencies=$(otool -L "$lib" | grep "\t/" | cut -f 1 | awk '{print $1}' | grep -v -E '^/usr/lib|^/System/Library' || true)
        for dep_path in $dependencies; do
            dep_name=$(basename "$dep_path")
            if [ -f "$LIB_DIR/$dep_name" ]; then
                install_name_tool -change "$dep_path" "@rpath/$dep_name" "$lib"
            fi
        done
    fi
done

# Fix rpaths for the executable
dependencies=$(otool -L "$EXECUTABLE" | grep "\t/" | cut -f 1 | awk '{print $1}' | grep -v -E '^/usr/lib|^/System/Library' || true)
for dep_path in $dependencies; do
    dep_name=$(basename "$dep_path")
    if [ -f "$LIB_DIR/$dep_name" ]; then
        install_name_tool -change "$dep_path" "@rpath/$dep_name" "$EXECUTABLE"
    fi
done

echo "rpath fixing complete for $APP_BUNDLE"
