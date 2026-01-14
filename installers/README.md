# ParthenonChain Installers

This directory contains scripts and configurations for building platform-specific installers for ParthenonChain.

## 📦 Available Installers

| Platform | Installer Type | Output File | Documentation |
|----------|---------------|-------------|---------------|
| **Windows** | NSIS Setup | `parthenon-1.0.0-windows-x64-setup.exe` | [windows/README.md](windows/README.md) |
| **macOS** | DMG Disk Image | `parthenon-1.0.0-macos.dmg` | [macos/README.md](macos/README.md) |
| **Linux (Debian/Ubuntu)** | DEB Package | `parthenon_1.0.0_amd64.deb` | [linux/README.md](linux/README.md) |
| **Linux (RHEL/Fedora)** | RPM Package | `parthenon-1.0.0-1.el8.x86_64.rpm` | [linux/README.md](linux/README.md) |

## 🚀 Quick Build

### Prerequisites

First, build the project binaries:

```bash
cd /path/to/PantheonChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Build All Installers (GitHub Actions)

The easiest way to build all installers is to push a version tag, which triggers the automated release workflow:

```bash
# Update version in VERSION file, CMakeLists.txt, and CHANGELOG.md
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

This automatically:
1. Builds for Windows, macOS, and Linux
2. Creates installers for all platforms
3. Runs tests on each platform
4. Generates checksums (SHA-256, SHA-512)
5. Signs artifacts with GPG (if configured)
6. Creates a GitHub Release draft
7. Uploads all installers

### Build Installers Locally

#### Windows (NSIS Installer)

**Requirements**: Windows 10+, NSIS 3.0+, Visual Studio 2019+

```powershell
cd installers\windows
powershell -ExecutionPolicy Bypass -File build.ps1
```

Output: `parthenon-1.0.0-windows-x64-setup.exe`

#### macOS (DMG)

**Requirements**: macOS 10.15+, Xcode Command Line Tools

```bash
cd installers/macos
./build.sh
```

Output: `parthenon-1.0.0-macos.dmg`

#### Linux (Debian/Ubuntu)

**Requirements**: Ubuntu 20.04+ or Debian 11+

```bash
cd installers/linux
./build-deb.sh
```

Output: `parthenon_1.0.0_amd64.deb`

#### Linux (RHEL/Fedora/CentOS)

**Requirements**: RHEL 8+, Fedora 35+, or Rocky Linux 8+, rpm-build

```bash
cd installers/linux
./build-rpm.sh
```

Output: `parthenon-1.0.0-1.el8.x86_64.rpm`

## 🔐 Security & Verification

### Generating Checksums

After building installers, generate checksums:

```bash
cd installers/checksums
./generate-checksums.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
```

This creates `parthenon-1.0.0-checksums.txt` with SHA-256 and SHA-512 hashes.

### Signing Releases

Sign installers with GPG for authenticity:

```bash
export GPG_KEY_ID="your-gpg-key-id"
cd installers/checksums
./sign-release.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
```

This creates `.asc` signature files for each installer.

### Verifying Downloads (For Users)

Users can verify downloaded installers:

```bash
# Verify GPG signature
gpg --verify parthenon-1.0.0-windows-x64-setup.exe.asc parthenon-1.0.0-windows-x64-setup.exe

# Verify checksum
sha256sum parthenon-1.0.0-windows-x64-setup.exe
# Compare with parthenon-1.0.0-checksums.txt
```

## 📋 Installer Features

### Windows Installer (NSIS)

✅ Modern UI with professional appearance  
✅ Component selection (daemon, CLI, GUI, documentation)  
✅ PATH environment variable modification  
✅ Start Menu shortcuts  
✅ Optional desktop shortcut  
✅ Complete uninstaller  
✅ Registry integration  
✅ Windows 10/11 compatibility  

