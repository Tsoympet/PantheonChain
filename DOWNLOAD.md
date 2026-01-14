# Download ParthenonChain

This guide will help you download and install ParthenonChain on your computer.

## Quick Download

Choose the installer for your operating system:

### 📥 Windows (10/11)
**[Download ParthenonChain for Windows](https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon-1.0.0-windows-x64-setup.exe)**
- File: `parthenon-1.0.0-windows-x64-setup.exe`
- Size: ~50 MB
- Requirements: Windows 10 or Windows 11 (64-bit)

### 📥 macOS
**[Download ParthenonChain for macOS](https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon-1.0.0-macos.dmg)**
- File: `parthenon-1.0.0-macos.dmg`
- Size: ~45 MB
- Requirements: macOS 10.15 (Catalina) or newer
- Supports: Intel and Apple Silicon (M1/M2/M3)

### 📥 Linux

#### Ubuntu/Debian
**[Download .deb Package](https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon_1.0.0_amd64.deb)**
- File: `parthenon_1.0.0_amd64.deb`
- Size: ~40 MB
- Requirements: Ubuntu 20.04+ or Debian 11+

#### Fedora/RHEL/CentOS
**[Download .rpm Package](https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon-1.0.0-1.el8.x86_64.rpm)**
- File: `parthenon-1.0.0-1.el8.x86_64.rpm`
- Size: ~40 MB
- Requirements: RHEL 8+, Fedora 35+, or Rocky Linux 8+

---

## Installation Instructions

### Windows

1. **Download** the installer (link above)
2. **Double-click** the downloaded `.exe` file
3. If Windows SmartScreen appears, click "More info" → "Run anyway"
4. Follow the installation wizard:
   - Accept the license agreement
   - Choose installation directory (default: `C:\Program Files\ParthenonChain`)
   - Select components to install:
     - ✅ **ParthenonChain Node** (required)
     - ✅ **Command-line Tools** (recommended)
     - ✅ **Desktop Wallet** (optional, for GUI users)
   - Click "Install"
5. **Launch** ParthenonChain from the Start Menu or Desktop shortcut

### macOS

1. **Download** the DMG file (link above)
2. **Open** the downloaded `.dmg` file
3. **Drag** the ParthenonChain icon to the Applications folder
4. **Open** Finder → Applications → ParthenonChain
5. If macOS shows "unidentified developer" warning:
   - Right-click the app → "Open"
   - Click "Open" in the dialog
   - (This is only needed the first time)

### Linux (Ubuntu/Debian)

```bash
# Download the package
wget https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon_1.0.0_amd64.deb

# Install
sudo dpkg -i parthenon_1.0.0_amd64.deb

# Install dependencies if needed
sudo apt-get install -f

# Start the service
sudo systemctl start parthenond
sudo systemctl enable parthenond  # Auto-start on boot

# Check status
sudo systemctl status parthenond
```

### Linux (Fedora/RHEL/CentOS)

```bash
# Download the package
wget https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon-1.0.0-1.el8.x86_64.rpm

# Install
sudo rpm -ivh parthenon-1.0.0-1.el8.x86_64.rpm

# Start the service
sudo systemctl start parthenond
sudo systemctl enable parthenond  # Auto-start on boot

# Check status
sudo systemctl status parthenond
```

---

## First Time Setup

### Running the Node

After installation, you can start using ParthenonChain:

**Windows (PowerShell or Command Prompt):**
```powershell
# Start the node
parthenond

# Or use the Desktop GUI
# Search for "ParthenonChain" in Start Menu
```

**macOS (Terminal):**
```bash
# Start the node
/Applications/ParthenonChain.app/Contents/MacOS/parthenond

# Or open the app from Applications folder
```

**Linux:**
```bash
# Node is already running as a service
# Use the command-line tools:
parthenon-cli getinfo
```

### Basic Commands

Once the node is running, use these commands:

