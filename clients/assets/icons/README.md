# ParthenonChain Client Icons

This directory contains the SVG icons used for ParthenonChain client applications.

## Purpose

All icons in this directory are the **canonical source** for client and application branding. They are used across desktop, CLI, and mobile platforms.

## Directory Structure

```
icons/
├── app/           # Application icons (desktop, mobile)
│   ├── parthenonchain.svg       # Main application icon
│   ├── parthenon-desktop.svg    # Desktop GUI application
│   └── parthenon-mobile.svg     # Mobile wallet
│
├── client/        # Client daemon/CLI icons
│   ├── parthenond.svg           # Full node daemon
│   └── parthenon-cli.svg        # Command-line interface
│
├── common/        # Common branding assets
│   ├── icon.svg                 # Main icon
│   ├── logo.svg                 # Main logo
│   └── favicon.svg              # Favicon
│
└── tokens/        # Token icons
    ├── token-drachma.svg        # Drachma token
    ├── token-obolos.svg         # Obolos token
    └── token-talanton.svg       # Talanton token
```

## Icon Specifications

### Format Requirements
- **Format**: SVG 1.1 / SVG 2.0 (vector-only)
- **No raster images**: PNG/JPG may be generated FROM SVG but SVG is canonical
- **Scalable**: All icons work from 16×16 to 1024×1024
- **Clean vectors**: No embedded rasters or external dependencies

### Design Constraints
- **Visual style**: Technical seriousness, cryptographic/protocol identity
- **Geometry**: Greek-inspired minimal geometry (no clipart, no mascots)
- **Colors**: Flat colors only (max 2 colors per icon, no gradients)
- **Symmetry**: Bilateral or radial symmetry throughout

### Color Palette
- Aegean Blue: `#1F3A5F` (trust, security, protocol)
- Bronze Gold: `#B08D57` (value, classical architecture)
- Marble White: `#F5F5F2`
- Obsidian Black: `#0F0F12`

## Export Guidelines

### PNG Export (when needed)
Use ImageMagick to generate PNG from SVG:

```bash
# Standard size (512×512)
convert -background none -density 300 icon.svg -resize 512x512 icon.png

# High-DPI (1024×1024)
convert -background none -density 300 icon.svg -resize 1024x1024 icon@2x.png

# Various sizes
for size in 16 32 48 64 128 256 512 1024; do
  convert -background none -density 300 icon.svg -resize ${size}x${size} icon-${size}.png
done
```

### Windows ICO (multi-size icon)
```bash
convert -background none -density 300 icon.svg \
  \( -clone 0 -resize 16x16 \) \
  \( -clone 0 -resize 32x32 \) \
  \( -clone 0 -resize 48x48 \) \
  \( -clone 0 -resize 256x256 \) \
  -delete 0 icon.ico
```

### macOS ICNS (icon set)
```bash
mkdir icon.iconset
for size in 16 32 64 128 256 512; do
  convert -background none -density 300 icon.svg -resize ${size}x${size} icon.iconset/icon_${size}x${size}.png
  convert -background none -density 300 icon.svg -resize $((size*2))x$((size*2)) icon.iconset/icon_${size}x${size}@2x.png
done
iconutil -c icns icon.iconset -o icon.icns
rm -rf icon.iconset
```

### Android Adaptive Icon
```bash
# Foreground layer (1024×1024)
convert -background none -density 300 icon.svg -resize 1024x1024 adaptive-icon.png
```

### iOS App Icon
```bash
# Multiple sizes required
for size in 20 29 40 58 60 76 80 87 120 152 167 180 1024; do
  convert -background none -density 300 icon.svg -resize ${size}x${size} AppIcon-${size}.png
done
```

## Platform-Specific Usage

### Desktop (Qt/Electron)
- **Windows**: Generate `.ico` from SVG
- **macOS**: Generate `.icns` from SVG
- **Linux**: Use SVG directly or generate PNG

### Mobile (React Native)
- **Android**: Generate `adaptive-icon.png` and various `mipmap` sizes
- **iOS**: Generate `AppIcon` set at required sizes

### CLI/Daemon
- Icons used for:
  - System tray applications
  - Documentation
  - Package manager listings
  - Desktop entries (Linux)

## Naming Conventions

All filenames follow deterministic naming:

**Application Icons:**
- `parthenonchain.svg` - Main protocol/network icon
- `parthenon-desktop.svg` - Desktop GUI
- `parthenon-mobile.svg` - Mobile wallet

**Client Icons:**
- `parthenond.svg` - Full node daemon
- `parthenon-cli.svg` - Command-line interface

**Common Branding:**
- `icon.svg` - Main icon
- `logo.svg` - Main logo
- `favicon.svg` - Favicon

**Token Icons:**
- `token-drachma.svg` - Drachma token
- `token-obolos.svg` - Obolos token
- `token-talanton.svg` - Talanton token

**No ambiguous names.** Each icon has a clear, singular purpose.

## License/Usage Rules

All icons are part of the ParthenonChain project and follow the same license as the repository (see LICENSE file in repository root).

### Usage Guidelines
- ✅ **Allowed**: Use in ParthenonChain official builds, documentation, marketing
- ✅ **Allowed**: Derivative works for ParthenonChain-compatible software (with attribution)
- ❌ **Not allowed**: Use in unrelated projects or competing blockchains
- ❌ **Not allowed**: Modification without maintaining design principles

## Quality Standards

All icons in this directory are **production-ready**:
- ✅ No mockups
- ✅ No temporary graphics  
- ✅ No TODOs
- ✅ Suitable for immediate release use

## Contributing

When adding or modifying icons:
1. Maintain SVG as canonical source
2. Follow Greek-inspired geometric design principles
3. Use only approved color palette
4. Ensure scalability from 16×16 to 1024×1024
5. Test exports on all target platforms
6. Update this README if adding new icon categories

## Contact

For icon design questions or issues:
- GitHub Issues: https://github.com/Tsoympet/PantheonChain/issues
- Label: `design` or `assets`
