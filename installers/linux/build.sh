#!/bin/bash
# ParthenonChain Linux Build Script
# Wrapper script for building DEB and RPM packages

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Building ParthenonChain Linux packages..."

# Build DEB package
if [ -f "./build-deb.sh" ]; then
    echo "Building DEB package..."
    bash ./build-deb.sh
fi

# Build RPM package (if spec file exists)
if [ -f "./parthenon.spec" ]; then
    echo "RPM spec exists but build-rpm.sh needed - skipping for now"
fi

echo "Linux package build complete!"
