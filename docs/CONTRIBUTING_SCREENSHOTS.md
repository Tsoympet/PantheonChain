# Contributing Screenshots to ParthenonChain

Thank you for your interest in contributing screenshots to the ParthenonChain project! This guide will help you capture, prepare, and submit high-quality screenshots of the ParthenonChain client applications.

## What We Need

We need screenshots for:

1. **Desktop GUI Wallet** - Qt-based application (Windows, macOS, Linux)
2. **Mobile Wallet** - React Native app (iOS and Android)

## Prerequisites

### For Desktop Screenshots

- A computer with display (Windows, macOS, or Linux)
- Ability to build and run the desktop wallet OR access to pre-built binaries
- Screenshot capture tool (built into most OS)
- Image editing software (optional, for optimization)

### For Mobile Screenshots

- An iOS device/simulator OR Android device/emulator
- Development environment set up for React Native
- Ability to build and run the mobile app
- USB cable for physical devices (optional)

## Quick Start

1. **Set Up Test Environment**
   - Build the application (or use pre-built binary)
   - Create test wallet with sample data
   - Connect to testnet or regtest (not mainnet)

2. **Capture Screenshots**
   - Follow platform-specific instructions below
   - Capture all required screenshots
   - Use consistent settings across screenshots

3. **Prepare Files**
   - Optimize images for web
   - Name files correctly
   - Organize in proper directories

4. **Submit**
   - Fork the repository
   - Add screenshots to appropriate directories
   - Create pull request

## Detailed Instructions

### Desktop Wallet Screenshots

#### Step 1: Build the Desktop Wallet

**Option A: Build from source**

```bash
# Clone repository
git clone --recursive https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Install Qt development libraries
# Ubuntu/Debian:
sudo apt-get install qt5-default qtbase5-dev
# macOS:
brew install qt@5
# Windows: Install Qt from qt.io

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target parthenon-qt -j$(nproc)

# Run
./clients/desktop/parthenon-qt
```

