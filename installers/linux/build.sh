#!/bin/bash
# ParthenonChain Linux Build Script
# Wrapper script for building DEB and RPM packages

set -e

PACKAGE_TYPE="${1:-all}"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
cd "$SCRIPT_DIR"

echo "Building ParthenonChain Linux packages (type: $PACKAGE_TYPE)..."

ensure_config_file() {
    local conf_source="${ROOT_DIR}/clients/core-daemon/parthenond.conf"
    local conf_dest="${BUILD_DIR}/clients/core-daemon/parthenond.conf"

    if [ -f "${conf_dest}" ]; then
        return
    fi

    echo "parthenond.conf missing from build output; preparing package-safe config"
    mkdir -p "$(dirname "${conf_dest}")"

    if [ -f "${conf_source}" ]; then
        cp "${conf_source}" "${conf_dest}"
    else
        cat > "${conf_dest}" << 'EOF'
# Auto-generated default configuration for package builds
network_port=8333
max_connections=125
network_timeout=30
rpc_enabled=true
rpc_port=8332
rpc_user=parthenonrpc
rpc_password=change-me
data_dir=/var/lib/parthenon
log_level=info
mining_enabled=false
EOF
    fi
}

verify_config_file() {
    local conf_dest="${BUILD_DIR}/clients/core-daemon/parthenond.conf"

    if [ ! -f "${conf_dest}" ]; then
        echo "ERROR: Required config file is still missing: ${conf_dest}"
        exit 1
    fi

    echo "Verified package config: ${conf_dest}"
}

if [ "$PACKAGE_TYPE" = "deb" ] || [ "$PACKAGE_TYPE" = "rpm" ] || [ "$PACKAGE_TYPE" = "all" ]; then
    ensure_config_file
    verify_config_file
fi

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
