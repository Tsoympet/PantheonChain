# ParthenonChain Mobile Wallet Screenshots

This directory contains screenshots of the ParthenonChain Mobile wallet application for iOS and Android.

## Screenshot List

The following screenshots should be captured to fully document the mobile wallet interface:

### 1. Splash Screen (`splash.png`)
**Initial loading screen showing:**
- ParthenonChain logo
- App name
- Version number
- Loading indicator

### 2. Home/Dashboard Screen (`home.png` or `home-ios.png` / `home-android.png`)
**Main wallet screen showing:**
- Navigation bar with app title
- Asset selector tabs or dropdown (TALN, DRM, OBL)
- Large balance display for selected asset
- All balances summary:
  - TALANTON (TALN): balance
  - DRACHMA (DRM): balance
  - OBOLOS (OBL): balance
- Quick action buttons:
  - Send
  - Receive
  - Mining
- Recent transactions preview (last 3-5 transactions)
- Network status indicator
- Bottom navigation menu (Home, Send, Receive, Transactions, Settings)

**Recommended state for screenshot:**
- Show with sample balances
- Connection status: Connected
- A few recent transactions visible

### 3. Send Screen (`send.png` or `send-ios.png` / `send-android.png`)
**Transaction sending interface showing:**
- "Send" screen title with back button
- Asset selector
- Recipient address field with QR scan button
- Amount input field with number pad or keyboard
- "MAX" button to send full balance
- Network fee display
- Total amount display (amount + fee)
- "Send" button (primary action button)
- Transaction confirmation dialog (optional, separate screenshot)

**Recommended state for screenshot:**
- Show with sample address and amount filled in
- QR scan button visible
- Fee calculation displayed

### 4. Receive Screen (`receive.png` or `receive-ios.png` / `receive-android.png`)
**Address receiving interface showing:**
- "Receive" screen title with back button
- Asset selector (TALN, DRM, OBL)
- Large QR code of receive address
- Receive address text (monospace, copyable)
- "Copy Address" button
- "Share" button
- "Generate New Address" button
- Address type indicator (if applicable)

**Recommended state for screenshot:**
- Show with QR code displayed
- Clear, centered QR code
- Address clearly visible below

### 5. Transaction History Screen (`transactions.png` or `transactions-ios.png` / `transactions-android.png`)
**Transaction list showing:**
- "Transactions" screen title
- Asset filter tabs or dropdown
- Search/filter options
- Transaction list items showing:
  - Date/time
  - Transaction type (sent/received) with icon
  - Amount with +/- indicator
  - Asset symbol
  - Address (truncated)
  - Confirmation status
- Pull to refresh indicator (optional)
- Infinite scroll or pagination

**Recommended state for screenshot:**
- Show 5-10 sample transactions
- Mix of sent and received transactions
- Different confirmation states
- Various assets represented

### 6. Transaction Detail Screen (`transaction-detail.png`)
**Individual transaction view showing:**
- Transaction type (Send/Receive)
- Amount and asset
- Full sender address
- Full recipient address
- Transaction ID (hash)
- Confirmations count
- Block height
- Timestamp
- Fee amount
- "Copy Transaction ID" button
- "View in Explorer" button (if applicable)

### 7. Mining Screen (`mining.png` or `mining-ios.png` / `mining-android.png`)
**Mobile mining interface showing:**
- "Mining" screen title
- Mining status (Active/Inactive)
- Start/Stop mining toggle
- Hashrate display (H/s)
- Shares submitted counter
- Estimated earnings
- Mining pool information
- CPU usage slider or indicator
- Temperature warning (if high)
- Battery status indicator

**Recommended state for screenshot:**
- Show both mining active and inactive states if possible
- Sample hashrate displayed when active
- Mining statistics visible

### 8. Settings Screen (`settings.png` or `settings-ios.png` / `settings-android.png`)
**Settings interface showing:**
- "Settings" screen title
- Settings sections:
  - **Wallet**
    - Backup wallet
    - Restore wallet
    - Change password
    - Security settings
  - **Network**
    - Network selection (Mainnet/Testnet)
    - RPC server settings
    - Connection status
  - **Mining**
    - Mining pool settings
    - CPU usage limit
    - Auto-mine settings
  - **Appearance**
    - Theme (Light/Dark)
    - Language
  - **About**
    - App version
    - Terms of Service
    - Privacy Policy

### 9. Backup/Restore Screen (`backup.png`)
**Wallet backup interface showing:**
- Backup seed phrase (12/24 words, blurred in screenshot)
- Warning message about securing seed phrase
- "I have written it down" checkbox
- "Copy to Clipboard" button
- "Continue" button

**IMPORTANT**: For security demonstration, blur or use placeholder words

### 10. QR Scanner Screen (`qr-scanner.png`)
**QR code scanning interface showing:**
- Camera viewfinder
- QR code detection overlay
- Instructions: "Scan QR code"
- Cancel button
- Manual entry button
- Permission request message (if applicable)

## Platform-Specific Screenshots

Capture screenshots for both iOS and Android to show platform differences:

### iOS Screenshots
- Use iPhone device frames (iPhone 12/13/14 recommended)
- Show native iOS UI elements
- Light and dark mode variants
- Naming: `*-ios.png`

