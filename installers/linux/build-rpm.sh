#!/bin/bash
# ParthenonChain RPM Build Script

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read version from VERSION file (supports plain semver or C/C++ #define format).
# Fallback to version.h for compatibility with older release automation.
VERSION_FILE="$PROJECT_ROOT/VERSION"
VERSION_HEADER_FILE="$PROJECT_ROOT/version.h"

extract_version() {
    local source_file="$1"
    if [ ! -f "$source_file" ]; then
        return
    fi

    # 1) Exact #define extraction, 2) exact plain semver line,
    # 3) last-resort first semver-like token found in the file.
    sed -nE 's/^#define[[:space:]]+PANTHEONCHAIN_VERSION[[:space:]]+"([^"]+)".*/\1/p' "$source_file" | head -n1
    sed -nE 's/^([0-9]+\.[0-9]+\.[0-9]+([-.+][A-Za-z0-9.-]+)?)$/\1/p' "$source_file" | head -n1
    grep -oE '[0-9]+\.[0-9]+\.[0-9]+([-.+][A-Za-z0-9.-]+)?' "$source_file" | head -n1
}

VERSION="$(extract_version "$VERSION_FILE" | sed -n '1p')"
if [ -z "$VERSION" ]; then
    VERSION="$(extract_version "$VERSION_HEADER_FILE" | sed -n '1p')"
fi
VERSION="$(printf '%s' "${VERSION:-1.0.0}" | tr -d '\r' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+([-.+][A-Za-z0-9.-]+)?' | head -n1)"
VERSION="${VERSION:-1.0.0}"
RELEASE="1"

echo "=== Building ParthenonChain RPM Package ==="
echo "Version: $VERSION-$RELEASE"

ensure_build_tools() {
    local os_id_like=""
    if [ -f /etc/os-release ]; then
        os_id_like="$(awk -F= '/^(ID|ID_LIKE)=/{print tolower($2)}' /etc/os-release | tr -d '"')"
    fi

    # Native RPM hosts should provide package names that satisfy BuildRequires.
    if echo "$os_id_like" | grep -Eq '(rhel|centos|fedora|rocky|almalinux|suse)'; then
        local missing=0
        for cmd in cmake g++ rpmbuild; do
            if ! command -v "$cmd" >/dev/null 2>&1; then
                missing=1
                break
            fi
        done

        if [ "$missing" -eq 1 ]; then
            if command -v dnf >/dev/null 2>&1; then
                echo "Installing RPM build dependencies with dnf..."
                sudo dnf install -y boost-devel cmake gcc-c++ openssl-devel rpm-build
            elif command -v yum >/dev/null 2>&1; then
                echo "Installing RPM build dependencies with yum..."
                sudo yum install -y boost-devel cmake gcc-c++ openssl-devel rpm-build
            else
                echo "ERROR: Missing build tools and no dnf/yum package manager found."
                exit 1
            fi
        fi
    # Debian/Ubuntu package names differ from RPM BuildRequires names.
    elif echo "$os_id_like" | grep -Eq '(debian|ubuntu)'; then
        if ! command -v rpmbuild >/dev/null 2>&1; then
            echo "Installing Debian/Ubuntu RPM build dependencies..."
            sudo apt-get update
            sudo apt-get install -y --no-install-recommends \
                rpm build-essential cmake g++ libssl-dev libboost-all-dev
            if ! command -v rpmbuild >/dev/null 2>&1; then
                sudo apt-get install -y --no-install-recommends rpm-build || true
            fi
        fi
    fi
}

ensure_build_tools

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
    "$SPEC_FILE"

# Build RPM
echo "Building RPM package..."
cd "$RPMBUILD_DIR"

RPMBUILD_ARGS=(-ba SPECS/parthenon.spec)

# Ubuntu runners provide Debian development packages (e.g. libssl-dev, libboost-all-dev)
# that do not satisfy RPM BuildRequires names (openssl-devel, boost-devel, gcc-c++).
# Keep strict dependency checks for native RPM environments and bypass only on Debian/Ubuntu.
if [ -f /etc/os-release ] && grep -Eq '^(ID|ID_LIKE)=.*(debian|ubuntu)' /etc/os-release; then
    echo "Detected Debian/Ubuntu host; building RPM with --nodeps due to cross-distro BuildRequires naming"
    RPMBUILD_ARGS=(--nodeps "${RPMBUILD_ARGS[@]}")
fi

rpmbuild "${RPMBUILD_ARGS[@]}"

# Copy built RPM to installers/linux directory
echo "Copying RPM to installers/linux..."
find "$RPMBUILD_DIR/RPMS" -name "parthenon-*.rpm" -exec cp {} "$SCRIPT_DIR/" \;

echo "=== RPM build complete ==="
echo "Package location: $SCRIPT_DIR/parthenon-${VERSION}-${RELEASE}.*.rpm"
