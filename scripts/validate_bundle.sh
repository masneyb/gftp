#!/bin/bash
#
# validate_bundle.sh - Validate macOS app bundle for gFTP
#
# Usage: ./validate_bundle.sh /path/to/gFTP.app
#

BUNDLE_PATH="$1"

if [ -z "$BUNDLE_PATH" ]; then
    echo "Usage: $0 /path/to/gFTP.app"
    exit 1
fi

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "Error: Bundle not found at $BUNDLE_PATH"
    exit 1
fi

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0

echo "========================================="
echo "   gFTP macOS Bundle Validation"
echo "========================================="
echo "Bundle: $BUNDLE_PATH"
echo ""

# Test 1: Bundle structure
echo "[1/10] Checking bundle structure..."
required_paths=(
    "Contents/Info.plist"
    "Contents/MacOS"
    "Contents/Resources/lib"
    "Contents/Resources/share"
)
for path in "${required_paths[@]}"; do
    if [ ! -e "$BUNDLE_PATH/$path" ]; then
        echo -e "  ${RED}✗ FAIL: Missing $path${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        exit 1
    fi
done

# Check for executable (either in MacOS or Resources/bin)
FOUND_EXEC=0
if [ -f "$BUNDLE_PATH/Contents/MacOS/gftp-gtk" ] || \
   [ -f "$BUNDLE_PATH/Contents/Resources/bin/gftp-gtk" ] || \
   [ -f "$BUNDLE_PATH/Contents/MacOS/gFTP" ]; then
    FOUND_EXEC=1
fi

if [ $FOUND_EXEC -eq 0 ]; then
    echo -e "  ${RED}✗ FAIL: No executable found${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    exit 1
fi

echo -e "  ${GREEN}✓ PASS${NC}"
PASS_COUNT=$((PASS_COUNT + 1))