### Android Screenshots
- Use modern Android device (Pixel or Samsung)
- Show Material Design UI elements  
- Light and dark mode variants
- Naming: `*-android.png`

## How to Capture Screenshots

### iOS (Physical Device)

1. Build and install the app:
```bash
cd clients/mobile/react-native
npx react-native run-ios --device
```

2. Take screenshot on device:
- Press `Side Button + Volume Up` simultaneously
- Screenshots save to Photos app

3. Transfer to computer via AirDrop or USB

### iOS (Simulator)

1. Run in simulator:
```bash
cd clients/mobile/react-native
npx react-native run-ios
```

2. Take screenshot:
- `Cmd + S` in simulator
- Or: Device menu → Trigger Screenshot

### Android (Physical Device)

1. Build and install the app:
```bash
cd clients/mobile/react-native
npx react-native run-android
```

2. Take screenshot on device:
- Press `Power + Volume Down` simultaneously
- Screenshots save to Pictures/Screenshots

3. Transfer via USB or ADB:
```bash
adb pull /sdcard/Pictures/Screenshots/ .
```

### Android (Emulator)

1. Run in emulator:
```bash
cd clients/mobile/react-native
npx react-native run-android
```

2. Take screenshot:
- Click camera icon in emulator toolbar
- Or use: `adb shell screencap -p /sdcard/screenshot.png`

## Screenshot Guidelines

- **Device**: Use standard device sizes (iPhone 14, Pixel 6, etc.)
- **Resolution**: Native device resolution
- **Format**: PNG format for best quality
- **Orientation**: Portrait mode (primary), landscape for specific screens if applicable
- **File naming**: Use descriptive names with platform suffix
- **Size**: Optimize for web (< 500KB per screenshot)
- **Privacy**: Use test data only, no real addresses or amounts
- **Consistency**: Same device, same theme throughout
- **Quality**: Clear, high-resolution images
- **Status Bar**: Show realistic status bar (time around 9:41, full battery, signal)

## Required Screenshots

### Minimum Set (Both Platforms)
1. ✅ `home-ios.png` / `home-android.png` - Main dashboard
2. ✅ `send-ios.png` / `send-android.png` - Send transaction
3. ✅ `receive-ios.png` / `receive-android.png` - Receive address with QR
4. ✅ `transactions-ios.png` / `transactions-android.png` - Transaction history
5. ✅ `mining-ios.png` / `mining-android.png` - Mining interface

### Optional but Recommended
6. ⬜ `transaction-detail.png` - Individual transaction
7. ⬜ `settings.png` - Settings screen
8. ⬜ `qr-scanner.png` - QR scanner
9. ⬜ `backup.png` - Backup screen
10. ⬜ `splash.png` - Splash screen

### Theme Variants (Optional)
- `home-ios-dark.png` - Dark mode variant
- `home-android-dark.png` - Dark mode variant
- (Apply to all main screens)

## Using Screenshots in Documentation

Reference screenshots in documentation:

```markdown
### iOS
![Home Screen](screenshots/home-ios.png)

### Android
![Home Screen](screenshots/home-android.png)
```

Or side-by-side:
```markdown
| iOS | Android |
|-----|---------|
| ![iOS](screenshots/home-ios.png) | ![Android](screenshots/home-android.png) |
```

## Screenshot Frames (Optional)

For marketing and documentation, you can add device frames using tools like:
- [Facebook Design Devices](https://facebook.design/devices)
- [MockUPhone](https://mockuphone.com/)
- [Smartmockups](https://smartmockups.com/)

## App Store Screenshots

For app store submissions, you'll need specific sizes:

### iOS App Store
- 6.7" (iPhone 14 Pro Max): 1290 x 2796
- 6.5" (iPhone 11 Pro Max): 1242 x 2688
- 5.5" (iPhone 8 Plus): 1242 x 2208

### Google Play Store
- Phone: 1080 x 1920 or higher
- Tablet: 1080 x 1920 or higher
- Feature graphic: 1024 x 500

## Current Status

🟡 **Screenshots Pending** - This directory structure is ready, but actual screenshots need to be captured by running the mobile app on a device or simulator with a display environment.

To contribute screenshots:
1. Build and run the mobile app on iOS/Android
2. Set up test wallet with sample data
3. Capture screenshots following the guidelines above
4. Optimize images for web (compress if needed)
5. Place them in this directory with proper naming
6. Update this README to mark screenshots as complete (✅)
7. Submit a pull request

---

**Note**: Use test mode or regtest network with sample data for screenshots. Never include real wallet addresses, private keys, or transaction data in public screenshots.

## Testing Data for Screenshots

To generate consistent screenshots, use these sample values:

**Balances:**
- TALN: 1,234.56789012
- DRM: 5,678.90123456
- OBL: 123.45678901

**Sample Addresses (testnet):**
- Receive: `tpn1qxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`
- Send to: `tpn1qyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy`

**Sample Transactions:**
1. Received 100.00000000 TALN (Confirmed)
2. Sent 50.00000000 DRM (Confirmed)
3. Received 25.00000000 OBL (Pending - 2 confirmations)
4. Sent 10.00000000 TALN (Confirmed)

**Mining Stats:**
- Hashrate: 1.2 MH/s
- Shares: 15
- Uptime: 2h 34m
