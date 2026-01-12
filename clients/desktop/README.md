# ParthenonChain Desktop Wallet

Qt-based desktop wallet for ParthenonChain.

## Status

This is a scaffold implementation. Production version will use Qt framework.

## Features (Planned)

- **Multi-Asset Wallet**
  - TALN, DRM, OBL balance tracking
  - Asset switching

- **Transaction Management**
  - Send transactions
  - Receive addresses
  - Transaction history

- **Settings**
  - Network configuration
  - RPC connection settings
  - Display preferences

## Building

Requires Qt 5.15+ for production version.

```bash
mkdir build
cd build
cmake ..
make parthenon-qt
```

## Running

```bash
./parthenon-qt
```

## Production Requirements

- Qt 5.15 or Qt 6.x
- QtWidgets
- QtNetwork
- Platform-specific Qt dependencies
