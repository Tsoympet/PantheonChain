# Installation Guide

Complete installation instructions for ParthenonChain on all supported platforms.

## System Requirements

### Minimum Requirements
- **CPU**: 2 cores, 2.0 GHz
- **RAM**: 4 GB
- **Disk**: 50 GB free space (SSD recommended)
- **Network**: Broadband internet connection

### Recommended Requirements
- **CPU**: 4+ cores, 3.0 GHz
- **RAM**: 8 GB
- **Disk**: 500 GB SSD
- **Network**: High-speed internet (unlimited data)

## Pre-built Installers

### Windows

**Download**: [parthenon-1.0.0-windows-x64-setup.exe](https://github.com/Tsoympet/PantheonChain/releases)

**Installation Steps**:

1. Download the installer
2. Verify checksum (see [Verification](#verification))
3. Run the installer as Administrator
4. Follow installation wizard
5. Choose components:
   - ✅ ParthenonChain Node (parthenond)
   - ✅ Command-line Tools (parthenon-cli)
   - ✅ Desktop Wallet (optional)
6. Select installation directory (default: `C:\Program Files\ParthenonChain`)
7. Complete installation

**Default Locations**:
- Executables: `C:\Program Files\ParthenonChain\bin\`
- Data directory: `C:\Users\<username>\AppData\Roaming\ParthenonChain\`
- Configuration: `C:\Users\<username>\AppData\Roaming\ParthenonChain\parthenond.conf`

**First Run**:
```powershell
# Open PowerShell
cd "C:\Program Files\ParthenonChain\bin"
.\parthenond.exe
```

### macOS

**Download**: [parthenon-1.0.0-macos.dmg](https://github.com/Tsoympet/PantheonChain/releases)

**Installation Steps**:

1. Download the DMG file
2. Verify checksum (see [Verification](#verification))
3. Open the DMG file
4. Drag ParthenonChain.app to Applications folder
5. Right-click the app and select "Open" (first time only, to bypass Gatekeeper)

**Default Locations**:
- Application: `/Applications/ParthenonChain.app`
- Data directory: `~/Library/Application Support/ParthenonChain/`
- Configuration: `~/Library/Application Support/ParthenonChain/parthenond.conf`

**First Run**:
```bash
# Open Terminal
/Applications/ParthenonChain.app/Contents/MacOS/parthenond
```

**macOS Security**:
- If you see "unidentified developer" warning, go to System Preferences > Security & Privacy and click "Open Anyway"
- For macOS 10.15+, the app is notarized by Apple

### Linux

#### Debian/Ubuntu (.deb)

**Download**: [parthenon_1.0.0_amd64.deb](https://github.com/Tsoympet/PantheonChain/releases)

**Installation**:
```bash
# Install
sudo dpkg -i parthenon_1.0.0_amd64.deb

# Install dependencies if needed
sudo apt-get install -f
```

**Start service**:
```bash
# Start node
sudo systemctl start parthenond

# Enable auto-start
sudo systemctl enable parthenond

# Check status
sudo systemctl status parthenond
```

**Default Locations**:
- Executables: `/usr/bin/`
- Configuration: `/etc/parthenon/parthenond.conf`
- Data directory: `/var/lib/parthenon/`
- Logs: `/var/log/parthenon/`
- Service file: `/lib/systemd/system/parthenond.service`

#### RHEL/Fedora/CentOS (.rpm)

**Download**: [parthenon-1.0.0-1.el8.x86_64.rpm](https://github.com/Tsoympet/PantheonChain/releases)

**Installation**:
```bash
# Install
sudo rpm -i parthenon-1.0.0-1.el8.x86_64.rpm

# Or with yum
sudo yum install parthenon-1.0.0-1.el8.x86_64.rpm
```

**Start service**:
```bash
# Start node
sudo systemctl start parthenond

# Enable auto-start
sudo systemctl enable parthenond
```

**Default Locations**: Same as Debian/Ubuntu

#### Generic Linux (Build from Source)

See [Building from Source](#building-from-source) below.

## Verification

### Checksum Verification

**Download checksums**:
```bash
wget https://github.com/Tsoympet/PantheonChain/releases/download/v1.0.0/parthenon-1.0.0-checksums.txt
```

**Verify (Linux/macOS)**:
```bash
# SHA-256
sha256sum -c parthenon-1.0.0-checksums.txt

# Or manually
sha256sum parthenon-1.0.0-windows-x64-setup.exe
```

**Verify (Windows)**:
```powershell
CertUtil -hashfile parthenon-1.0.0-windows-x64-setup.exe SHA256
```

### GPG Signature Verification

**Import release key**:
```bash
wget https://github.com/Tsoympet/PantheonChain/releases/download/v1.0.0/parthenon-release-key.asc
gpg --import parthenon-release-key.asc
```

**Verify signature**:
```bash
gpg --verify parthenon-1.0.0-windows-x64-setup.exe.asc parthenon-1.0.0-windows-x64-setup.exe
```

Expected output:
```
gpg: Good signature from "ParthenonChain Release Signing Key"
```

## Building from Source

### Prerequisites

**All Platforms**:
- CMake 3.15+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Git

**Linux**:
```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install build-essential cmake git libssl-dev

# RHEL/Fedora
sudo yum install gcc gcc-c++ cmake git openssl-devel

# Arch
sudo pacman -S base-devel cmake git openssl
```

**macOS**:
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl
```

**Windows**:
- Install Visual Studio 2017 or later
- Install CMake from https://cmake.org/download/
- Install Git from https://git-scm.com/download/win

### Build Steps

```bash
# Clone repository
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Initialize submodules
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build (Linux/macOS)
make -j$(nproc)

# Build (Windows)
cmake --build . --config Release

# Run tests
ctest

# Install (optional, Linux/macOS)
sudo make install
```

### Build Options

```bash
# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build (with symbols)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build specific components only
cmake -DBUILD_DAEMON=ON -DBUILD_CLI=ON -DBUILD_GUI=OFF ..

# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/parthenon ..
```

## Configuration

### Creating Config File

**Linux/macOS**:
```bash
mkdir -p ~/.parthenon
nano ~/.parthenon/parthenond.conf
```

**Windows**:
```powershell
mkdir $env:APPDATA\ParthenonChain
notepad $env:APPDATA\ParthenonChain\parthenond.conf
```

### Basic Configuration

```conf
# Network
network.port=8333
network.max_connections=125

# RPC
rpc.enabled=true
rpc.port=8332
rpc.user=yourusername
rpc.password=CHANGE_THIS_TO_STRONG_PASSWORD
# Dev-only escape hatch (NOT recommended for production):
# rpc.allow_unauthenticated=true

# Data
data_dir=~/.parthenon/data
log_level=info

# Mining (optional)
mining.enabled=false
```

### Advanced Configuration

```conf
# Performance
mempool.max_size=300
dbcache=450

# Network
network.timeout=60
network.listen=true
network.bind=0.0.0.0

# Pruning (save disk space)
prune=5000  # Keep last 5 GB

# Logging
log_level=debug
log_file=~/.parthenon/debug.log
```

## Running the Node

### First Sync

Initial blockchain sync will take several hours depending on your internet speed.

**Linux/macOS**:
```bash
parthenond
```

**Windows**:
```powershell
cd "C:\Program Files\ParthenonChain\bin"
.\parthenond.exe
```

**Progress monitoring**:
```bash
# Check sync progress
parthenon-cli getblockchaininfo

# Check peer count
parthenon-cli getpeerinfo

# Check mempool
parthenon-cli getmempoolinfo
```

### Systemd Service (Linux)

Already configured if you installed via .deb or .rpm.

**Manual setup** (if built from source):

```bash
# Create service file
sudo nano /etc/systemd/system/parthenond.service
```

```ini
[Unit]
Description=ParthenonChain Daemon
After=network.target

[Service]
Type=simple
User=parthenon
Group=parthenon
ExecStart=/usr/local/bin/parthenond
Restart=on-failure
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

[Install]
WantedBy=multi-user.target
```

```bash
# Create user
sudo useradd -r -s /bin/false parthenon

# Set permissions
sudo chown -R parthenon:parthenon /var/lib/parthenon

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable parthenond
sudo systemctl start parthenond
```

## Firewall Configuration

### Linux (ufw)

```bash
# Allow P2P port
sudo ufw allow 8333/tcp

# Allow RPC (localhost only)
sudo ufw allow from 127.0.0.1 to any port 8332
```

### Windows Firewall

```powershell
# Allow inbound P2P
New-NetFirewallRule -DisplayName "ParthenonChain P2P" -Direction Inbound -Protocol TCP -LocalPort 8333 -Action Allow
```

### macOS Firewall

System Preferences > Security & Privacy > Firewall > Firewall Options > Allow ParthenonChain

## Uninstallation

### Windows
Control Panel > Programs and Features > ParthenonChain > Uninstall

### macOS
```bash
# Remove application
rm -rf /Applications/ParthenonChain.app

# Remove data (optional)
rm -rf ~/Library/Application\ Support/ParthenonChain
```

### Linux (Debian/Ubuntu)
```bash
sudo apt-get remove parthenon
sudo apt-get purge parthenon  # Also remove config
```

### Linux (RHEL/Fedora)
```bash
sudo yum remove parthenon
```

## Troubleshooting

### Node won't start

**Check logs**:
```bash
# Linux
tail -f ~/.parthenon/debug.log

# Windows
type %APPDATA%\ParthenonChain\debug.log
```

**Common issues**:
- Port 8333 already in use → Change `network.port` in config
- Disk full → Reduce `dbcache` or enable pruning
- Corrupted database → Delete data directory and resync

### Can't connect to peers

- Check firewall allows port 8333
- Verify internet connection
- Try adding manual peers: `addnode=<ip>:8333` in config

### RPC connection refused

- Ensure `rpc.enabled=true`
- Check `rpc.user` and `rpc.password` are set
- By default, daemon refuses unauthenticated RPC unless `rpc.allow_unauthenticated=true`
- Verify firewall allows port 8332 (localhost only)

## Getting Help

- **Documentation**: https://github.com/Tsoympet/PantheonChain/tree/main/docs
- **Issues**: https://github.com/Tsoympet/PantheonChain/issues
- **Community**: [TBD - Discord/Telegram/Forum]

---

**Next Steps**:
- Read [architecture.md](architecture.md) to understand the system
- Review [SECURITY.md](../SECURITY.md) for security best practices
- See [Layer 1 Core](LAYER1_CORE.md) for technical details
