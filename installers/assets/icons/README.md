# ParthenonChain Installer Icons

This directory contains the SVG icons used for ParthenonChain platform installers.

## Purpose

These icons are used in the installation packages for Windows, macOS, and Linux distributions. They provide consistent branding across all installer platforms.

## Icon Files

- **parthenon-installer-win.svg** - Windows installer icon (NSIS, MSI)
- **parthenon-installer-mac.svg** - macOS DMG installer icon
- **parthenon-installer-linux.svg** - Linux package icon (.deb, .rpm, AppImage)

## Icon Specifications

### Format
- **Primary format**: SVG 1.1 / SVG 2.0 (vector-only, canonical source)
- **Scalable**: Works from 16×16 to 512×512
- **No raster**: PNG exports generated FROM SVG when required by platform

### Design
- **Style**: Technical, cryptographic, Greek-inspired geometry
- **Colors**: Max 2 colors per icon, no gradients
- **Geometry**: Pure geometric primitives (circles, squares, triangles, lines)

### Color Palette
- Aegean Blue: `#1F3A5F`
- Bronze Gold: `#B08D57`
- Marble White: `#F5F5F2`
- Obsidian Black: `#0F0F12`

## Platform-Specific Export Guidelines

### Windows Installer (NSIS/MSI)

Windows installers require `.ico` format with multiple embedded sizes:

```bash
# Generate Windows ICO from SVG
convert -background none -density 300 parthenon-installer-win.svg \
  \( -clone 0 -resize 16x16 \) \
  \( -clone 0 -resize 32x32 \) \
  \( -clone 0 -resize 48x48 \) \
  \( -clone 0 -resize 64x64 \) \
  \( -clone 0 -resize 128x128 \) \
  \( -clone 0 -resize 256x256 \) \
  -delete 0 parthenon-installer.ico
```

Place the generated `.ico` file in `/installers/windows/nsis/` for use by the NSIS script.

### macOS Installer (DMG)

macOS DMG background and icon:

```bash
# Generate high-resolution PNG for DMG
convert -background none -density 300 parthenon-installer-mac.svg \
  -resize 512x512 parthenon-installer.png

# Or generate ICNS for the DMG volume icon
mkdir installer.iconset
for size in 16 32 64 128 256 512; do
  convert -background none -density 300 parthenon-installer-mac.svg \
    -resize ${size}x${size} installer.iconset/icon_${size}x${size}.png
  convert -background none -density 300 parthenon-installer-mac.svg \
    -resize $((size*2))x$((size*2)) installer.iconset/icon_${size}x${size}@2x.png
done
iconutil -c icns installer.iconset -o parthenon-installer.icns
rm -rf installer.iconset
```

Place generated files in `/installers/macos/dmg/`.

### Linux Packages (.deb, .rpm, AppImage)

Linux packages can use SVG directly or PNG exports:

```bash
# Generate PNG for package managers
convert -background none -density 300 parthenon-installer-linux.svg \
  -resize 256x256 parthenon-installer.png

# Generate multiple sizes for desktop integration
for size in 16 32 48 64 128 256; do
  convert -background none -density 300 parthenon-installer-linux.svg \
    -resize ${size}x${size} parthenon-${size}.png
done
```

Place generated files in `/installers/linux/deb/` or `/installers/linux/rpm/` as appropriate.

## Usage in Installers

### NSIS (Windows)
Reference the icon in the NSIS script (`parthenon.nsi`):
```nsis
!define MUI_ICON "parthenon-installer.ico"
!define MUI_UNICON "parthenon-installer.ico"
```

### DMG (macOS)
Set the DMG background and volume icon in the build script.

### DEB/RPM (Linux)
Include in package metadata:
- `.deb`: Specified in `control` file and desktop entry
- `.rpm`: Specified in `.spec` file
- Desktop entry: Icon referenced in `.desktop` file

## Naming Conventions

Filenames are deterministic and platform-specific:
- `parthenon-installer-win.svg` → Windows
- `parthenon-installer-mac.svg` → macOS  
- `parthenon-installer-linux.svg` → Linux

No ambiguous names. One icon per platform.

## Quality Standards

All icons are production-ready:
- ✅ No placeholders
- ✅ No mockups
- ✅ No TODOs
- ✅ Suitable for immediate release

## License

These icons are part of the ParthenonChain project and follow the repository license (see LICENSE file).

### Usage
- ✅ Use in official ParthenonChain installers
- ✅ Use in documentation and release notes
- ❌ Do not use in competing projects
- ❌ Do not modify without maintaining design principles

## Contributing

When modifying installer icons:
1. Keep SVG as canonical source
2. Follow design constraints (geometry, colors)
3. Test exports on target platforms
4. Update installer build scripts if needed
5. Verify icons display correctly in installers

## Contact

For questions or issues:
- GitHub: https://github.com/Tsoympet/PantheonChain/issues
- Label: `installers` or `assets`
