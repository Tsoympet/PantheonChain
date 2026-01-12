# macOS Installer

This directory contains scripts for creating macOS DMG installers for ParthenonChain.

## Prerequisites

- macOS 10.15 or later
- Xcode Command Line Tools
- Built binaries from the main project
- (Optional) Apple Developer account for code signing

## Building the DMG

1. Build the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

2. Create the DMG:
   ```bash
   cd installers/macos
   ./create-dmg.sh
   ```

3. Output: `parthenon-1.0.0-macos.dmg`

## Code Signing (Optional)

Set environment variables before running the script:

```bash
export CODESIGN_IDENTITY="Developer ID Application: Your Name (TEAM_ID)"
export NOTARIZE_USERNAME="your-apple-id@example.com"
export NOTARIZE_PASSWORD="app-specific-password"
export TEAM_ID="YOUR_TEAM_ID"
./create-dmg.sh
```

## Features

- Professional DMG presentation
- Drag-to-Applications installation
- Proper .app bundle structure
- Code signing support
- Notarization support for macOS 10.15+
- Universal binary support (Intel + Apple Silicon)

## Installation

Users simply drag the ParthenonChain.app to their Applications folder.

## Testing

Test the DMG on:
- macOS Intel (x86_64)
- macOS Apple Silicon (arm64)
- Verify Gatekeeper allows execution
- Check all executables launch correctly
