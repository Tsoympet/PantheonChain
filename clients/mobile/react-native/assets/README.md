# Mobile App Assets

This directory contains the visual assets for the ParthenonChain mobile wallet application.

## Icon Files

- **icon.png** (1024x1024) - Main app icon
- **splash.png** (2048x2048) - Splash screen image
- **adaptive-icon.png** (1024x1024) - Android adaptive icon foreground
- **favicon.png** (32x32) - Web favicon

## Generation

These PNG files are generated from the master SVG files in `/assets`:
- Source: `/assets/icon.svg`
- Source: `/assets/logo.svg`
- Source: `/assets/favicon.svg`

To regenerate PNG files from SVG:
```bash
# Using ImageMagick or similar tool
convert -background none -density 300 ../../../assets/icon.svg -resize 1024x1024 icon.png
convert -background none -density 300 ../../../assets/logo.svg -resize 2048x2048 splash.png
convert -background none -density 300 ../../../assets/icon.svg -resize 1024x1024 adaptive-icon.png
convert -background none -density 300 ../../../assets/favicon.svg -resize 32x32 favicon.png
```

## Usage

These assets are referenced in `app.json` and used by React Native for iOS and Android builds.
