# Hardware Wallet Integration

ParthenonChain supports hardware wallet integration for enhanced security. Hardware wallets keep private keys isolated on a secure device, protecting them from malware and theft.

## Supported Devices

- **Ledger Nano S/X** - Popular hardware wallet with secure element
- **Trezor One/Model T** - Open-source hardware wallet
- **KeepKey** - Large-screen hardware wallet
- **Generic HID** - Support for custom hardware wallet implementations

## Features

### BIP-32 Hierarchical Deterministic Keys

Hardware wallets use BIP-32 derivation paths to generate addresses:

```
m/44'/0'/0'/0/0  - First receiving address
m/44'/0'/0'/0/1  - Second receiving address
m/44'/0'/0'/1/0  - First change address
```

The hardened derivation (`'`) ensures that child keys cannot be used to derive parent keys, providing additional security.

### Transaction Signing

All transaction signing is performed on the hardware device:

1. Transaction is sent to the device
2. User reviews transaction details on device screen
3. User confirms transaction with physical button press
4. Device creates Schnorr signature using secure private key
5. Signed transaction is returned to the computer

Private keys never leave the hardware device.

### Address Verification

Hardware wallets allow visual verification of addresses:

1. Request address from device
2. Device displays full address on screen
3. User visually verifies address matches expected value
4. User confirms on device

This prevents address substitution attacks.

## Usage

### C++ API

```cpp
#include "wallet/hardware/hardware_wallet.h"

using namespace parthenon::wallet::hardware;

// Create manager
HardwareWalletManager manager;

// Enumerate connected devices
auto devices = manager.EnumerateDevices();
for (const auto& device : devices) {
    std::cout << "Found: " << device.model 
              << " (v" << device.version << ")\n";
}

// Connect to first device
auto wallet = manager.ConnectFirstDevice();
if (!wallet) {
    std::cerr << "No hardware wallet found\n";
    return 1;
}

// Unlock with PIN
if (!wallet->UnlockWithPin("1234")) {
    std::cerr << "Invalid PIN\n";
    return 1;
}

// Get address at derivation path
auto path = DerivationPath::Parse("m/44'/0'/0'/0/0");
if (path) {
    auto address = wallet->GetAddress(*path, true);  // Display on device
    if (address) {
        std::cout << "Address: " << *address << "\n";
    }
}

// Sign transaction
std::vector<DerivationPath> input_paths = {
    *DerivationPath::Parse("m/44'/0'/0'/0/0"),
    *DerivationPath::Parse("m/44'/0'/0'/0/1")
};

auto signed_tx = wallet->SignTransaction(transaction, input_paths);
if (signed_tx) {
    // Broadcast signed transaction
    network.BroadcastTransaction(*signed_tx);
}
```

### RPC Interface

Hardware wallet support is also available via RPC:

```bash
# Enumerate hardware wallets
parthenon-cli hardwarewallet list

# Get address from hardware wallet
parthenon-cli hardwarewallet getaddress "m/44'/0'/0'/0/0" --verify

# Sign transaction with hardware wallet
parthenon-cli hardwarewallet signtransaction <txid> <input_paths>
```

### Desktop Wallet Integration

The ParthenonChain desktop wallet (Qt-based) includes hardware wallet support:

1. **Settings → Hardware Wallet** - Configure hardware wallet
2. **Connect Device** - Detect and connect to hardware wallet
3. **Send with Hardware Wallet** - Sign transactions on device
4. **Verify Addresses** - Confirm addresses on device screen

## Security Considerations

### Device Verification

Always verify your hardware wallet is genuine:

- Purchase from official vendor or authorized reseller
- Verify tamper-evident seals
- Check device serial number
- Verify firmware signatures

### PIN Protection

Hardware wallets are protected by a PIN:

- Use a strong, unique PIN (6-8 digits recommended)
- Don't share your PIN with anyone
- Device will wipe after multiple failed PIN attempts
- Some devices support additional passphrase protection

### Recovery Seed

Hardware wallets use a 12-24 word recovery seed:

- **Write down your recovery seed** and store it securely
- Never store recovery seed digitally
- Keep recovery seed separate from hardware device
- Consider using metal backup for fire/water resistance
- **Never share your recovery seed** with anyone

### Physical Security

Protect your hardware wallet:

- Keep device in a secure location when not in use
- Don't leave device connected to untrusted computers
- Verify all transaction details on device screen
- Be aware of supply chain attacks (always buy from authorized sources)

## Multi-Asset Support

ParthenonChain's hardware wallet integration supports all three native tokens:

- **TALANTON** (TALN) - Store of value token
- **DRACHMA** (DRM) - Medium of exchange token
- **OBOLOS** (OBL) - Smart contract gas token

Each asset uses different derivation paths:

```
TALN:  m/44'/0'/0'/0/x
DRM:   m/44'/1'/0'/0/x
OBL:   m/44'/2'/0'/0/x
```

## Development

### Adding Support for New Devices

To add support for a new hardware wallet device:

1. Implement the `HardwareWallet` interface in `hardware_wallet.h`
2. Add device-specific USB/HID communication
3. Implement BIP-32 key derivation protocol
4. Add transaction signing logic
5. Register device with `HardwareWalletManager`

Example:

```cpp
class CustomHardwareWallet : public HardwareWallet {
public:
    DeviceType GetType() const override {
        return DeviceType::GENERIC;
    }
    
    // Implement other interface methods...
};
```

### Testing

Hardware wallet functionality includes comprehensive tests:

```bash
# Build and run hardware wallet tests
cd build
cmake --build . --target test_hardware_wallet
./tests/unit/wallet/test_hardware_wallet
```

Tests cover:
- BIP-32 derivation path parsing
- Address generation
- Transaction signing
- Device enumeration
- Error handling

## Troubleshooting

### Device Not Detected

If your hardware wallet is not detected:

1. Check USB connection
2. Ensure device is unlocked
3. Install device drivers (Windows)
4. Check udev rules (Linux):
   ```bash
   # Add rules for Ledger
   sudo sh -c 'cat > /etc/udev/rules.d/20-hw-wallet.rules <<EOF
   SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", MODE="0660", GROUP="plugdev"
   EOF'
   sudo udevadm control --reload-rules
   ```
5. Verify device firmware is up to date

### Transaction Signing Fails

If transaction signing fails:

1. Verify device is unlocked with correct PIN
2. Check derivation paths match wallet configuration
3. Ensure device firmware supports Schnorr signatures
4. Review transaction details on device screen
5. Check device has sufficient battery/power

### Connection Errors

If you experience connection errors:

1. Try a different USB cable
2. Connect directly to computer (not through hub)
3. Close other applications using the device
4. Restart the hardware wallet
5. Update device firmware to latest version

## References

- BIP-32: Hierarchical Deterministic Wallets
- BIP-39: Mnemonic code for generating deterministic keys
- BIP-44: Multi-Account Hierarchy for Deterministic Wallets
- BIP-340: Schnorr Signatures for secp256k1

## See Also

- [Wallet API Documentation](NETWORKING_RPC.md#wallet-commands)
- [Security Model](SECURITY_MODEL.md)
- [Advanced Features](ADVANCED_FEATURES.md)
