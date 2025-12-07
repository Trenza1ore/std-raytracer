#!/bin/bash

# Build script for RunRaytracer
# Exit on any error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "========================================="
echo "Building RunRaytracer..."
echo "========================================="

# Build the raytracer using the Makefile
cd Code
make clean
make all
cd ..

# Check if the binary was created
if [ ! -f "bin/RunRaytracer" ]; then
    echo -e "${RED}ERROR: Failed to build bin/RunRaytracer${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}Build successful!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "Binary location: bin/RunRaytracer"
