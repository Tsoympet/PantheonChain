# ParthenonChain Assets

This directory contains the official branding and visual assets for ParthenonChain.

## Contents

### Application Icons

All icons are provided in SVG (Scalable Vector Graphics) format for maximum flexibility and quality at any size.

- **logo.svg** - Full ParthenonChain logo with Parthenon architecture and blockchain elements
- **icon.svg** - App icon optimized for desktop and mobile applications (1024x1024 viewBox)
- **favicon.svg** - Simplified favicon for web use (32x32 viewBox)

### Token Icons

Individual SVG icons for the three native tokens (256x256 viewBox):

- **token-talanton.svg** - TALANTON (TAL) token icon
  - **Color:** Gold (`#fbbf24` to `#d97706`)
  - **Design:** Premium coin with Greek styling, letter "T", laurel wreath
  - **Purpose:** Store of value, mining rewards (21M max supply)
  
- **token-drachma.svg** - DRACHMA (DRA) token icon
  - **Color:** Silver (`#e5e7eb` to `#6b7280`)
  - **Design:** Trade coin with exchange arrows, letter "D"
  - **Purpose:** Medium of exchange (41M max supply)
  
- **token-obolos.svg** - OBOLOS (OBL) token icon
  - **Color:** Bronze/Copper (`#f97316` to `#c2410c`)
  - **Design:** Energy coin with circuit pattern and flame, letter "O"
  - **Purpose:** Smart contract gas (61M max supply)

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

**Token Icons (PNG):**
```bash
# Generate token icons at common sizes
convert -background none -density 300 token-talanton.svg -resize 256x256 token-talanton-256.png
convert -background none -density 300 token-drachma.svg -resize 256x256 token-drachma-256.png
convert -background none -density 300 token-obolos.svg -resize 256x256 token-obolos-256.png

# Smaller sizes for UI elements
convert -background none -density 300 token-talanton.svg -resize 64x64 token-talanton-64.png
convert -background none -density 300 token-drachma.svg -resize 64x64 token-drachma-64.png
convert -background none -density 300 token-obolos.svg -resize 64x64 token-obolos-64.png
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
