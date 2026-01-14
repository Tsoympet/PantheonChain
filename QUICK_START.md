# Quick Start Guide

Welcome to ParthenonChain! This guide will help you get started in just a few minutes.

## Step 1: Download ParthenonChain

Choose your operating system and download the installer:

### Windows Users
1. Go to: https://github.com/Tsoympet/PantheonChain/releases/latest
2. Download: `parthenon-1.0.0-windows-x64-setup.exe`
3. Run the installer and follow the wizard

### Mac Users
1. Go to: https://github.com/Tsoympet/PantheonChain/releases/latest
2. Download: `parthenon-1.0.0-macos.dmg`
3. Open the DMG and drag ParthenonChain to Applications

### Linux Users
1. Go to: https://github.com/Tsoympet/PantheonChain/releases/latest
2. Download the `.deb` (Ubuntu/Debian) or `.rpm` (Fedora/RHEL) package
3. Install using your package manager

**Need detailed instructions?** See [DOWNLOAD.md](DOWNLOAD.md)

---

## Step 2: Launch ParthenonChain

### Windows
- Click the ParthenonChain icon on your desktop, OR
- Search for "ParthenonChain" in the Start Menu

### macOS
- Open **Applications** folder
- Double-click **ParthenonChain**

### Linux
The service starts automatically! Use the terminal:
```bash
parthenon-cli getinfo
```

---

## Step 3: Wait for Initial Sync

When you first run ParthenonChain, it needs to download and verify the blockchain.

**This will take a few hours** depending on your internet speed.

You can monitor progress:
```bash
parthenon-cli getblockchaininfo
```

Look for the `"verificationprogress"` field - it should reach `1.0` (100%) when complete.

---

## Step 4: Create Your First Wallet

Once synced, create a wallet address to receive funds:

```bash
parthenon-cli getnewaddress
```

This will output an address like: `PTH1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9`

**⚠️ IMPORTANT**: Back up your wallet immediately!

```bash
# Backup your wallet file (keep it safe!)
# Location varies by OS:
# Windows: C:\Users\<you>\AppData\Roaming\ParthenonChain\wallet.dat
# macOS: ~/Library/Application Support/ParthenonChain/wallet.dat
# Linux: ~/.parthenon/wallet.dat
```

---

## Step 5: Check Your Balance

View your wallet balance:

```bash
parthenon-cli getbalance
```

This shows your balance in all three ParthenonChain currencies:
- **TALANTON** - Store of value (like Bitcoin)
- **DRACHMA** - Medium of exchange (for transactions)
- **OBOLOS** - Smart contract gas (for running contracts)

---

## Step 6: Send Transactions

To send TALANTON to another address:

```bash
parthenon-cli sendtoaddress <recipient_address> <amount> TALANTON
```

Example:
```bash
parthenon-cli sendtoaddress PTH1abc...xyz 10.5 TALANTON
```

To send DRACHMA or OBOLOS, replace `TALANTON` with the asset name.

---

## Using the Desktop Wallet (GUI)

If you installed the Desktop Wallet component, you can:

1. **Launch the GUI** - Search for "ParthenonChain Wallet" in your applications
2. **View Balance** - See all your assets at a glance
3. **Send/Receive** - Click "Send" or "Receive" buttons
4. **Transaction History** - View all past transactions
5. **Settings** - Configure network, backup, and more

The GUI is easier for beginners than command-line tools!

---

## Common Commands Cheat Sheet

```bash
# Node Status
parthenon-cli getinfo                    # General node information
parthenon-cli getblockchaininfo          # Blockchain sync status
parthenon-cli getnetworkinfo             # Network connection info
parthenon-cli getpeerinfo                # Connected peers

# Wallet Operations
parthenon-cli getnewaddress              # Create new receiving address
parthenon-cli getbalance                 # Show wallet balance
parthenon-cli listunspent                # List available UTXOs
parthenon-cli listtransactions           # Show recent transactions

# Sending Transactions
parthenon-cli sendtoaddress <addr> <amt> TALANTON   # Send TALANTON
parthenon-cli sendtoaddress <addr> <amt> DRACHMA    # Send DRACHMA
parthenon-cli sendtoaddress <addr> <amt> OBOLOS     # Send OBOLOS

# Mining (Optional)
parthenon-cli setgenerate true 4         # Start mining with 4 threads
parthenon-cli getmininginfo              # Check mining status
parthenon-cli setgenerate false          # Stop mining

# Smart Contracts
parthenon-cli deploycontract <bytecode> <gas>       # Deploy contract
parthenon-cli callcontract <addr> <data> <gas>      # Call contract
parthenon-cli getcontractinfo <addr>                # Get contract info

# Help
parthenon-cli help                       # List all commands
parthenon-cli help <command>             # Get help for specific command
```

---

## Understanding the Three Currencies

ParthenonChain has three native cryptocurrencies:

### 🏛️ TALANTON (Store of Value)
- **Purpose**: Long-term savings, like digital gold
- **Supply**: Limited issuance (scarce)
- **Use Case**: Investment, savings, large transfers
- **Think of it as**: Bitcoin-like digital gold

### 💰 DRACHMA (Medium of Exchange)
- **Purpose**: Daily transactions and payments
- **Supply**: Moderate issuance
- **Use Case**: Buying goods, services, everyday payments
- **Think of it as**: Digital cash for spending