```bash
# Get node information
parthenon-cli getinfo

# Get blockchain status
parthenon-cli getblockchaininfo

# Create a new wallet address
parthenon-cli getnewaddress

# Check your balance
parthenon-cli getbalance

# Send TALANTON (main currency)
parthenon-cli sendtoaddress <address> <amount> TALANTON
```

---

## System Requirements

### Minimum Requirements
- **CPU**: 2 cores @ 2.0 GHz
- **RAM**: 4 GB
- **Disk**: 50 GB free space
- **Network**: Broadband internet connection

### Recommended Requirements
- **CPU**: 4+ cores @ 3.0 GHz
- **RAM**: 8 GB or more
- **Disk**: 500 GB SSD (for better performance)
- **Network**: High-speed internet with unlimited data

---

## Verifying Your Download

For security, you should verify the downloaded file using checksums:

1. **Download checksums file**:
   - [SHA256SUMS](https://github.com/Tsoympet/PantheonChain/releases/latest/download/parthenon-1.0.0-checksums.txt)

2. **Verify** (replace filename with your downloaded file):

**Windows (PowerShell):**
```powershell
Get-FileHash -Algorithm SHA256 parthenon-1.0.0-windows-x64-setup.exe
# Compare with checksums file
```

**macOS/Linux:**
```bash
sha256sum parthenon-1.0.0-macos.dmg
# Compare with checksums file
```

---

## Alternative: Build from Source

If you prefer to build from source code:

```bash
# Clone repository
git clone --recursive https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Install (optional)
sudo cmake --install .
```

See [Building from Source](README.md#building-from-source) for detailed instructions.

---

## Getting Help

### Documentation
- 📖 [User Guide](README.md)
- 🔧 [Installation Guide](docs/INSTALLATION.md)
- 💡 [Quick Start Guide](QUICK_START.md)
- ❓ [FAQ](https://github.com/Tsoympet/PantheonChain/wiki/FAQ)

### Support
- 💬 [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions)
- 🐛 [Report Issues](https://github.com/Tsoympet/PantheonChain/issues)
- 📧 Email: support@parthenonchain.org

### Community
- Join our community forums
- Follow us on social media
- Subscribe to newsletter for updates

---

## What's Included

When you install ParthenonChain, you get:

1. **parthenond** - Full blockchain node software
2. **parthenon-cli** - Command-line wallet and tools
3. **Desktop Wallet** - User-friendly graphical interface (optional)
4. **Documentation** - Complete user and developer guides
5. **Sample Configuration** - Pre-configured settings

---

## Next Steps

After installation:

1. ✅ **Let the node sync** - First sync takes several hours (downloads blockchain)
2. ✅ **Create a wallet** - Use `parthenon-cli getnewaddress`
3. ✅ **Backup your wallet** - Keep your recovery phrase safe!
4. ✅ **Start mining** (optional) - Help secure the network
5. ✅ **Join the community** - Connect with other users

---

## Troubleshooting

### Windows: "Windows protected your PC"
This is normal for new software. Click "More info" → "Run anyway"

### macOS: "Cannot be opened because the developer cannot be verified"
Right-click the app → "Open" → "Open" in the confirmation dialog

### Linux: Service won't start
```bash
# Check logs
sudo journalctl -u parthenond -f

# Verify installation
dpkg -l | grep parthenon
```

### Node won't sync
- Check your internet connection
- Ensure ports 8333 (P2P) is not blocked by firewall
- Check disk space (need at least 50 GB free)

### Need more help?
Visit our [Troubleshooting Guide](docs/INSTALLATION.md#troubleshooting) or ask in [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions).

---

<p align="center">
  <strong>Welcome to ParthenonChain! 🏛️</strong>
</p>

<p align="center">
  <a href="https://github.com/Tsoympet/PantheonChain">GitHub</a> •
  <a href="README.md">Documentation</a> •
  <a href="https://github.com/Tsoympet/PantheonChain/releases">All Releases</a>
</p>
