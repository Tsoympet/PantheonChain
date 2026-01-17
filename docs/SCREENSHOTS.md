# ParthenonChain Client Screenshots Guide

This guide provides comprehensive information about capturing, organizing, and using screenshots of ParthenonChain client applications.

## Overview

ParthenonChain provides multiple client applications:
- **Desktop GUI Wallet** (Qt-based) - Windows, macOS, Linux
- **Mobile Wallet** (React Native) - iOS, Android
- **Command-Line Tools** - parthenond, parthenon-cli

This document focuses on capturing visual screenshots for the GUI clients (Desktop and Mobile).

## Screenshot Directories

Screenshots are organized by client type:

```
clients/
├── desktop/
│   └── screenshots/
│       ├── README.md           # Desktop screenshot guidelines
│       ├── overview.png        # Main dashboard
│       ├── send.png            # Send transaction page
│       ├── receive.png         # Receive address page
│       ├── transactions.png    # Transaction history
│       └── ...                 # Additional screenshots
│
└── mobile/
    └── screenshots/
        ├── README.md           # Mobile screenshot guidelines
        ├── home-ios.png        # iOS home screen
        ├── home-android.png    # Android home screen
        ├── send-ios.png        # iOS send screen
        ├── send-android.png    # Android send screen
        └── ...                 # Additional screenshots
```

## Quick Links

- **Desktop Screenshots Guide**: [clients/desktop/screenshots/README.md](../clients/desktop/screenshots/README.md)
- **Mobile Screenshots Guide**: [clients/mobile/screenshots/README.md](../clients/mobile/screenshots/README.md)

## Why Screenshots Matter

Screenshots are essential for:

1. **User Documentation** - Helping users understand the interface
2. **Feature Demonstration** - Showing capabilities to potential users
3. **Development Reference** - Visual reference for UI improvements
4. **App Store Listings** - Required for iOS App Store and Google Play
5. **Marketing Materials** - Promotional content and presentations
6. **Bug Reports** - Visual evidence of issues
7. **Tutorial Content** - Step-by-step guides with visuals

## Screenshot Requirements

### Desktop GUI Screenshots

**Required Screenshots** (Minimum):
1. ✅ Overview/Dashboard page
2. ✅ Send transaction page
3. ✅ Receive address page
4. ✅ Transaction history page

**Recommended Screenshots**:
5. ⬜ Menu and toolbar
6. ⬜ Settings dialog
7. ⬜ About dialog
8. ⬜ Mining control interface
9. ⬜ Smart contract interaction (if applicable)

**Technical Specifications**:
- Format: PNG (lossless)
- Resolution: 1920x1080 or actual window size (1000x700)
- Color depth: 24-bit or 32-bit
- File size: < 500KB per screenshot (optimize if needed)

### Mobile Wallet Screenshots

**Required Screenshots** (Both iOS and Android):
1. ✅ Home/Dashboard screen
2. ✅ Send transaction screen
3. ✅ Receive address screen (with QR code)
4. ✅ Transaction history screen
5. ✅ Mining interface screen

**Recommended Screenshots**:
6. ⬜ Transaction detail view
7. ⬜ Settings screen
8. ⬜ QR scanner interface
9. ⬜ Wallet backup screen
10. ⬜ Splash screen

**Technical Specifications**:
- Format: PNG
- iOS Resolution: 
  - iPhone 14 Pro Max: 1290 x 2796
  - iPhone 11 Pro Max: 1242 x 2688
- Android Resolution:
  - Pixel 6: 1080 x 2340
  - Standard: 1080 x 1920 or higher
- Orientation: Portrait (primary)
- File size: < 500KB per screenshot

## How to Capture Screenshots

### Desktop GUI

#### Linux
```bash
# Build and run desktop wallet
cd PantheonChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make parthenon-qt
./clients/desktop/parthenon-qt

# Capture screenshot (GNOME)
gnome-screenshot -w  # Window only
gnome-screenshot -a  # Area selection

# Or use scrot
scrot -u  # Current window
scrot -s  # Select area
```

#### macOS
```bash
# Build and run desktop wallet
cd PantheonChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make parthenon-qt
./clients/desktop/parthenon-qt

# Capture screenshot
# Press: Cmd + Shift + 4, then Space, then click window
# Or use Grab app from Utilities
```