**Option B: Use pre-built binary**
- Download from [Releases page](https://github.com/Tsoympet/PantheonChain/releases)
- Install and run

#### Step 2: Set Up Test Wallet

```bash
# Start in testnet mode (recommended)
./parthenon-qt --testnet

# Or use regtest for complete control
./parthenon-qt --regtest
```

**Create sample data:**
- Generate receive addresses
- If possible, create some test transactions
- Mine some blocks (in regtest mode)
- Populate with sample balances:
  - TALN: 1,234.56789012
  - DRM: 5,678.90123456
  - OBL: 123.45678901

#### Step 3: Capture Screenshots

**Required screenshots:**

1. **Overview Page** (`overview.png`)
   - Navigate to Overview page
   - Ensure balances are visible
   - Window size: 1000x700 (default)
   - Capture window only (not full screen)

2. **Send Page** (`send.png`)
   - Navigate to Send page
   - Fill in sample address and amount
   - Don't actually send (just for screenshot)

3. **Receive Page** (`receive.png`)
   - Navigate to Receive page
   - Ensure QR code is visible
   - Address should be displayed

4. **Transactions Page** (`transactions.png`)
   - Navigate to Transactions page
   - Ensure multiple transactions are visible
   - Show different transaction types

**Platform-specific capture:**

**Linux:**
```bash
# GNOME Screenshot
gnome-screenshot -w  # Window only
gnome-screenshot -d 3 -w  # 3 second delay

# Or Spectacle (KDE)
spectacle -a  # Active window

# Or scrot
scrot -u  # Current window
scrot -d 3 -u  # 3 second delay
```

**macOS:**
```bash
# Built-in screenshot
# Press: Cmd + Shift + 4, then Space, then click window

# Or from Terminal
screencapture -w -o screenshot.png  # Window only
```

**Windows:**
```bash
# Built-in
# Press: Alt + PrtScn (captures active window)
# Then paste into image editor and save

# Or use Snipping Tool
# Windows key, type "snipping", select Window Snip
```

#### Step 4: Save and Optimize

Save screenshots to:
```
PantheonChain/clients/desktop/screenshots/
```

File names:
- `overview.png`
- `send.png`
- `receive.png`
- `transactions.png`
- `menu.png` (optional)
- `settings.png` (optional)
- `about.png` (optional)

Optimize file size:
```bash
# Using optipng
optipng -o7 overview.png

# Using ImageMagick
convert overview.png -quality 95 overview-optimized.png

# Using pngquant (lossy but smaller)
pngquant --quality=80-95 overview.png -o overview-optimized.png
```

Target: < 500KB per file

---

### Mobile Wallet Screenshots

#### Step 1: Set Up Development Environment

**iOS:**
```bash
cd PantheonChain/clients/mobile/react-native

# Install dependencies
npm install

# Install iOS pods
cd ios && pod install && cd ..
```

**Android:**
```bash
cd PantheonChain/clients/mobile/react-native

# Install dependencies
npm install
```

#### Step 2: Run the App

**iOS Simulator:**
```bash
npx react-native run-ios
```

**iOS Device:**
```bash
npx react-native run-ios --device
```

**Android Emulator:**
```bash
npx react-native run-android
```

**Android Device:**
```bash
# Enable USB debugging on device
# Connect via USB
npx react-native run-android
```

#### Step 3: Set Up Test Data

In the app:
- Create test wallet
- Add sample balances
- Generate receive addresses
- Create a few test transactions (if possible)

Use these sample values:
- TALN: 1,234.56789012
- DRM: 5,678.90123456
- OBL: 123.45678901

#### Step 4: Capture Screenshots

**Required screenshots (both iOS and Android):**

1. **Home Screen** (`home-ios.png` / `home-android.png`)
2. **Send Screen** (`send-ios.png` / `send-android.png`)
3. **Receive Screen** (`receive-ios.png` / `receive-android.png`)
4. **Transactions Screen** (`transactions-ios.png` / `transactions-android.png`)
5. **Mining Screen** (`mining-ios.png` / `mining-android.png`)

**iOS Simulator:**
```bash
# While simulator is running
# Press: Cmd + S

# Screenshots save to Desktop
# Rename and move to screenshots directory
```

**iOS Device:**
```bash
# On device: Press Side Button + Volume Up
# Screenshots save to Photos app
# Transfer via AirDrop or connect via USB
```

**Android Emulator:**
```bash
# Click camera icon in emulator toolbar
# Or press: Cmd + S (Mac) / Ctrl + S (Windows)

# Or via adb
adb shell screencap -p /sdcard/screenshot.png
adb pull /sdcard/screenshot.png
```

**Android Device:**
```bash
# On device: Press Power + Volume Down
# Screenshots save to Pictures/Screenshots/

# Transfer via USB
adb pull /sdcard/Pictures/Screenshots/
```

#### Step 5: Save and Organize

Save screenshots to:
```
PantheonChain/clients/mobile/screenshots/
```

File naming convention:
- iOS: `<screen>-ios.png` (e.g., `home-ios.png`)
- Android: `<screen>-android.png` (e.g., `home-android.png`)

Optimize for web:
```bash
# Target file size: < 500KB per screenshot
# Use ImageMagick, optipng, or pngquant as shown above
```

---

## Screenshot Checklist

Before submitting, ensure:

- [ ] Screenshots are in PNG format
- [ ] File sizes are < 500KB each
- [ ] Files are named correctly
- [ ] No real addresses, private keys, or sensitive data visible
- [ ] Screenshots show test/sample data only
- [ ] Images are clear and properly focused
- [ ] Consistent theme across related screenshots
- [ ] Status bar shows appropriate time (9:41) and full battery (mobile)
- [ ] Screenshots are placed in correct directories
- [ ] Platform suffix added for mobile (`-ios`, `-android`)

## Submitting Your Screenshots

### Via GitHub Pull Request (Recommended)

1. **Fork the repository**
   ```bash
   # Click "Fork" button on GitHub
   ```

2. **Clone your fork**
   ```bash
   git clone https://github.com/YOUR-USERNAME/PantheonChain.git
   cd PantheonChain
   ```

3. **Create a new branch**
   ```bash
   git checkout -b add-screenshots
   ```

4. **Add your screenshots**
   ```bash
   # Copy screenshots to appropriate directory
   cp ~/screenshots/*.png clients/desktop/screenshots/
   # or
   cp ~/screenshots/*-ios.png clients/mobile/screenshots/
   cp ~/screenshots/*-android.png clients/mobile/screenshots/
   ```

5. **Commit your changes**
   ```bash
   git add clients/desktop/screenshots/*.png
   # or
   git add clients/mobile/screenshots/*.png
   
   git commit -m "Add desktop wallet screenshots" 
   # or
   git commit -m "Add mobile wallet screenshots (iOS and Android)"
   ```

6. **Push to your fork**
   ```bash
   git push origin add-screenshots
   ```

7. **Create Pull Request**
   - Go to your fork on GitHub
   - Click "Pull Request" button
   - Provide description of screenshots added
   - Submit PR

### PR Description Template

```markdown
## Description
Added [desktop/mobile] wallet screenshots for v1.0.0

## Screenshots Added
- [x] Overview/Home page
- [x] Send transaction page
- [x] Receive address page
- [x] Transaction history page
- [x] [Additional screenshots]

## Platform
- [ ] Desktop (Windows/macOS/Linux)
- [ ] Mobile iOS
- [ ] Mobile Android

## Checklist
- [x] Screenshots are PNG format
- [x] File sizes < 500KB each
- [x] Correct file names used
- [x] Only test data visible
- [x] Images optimized for web
- [x] Screenshots are clear and focused

## Notes
[Any additional notes about the screenshots]
```

## Tips for Great Screenshots

### Content

1. **Use realistic but fake data**
   - Don't use real wallet addresses or amounts
   - Use the suggested sample values
   - Create diverse transaction history

2. **Show the app in use**
   - Fill in forms with example data
   - Show features being used, not just empty screens
   - Demonstrate key functionality

3. **Maintain consistency**
   - Same theme/colors across all screenshots
   - Same window size for desktop
   - Same device/orientation for mobile
   - Consistent time in status bar (9:41)

### Quality

1. **Technical quality**
   - Native resolution (no upscaling or downscaling)
   - Sharp, clear images
   - Good contrast
   - No compression artifacts

2. **Composition**
   - Center important elements
   - Include relevant UI elements
   - Exclude unnecessary desktop/system elements
   - Consistent framing

3. **File management**
   - Descriptive file names
   - Optimized file sizes
   - Proper directory structure
   - Correct platform suffix for mobile

### Privacy and Security

1. **Never include:**
   - Real wallet addresses
   - Real private keys or seed phrases
   - Real transaction data
   - Personal information
   - Real usernames or emails

2. **Always use:**
   - Test network (testnet/regtest)
   - Sample/placeholder data
   - Generic device names
   - Blurred sensitive info if accidentally captured

## Common Issues and Solutions

### Issue: Application won't build

**Solution:**
- Ensure all dependencies are installed
- Check Qt version (5.15+ or 6.x)
- For mobile, ensure React Native environment is set up correctly
- Consult [README.md](../README.md) and [clients/README.md](../clients/README.md)

### Issue: Can't generate test data

**Solution:**
- Use regtest mode for full control
- Manually edit wallet data (for screenshots only, not for production!)
- Use provided sample values

### Issue: Screenshots are too large

**Solution:**
- Use PNG optimization tools: optipng, pngquant
- Compress with ImageMagick
- Target < 500KB per file
- Consider reducing dimensions slightly if needed

### Issue: App looks different on my system

**Solution:**
- This is normal due to platform differences
- Capture your platform-specific screenshots
- Maintain consistency within your platform
- Note platform in PR description

## Questions?

If you have questions about contributing screenshots:

- Check [docs/SCREENSHOTS.md](../docs/SCREENSHOTS.md)
- Check platform-specific README:
  - [clients/desktop/screenshots/README.md](../clients/desktop/screenshots/README.md)
  - [clients/mobile/screenshots/README.md](../clients/mobile/screenshots/README.md)
- Open an issue on GitHub
- Ask in GitHub Discussions

## Thank You!

Your contribution helps make ParthenonChain better documented and more accessible to users. We appreciate your time and effort in capturing and submitting these screenshots!

---

**Ready to start?** Pick a platform (desktop or mobile), follow the steps above, and submit your screenshots via pull request. We look forward to your contribution!