**Installation Locations**:
- Program Files: `C:\Program Files\ParthenonChain\`
- Data Directory: `%APPDATA%\ParthenonChain\`
- Start Menu: `%STARTMENU%\ParthenonChain\`

### macOS Installer (DMG)

✅ Professional DMG presentation  
✅ Drag-to-Applications installation  
✅ Proper .app bundle structure  
✅ Code signing support  
✅ Notarization support for macOS 10.15+  
✅ Universal binary support (Intel + Apple Silicon)  

**Installation Locations**:
- Application: `/Applications/ParthenonChain.app`
- Data Directory: `~/Library/Application Support/ParthenonChain/`

### Linux Packages (DEB/RPM)

✅ Systemd service integration  
✅ Automatic user creation (parthenon)  
✅ FHS-compliant installation  
✅ Man pages for all executables  
✅ Pre/post install scripts  
✅ Dependency management  
✅ Clean uninstallation  

**Installation Locations**:
- Binaries: `/usr/bin/`
- Configuration: `/etc/parthenon/`
- Data: `/var/lib/parthenon/`
- Service: `/lib/systemd/system/parthenond.service`
- Man pages: `/usr/share/man/man1/`

## 🧪 Testing Installers

### Pre-Release Testing

Before releasing, test installers on clean systems:

#### Windows
- Test on Windows 10 (21H2+)
- Test on Windows 11
- Verify all components install
- Check shortcuts work
- Test uninstaller

#### macOS
- Test on macOS Intel (x86_64)
- Test on macOS Apple Silicon (arm64)
- Verify Gatekeeper allows execution
- Check all executables launch

#### Linux
- Test on Ubuntu 20.04, 22.04, 24.04
- Test on Debian 11, 12
- Test on Fedora 38, 39, 40
- Test on RHEL 8, 9
- Verify service starts automatically
- Check all commands work

### Automated Testing

The `.github/workflows/installers.yml` workflow runs automated tests on all platforms for every PR.

## 🛠️ Customization

### Modifying Installer Scripts

Each platform has its own build scripts and configuration:

- **Windows**: Edit `windows/parthenon-installer.nsi` and `windows/build.ps1`
- **macOS**: Edit `macos/create-dmg.sh` and `macos/build.sh`
- **Linux**: Edit `linux/build-deb.sh`, `linux/build-rpm.sh`, or `linux/parthenon.spec`

### Changing Icons/Branding

Icons and assets are in `assets/icons/`:

```
assets/icons/
├── parthenon-128.png
├── parthenon-256.png
├── parthenon-512.png
├── parthenon.icns (macOS)
└── parthenon.ico (Windows)
```

Update references in installer scripts after changing icons.

## 📚 Platform-Specific Documentation

- **Windows**: [windows/README.md](windows/README.md)
- **macOS**: [macos/README.md](macos/README.md)
- **Linux**: [linux/README.md](linux/README.md)
- **Checksums**: [checksums/README.md](checksums/README.md)

## 🔄 Release Process

### Manual Release

1. **Update version numbers**:
   - `VERSION` file
   - `CMakeLists.txt` (project VERSION)
   - `CHANGELOG.md`

2. **Build installers** (locally or via CI)

3. **Generate checksums**:
   ```bash
   cd installers/checksums
   ./generate-checksums.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
   ```

4. **Sign artifacts** (optional but recommended):
   ```bash
   GPG_KEY_ID=<your-key-id> ./sign-release.sh ../windows/*.exe ../macos/*.dmg ../linux/*.deb ../linux/*.rpm
   ```

5. **Create GitHub Release**:
   - Go to https://github.com/Tsoympet/PantheonChain/releases
   - Click "Draft a new release"
   - Create tag (e.g., `v1.0.0`)
   - Upload installers, checksums, and signatures
   - Write release notes
   - Publish

### Automated Release (Recommended)

Simply push a version tag:

```bash
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

The `.github/workflows/release.yml` workflow handles everything automatically.

## 🌐 Distribution

After building installers:

1. **GitHub Releases** - Primary distribution (automatic via workflow)
2. **Website** - Link to GitHub releases or host directly
3. **Package Managers** - Submit to Homebrew, Chocolatey, apt repos, etc.
4. **Mirrors** - Set up CDN mirrors for faster downloads

## 📊 Analytics

Track installer downloads using:
- GitHub release download statistics
- Website analytics
- Package manager statistics

## ❓ Troubleshooting

### Build Fails on Windows

- Ensure NSIS is installed: `choco install nsis`
- Verify Visual Studio 2019+ with C++ support
- Check CMake is in PATH
- Run as Administrator

### Build Fails on macOS

- Install Xcode Command Line Tools: `xcode-select --install`
- Verify Homebrew dependencies: `brew install cmake qt`
- Check code signing certificate (if signing)

### Build Fails on Linux

- Install build dependencies:
  ```bash
  sudo apt-get install build-essential cmake libssl-dev libboost-all-dev dpkg-dev
  ```
- For RPM: `sudo dnf install rpm-build`

### Installer Won't Run

- **Windows**: Check SmartScreen settings, run as Administrator
- **macOS**: Right-click → "Open" to bypass Gatekeeper
- **Linux**: Verify package integrity with `dpkg --info` or `rpm -qip`

## 📞 Support

For installer-related issues:

- **Documentation**: See platform-specific READMEs
- **Issues**: https://github.com/Tsoympet/PantheonChain/issues
- **Discussions**: https://github.com/Tsoympet/PantheonChain/discussions

## 📜 License

All installer scripts and configurations are licensed under the MIT License, same as ParthenonChain.

---

<p align="center">
  <strong>Build professional installers for ParthenonChain! 🏛️</strong>
</p>