#### Windows
```bash
# Build using Visual Studio or CMake
# Run: parthenon-qt.exe

# Capture screenshot
# Press: Alt + PrtScn (active window)
# Or use: Snipping Tool / Snip & Sketch
```

### Mobile Wallet

#### iOS Simulator
```bash
cd clients/mobile/react-native
npm install
npx react-native run-ios

# In simulator: Cmd + S to save screenshot
# Or: Device menu → Trigger Screenshot
```

#### iOS Device
```bash
# Build and deploy to device
npx react-native run-ios --device

# On device: Side Button + Volume Up
# Screenshots save to Photos app
# Transfer via AirDrop or USB
```

#### Android Emulator
```bash
cd clients/mobile/react-native
npm install
npx react-native run-android

# Click camera icon in emulator toolbar
# Or use adb:
adb shell screencap -p /sdcard/screenshot.png
adb pull /sdcard/screenshot.png
```

#### Android Device
```bash
# Build and deploy to device
npx react-native run-android

# On device: Power + Volume Down
# Screenshots save to Pictures/Screenshots
# Transfer via USB:
adb pull /sdcard/Pictures/Screenshots/
```

## Screenshot Best Practices

### Content

1. **Use Test Data**
   - Never include real wallet addresses
   - Never include real private keys or seed phrases
   - Use sample/placeholder data
   - Use testnet or regtest mode

2. **Sample Data for Screenshots**
   - Balances:
     - TALN: 1,234.56789012
     - DRM: 5,678.90123456
     - OBL: 123.45678901
   - Addresses: Use testnet addresses starting with `tpn1`
   - Transactions: Mix of sent/received with various confirmation states

3. **Privacy**
   - Blur sensitive information if accidentally captured
   - Don't show real usernames, emails, or personal data
   - Use generic computer/device names

### Quality

1. **Image Quality**
   - Use native resolution (no scaling)
   - Ensure clear, sharp images
   - Proper lighting (for device photos)
   - No reflections or glare

2. **Composition**
   - Center the content
   - Include relevant UI elements
   - Exclude unnecessary desktop/system UI when possible
   - Consistent window size across screenshots

3. **File Management**
   - Use descriptive filenames
   - Add platform suffix for mobile: `-ios`, `-android`
   - Optimize file size (PNG compression)
   - Keep organized in appropriate directories

### Consistency

1. **Visual Consistency**
   - Same theme across all screenshots (light or dark)
   - Same window size for desktop screenshots
   - Same device frame for mobile screenshots
   - Consistent time and status bar (9:41, full battery)

2. **State Consistency**
   - Connected to network (not offline)
   - Reasonable block height
   - Similar data across related screenshots

## Post-Processing

### Optimization

Reduce file size while maintaining quality:

```bash
# Using ImageMagick
convert input.png -quality 95 -define png:compression-level=9 output.png

# Using pngquant
pngquant --quality=80-95 input.png -o output.png

# Using optipng
optipng -o7 screenshot.png
```

### Adding Device Frames (Optional)

For marketing, you can add device frames:

