# ParthenonChain Client UI Reference

This document provides a detailed textual description of the ParthenonChain client interfaces. Use this as a reference for UI design and as a guide for capturing screenshots.

## Desktop Wallet (Qt GUI)

### Window Structure

**Main Window**
- **Title**: "ParthenonChain Wallet"
- **Size**: 1000px width × 700px height
- **Components**:
  - Menu bar (top)
  - Toolbar (below menu bar)
  - Main content area (stacked pages)
  - Status bar (bottom)

### Menu Bar

**File Menu**
- New Wallet
- Open Wallet
- Backup Wallet
- Restore Wallet
- Settings
- Exit

**Edit Menu**
- Copy Address
- Paste Address
- Preferences

**View Menu**
- Overview (default)
- Send
- Receive
- Transactions

**Tools Menu**
- Mining Control
- Network Information
- Debug Console
- Sign Message
- Verify Message

**Help Menu**
- Documentation
- About ParthenonChain Wallet

### Toolbar

**Navigation Buttons** (left to right):
1. Overview (house icon)
2. Send (arrow up icon)
3. Receive (arrow down icon)
4. Transactions (list icon)

### Status Bar

**Left Section**: Connection status
- "Connected to ParthenonChain network" (when connected, green indicator)
- "Connecting..." (when connecting, yellow indicator)
- "Not connected" (when disconnected, red indicator)

**Right Section**: Blockchain status
- "Block: [height]" (e.g., "Block: 150000")
- "Last updated: [time]" (e.g., "Last updated: 2 minutes ago")

---

## Page 1: Overview

### Layout

```
┌─────────────────────────────────────────────────────────┐
│  Wallet Overview                                        │
│                                                         │
│  Select Asset: [TALN ▼]                                │
│                                                         │
│  ┌─ Current Balance ─────────────────────────────────┐ │
│  │  TALANTON (TALN)                                  │ │
│  │  1234.56789012                    [large font]    │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  ┌─ All Assets ──────────────────────────────────────┐ │
│  │  TALANTON (TALN):  1234.56789012                 │ │
│  │  DRACHMA (DRM):    5678.90123456                 │ │
│  │  OBOLOS (OBL):      123.45678901                 │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  [ Send ]                           [ Receive ]         │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Elements

**Title Section**
- Text: "Wallet Overview"
- Font: 18pt, bold

**Asset Selector**
- Label: "Select Asset:"
- Dropdown options: TALN, DRM, OBL
- Default: TALN

**Current Balance Box**
- Group box with title "Current Balance"
- Asset name: Font 12pt (e.g., "TALANTON (TALN)")
- Balance value: Font 24pt, bold (e.g., "1234.56789012")
- Precision: 8 decimal places

**All Assets Box**
- Group box with title "All Assets"
- Three rows, one per asset:
  - "TALANTON (TALN): [balance]"
  - "DRACHMA (DRM): [balance]"
  - "OBOLOS (OBL): [balance]"
- Standard font

**Quick Action Buttons**
- Two buttons: "Send" and "Receive"
- Height: 40px minimum
- Full width (50% each)

---

## Page 2: Send Transaction

### Layout

```
┌─────────────────────────────────────────────────────────┐
│  Send Transaction                                       │
│                                                         │
│  Asset: [TALN ▼]                                       │
│                                                         │
│  ┌─ Recipient ────────────────────────────────────────┐│
│  │  Pay To:                                           ││
│  │  [address input field________________] [Paste]    ││
│  │                                                    ││
│  │  Label (optional):                                ││
│  │  [label input field___________________]           ││
│  └────────────────────────────────────────────────────┘│
│                                                         │
│  ┌─ Amount ───────────────────────────────────────────┐│
│  │  Amount:                                           ││
│  │  [amount input field_______] [MAX]                ││
│  │  Available: 1234.56789012 TALN                    ││
│  └────────────────────────────────────────────────────┘│
│                                                         │
│  ┌─ Fee ──────────────────────────────────────────────┐│
│  │  Transaction Fee: 0.00001000 TALN                 ││
│  │  Total: 10.00001000 TALN                          ││
│  └────────────────────────────────────────────────────┘│
│                                                         │
│                            [Clear] [Send Transaction]   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Elements

**Title Section**
- Text: "Send Transaction"
- Font: 18pt, bold

**Asset Selector**
- Label: "Asset:"
- Dropdown: TALN, DRM, OBL

**Recipient Section** (Group box)
- **Pay To field**:
  - Label: "Pay To:"
  - Input: Text field for address
  - Button: "Paste" (pastes from clipboard)
