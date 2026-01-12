# Windows Installer

This directory contains the NSIS installer script for creating Windows installers for ParthenonChain.

## Prerequisites

- NSIS (Nullsoft Scriptable Install System) 3.0 or later
- Built binaries from the main project

## Building the Installer

1. Build the project:
   ```cmd
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

2. Run NSIS:
   ```cmd
   cd installers\windows
   makensis parthenon-installer.nsi
   ```

3. Output: `parthenon-1.0.0-windows-x64-setup.exe`

## Features

- Modern UI with professional appearance
- Component selection (daemon, CLI, GUI, documentation)
- PATH environment variable modification
- Start Menu shortcuts
- Optional desktop shortcut
- Complete uninstaller
- Registry integration

## Installation Locations

- Program Files: `C:\Program Files\ParthenonChain\`
- Data Directory: `%APPDATA%\ParthenonChain\`
- Start Menu: `%STARTMENU%\ParthenonChain\`

## Testing

Test the installer on a clean Windows 10 or Windows 11 system to ensure:
- Installation completes successfully
- All components are installed
- Shortcuts work correctly
- Uninstaller removes all files
