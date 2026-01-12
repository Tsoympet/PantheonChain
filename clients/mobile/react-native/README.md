# ParthenonChain Mobile Wallet

React Native mobile wallet with integrated share mining.

## Features

- **Multi-Asset Wallet**
  - Support for TALN, DRM, OBL
  - Real-time balance tracking
  - Asset switching

- **Transaction Management**
  - Send transactions
  - Receive addresses
  - Transaction history

- **Share Mining**
  - Mobile CPU mining
  - Hashrate monitoring
  - Mining toggle

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