- **Label field**:
  - Label: "Label (optional):"
  - Input: Text field for memo/note

**Amount Section** (Group box)
- **Amount field**:
  - Label: "Amount:"
  - Input: Decimal number field (8 decimal places)
  - Button: "MAX" (fills with maximum available)
- **Available balance**:
  - Display: "Available: [balance] [asset]"
  - Updates based on selected asset

**Fee Section** (Group box)
- **Transaction fee display**:
  - Text: "Transaction Fee: [fee] [asset]"
  - Auto-calculated
- **Total display**:
  - Text: "Total: [amount + fee] [asset]"
  - Bold font

**Action Buttons**
- "Clear" button (secondary)
- "Send Transaction" button (primary, larger)

### Validation
- Address format validation
- Amount must be > 0 and <= available balance
- Minimum fee validation
- Confirmation dialog before sending

---

## Page 3: Receive

### Layout

```
┌─────────────────────────────────────────────────────────┐
│  Receive Payment                                        │
│                                                         │
│  Asset: [TALN ▼]                                       │
│                                                         │
│  ┌─ Your Receive Address ─────────────────────────────┐│
│  │                                                     ││
│  │  ┌─────────────────────┐                          ││
│  │  │                     │                          ││
│  │  │   [QR CODE]        │                          ││
│  │  │                     │                          ││
│  │  └─────────────────────┘                          ││
│  │                                                     ││
│  │  tpn1qxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx          ││
│  │                                                     ││
│  │  [Copy Address]                                    ││
│  │                                                     ││
│  └─────────────────────────────────────────────────────┘│
│                                                         │
│  ┌─ Request Payment ───────────────────────────────────┐│
│  │  Amount (optional):                                ││
│  │  [amount field________]                            ││
│  │                                                     ││
│  │  Message (optional):                               ││
│  │  [message field_______]                            ││
│  │                                                     ││
│  │  [Create Payment Request]                          ││
│  └─────────────────────────────────────────────────────┘│
│                                                         │
│  [Generate New Address]                                 │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Elements

**Title Section**
- Text: "Receive Payment"
- Font: 18pt, bold

**Asset Selector**
- Label: "Asset:"
- Dropdown: TALN, DRM, OBL

**Your Receive Address** (Group box)
- **QR Code**:
  - Size: 200x200 pixels
  - Centered
  - Contains receive address
- **Address Display**:
  - Monospace font
  - Full address shown
  - Centered
- **Copy Button**:
  - "Copy Address" button
  - Copies address to clipboard

**Request Payment** (Group box, optional)
- **Amount field**:
  - Label: "Amount (optional):"
  - Decimal input
- **Message field**:
  - Label: "Message (optional):"
  - Text input
- **Create button**:
  - "Create Payment Request"
  - Generates payment URI with QR

**New Address Button**
- "Generate New Address"
- Creates new receiving address
- Updates QR code and display

---

## Page 4: Transactions

### Layout

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│  Transaction History                                                                    │
│                                                                                         │
│  Filter: [All Transactions ▼]  Asset: [All Assets ▼]         [Search: ________]       │
│                                                                                         │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ Date/Time          Type     Address                 Amount        Asset  Status  │  │
│  ├──────────────────────────────────────────────────────────────────────────────────┤  │
│  │ 2026-01-17 14:23  Received  tpn1qxx...xxx  +100.00000000   TALN   Confirmed    │  │
│  │ 2026-01-17 12:45  Sent      tpn1qyy...yyy   -50.00000000   DRM    Confirmed    │  │
│  │ 2026-01-17 11:30  Received  tpn1qzz...zzz   +25.00000000   OBL    Pending (2)  │  │
│  │ 2026-01-16 18:20  Sent      tpn1qaa...aaa   -10.00000000   TALN   Confirmed    │  │
│  │ 2026-01-16 15:10  Received  tpn1qbb...bbb  +500.00000000   DRM    Confirmed    │  │
│  │ 2026-01-16 10:05  Sent      tpn1qcc...ccc   -75.00000000   TALN   Confirmed    │  │
│  │ ...                                                                              │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                         │
│  Showing 1-20 of 156 transactions                            [< Previous] [Next >]     │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### Elements

**Title Section**
- Text: "Transaction History"
- Font: 18pt, bold

**Filter Controls**
- **Type filter**:
  - Dropdown: All Transactions, Sent, Received
- **Asset filter**:
  - Dropdown: All Assets, TALN, DRM, OBL
- **Search field**:
  - Text input for searching addresses/transactions

**Transaction Table**
- **Columns**:
  1. Date/Time (sortable)
  2. Type (Sent/Received with icon)
  3. Address (truncated with ellipsis)
  4. Amount (with +/- sign, 8 decimals)
  5. Asset (TALN/DRM/OBL)
  6. Status (Confirmed, Pending with confirmations)

- **Formatting**:
  - Received amounts: Green with + prefix
  - Sent amounts: Red with - prefix
  - Pending: Yellow/orange indicator
  - Confirmed: Green checkmark

**Pagination**
- Status text: "Showing X-Y of Z transactions"
- Previous/Next buttons

**Double-click**: Opens transaction detail dialog

---

## Mobile Wallet (React Native)

### Common UI Elements

**Bottom Navigation Bar**
- Icons with labels
- 5 tabs:
  1. Home (house icon)
  2. Send (arrow up icon)
  3. Receive (arrow down icon)
  4. Transactions (list icon)
  5. Settings (gear icon)

**Status Bar**
- Time: 9:41
- Signal strength: Full
- WiFi: Connected
- Battery: Full

**Header Bar**
- Left: Back button (on sub-pages)
- Center: Page title
- Right: Menu/settings button (context-dependent)

---

## Mobile Screen 1: Home/Dashboard

### Layout (Portrait)

```
┌─────────────────────────┐
│ ⬅  ParthenonChain   ⋮  │ Header
├─────────────────────────┤
│                         │
│  [TALN] [DRM] [OBL]    │ Asset tabs
│                         │
│  ┌───────────────────┐  │
│  │   TALANTON        │  │
│  │                   │  │
│  │   1234.56789012  │  │ Large balance
│  │                   │  │
│  └───────────────────┘  │
│                         │
│  All Balances:          │
│  TALN: 1234.56789012   │
│  DRM:  5678.90123456   │
│  OBL:   123.45678901   │
│                         │
│  ┌──────┐  ┌──────┐    │
│  │ Send │  │ Rcv  │    │ Quick actions
│  └──────┘  └──────┘    │
│                         │
│  ┌──────────────────┐   │
│  │ ⛏ Mining        │   │
│  │ Start Mining    │   │ Mining card
│  └──────────────────┘   │
│                         │
│  Recent Transactions:   │
│  ┌───────────────────┐  │
│  │ ⬆ Sent 10 TALN   │  │
│  │ 5 min ago        │  │
│  ├───────────────────┤  │
│  │ ⬇ Rcvd 100 TALN  │  │
│  │ 1 hour ago       │  │
│  └───────────────────┘  │
│                         │
├─────────────────────────┤
│ 🏠 📤 📥 📋 ⚙         │ Bottom nav
└─────────────────────────┘
```

### Elements

**Header**
- Back button (if applicable)
- App name: "ParthenonChain"
- Menu button (three dots)

**Asset Tabs**
- Three tabs: TALN, DRM, OBL
- Active tab highlighted
- Swipeable

**Balance Card**
- Asset name (e.g., "TALANTON")
- Large balance display (font size 32sp)
- Gradient background
- Drop shadow

**All Balances Section**
- List of all three assets with balances
- Smaller font (16sp)
- Tap to switch to that asset

**Quick Action Buttons**
- Two large buttons side-by-side
- "Send" button (left)
- "Receive" button (right)
- Icons + text
- Minimum touch target: 48dp

**Mining Card**
- Pickaxe icon
- "Mining" label
- "Start Mining" / "Stop Mining" button
- Shows hashrate when active

**Recent Transactions**
- Title: "Recent Transactions"
- Last 3-5 transactions
- Each row: Icon, Amount, Asset, Time
- Tap to view details
- "View All" link at bottom

---

## Mobile Screen 2: Send

### Layout

```
┌─────────────────────────┐
│ ⬅  Send             ⋮  │
├─────────────────────────┤
│                         │
│  Asset: [TALN ▼]       │
│                         │
│  ┌───────────────────┐  │
│  │ To Address:       │  │
│  │ [____________] 📷 │  │ QR scan
│  └───────────────────┘  │
│                         │
│  ┌───────────────────┐  │
│  │ Amount:           │  │
│  │ [____________]    │  │
│  │         [MAX]     │  │
│  └───────────────────┘  │
│                         │
│  Available:             │
│  1234.56789012 TALN    │
│                         │
│  ┌───────────────────┐  │
│  │ Fee: 0.00001 TALN │  │
│  │ Total: 10.00001   │  │
│  └───────────────────┘  │
│                         │
│  ┌─────────────────┐    │
│  │  SEND           │    │ Primary button
│  └─────────────────┘    │
│                         │
├─────────────────────────┤
│ 🏠 📤 📥 📋 ⚙         │
└─────────────────────────┘
```

### Elements

**Header**
- Back button
- Title: "Send"
- Menu button

**Asset Selector**
- Dropdown: TALN, DRM, OBL

**To Address Field**
- Input field
- QR scanner button (camera icon)
- Paste button appears when clipboard has address

**Amount Field**
- Numeric input
- MAX button (fills maximum)
- Validation: Must be <= available

**Available Balance Display**
- Shows current balance of selected asset
- Updates when asset changes

**Fee Display**
- Shows estimated transaction fee
- Shows total (amount + fee)
- Auto-calculated

**Send Button**
- Large, full-width button
- Primary color
- Minimum height: 48dp
- Shows confirmation dialog on tap

---

## Mobile Screen 3: Receive

### Layout

```
┌─────────────────────────┐
│ ⬅  Receive          ⋮  │
├─────────────────────────┤
│                         │
│  Asset: [TALN ▼]       │
│                         │
│  ┌───────────────────┐  │
│  │                   │  │
│  │   ▆▆▆▆▆▆▆▆▆▆▆   │  │
│  │   ▆         ▆    │  │
│  │   ▆  QR     ▆    │  │ Large QR code
│  │   ▆  CODE   ▆    │  │
│  │   ▆         ▆    │  │
│  │   ▆▆▆▆▆▆▆▆▆▆▆   │  │
│  │                   │  │
│  └───────────────────┘  │
│                         │
│  tpn1qxxxxxxxxxx...     │ Address
│  ...xxxxxxxxxxx         │ (wrapped)
│                         │
│  ┌─────────────────┐    │
│  │  COPY ADDRESS   │    │
│  └─────────────────┘    │
│                         │
│  ┌─────────────────┐    │
│  │  SHARE          │    │
│  └─────────────────┘    │
│                         │
│  ┌─────────────────┐    │
│  │  NEW ADDRESS    │    │
│  └─────────────────┘    │
│                         │
├─────────────────────────┤
│ 🏠 📤 📥 📋 ⚙         │
└─────────────────────────┘
```

### Elements

**Header**
- Back button
- Title: "Receive"
- Menu button

**Asset Selector**
- Dropdown: TALN, DRM, OBL

**QR Code**
- Large QR code (250x250 dp)
- Centered
- Contains receive address
- High contrast

**Address Display**
- Monospace font
- Full address
- Word-wrapped if needed
- Tap to copy

**Action Buttons**
- "Copy Address" - Copies to clipboard with confirmation
- "Share" - Opens share sheet
- "New Address" - Generates new address

---

## Mobile Screen 4: Transactions

### Layout

```
┌─────────────────────────┐
│ ⬅  Transactions     ⋮  │
├─────────────────────────┤
│                         │
│  [All ▼] [All Assets ▼]│ Filters
│  [Search________]  🔍  │
│                         │
│  ┌───────────────────┐  │
│  │ ⬇ Received        │  │
│  │ +100.00000000     │  │
│  │ TALN              │  │
│  │ 2 hours ago  ✓   │  │
│  ├───────────────────┤  │
│  │ ⬆ Sent            │  │
│  │ -50.00000000      │  │
│  │ DRM               │  │
│  │ 5 hours ago  ✓   │  │
│  ├───────────────────┤  │
│  │ ⬇ Received        │  │
│  │ +25.00000000      │  │
│  │ OBL               │  │
│  │ 1 day ago  (2)   │  │ Pending
│  ├───────────────────┤  │
│  │ ...               │  │
│  └───────────────────┘  │
│                         │
│  [Pull to refresh]      │
│                         │
├─────────────────────────┤
│ 🏠 📤 📥 📋 ⚙         │
└─────────────────────────┘
```

### Elements

**Header**
- Back button
- Title: "Transactions"
- Filter button

**Filters**
- Type dropdown: All, Sent, Received
- Asset dropdown: All, TALN, DRM, OBL
- Search field

**Transaction List**
- Scrollable list
- Each item shows:
  - Icon (up/down arrow)
  - Type (Sent/Received)
  - Amount (with +/-)
  - Asset
  - Time ago
  - Status (checkmark or confirmations)
- Color coding:
  - Green for received
  - Red for sent
  - Yellow for pending
- Tap to view details
- Pull to refresh

**Empty State** (when no transactions)
- Icon
- "No transactions yet"
- "Start by receiving some funds"

---

## Mobile Screen 5: Mining

### Layout

```
┌─────────────────────────┐
│ ⬅  Mining           ⋮  │
├─────────────────────────┤
│                         │
│  ┌───────────────────┐  │
│  │  ⛏ Mining         │  │
│  │                   │  │
│  │  Status: Active   │  │
│  │  ┌─────────────┐  │  │
│  │  │ STOP MINING │  │  │ Toggle button
│  │  └─────────────┘  │  │
│  └───────────────────┘  │
│                         │
│  ┌───────────────────┐  │
│  │ Hashrate          │  │
│  │ 1.2 MH/s         │  │ Large numbers
│  └───────────────────┘  │
│                         │
│  ┌───────────────────┐  │
│  │ Shares Submitted  │  │
│  │ 15                │  │
│  └───────────────────┘  │
│                         │
│  ┌───────────────────┐  │
│  │ Uptime            │  │
│  │ 2h 34m           │  │
│  └───────────────────┘  │
│                         │
│  ┌───────────────────┐  │
│  │ CPU Usage         │  │
│  │ [====•    ] 50%  │  │ Slider
│  └───────────────────┘  │
│                         │
│  Est. Earnings: 0.05 OBL│
│  per day                │
│                         │
├─────────────────────────┤
│ 🏠 📤 📥 📋 ⚙         │
└─────────────────────────┘
```

### Elements

**Header**
- Back button
- Title: "Mining"
- Settings button

**Mining Status Card**
- Mining icon
- Status: Active/Inactive
- Toggle button: "Start Mining" / "Stop Mining"
- Primary color when active

**Statistics Cards**
- **Hashrate**: Large number display (MH/s)
- **Shares**: Count of submitted shares
- **Uptime**: Duration (hours, minutes)
- Each in a separate card

**CPU Usage Control**
- Slider (0-100%)
- Current percentage display
- Affects mining intensity

**Earnings Display**
- Estimated daily earnings
- Based on current hashrate
- Shows in OBL (gas token)

**Warnings** (if applicable)
- High temperature warning
- Low battery warning
- Background mining notification

---

## Color Scheme

### Desktop Wallet (Light Theme)
- **Primary**: #2C3E50 (dark blue-gray)
- **Secondary**: #3498DB (blue)
- **Success**: #27AE60 (green) - for received transactions
- **Danger**: #E74C3C (red) - for sent transactions
- **Warning**: #F39C12 (orange) - for pending
- **Background**: #ECF0F1 (light gray)
- **Text**: #2C3E50 (dark gray)

### Mobile Wallet
- **Primary**: #3498DB (blue)
- **Accent**: #2ECC71 (green)
- **Background**: #FFFFFF (white)
- **Card**: #F8F9FA (light gray)
- **Text Primary**: #212529 (dark)
- **Text Secondary**: #6C757D (gray)
- **Success**: #28A745 (green)
- **Danger**: #DC3545 (red)
- **Warning**: #FFC107 (amber)

### Dark Mode (Both)
- **Background**: #1E1E1E (dark gray)
- **Surface**: #2D2D2D (lighter gray)
- **Primary**: #64B5F6 (light blue)
- **Text**: #FFFFFF (white)
- **Text Secondary**: #B0B0B0 (light gray)

---

## Typography

### Desktop
- **Title**: 18pt, Bold
- **Subtitle**: 14pt, Semi-bold
- **Body**: 11pt, Regular
- **Balance**: 24pt, Bold
- **Address**: Monospace, 10pt
- **Font Family**: System default (Segoe UI, San Francisco, Ubuntu)

### Mobile
- **Heading 1**: 24sp, Bold
- **Heading 2**: 20sp, Semi-bold
- **Body**: 16sp, Regular
- **Balance**: 32sp, Bold
- **Caption**: 14sp, Regular
- **Address**: Monospace, 14sp
- **Font Family**: System default (SF Pro, Roboto)

---

## Icons

### Desktop
- Uses system icons where available
- Custom icons for ParthenonChain-specific actions
- 16x16 or 24x24 pixels for toolbar
- Vector format (SVG) preferred

### Mobile
- Material Design icons (Android)
- SF Symbols (iOS)
- 24dp standard size
- 48dp for tab bar icons
- Filled style for active states
- Outlined style for inactive states

---

This reference document describes the UI in detail. For actual visual screenshots, please refer to the screenshots directories once they are captured.
