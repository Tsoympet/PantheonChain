# ParthenonChain Mobile Wallet

React Native mobile wallet with integrated share mining.

## Features

- **Multi-Asset Wallet**
  - Support for TALN, DRM, OBL
  - Real-time balance tracking
  - Asset switching

- **Transaction Management**
  - Send transactions
  - Receive addresses with QR codes
  - Transaction history
  - QR code scanner for addresses

- **Share Mining**
  - Mobile CPU mining
  - Hashrate monitoring
  - Mining toggle
  - Pool integration

- **Security**
  - Biometric authentication (Touch ID / Face ID)
  - Secure key storage
  - Encrypted wallet backup

- **User Experience**
  - SPV lightweight mode (no full blockchain required)
  - Push notifications for transactions
  - Native iOS and Android UI
  - Dark mode support

## Screenshots

📸 **See [screenshots/README.md](screenshots/README.md) for detailed screenshot documentation and capture guidelines.**

The mobile wallet features a modern React Native interface optimized for both iOS and Android:

### Main Screens

1. **Home/Dashboard**
   - Asset selector
   - Large balance display
   - All balances summary
   - Quick action buttons (Send, Receive, Mining)
   - Recent transactions preview
   - Bottom navigation

2. **Send Screen**
   - Asset selection
   - Recipient address with QR scanner button
   - Amount input with MAX button
   - Fee calculation
   - Send confirmation

3. **Receive Screen**
   - Large QR code for receiving
   - Address display with copy button
   - Share functionality
   - New address generation

4. **Transaction History**
   - Filterable transaction list
   - Transaction details view
   - Pull to refresh
   - Confirmation status

5. **Mining Screen**
   - Start/Stop mining toggle
   - Hashrate display
   - Share statistics
   - CPU usage controls
   - Estimated earnings

6. **Settings**
   - Wallet backup/restore
   - Network selection
   - Mining configuration
   - Theme and language
   - Security settings

For actual screenshots of the mobile app on iOS and Android, see the [screenshots directory](screenshots/).

## Setup

### Prerequisites

- Node.js 16+
- React Native CLI
- Android Studio (for Android)
- Xcode (for iOS, macOS only)

### Installation

```bash
cd clients/mobile/react-native
npm install
```

### Running

#### Android

```bash
npm run android
```

#### iOS

```bash
cd ios && pod install && cd ..
npm run ios
```

### Development

```bash
npm start
```


### EAS Build Prerequisites (CI and local)

Before running EAS builds (especially in CI):

- Use **Expo SDK 41+** (this project is configured for SDK 49 via `expo` and `app.json`).
- Ensure `EXPO_TOKEN` is configured in your CI secrets.
- Do not rely on credential generation in `--non-interactive` mode.
  - Create Android and iOS credentials ahead of time with `npx eas credentials -p android` and `npx eas credentials -p ios`.
  - For iOS CI builds, make sure the **Distribution Certificate** and **Provisioning Profile** are uploaded in Expo and mapped to the build profile.
  - Upload credentials to Expo (remote) before CI builds, or use local credentials securely.

Install dependencies after Expo SDK updates:

```bash
npm install
```

## Building for Production

### Android

```bash
cd android
./gradlew assembleRelease
```

APK will be in `android/app/build/outputs/apk/release/`

### iOS

```bash
cd ios
xcodebuild -workspace ParthenonMobile.xcworkspace \
  -scheme ParthenonMobile \
  -configuration Release
```

## Mining Module

The mining module uses native code for performance:
- Android: C++ via JNI
- iOS: C++ via Objective-C++

Mining algorithm: SHA-256d (same as Layer 1)

## Architecture

```
src/
  App.js          - Main application
  screens/        - Screen components
  components/     - Reusable components
  services/       - RPC and mining services
  utils/          - Utilities
```

## License

MIT License - See LICENSE file
