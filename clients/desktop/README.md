# ParthenonChain Desktop Wallet

Qt-based desktop wallet for ParthenonChain.

## Status

✅ **Complete** - Full Qt5 implementation with all features

## Features

- **Multi-Asset Wallet**
  - TALN, DRM, OBL balance tracking
  - Real-time balance updates
  - Asset switching

- **Transaction Management**
  - Send transactions with confirmation
  - Receive addresses with QR codes
  - Transaction history with filtering
  - Memo/note support

- **RPC Integration**
  - JSON-RPC client for backend communication
  - Connection status monitoring
  - Block height synchronization

- **User Interface**
  - Modern Qt5 Widgets interface
  - Menu bar and toolbar navigation
  - Status bar with connection info
  - Overview, Send, Receive, and Transactions pages

## Building

Requires Qt 5.15+ (Qt 6.x also supported).

```bash
# Install Qt5 development libraries (Ubuntu/Debian)
sudo apt-get install qt5-default qtbase5-dev

# Or on Fedora/RHEL
sudo dnf install qt5-qtbase-devel

mkdir build
cd build
cmake ..
make parthenon-qt
```

## Running

```bash
./parthenon-qt
```

The wallet will automatically connect to the RPC server at `127.0.0.1:8332`.

## Dependencies

- Qt 5.15 or Qt 6.x
- QtWidgets module
- QtNetwork module

## Screenshots

The wallet provides:
- Overview page with multi-asset balance display
- Send page with amount validation and MAX button
- Receive page with address generation
- Transaction history with filtering options
