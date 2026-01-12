#!/bin/bash
# ParthenonChain macOS DMG Creation Script

set -e

VERSION="1.0.0"
APP_NAME="ParthenonChain"
DMG_NAME="parthenon-${VERSION}-macos"
BUILD_DIR="../../build"
STAGING_DIR="dmg_staging"

echo "=== Creating macOS DMG for ParthenonChain v${VERSION} ==="

# Clean previous staging
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}"

# Create app bundle structure
APP_BUNDLE="${STAGING_DIR}/${APP_NAME}.app"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"
mkdir -p "${APP_BUNDLE}/Contents/Frameworks"

# Copy executables
cp "${BUILD_DIR}/clients/core-daemon/parthenond" "${APP_BUNDLE}/Contents/MacOS/"
cp "${BUILD_DIR}/clients/cli/parthenon-cli" "${APP_BUNDLE}/Contents/MacOS/"
cp "${BUILD_DIR}/clients/desktop/parthenon-qt" "${APP_BUNDLE}/Contents/MacOS/"

# Make executables executable
chmod +x "${APP_BUNDLE}/Contents/MacOS"/*

# Create Info.plist
cat > "${APP_BUNDLE}/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>parthenon-qt</string>
    <key>CFBundleIdentifier</key>
    <string>org.parthenonchain.wallet</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# Copy documentation
mkdir -p "${STAGING_DIR}/Documentation"
cp ../../README.md "${STAGING_DIR}/Documentation/"
cp ../../LICENSE "${STAGING_DIR}/Documentation/"

# Create symbolic link to Applications
ln -s /Applications "${STAGING_DIR}/Applications"

# Create DMG
echo "Creating DMG..."
hdiutil create -volname "${APP_NAME}" -srcfolder "${STAGING_DIR}" -ov -format UDZO "${DMG_NAME}.dmg"

# Code signing (if certificate available)
if [ -n "$CODESIGN_IDENTITY" ]; then
    echo "Signing DMG..."
    codesign --force --sign "$CODESIGN_IDENTITY" "${DMG_NAME}.dmg"
    
    # Notarization (if credentials available)
    if [ -n "$NOTARIZE_USERNAME" ] && [ -n "$NOTARIZE_PASSWORD" ]; then
        echo "Notarizing DMG..."
        xcrun notarytool submit "${DMG_NAME}.dmg" \
            --apple-id "$NOTARIZE_USERNAME" \
            --password "$NOTARIZE_PASSWORD" \
            --team-id "$TEAM_ID" \
            --wait
        
        echo "Stapling notarization..."
        xcrun stapler staple "${DMG_NAME}.dmg"
    fi
fi

echo "=== DMG created: ${DMG_NAME}.dmg ==="
echo "Size: $(du -h ${DMG_NAME}.dmg | cut -f1)"

# Cleanup
rm -rf "${STAGING_DIR}"
