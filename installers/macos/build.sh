#!/bin/bash
# ParthenonChain macOS Build Script
# Creates DMG installer for macOS

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Building ParthenonChain macOS installer..."

# Check if create-dmg.sh exists
if [ ! -f "./create-dmg.sh" ]; then
    echo "Error: create-dmg.sh not found"
    exit 1
fi

# Run the DMG creation script
bash ./create-dmg.sh

echo "macOS DMG build complete!"
