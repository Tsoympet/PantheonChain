# ParthenonChain Assets

This directory contains the official branding and visual assets for ParthenonChain.

## Contents

### SVG Icons

All icons are provided in SVG (Scalable Vector Graphics) format for maximum flexibility and quality at any size.

- **logo.svg** - Full ParthenonChain logo with Parthenon architecture and blockchain elements
- **icon.svg** - App icon optimized for desktop and mobile applications (1024x1024 viewBox)
- **favicon.svg** - Simplified favicon for web use (32x32 viewBox)

## Design

The ParthenonChain visual identity draws inspiration from:
- **Ancient Greek architecture** - Specifically the Parthenon, representing strength, permanence, and classical foundations
- **Blockchain elements** - Linked blocks symbolizing the distributed ledger
- **Color scheme:**
  - Deep blue (`#1e3a8a` to `#3b82f6`) - Trust, security, technology
  - Gold/amber (`#fbbf24` to `#f59e0b`) - Value, prestige, classical antiquity

## Usage

### For Developers

Generate platform-specific icon formats from these SVG files:

**PNG (various sizes):**
```bash
# Using ImageMagick
convert -background none -density 300 icon.svg -resize 512x512 icon-512.png
convert -background none -density 300 icon.svg -resize 256x256 icon-256.png
convert -background none -density 300 icon.svg -resize 128x128 icon-128.png
```

**Windows ICO:**
```bash
convert -background none -density 300 icon.svg \
  \( -clone 0 -resize 16x16 \) \
  \( -clone 0 -resize 32x32 \) \
  \( -clone 0 -resize 48x48 \) \
  \( -clone 0 -resize 256x256 \) \
  -delete 0 parthenon.ico
```

**macOS ICNS:**
```bash
mkdir icon.iconset
for size in 16 32 128 256 512; do
  convert -background none -density 300 icon.svg -resize ${size}x${size} icon.iconset/icon_${size}x${size}.png
  convert -background none -density 300 icon.svg -resize $((size*2))x$((size*2)) icon.iconset/icon_${size}x${size}@2x.png
done
iconutil -c icns icon.iconset -o parthenon.icns
```

### Platform-Specific Assets

See client-specific directories for pre-generated assets:
- `clients/desktop/assets/` - Desktop application icons (ICO, ICNS, PNG)
- `clients/mobile/react-native/assets/` - Mobile app icons (PNG at various sizes)

## License

These visual assets are part of the ParthenonChain project and are subject to the same license terms.

## Trademark

"ParthenonChain" and associated logos are trademarks of ParthenonChain Foundation. Use of these marks must comply with trademark guidelines.

For questions or custom asset requests, please open an issue at:
https://github.com/Tsoympet/PantheonChain/issues
