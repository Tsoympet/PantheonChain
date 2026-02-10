#!/bin/bash
# ParthenonChain RPM Build Script

# Some CI jobs invoke this script with `sh`, which breaks bash-specific features
# (`local`, arrays, BASH_SOURCE). Re-exec under bash early when needed.
running_shell="$(ps -p $$ -o comm= 2>/dev/null | tr -d '[:space:]' || true)"
if [ -z "${BASH_VERSION:-}" ] || [ -z "${BASH_SOURCE:-}" ] || [ "$running_shell" != "bash" ]; then
    exec bash "$0" "$@"
fi

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Read version from VERSION file (supports plain semver or C/C++ #define format).
# Fallback to version.h for compatibility with older release automation.
VERSION_FILE="$PROJECT_ROOT/VERSION"
VERSION_HEADER_FILE="$PROJECT_ROOT/version.h"

extract_version() {
    source_file="$1"
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
CHANGELOG_DATE="$(LC_ALL=C date '+%a %b %d %Y')"

echo "=== Building ParthenonChain RPM Package ==="
echo "Version: $VERSION-$RELEASE"

ensure_build_tools() {
    run_pkg_install() {
        if command -v sudo >/dev/null 2>&1; then
            sudo "$@"
        else
            "$@"
        fi
    }

    local os_id_like=""
    if [ -f /etc/os-release ]; then
        os_id_like="$(awk -F= '/^(ID|ID_LIKE)=/{print tolower($2)}' /etc/os-release | tr -d '"')"
    fi

    OS_ID_LIKE="$os_id_like"

    # Native RPM hosts should provide package names that satisfy BuildRequires.
    if echo "$os_id_like" | grep -Eq '(rhel|centos|fedora|rocky|almalinux|suse)'; then
        missing=0
        for cmd in cmake g++ rpmbuild; do
            if ! command -v "$cmd" >/dev/null 2>&1; then
                missing=1
                break
            fi
        done

        if [ "$missing" -eq 1 ]; then
            if command -v dnf >/dev/null 2>&1; then
                echo "Installing RPM build dependencies with dnf..."
                run_pkg_install dnf install -y boost-devel cmake gcc-c++ openssl-devel rpm-build
            elif command -v yum >/dev/null 2>&1; then
                echo "Installing RPM build dependencies with yum..."
                run_pkg_install yum install -y boost-devel cmake gcc-c++ openssl-devel rpm-build
            else
                echo "ERROR: Missing build tools and no dnf/yum package manager found."
                exit 1
            fi
        fi
    # Debian/Ubuntu package names differ from RPM BuildRequires names.
    elif echo "$os_id_like" | grep -Eq '(debian|ubuntu)'; then
        if ! command -v rpmbuild >/dev/null 2>&1; then
            echo "Installing Debian/Ubuntu RPM build dependencies..."
            run_pkg_install apt-get update
            run_pkg_install apt-get install -y --no-install-recommends \
                rpm build-essential cmake g++ libssl-dev libboost-all-dev
            if ! command -v rpmbuild >/dev/null 2>&1; then
                run_pkg_install apt-get install -y --no-install-recommends rpm-build || true
            fi
        fi
    fi
}

ensure_build_tools

ensure_git_safe_directory() {
    local existing_safe
    local current_dir

    existing_safe="$(git config --global --get-all safe.directory 2>/dev/null || true)"

    add_safe_directory() {
        local dir="$1"
        if [ -z "$dir" ]; then
            return
        fi

        if ! printf '%s\n' "$existing_safe" | grep -Fxq "$dir"; then
            echo "Configuring git safe.directory for: $dir"
            git config --global --add safe.directory "$dir"
            existing_safe="$(printf '%s\n%s\n' "$existing_safe" "$dir" | sed '/^$/d')"
        fi
    }

    # CI environments often mount workspaces with ownership that differs from the current user.
    # Walk upward to register any parent repo roots (e.g. /workspace), then project root itself.
    current_dir="$PROJECT_ROOT"
    while [ "$current_dir" != "/" ]; do
        if [ -d "$current_dir/.git" ] || [ -f "$current_dir/.git" ]; then
            add_safe_directory "$current_dir"
        fi
        current_dir="$(dirname "$current_dir")"
    done

    add_safe_directory "$PROJECT_ROOT"
}

ensure_git_safe_directory

print_git_safe_directories() {
    echo "Current git safe.directory entries:"
    if ! git config --global --get-all safe.directory 2>/dev/null; then
        echo "(none)"
    fi
}

print_git_safe_directories

create_source_tarball() {
    local tarball_path="$1"

    # RPM builds require vendored third_party submodules (secp256k1/leveldb/etc.).
    # `git archive` omits submodule contents, so package from the working tree instead.
    if [ -f "$PROJECT_ROOT/.gitmodules" ]; then
        git -C "$PROJECT_ROOT" submodule update --init --recursive
    fi

    tar \
        --exclude-vcs \
        --exclude='./installers/linux/parthenon-*.rpm' \
        -czf "$tarball_path" \
        --transform "s|^\.|parthenon-${VERSION}|" \
        -C "$PROJECT_ROOT" .
}
# Create RPM build directory structure
RPMBUILD_DIR="$HOME/rpmbuild"
mkdir -p "$RPMBUILD_DIR"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball
TARBALL="parthenon-${VERSION}.tar.gz"
echo "Creating source tarball: $TARBALL"
cd "$PROJECT_ROOT"
create_source_tarball "$RPMBUILD_DIR/SOURCES/$TARBALL"

# Copy spec file to SPECS directory and template dynamic values
echo "Copying spec file..."
SPEC_FILE="$RPMBUILD_DIR/SPECS/parthenon.spec"
cp "$SCRIPT_DIR/parthenon.spec" "$SPEC_FILE"

sed -i \
    -e "s/^Version:[[:space:]].*/Version:        ${VERSION}/" \
    -e "0,/^\* [A-Za-z]{3} [A-Za-z]{3} [ 0-9]{2} [0-9]{4} ParthenonChain Foundation <dev@parthenonchain.org> - [0-9][^[:space:]]*/s//\* ${CHANGELOG_DATE} ParthenonChain Foundation <dev@parthenonchain.org> - ${VERSION}-${RELEASE}/" \
    "$SPEC_FILE"

# Build RPM
echo "Building RPM package..."
cd "$RPMBUILD_DIR"

RPMBUILD_ARGS=(-ba SPECS/parthenon.spec)
if echo "$OS_ID_LIKE" | grep -Eq '(debian|ubuntu)'; then
    # Debian/Ubuntu use different package naming than RPM BuildRequires.
    # Toolchain dependencies are installed via apt above, so skip rpm db checks here.
    RPMBUILD_ARGS=(--nodeps --define "_unitdir /usr/lib/systemd/system" -ba SPECS/parthenon.spec)
fi

rpmbuild "${RPMBUILD_ARGS[@]}"

# Copy built RPM to installers/linux directory
echo "Copying RPM to installers/linux..."
find "$RPMBUILD_DIR/RPMS" -name "parthenon-*.rpm" -exec cp {} "$SCRIPT_DIR/" \;

echo "=== RPM build complete ==="
echo "Package location: $SCRIPT_DIR/parthenon-${VERSION}-${RELEASE}.*.rpm"