### ⚡ OBOLOS (Gas Token)
- **Purpose**: Paying for smart contract execution
- **Supply**: Controlled issuance for network fees
- **Use Case**: Running smart contracts, DApps
- **Think of it as**: Ethereum's gas, but as a separate currency

All three currencies are mined together - when miners create a new block, they receive rewards in all three!

---

## Mining ParthenonChain (Optional)

Want to help secure the network and earn rewards? Start mining:

```bash
# Start mining with 4 CPU threads
parthenon-cli setgenerate true 4

# Check mining status
parthenon-cli getmininginfo

# Stop mining
parthenon-cli setgenerate false
```

**Note**: CPU mining is suitable for testnet or small-scale participation. For serious mining, consider:
- GPU mining software (see [docs/GPU_ACCELERATION.md](docs/GPU_ACCELERATION.md))
- Mining pools (join forces with other miners)
- ASIC miners (specialized hardware)

---

## Security Best Practices

### 🔒 Keep Your Wallet Safe

1. **Backup your wallet file** regularly
2. **Store backups securely** (encrypted, offline)
3. **Never share your private keys** with anyone
4. **Use strong passwords** for encryption
5. **Enable 2FA** if using exchanges

### 🛡️ Verify Downloads

Always verify downloaded installers:
```bash
# Check SHA256 checksum
sha256sum parthenon-1.0.0-*.exe
# Compare with official checksums
```

### 🔐 Network Security

- Keep your firewall enabled
- Only open required ports (8333 for P2P)
- Don't expose RPC to the internet
- Keep software updated

---

## Getting Help

### 📚 Documentation
- [Full User Guide](README.md)
- [Installation Guide](docs/INSTALLATION.md)
- [Download Instructions](DOWNLOAD.md)
- [Architecture Overview](docs/ARCHITECTURE.md)

### 💬 Community Support
- [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions) - Ask questions
- [GitHub Issues](https://github.com/Tsoympet/PantheonChain/issues) - Report bugs
- Email: support@parthenonchain.org

### 🎓 Learning Resources
- [Whitepaper](WHITEPAPER.md) - Technical details
- [API Documentation](docs/NETWORKING_RPC.md) - For developers
- [Smart Contracts Guide](docs/ADVANCED_FEATURES.md) - Build DApps

---

## Troubleshooting

### Node Won't Start
```bash
# Check if already running
ps aux | grep parthenond

# View logs
tail -f ~/.parthenon/debug.log  # Linux/Mac
type %APPDATA%\ParthenonChain\debug.log  # Windows
```

### Sync is Stuck
1. Check internet connection
2. Verify firewall isn't blocking port 8333
3. Try adding nodes manually:
```bash
parthenon-cli addnode <ip:port> add
```

### Can't Send Transaction
- Ensure you have sufficient balance
- Check if node is fully synced
- Verify recipient address is correct
- Make sure you have OBOLOS for transaction fees

### Need More Help?
See the [Troubleshooting Section](TROUBLESHOOTING.md) in our documentation.

---

## What's Next?

Now that you're running ParthenonChain:

1. **Join the Community** - Connect with other users
2. **Explore Smart Contracts** - Build decentralized applications
3. **Set Up Mining** - Earn rewards while securing the network
4. **Develop DApps** - Use the EVM-compatible smart contract platform
5. **Contribute** - Help improve ParthenonChain (it's open source!)

### For Developers
- 🔧 [Developer Guide](CONTRIBUTING.md)
- 🏗️ [Build from Source](README.md#building-from-source)
- 📖 [API Reference](docs/NETWORKING_RPC.md)
- 🔌 [SDK Documentation](tools/mobile_sdks/README.md)

### For Miners
- ⛏️ [Mining Guide](docs/ADVANCED_FEATURES.md)
- 🖥️ [GPU Acceleration](docs/GPU_ACCELERATION.md)
- 🏊 [Mining Pool Setup](docs/NETWORKING_RPC.md)

### For Users
- 💼 [Desktop Wallet Guide](clients/desktop/README.md)
- 📱 [Mobile Wallet](clients/mobile/README.md)
- 🔐 [Hardware Wallet Setup](docs/HARDWARE_WALLET.md)

---

## Frequently Asked Questions

**Q: How long does the initial sync take?**  
A: Usually 2-6 hours depending on your internet speed and hardware.

**Q: Do I need all three currencies?**  
A: You'll receive all three when mining or receiving payments. Each has its own purpose.

**Q: Can I use ParthenonChain on mobile?**  
A: Yes! We have mobile wallets for iOS and Android (see [clients/mobile](clients/mobile/README.md)).

**Q: Is mining profitable?**  
A: It depends on your electricity costs and hardware. GPU/ASIC mining is more efficient than CPU.

**Q: Can I run a light client instead of full node?**  
A: Yes, SPV (lightweight) clients are available in the mobile wallets.

**Q: How do I update ParthenonChain?**  
A: Download the latest installer and run it. Your wallet and data are preserved.

---

<p align="center">
  <strong>You're all set! Welcome to ParthenonChain! 🎉</strong>
</p>

<p align="center">
  <a href="DOWNLOAD.md">Download Guide</a> •
  <a href="README.md">Full Documentation</a> •
  <a href="https://github.com/Tsoympet/PantheonChain/discussions">Community</a>
</p>
