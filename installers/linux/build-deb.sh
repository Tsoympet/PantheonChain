#!/bin/bash
# Build Debian package for ParthenonChain

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

VERSION="1.0.0"
ARCH="amd64"
PACKAGE_NAME="parthenon"
BUILD_DIR="${ROOT_DIR}/build"
DEB_DIR="${SCRIPT_DIR}/deb_build"

echo "=== Building Debian package for ParthenonChain v${VERSION} ==="

# Clean previous build
rm -rf "${DEB_DIR}"
mkdir -p "${DEB_DIR}/DEBIAN"
mkdir -p "${DEB_DIR}/usr/bin"
mkdir -p "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}"
mkdir -p "${DEB_DIR}/usr/share/man/man1"
mkdir -p "${DEB_DIR}/etc/parthenon"
mkdir -p "${DEB_DIR}/lib/systemd/system"

# Create control file
cat > "${DEB_DIR}/DEBIAN/control" << EOF
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: net
Priority: optional
Architecture: ${ARCH}
Depends: libssl3 (>= 3.0.0), libboost-system1.74.0, libboost-filesystem1.74.0, libboost-thread1.74.0
Maintainer: ParthenonChain Foundation <dev@parthenonchain.org>
Description: ParthenonChain - Multi-Asset Proof-of-Work Blockchain
 ParthenonChain is a production-grade Layer-1 blockchain with:
  - Three native assets (TALN, DRM, OBL)
  - SHA-256d Proof-of-Work consensus
  - EVM-compatible smart contracts (OBOLOS)
  - Payment channels and Layer 2 support
  - Full node daemon, CLI, and GUI wallet
Homepage: https://parthenonchain.org
EOF

