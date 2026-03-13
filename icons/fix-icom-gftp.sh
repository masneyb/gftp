#!/bin/bash
set -e

ICON_NAME="gftp"
SVG_PATH="scalable/${ICON_NAME}.svg"
ICONSET_DIR="${ICON_NAME}.iconset"

echo "Building macOS iconset from ${SVG_PATH}"

if [ ! -f "$SVG_PATH" ]; then
    echo "Error: $SVG_PATH not found."
    exit 1
fi

rm -rf "$ICONSET_DIR"
mkdir "$ICONSET_DIR"

# Apple-required sizes
sizes=(16 32 128 256 512 1024)

for size in "${sizes[@]}"; do
    echo "Generating ${size}x${size}"
    
    inkscape "$SVG_PATH" \
        --export-type=png \
        --export-width=$size \
        --export-filename="$ICONSET_DIR/icon_${size}x${size}.png"
    
    retina=$((size * 2))
    
    echo "Generating ${size}x${size}@2x (${retina}x${retina})"
    
    inkscape "$SVG_PATH" \
        --export-type=png \
        --export-width=$retina \
        --export-filename="$ICONSET_DIR/icon_${size}x${size}@2x.png"
done

echo "Creating icns..."
iconutil -c icns "$ICONSET_DIR"

echo "Done. Generated ${ICON_NAME}.icns"

 