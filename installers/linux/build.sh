#!/bin/bash
# ParthenonChain Linux Build Script
# Wrapper script for building DEB and RPM packages

set -e

PACKAGE_TYPE="${1:-all}"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Building ParthenonChain Linux packages (type: $PACKAGE_TYPE)..."

if [ "$PACKAGE_TYPE" = "deb" ] || [ "$PACKAGE_TYPE" = "all" ]; then
    # Build DEB package
    if [ -f "./build-deb.sh" ]; then
        echo "Building DEB package..."
        bash ./build-deb.sh
    else
        echo "Warning: build-deb.sh not found, skipping DEB build"
    fi
fi

if [ "$PACKAGE_TYPE" = "rpm" ] || [ "$PACKAGE_TYPE" = "all" ]; then
    # Build RPM package
    if [ -f "./build-rpm.sh" ]; then
        echo "Building RPM package..."
        bash ./build-rpm.sh
    elif [ -f "./parthenon.spec" ]; then
        echo "RPM spec exists but build-rpm.sh needed - skipping for now"
    else
        echo "Warning: No RPM build files found, skipping RPM build"
    fi
fi

echo "Linux package build complete!"
