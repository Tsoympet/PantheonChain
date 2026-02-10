# Linux Packages

This directory contains scripts for creating Debian (.deb) and RPM (.rpm) packages for ParthenonChain.

## Prerequisites

### For Debian Packages
- Debian or Ubuntu system
- `dpkg-deb` utility
- Built binaries from the main project

### For RPM Packages
- RHEL/Fedora/CentOS system (or any system with rpm-build)
- `rpmbuild` utility
- Built binaries from the main project

## Building Debian Package

1. Build the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

2. Create the package:
   ```bash
   cd installers/linux
   ./build-deb.sh
   ```

3. Output: `parthenon_1.0.0_amd64.deb`

## Building RPM Package

1. Build the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

2. Create the package:
   ```bash
   cd installers/linux
   ./build-rpm.sh
   ```

   Notes:
   - `build-rpm.sh` normalizes the spec `%changelog` header date to a valid RPM format before running `rpmbuild`.
   - On Debian/Ubuntu hosts, the script automatically uses `rpmbuild --nodeps` because RPM `BuildRequires` names (such as `openssl-devel` and `boost-devel`) do not map 1:1 to Debian package names (`libssl-dev`, `libboost-all-dev`).

3. Output: `parthenon-1.0.0-1.*.rpm`

## Installation

### Debian/Ubuntu
```bash
sudo dpkg -i parthenon_1.0.0_amd64.deb
sudo systemctl enable parthenond
sudo systemctl start parthenond
```

### RHEL/Fedora/CentOS
```bash
sudo rpm -ivh parthenon-1.0.0-1.el8.x86_64.rpm
sudo systemctl enable parthenond
sudo systemctl start parthenond
```

## Features

- Systemd service integration
- Automatic user creation (parthenon)
- FHS-compliant installation
- Man pages for all executables
- Pre/post install scripts
- Dependency management
- Clean uninstallation

## File Locations

- Binaries: `/usr/bin/`
- Configuration: `/etc/parthenon/`
- Data: `/var/lib/parthenon/`
- Service: `/lib/systemd/system/parthenond.service`
- Man pages: `/usr/share/man/man1/`

## Testing

Test on:
- Debian 11+
- Ubuntu 20.04+
- RHEL 8+
- Fedora 35+
- Rocky Linux 8+

Verify:
- Package installs without errors
- Service starts and runs
- All commands work
- Uninstallation cleans up properly