# Test 2: Info.plist validation
echo "[2/10] Validating Info.plist..."
if plutil -lint "$BUNDLE_PATH/Contents/Info.plist" >/dev/null 2>&1; then
    echo -e "  ${GREEN}✓ PASS${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "  ${RED}✗ FAIL: Invalid Info.plist${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 3: Check dylib dependencies
echo "[3/10] Checking library dependencies..."
# Find the actual executable
EXECUTABLE=""
for candidate in "$BUNDLE_PATH/Contents/Resources/bin/gftp-gtk" \
                 "$BUNDLE_PATH/Contents/MacOS/gftp-gtk" \
                 "$BUNDLE_PATH/Contents/MacOS/gFTP"; do
    if [ -f "$candidate" ] && file "$candidate" | grep -q "Mach-O"; then
        EXECUTABLE="$candidate"
        break
    fi
done

if [ -f "$EXECUTABLE" ] && file "$EXECUTABLE" | grep -q "Mach-O"; then
    bad_deps=$(otool -L "$EXECUTABLE" 2>/dev/null | grep -v "@" | grep -v "/usr/lib" | grep -v "/System" | grep -v ":" | wc -l | tr -d ' ')
    if [ "$bad_deps" -gt 0 ]; then
        echo -e "  ${YELLOW}⚠ WARNING: Found $bad_deps non-relocatable dependencies${NC}"
        otool -L "$EXECUTABLE" | grep -v "@" | grep -v "/usr/lib" | grep -v "/System" | grep -v ":"
        WARN_COUNT=$((WARN_COUNT + 1))
    else
        echo -e "  ${GREEN}✓ PASS: All dependencies are relocatable${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    fi
else
    echo -e "  ${RED}✗ FAIL: Cannot find valid Mach-O executable${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 4: Check all dylibs are present
echo "[4/10] Checking bundled libraries..."
if [ -f "$EXECUTABLE" ] && file "$EXECUTABLE" | grep -q "Mach-O"; then
    missing=0
    for lib in $(otool -L "$EXECUTABLE" 2>/dev/null | grep "@rpath" | awk '{print $1}' | sed 's/@rpath\///'); do
        if [ ! -f "$BUNDLE_PATH/Contents/Resources/lib/$lib" ]; then
            echo -e "  ${RED}✗ Missing library: $lib${NC}"
            missing=1
        fi
    done
    if [ "$missing" -eq 1 ]; then
        echo -e "  ${RED}✗ FAIL${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    else
        echo -e "  ${GREEN}✓ PASS${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    fi
else
    echo -e "  ${YELLOW}⚠ SKIP: No Mach-O executable to check${NC}"
    WARN_COUNT=$((WARN_COUNT + 1))
fi

# Test 5: Check GSettings schemas
echo "[5/10] Checking GSettings schemas..."
SCHEMA_FILE="$BUNDLE_PATH/Contents/Resources/share/glib-2.0/schemas/gschemas.compiled"
if [ -f "$SCHEMA_FILE" ]; then
    echo -e "  ${GREEN}✓ PASS: Compiled GSettings schemas found${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "  ${RED}✗ FAIL: Missing compiled GSettings schemas${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 6: Check translations
echo "[6/10] Checking translations..."
LOCALE_DIR="$BUNDLE_PATH/Contents/Resources/share/locale"
if [ -d "$LOCALE_DIR" ]; then
    mo_count=$(find "$LOCALE_DIR" -name "gftp.mo" 2>/dev/null | wc -l | tr -d ' ')
    if [ "$mo_count" -lt 60 ]; then
        echo -e "  ${YELLOW}⚠ WARNING: Only $mo_count translations found (expected 64)${NC}"
        WARN_COUNT=$((WARN_COUNT + 1))
    else
        echo -e "  ${GREEN}✓ PASS: $mo_count translations found${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    fi
else
    echo -e "  ${RED}✗ FAIL: No translations directory found${NC}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test 7: Check GTK resources
echo "[7/10] Checking GTK resources..."
if [ -d "$BUNDLE_PATH/Contents/Resources/share/gtk-3.0" ]; then
    echo -e "  ${GREEN}✓ PASS: GTK-3.0 resources found${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "  ${YELLOW}⚠ WARNING: GTK-3.0 resources not found${NC}"
    WARN_COUNT=$((WARN_COUNT + 1))
fi

# Test 8: Check icon resources
echo "[8/10] Checking icon resources..."
if [ -f "$BUNDLE_PATH/Contents/Resources/icon.icns" ]; then
    echo -e "  ${GREEN}✓ PASS: App icon found${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "  ${YELLOW}⚠ WARNING: No app icon found${NC}"
    WARN_COUNT=$((WARN_COUNT + 1))
fi

# Test 9: Code signature
echo "[9/10] Checking code signature..."
if codesign -dv "$BUNDLE_PATH" 2>&1 | grep -q "Signature="; then
    if codesign --verify --deep --strict --verbose=2 "$BUNDLE_PATH" 2>&1; then
        echo -e "  ${GREEN}✓ PASS: Valid signature${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "  ${RED}✗ FAIL: Invalid signature${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
else
    echo -e "  ${YELLOW}⚠ WARNING: Not signed${NC}"
    WARN_COUNT=$((WARN_COUNT + 1))
fi

# Test 10: Bundle size
echo "[10/10] Checking bundle size..."
BUNDLE_SIZE=$(du -sm "$BUNDLE_PATH" | awk '{print $1}')
if [ "$BUNDLE_SIZE" -lt 250 ]; then
    echo -e "  ${GREEN}✓ PASS: Bundle size ${BUNDLE_SIZE}MB < 250MB${NC}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo -e "  ${YELLOW}⚠ WARNING: Bundle size ${BUNDLE_SIZE}MB exceeds 250MB${NC}"
    WARN_COUNT=$((WARN_COUNT + 1))
fi

echo ""
echo "========================================="
echo "   Validation Summary"
echo "========================================="
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${YELLOW}Warnings: $WARN_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"
echo ""

if [ "$FAIL_COUNT" -gt 0 ]; then
    echo -e "${RED}Bundle validation FAILED${NC}"
    exit 1
elif [ "$WARN_COUNT" -gt 0 ]; then
    echo -e "${YELLOW}Bundle validation passed with warnings${NC}"
    exit 0
else
    echo -e "${GREEN}Bundle validation PASSED${NC}"
    exit 0
fi
