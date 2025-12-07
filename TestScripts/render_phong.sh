#!/bin/bash

# Test script to render phong_scene.json
# Exit on any error (but we'll disable it for comparisons)
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

OUTPUT_DIR="TestOutput"
REFERENCE_DIR="TestSuite"
SCENE_DIR="Resources"
SCENE_NAME="phong_scene"

# Check if binary exists
if [ ! -f "bin/RunRaytracer" ]; then
    echo -e "${RED}ERROR: bin/RunRaytracer not found${NC}"
    echo "Please run ./TestScripts/build.sh first"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Check if scene file exists
if [ ! -f "$SCENE_DIR/$SCENE_NAME.json" ]; then
    echo -e "${RED}ERROR: Scene file $SCENE_DIR/$SCENE_NAME.json not found${NC}"
    exit 1
fi

# Function to compare PPM files
compare_ppm() {
    local actual_file="$1"
    local reference_file="$2"
    local file_name="$3"
    
    if [ ! -f "$actual_file" ]; then
        echo -e "${RED}  ✗ $file_name: Actual file not found${NC}"
        return 1
    fi
    
    if [ ! -f "$reference_file" ]; then
        echo -e "${YELLOW}  ⚠ $file_name: Reference file not found (skipping comparison)${NC}"
        return 2
    fi
    
    if cmp -s "$actual_file" "$reference_file"; then
        echo -e "${GREEN}  ✓ $file_name: Matches reference${NC}"
        return 0
    else
        echo -e "${RED}  ✗ $file_name: Does NOT match reference${NC}"
        return 1
    fi
}

# Render phong_scene
echo "========================================="
echo "Rendering $SCENE_NAME.json..."
echo "========================================="
echo ""

./bin/RunRaytracer -s "$SCENE_DIR/" -r "$OUTPUT_DIR/" "$SCENE_NAME"

echo ""
echo "========================================="
echo "Comparing outputs with reference files..."
echo "========================================="
echo ""

# Disable exit on error for comparisons (some reference files might not exist)
set +e

# Compare phong_scene outputs
echo "Comparing $SCENE_NAME outputs:"
compare_ppm "$OUTPUT_DIR/$SCENE_NAME.ppm" "$REFERENCE_DIR/$SCENE_NAME.ppm" "$SCENE_NAME.ppm"
compare_ppm "$OUTPUT_DIR/${SCENE_NAME}_RAW.ppm" "$REFERENCE_DIR/${SCENE_NAME}_RAW.ppm" "${SCENE_NAME}_RAW.ppm"

# Re-enable exit on error
set -e

echo ""
echo "========================================="
echo "Render complete!"
echo "========================================="
echo "Output files are in $OUTPUT_DIR/ directory"
echo "Reference files are in $REFERENCE_DIR/ directory"
