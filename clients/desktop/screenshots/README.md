# ParthenonChain Desktop Wallet Screenshots

This directory contains screenshots of the ParthenonChain Desktop GUI wallet application.

## Screenshot List

The following screenshots should be captured to fully document the desktop wallet interface:

### 1. Overview Page (`overview.png`)
**Main wallet dashboard showing:**
- Window title: "ParthenonChain Wallet"
- Wallet Overview title
- Asset selector dropdown (TALN, DRM, OBL)
- Current balance display with large font showing selected asset balance
- All assets section displaying:
  - TALANTON (TALN): balance
  - DRACHMA (DRM): balance
  - OBOLOS (OBL): balance
- Quick action buttons: "Send" and "Receive"
- Menu bar: File, Edit, View, Tools, Help
- Toolbar with navigation icons
- Status bar showing connection status and block height

**Recommended state for screenshot:**
- Show with sample balances (e.g., TALN: 1234.56789012, DRM: 5000.00000000, OBL: 100.00000000)
- Connection status: "Connected to ParthenonChain network"
- Block height showing (e.g., "Block: 150000")

### 2. Send Page (`send.png`)
**Transaction sending interface showing:**
- "Send" page title
- Asset selector (TALN, DRM, OBL)
- Send To Address field (text input)
- Amount field with decimal validation
- "MAX" button to send entire balance
- Fee display showing network fee
- "Send" transaction button
- Transaction confirmation section

**Recommended state for screenshot:**
- Show with sample address entered
- Sample amount (e.g., 10.00000000 TALN)
- Fee display showing estimated fee
- All fields properly filled in to show complete workflow

### 3. Receive Page (`receive.png`)
**Address receiving interface showing:**
- "Receive" page title
- Asset selector (TALN, DRM, OBL)
- Current receive address display (large, monospace font)
- QR code of the address (if QR code generation is implemented)
- "Copy Address" button
- "Generate New Address" button
- Address history table (if implemented)

**Recommended state for screenshot:**
- Show a sample receive address
- QR code displayed
- Clear, centered layout

### 4. Transactions Page (`transactions.png`)
**Transaction history view showing:**
- "Transaction History" title
- Asset filter dropdown
- Transaction table with columns:
  - Date/Time
  - Type (Send/Receive)
  - Address
  - Amount
  - Asset
  - Confirmations
  - Status
- Sample transaction entries
- Pagination controls (if implemented)

**Recommended state for screenshot:**
- Show multiple sample transactions (mix of sent/received)
- Different assets represented
- Various confirmation states (confirmed, pending)
- Scrollable list with several entries

### 5. Menu and Toolbar (`menu.png`)
**Application menus showing:**
- File menu (New Wallet, Open Wallet, Backup, Settings, Exit)
- Edit menu (if applicable)
- View menu (Overview, Send, Receive, Transactions)
- Tools menu (Mining Control, Network Info, Debug Console)
- Help menu (About, Documentation)

**Recommended state for screenshot:**
- Show with one menu expanded to demonstrate menu structure

### 6. Settings Dialog (`settings.png`)
**Settings/preferences window showing:**
- RPC connection settings (host, port, credentials)
- Network settings
- Display preferences
- Language selection
- Theme options (if implemented)
- Apply/Cancel buttons

### 7. About Dialog (`about.png`)
**About window showing:**
- ParthenonChain Wallet logo
- Version number (v1.0.0)
- Description of multi-asset support
- List of supported assets:
  - TALANTON (TALN) - 21M max supply
  - DRACHMA (DRM) - 84M max supply
  - OBOLOS (OBL) - Gas token
- Copyright notice
- License information

## How to Capture Screenshots

### On Linux

1. Build and run the desktop wallet:
```bash
cd /home/runner/work/PantheonChain/PantheonChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make parthenon-qt
./clients/desktop/parthenon-qt
```

2. Use GNOME Screenshot or similar tool:
```bash
gnome-screenshot -w  # Window screenshot
gnome-screenshot -a  # Area selection
```

3. Or use scrot:
```bash
scrot -u  # Current window
```

### On macOS

1. Build and run the desktop wallet:
```bash
cd /Users/yourname/PantheonChain
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make parthenon-qt
./clients/desktop/parthenon-qt
```

2. Use built-in screenshot tools:
- `Cmd + Shift + 4` then `Space` for window screenshot
- Or use Grab.app

### On Windows

1. Build and run the desktop wallet using Visual Studio or CMake

2. Use built-in screenshot tools:
- `Alt + PrtScn` for active window
- Or use Snipping Tool / Snip & Sketch

## Screenshot Guidelines

- **Resolution**: Capture at standard resolution (1920x1080 or similar)
- **Format**: PNG format for best quality (lossless)
- **File naming**: Use descriptive names as listed above
- **Size**: Keep file sizes reasonable (< 500KB if possible)
- **Privacy**: Ensure no real addresses or sensitive data are visible
- **Consistency**: Use the same window size and theme for all screenshots
- **Quality**: Clear, well-lit, properly focused images

## Required Screenshots

The minimum set of screenshots needed:
1. ✅ `overview.png` - Main dashboard
2. ✅ `send.png` - Send transaction page
3. ✅ `receive.png` - Receive address page
4. ✅ `transactions.png` - Transaction history

Optional but recommended:
5. ✅ `menu.png` - Menu structure
6. ✅ `settings.png` - Settings dialog
7. ✅ `about.png` - About dialog

## Using Screenshots in Documentation

Once captured, reference screenshots in documentation like this:

```markdown
![Overview Page](screenshots/overview.png)
*ParthenonChain Desktop Wallet - Overview Page*
```

Or in README files:
```markdown
## Screenshots

### Main Dashboard
![Overview](screenshots/overview.png)

### Send Transaction
![Send](screenshots/send.png)
```

## Current Status

🟡 **Screenshots Provided** - Placeholder images are present; replace them with real captures from the running desktop wallet when available.

To contribute screenshots:
1. Build and run the desktop wallet on your system
2. Capture screenshots following the guidelines above
3. Place them in this directory
4. Update this README to mark screenshots as complete (✅)
5. Submit a pull request

---

**Note**: Screenshots should be captured with the RPC server running and the wallet connected to show realistic data. You can use testnet or regtest mode with sample data for demonstration purposes.
