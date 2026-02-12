# Desktop App Assets

This directory contains the visual assets for the ParthenonChain desktop wallet application.

## Icon Files

Platform-specific icon formats:
- **icon.png** (512x512) - Standard PNG icon
- **icon.ico** (Windows icon, multiple sizes: 16x16, 32x32, 48x48, 256x256)
- **icon.icns** (macOS icon bundle)

## UI Icons

Navigation and action icons used in the desktop interface:
- **icon-home.svg**
- **icon-send.svg**
- **icon-receive.svg**
- **icon-transactions.svg**
- **icon-mining.svg**
- **icon-wallet.svg**
- **icon-settings.svg**

## Generation

These icon files are generated from the master SVG in `/assets/icon.svg`:

### Windows ICO
```bash
# Using ImageMagick
convert -background none -density 300 ../../assets/icon.svg \
  \( -clone 0 -resize 16x16 \) \
  \( -clone 0 -resize 32x32 \) \
  \( -clone 0 -resize 48x48 \) \
  \( -clone 0 -resize 256x256 \) \
  -delete 0 icon.ico
```

### macOS ICNS
```bash
# Create iconset directory
mkdir icon.iconset
# Generate all required sizes
for size in 16 32 64 128 256 512 1024; do
  convert -background none -density 300 ../../assets/icon.svg -resize ${size}x${size} icon.iconset/icon_${size}x${size}.png
  convert -background none -density 300 ../../assets/icon.svg -resize $((size*2))x$((size*2)) icon.iconset/icon_${size}x${size}@2x.png
done
# Create ICNS
iconutil -c icns icon.iconset -o icon.icns
rm -rf icon.iconset
```

### PNG
```bash
convert -background none -density 300 ../../assets/icon.svg -resize 512x512 icon.png
```

## Usage

These icons are used by:
- Windows installer (NSIS)
- macOS DMG and app bundle
- Linux desktop entries
- Qt desktop application