# Copy binaries
cp "${BUILD_DIR}/clients/core-daemon/parthenond" "${DEB_DIR}/usr/bin/"
cp "${BUILD_DIR}/clients/cli/parthenon-cli" "${DEB_DIR}/usr/bin/"
cp "${BUILD_DIR}/clients/desktop/parthenon-qt" "${DEB_DIR}/usr/bin/"
chmod 755 "${DEB_DIR}/usr/bin"/*

# Ensure configuration exists, then copy it
CONFIG_PATH="${BUILD_DIR}/clients/core-daemon/parthenond.conf"
SOURCE_CONFIG_PATH="${ROOT_DIR}/clients/core-daemon/parthenond.conf"
EXAMPLE_CONFIG_PATH="${ROOT_DIR}/parthenond.conf.example"

resolve_config_source() {
    # 1) CMake-generated file in build tree
    if [ -f "${CONFIG_PATH}" ]; then
        echo "${CONFIG_PATH}"
        return
    fi

    # 2) Repository daemon config (if tracked)
    if [ -f "${SOURCE_CONFIG_PATH}" ]; then
        echo "${SOURCE_CONFIG_PATH}"
        return
    fi

    # 3) Top-level example config
    if [ -f "${EXAMPLE_CONFIG_PATH}" ]; then
        echo "${EXAMPLE_CONFIG_PATH}"
        return
    fi

    # 4) Last-resort synthesized config
    mkdir -p "$(dirname "${CONFIG_PATH}")"
    cat > "${CONFIG_PATH}" << 'EOF'
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
    echo "${CONFIG_PATH}"
}

RESOLVED_CONFIG_PATH="$(resolve_config_source)"
echo "Using daemon config: ${RESOLVED_CONFIG_PATH}"
cp "${RESOLVED_CONFIG_PATH}" "${DEB_DIR}/etc/parthenon/parthenond.conf"

# Copy documentation
cp "${ROOT_DIR}/README.md" "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}/"
cp "${ROOT_DIR}/EULA.md" "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}/"
cp "${ROOT_DIR}/WHITEPAPER.md" "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}/"
if [ -f "${ROOT_DIR}/LICENSE" ]; then
    cp "${ROOT_DIR}/LICENSE" "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}/"
fi
cp "${ROOT_DIR}/CHANGELOG.md" "${DEB_DIR}/usr/share/doc/${PACKAGE_NAME}/"

# Copy icon if available
if [ -f "${ROOT_DIR}/clients/desktop/assets/icon.png" ]; then
    mkdir -p "${DEB_DIR}/usr/share/pixmaps"
    cp "${ROOT_DIR}/clients/desktop/assets/icon.png" "${DEB_DIR}/usr/share/pixmaps/parthenon.png"
fi

# Create desktop entry
mkdir -p "${DEB_DIR}/usr/share/applications"
cat > "${DEB_DIR}/usr/share/applications/parthenon-qt.desktop" << EOF
[Desktop Entry]
Version=1.0
Name=ParthenonChain Wallet
Comment=Multi-Asset Blockchain Wallet
Exec=/usr/bin/parthenon-qt
Icon=parthenon
Terminal=false
Type=Application
Categories=Finance;Network;
Keywords=blockchain;cryptocurrency;wallet;
EOF

# Create systemd service
cat > "${DEB_DIR}/lib/systemd/system/parthenond.service" << EOF
[Unit]
Description=ParthenonChain Daemon
After=network.target

[Service]
Type=simple
User=parthenon
Group=parthenon
ExecStart=/usr/bin/parthenond
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Create man pages
cat > "${DEB_DIR}/usr/share/man/man1/parthenond.1" << EOF
.TH PARTHENOND 1 "$(date +%Y-%m-%d)" "ParthenonChain ${VERSION}" "User Commands"
.SH NAME
parthenond \- ParthenonChain full node daemon
.SH SYNOPSIS
.B parthenond
[\fI\,OPTIONS\/\fR]
.SH DESCRIPTION
parthenond is the full node daemon for the ParthenonChain network.
It maintains the complete blockchain, validates transactions and blocks,
and participates in the peer-to-peer network.
.SH OPTIONS
.TP
\fB\-\-datadir=DIR\fR
Specify data directory
.TP
\fB\-\-conf=FILE\fR
Specify configuration file
.TP
\fB\-\-daemon\fR
Run in background as daemon
.SH FILES
.TP
\fI/etc/parthenon/parthenond.conf\fR
Default configuration file
.SH SEE ALSO
parthenon-cli(1), parthenon-qt(1)
EOF

cat > "${DEB_DIR}/usr/share/man/man1/parthenon-cli.1" << EOF
.TH PARTHENON-CLI 1 "$(date +%Y-%m-%d)" "ParthenonChain ${VERSION}" "User Commands"
.SH NAME
parthenon-cli \- ParthenonChain RPC client
.SH SYNOPSIS
.B parthenon-cli
[\fI\,COMMAND\/\fR] [\fI\,ARGS\/\fR]
.SH DESCRIPTION
parthenon-cli is the command-line RPC client for interacting with parthenond.
.SH COMMANDS
.TP
\fBgetinfo\fR
Get general information about the node
.TP
\fBgetblockcount\fR
Get current blockchain height
.TP
\fBgetbalance ASSET\fR
Get balance for specified asset (TALN, DRM, OBL)
.SH SEE ALSO
parthenond(1)
EOF

# Compress man pages
gzip -9 "${DEB_DIR}/usr/share/man/man1"/*.1

# Post-install script
cat > "${DEB_DIR}/DEBIAN/postinst" << EOF
#!/bin/bash
set -e

# Create user if doesn't exist
if ! id -u parthenon > /dev/null 2>&1; then
    useradd -r -s /bin/false -d /var/lib/parthenon parthenon
fi

# Create data directory
mkdir -p /var/lib/parthenon
chown parthenon:parthenon /var/lib/parthenon

# Reload systemd
if [ -d /run/systemd/system ]; then
    systemctl daemon-reload
fi

exit 0
EOF

chmod 755 "${DEB_DIR}/DEBIAN/postinst"

# Post-remove script
cat > "${DEB_DIR}/DEBIAN/postrm" << EOF
#!/bin/bash
set -e

if [ "\$1" = "purge" ]; then
    rm -rf /var/lib/parthenon
    userdel parthenon 2>/dev/null || true
fi

exit 0
EOF

chmod 755 "${DEB_DIR}/DEBIAN/postrm"

# Build package
OUTPUT_DEB="${SCRIPT_DIR}/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build "${DEB_DIR}" "${OUTPUT_DEB}"

echo "=== Debian package created: ${OUTPUT_DEB} ==="
echo "Size: $(du -h "${OUTPUT_DEB}" | cut -f1)"

# Cleanup
rm -rf "${DEB_DIR}"
