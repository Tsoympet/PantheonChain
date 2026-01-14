#!/bin/bash
# ParthenonChain RPM Build Script

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read version from VERSION file
VERSION=$(cat "$PROJECT_ROOT/VERSION" 2>/dev/null || echo "1.0.0")
RELEASE="1"

echo "=== Building ParthenonChain RPM Package ==="
echo "Version: $VERSION-$RELEASE"

# Create RPM build directory structure
RPMBUILD_DIR="$HOME/rpmbuild"
mkdir -p "$RPMBUILD_DIR"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball
TARBALL="parthenon-${VERSION}.tar.gz"
echo "Creating source tarball: $TARBALL"
cd "$PROJECT_ROOT"
git archive --format=tar.gz --prefix="parthenon-${VERSION}/" HEAD > "$RPMBUILD_DIR/SOURCES/$TARBALL"

# Copy spec file to SPECS directory
echo "Copying spec file..."
cp "$SCRIPT_DIR/parthenon.spec" "$RPMBUILD_DIR/SPECS/"

# Build RPM
echo "Building RPM package..."
cd "$RPMBUILD_DIR"
rpmbuild -ba SPECS/parthenon.spec

# Copy built RPM to installers/linux directory
echo "Copying RPM to installers/linux..."
find "$RPMBUILD_DIR/RPMS" -name "parthenon-*.rpm" -exec cp {} "$SCRIPT_DIR/" \;

echo "=== RPM build complete ==="
echo "Package location: $SCRIPT_DIR/parthenon-${VERSION}-${RELEASE}.*.rpm"
