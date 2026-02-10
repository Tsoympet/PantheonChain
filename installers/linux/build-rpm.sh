#!/bin/bash
# ParthenonChain RPM Build Script

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read version from VERSION file (supports plain semver or C/C++ #define format).
# Fallback to version.h for compatibility with older release automation.
VERSION_FILE="$PROJECT_ROOT/VERSION"
VERSION_HEADER_FILE="$PROJECT_ROOT/version.h"
if [ -f "$VERSION_FILE" ]; then
    VERSION=$(sed -nE 's/^#define[[:space:]]+PANTHEONCHAIN_VERSION[[:space:]]+"([^"]+)".*/\1/p' "$VERSION_FILE" | head -n1)
    if [ -z "$VERSION" ]; then
        VERSION=$(sed -nE 's/^([0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.-]+)?)$/\1/p' "$VERSION_FILE" | head -n1)
    fi
fi
if [ -z "$VERSION" ] && [ -f "$VERSION_HEADER_FILE" ]; then
    VERSION=$(sed -nE 's/^#define[[:space:]]+PANTHEONCHAIN_VERSION[[:space:]]+"([^"]+)".*/\1/p' "$VERSION_HEADER_FILE" | head -n1)
fi
VERSION="${VERSION:-1.0.0}"
RELEASE="1"
CHANGELOG_DATE="$(LC_ALL=C date '+%a %b %d %Y')"

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

# Copy spec file to SPECS directory and template dynamic values
echo "Copying spec file..."
SPEC_FILE="$RPMBUILD_DIR/SPECS/parthenon.spec"
cp "$SCRIPT_DIR/parthenon.spec" "$SPEC_FILE"

sed -i \
    -e "s/^Version:[[:space:]].*/Version:        ${VERSION}/" \
    -e "s/^\* .\+ ParthenonChain Foundation <dev@parthenonchain\.org> - .\+$/\* ${CHANGELOG_DATE} ParthenonChain Foundation <dev@parthenonchain.org> - ${VERSION}-${RELEASE}/" \
    "$SPEC_FILE"

# Build RPM
echo "Building RPM package..."
cd "$RPMBUILD_DIR"
rpmbuild -ba SPECS/parthenon.spec

# Copy built RPM to installers/linux directory
echo "Copying RPM to installers/linux..."
find "$RPMBUILD_DIR/RPMS" -name "parthenon-*.rpm" -exec cp {} "$SCRIPT_DIR/" \;

echo "=== RPM build complete ==="
echo "Package location: $SCRIPT_DIR/parthenon-${VERSION}-${RELEASE}.*.rpm"
