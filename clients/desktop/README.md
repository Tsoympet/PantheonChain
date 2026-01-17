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

📸 **See [screenshots/README.md](screenshots/README.md) for detailed screenshot documentation and capture guidelines.**

The wallet provides:
- **Overview page** - Multi-asset balance display with quick action buttons
- **Send page** - Transaction sending with amount validation and MAX button
- **Receive page** - Address generation with QR codes
- **Transaction history** - Filterable transaction list with detailed information

### Main Interface

The desktop wallet features a modern Qt-based interface with:

1. **Main Window**
   - Menu bar (File, Edit, View, Tools, Help)
   - Navigation toolbar
   - Status bar with connection status and block height
   - Window size: 1000x700 pixels

2. **Overview Page**
   - Asset selector dropdown (TALN, DRM, OBL)
   - Large balance display for selected asset
   - Summary of all asset balances
   - Quick Send and Receive buttons

3. **Send Page**
   - Asset selection
   - Recipient address input
   - Amount field with MAX button
   - Fee calculation and display
   - Send confirmation

4. **Receive Page**
   - Current receive address display
   - QR code generation
   - Copy address button
   - New address generation

5. **Transactions Page**
   - Comprehensive transaction history
   - Asset filtering
   - Transaction details (date, type, address, amount, confirmations)
   - Sortable columns

For actual screenshots and detailed UI documentation, see the [screenshots directory](screenshots/).