1. **Tools**
   - [Facebook Design Devices](https://facebook.design/devices)
   - [MockUPhone](https://mockuphone.com/)
   - [Smartmockups](https://smartmockups.com/)

2. **When to Use Frames**
   - Marketing materials
   - Website galleries
   - App store screenshots
   - Documentation headers

3. **When NOT to Use Frames**
   - Technical documentation
   - Bug reports
   - Development references

## Using Screenshots in Documentation

### Markdown Examples

```markdown
# Single Screenshot
![Overview Page](clients/desktop/screenshots/overview.png)
*Desktop Wallet - Overview Page showing multi-asset balances*

# Side-by-Side Comparison
| iOS | Android |
|-----|---------|
| ![iOS](clients/mobile/screenshots/home-ios.png) | ![Android](clients/mobile/screenshots/home-android.png) |

# Inline Screenshot
See the [Send page](clients/desktop/screenshots/send.png) for transaction interface.
```

### HTML Examples

```html
<!-- Responsive Image -->
<img src="clients/desktop/screenshots/overview.png" 
     alt="Desktop Wallet Overview" 
     style="max-width: 100%; height: auto;">

<!-- Image with Caption -->
<figure>
  <img src="clients/mobile/screenshots/home-ios.png" alt="iOS Home Screen">
  <figcaption>ParthenonChain Mobile Wallet - iOS Home Screen</figcaption>
</figure>
```

## App Store Requirements

### iOS App Store

**Required Screenshots** (per device size):
- 6.7" Display (iPhone 14 Pro Max): 1290 x 2796
- 6.5" Display (iPhone 11 Pro Max): 1242 x 2688
- 5.5" Display (iPhone 8 Plus): 1242 x 2208

**Minimum**: 3-10 screenshots per device size

**Best Practices**:
- First screenshot is most important
- Show key features
- Use device frames
- Add text overlays highlighting features
- Show app in use, not just static UI

### Google Play Store

**Required Screenshots**:
- Minimum: 2 screenshots
- Maximum: 8 screenshots
- Phone: 1080 x 1920 minimum
- Tablet: 1080 x 1920 minimum (7-inch)
- Feature Graphic: 1024 x 500 (required)

**Best Practices**:
- Landscape and portrait orientations
- Show app running on device
- Highlight key features with text
- Use consistent branding

## Screenshot Checklist

Before committing screenshots:

- [ ] Screenshots are in PNG format
- [ ] File sizes are optimized (< 500KB each)
- [ ] Filenames are descriptive and follow naming convention
- [ ] No real/sensitive data is visible
- [ ] Images are clear and properly focused
- [ ] Consistent theme/style across related screenshots
- [ ] Screenshots are in correct directory (desktop/ or mobile/)
- [ ] Platform suffix added for mobile screenshots (-ios, -android)
- [ ] README.md updated if adding new screenshot types
- [ ] Screenshots reference test/sample data only

## Maintenance

Screenshots should be updated when:

1. **UI Changes** - Major interface redesigns
2. **New Features** - Adding new functionality
3. **Rebranding** - Logo or color scheme changes
4. **Platform Updates** - OS-level UI changes (iOS/Android versions)
5. **Bug Fixes** - Correcting visual issues shown in screenshots

## Contributing Screenshots

To contribute screenshots:

1. **Fork** the repository
2. **Build** and run the client application
3. **Set up** test environment with sample data
4. **Capture** screenshots following guidelines above
5. **Optimize** image files for size
6. **Place** in appropriate directory with proper naming
7. **Update** README if adding new screenshot types
8. **Test** that images display correctly in documentation
9. **Commit** with descriptive message
10. **Submit** pull request

Example commit message:
```
Add desktop wallet screenshots for v1.0.0

- Add overview page screenshot showing multi-asset balances
- Add send page screenshot with form filled
- Add receive page screenshot with QR code
- Add transaction history screenshot with sample data
- All screenshots use test data, optimized to < 300KB
```

## Resources

### Screenshot Tools

- **Linux**: GNOME Screenshot, Spectacle, Flameshot, scrot
- **macOS**: Built-in (Cmd+Shift+4), Grab, Skitch
- **Windows**: Snipping Tool, Snip & Sketch, Greenshot
- **iOS**: Simulator (Cmd+S), Device (Side Button + Volume Up)
- **Android**: Emulator (Camera icon), Device (Power + Volume Down)

### Image Optimization Tools

- **ImageMagick**: Command-line image processing
- **pngquant**: Lossy PNG compression
- **optipng**: Lossless PNG optimization
- **TinyPNG**: Web-based PNG/JPEG compression
- **Squoosh**: Web-based image optimizer

### Device Mockup Tools

- **Facebook Design**: Device frames and mockups
- **MockUPhone**: Free online device mockup generator
- **Smartmockups**: Premium mockup generator
- **Placeit**: Device mockups and templates

## Support

For questions about screenshots:

- **Desktop Screenshots**: See [clients/desktop/screenshots/README.md](../clients/desktop/screenshots/README.md)
- **Mobile Screenshots**: See [clients/mobile/screenshots/README.md](../clients/mobile/screenshots/README.md)
- **General Issues**: [GitHub Issues](https://github.com/Tsoympet/PantheonChain/issues)
- **Documentation**: [Main README](../README.md)

---

**Status**: Screenshot infrastructure is in place. Community contributions of actual screenshots are welcome! See individual screenshot directories for specific guidelines.
