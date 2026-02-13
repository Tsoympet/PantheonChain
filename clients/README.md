# ParthenonChain Clients

End-user applications for interacting with the ParthenonChain blockchain.

## Overview

ParthenonChain provides multiple client applications for different use cases and platforms.

## Available Clients

### 1. parthenond (Full Node Daemon)

**Location**: `core-daemon/`

**Purpose**: Full blockchain node

**Capabilities**:
- Full block validation
- UTXO set maintenance
- Transaction mempool
- P2P networking
- Mining support
- RPC server

**Target Users**: Node operators, miners, developers

**See**: [core-daemon/README.md](core-daemon/README.md)

### 2. parthenon-cli (Command-Line Interface)

**Location**: `cli/`

**Purpose**: Command-line wallet and blockchain tool

**Capabilities**:
- Wallet management
- Send/receive transactions
- Query blockchain
- Interact with smart contracts
- Mining control

**Target Users**: Advanced users, developers, automation

**See**: [cli/README.md](cli/README.md)

### 3. Desktop GUI

**Location**: `desktop/`

**Purpose**: Graphical desktop wallet

**Technology**: Qt framework

**Capabilities**:
- User-friendly transaction interface
- Address book
- Transaction history
- Network statistics
- Settings management

**Platforms**: Windows, macOS, Linux

**Target Users**: General users

**See**: [desktop/README.md](desktop/README.md)  
**Screenshots**: [desktop/screenshots/](desktop/screenshots/)

### 4. Mobile Wallet

**Location**: `mobile/`

**Purpose**: Mobile wallet with share-mining

**Technology**: React Native

**Capabilities**:
- Send/receive transactions
- QR code scanning
- SPV mode (lightweight)
- Share-mining participation
- Push notifications

**Platforms**: iOS, Android

**Target Users**: Mobile users, miners

**See**: [mobile/react-native/README.md](mobile/react-native/README.md)  
**Screenshots**: [mobile/screenshots/](mobile/screenshots/)

## Quick Start

### Running a Full Node

```bash
# Build all clients
mkdir build && cd build
cmake ..
make

# Start node
./clients/core-daemon/parthenond
```

### Using the CLI

```bash
# Get blockchain info
./clients/cli/parthenon-cli getinfo

# Get wallet balance
./clients/cli/parthenon-cli getbalance

# Send transaction
./clients/cli/parthenon-cli send <address> <amount>
```

### Running Desktop GUI

```bash
# Launch GUI
./clients/desktop/parthenon-qt
```

## Architecture

```
┌─────────────────────────────────────────────────┐
│                   Clients                       │
├──────────────┬──────────────┬──────────────────┤
│  parthenond  │ parthenon-cli│  Desktop/Mobile  │
│  (Full Node) │  (CLI Tool)  │  (Wallets)       │
└──────┬───────┴──────┬───────┴────────┬─────────┘
       │              │                │
       │              │                │
       └──────────────┴────────────────┘
                      │
       ┌──────────────▼──────────────┐
       │         RPC/API             │
       └──────────────┬──────────────┘
                      │
       ┌──────────────▼──────────────┐
       │     Layer 1 + Layer 2       │
       │    (Blockchain Core)        │
       └─────────────────────────────┘
```

## Client Comparison

| Feature | parthenond | parthenon-cli | Desktop GUI | Mobile |
|---------|------------|---------------|-------------|--------|
| Full Node | ✅ | ❌ | ✅ | ❌ |
| Wallet | ✅ | ✅ | ✅ | ✅ |
| Mining | ✅ | ✅ | ✅ | ✅ (share) |
| GUI | ❌ | ❌ | ✅ | ✅ |
| SPV Mode | ❌ | ❌ | ❌ | ✅ |
| Platform | All | All | Desktop | Mobile |
| Disk Usage | High | Low | High | Low |
| Memory | High | Low | Medium | Low |

## Building

### All Clients

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Individual Clients

```bash
# Build only daemon
cmake -DBUILD_DAEMON=ON -DBUILD_CLI=OFF -DBUILD_GUI=OFF ..
make parthenond

# Build only CLI
cmake -DBUILD_DAEMON=OFF -DBUILD_CLI=ON -DBUILD_GUI=OFF ..
make parthenon-cli

# Build only GUI
cmake -DBUILD_DAEMON=OFF -DBUILD_CLI=OFF -DBUILD_GUI=ON ..
make parthenon-qt
```

### Mobile

```bash
cd mobile/react-native

# Install dependencies
npm install

# iOS
cd ios && pod install && cd ..
npx react-native run-ios

# Android
npx react-native run-android
```

## Configuration

### Daemon (parthenond.conf)

```conf
# Network
network.port=8333
network.max_connections=125

# RPC
rpc.enabled=true
rpc.port=8332
rpc.user=username
rpc.password=password

# Data
data_dir=~/.parthenon/data
log_level=info

# Mining
mining.enabled=false
```

### CLI

Uses same config file as daemon. Connects via RPC.

### Desktop GUI

Settings available through GUI preferences.

### Mobile

Settings available in app settings screen.

## RPC Interface

All clients use RPC for communication with the node.

**Common RPC Methods**:
- `getinfo` - Get node information
- `getblockchaininfo` - Get blockchain state
- `getbalance` - Get wallet balance
- `sendtoaddress` - Send transaction
- `listunspent` - List UTXOs
- `getblock` - Get block data
- `gettransaction` - Get transaction data

**Example**:
```bash
curl --user username:password \
  --data-binary '{"jsonrpc":"1.0","method":"getinfo"}' \
  http://localhost:8332/
```

## Security

### Full Node (parthenond)

- Secure RPC credentials
- Firewall configuration
- Regular backups
- Keep software updated

### Wallets

- Encrypt wallet file
- Backup seed phrase
- Use strong passwords
- Verify addresses
- Test with small amounts first

## Data Directories

**Linux**:
- Daemon: `~/.parthenon/`
- CLI: Uses daemon's directory
- Desktop: `~/.parthenon/`
- Mobile: App-specific storage

**macOS**:
- Daemon: `~/Library/Application Support/ParthenonChain/`
- CLI: Same as daemon
- Desktop: Same as daemon
- Mobile: App-specific storage

**Windows**:
- Daemon: `C:\Users\<user>\AppData\Roaming\ParthenonChain\`
- CLI: Same as daemon
- Desktop: Same as daemon

## Networking

**Ports**:
- P2P: 8333 (TCP)
- RPC: 8332 (TCP, localhost only by default)
- Mining Stratum: 3333 (TCP, optional)

**Firewall Rules**:
```bash
# Allow P2P (required for full nodes)
sudo ufw allow 8333/tcp

# Allow RPC from localhost only (don't expose to internet)
sudo ufw allow from 127.0.0.1 to any port 8332
```

## Testing

```bash
# Test all clients
ctest -R clients

# Test individual clients
ctest -R parthenond
ctest -R parthenon-cli
```

## Deployment

### Production Deployment

1. **Node Operators**: Run `parthenond` on server
2. **Exchanges**: Run `parthenond` with RPC for deposits/withdrawals
3. **Users**: Install Desktop GUI or Mobile app
4. **Developers**: Use `parthenon-cli` for automation

### System Requirements

**Full Node**:
- CPU: 2+ cores
- RAM: 4+ GB
- Disk: 50+ GB (SSD recommended)

**Light Client** (Mobile):
- Storage: < 1 GB
- RAM: 512 MB
- Network: Moderate

## Development

### Adding a New RPC Method

1. Define method in `core-daemon/rpc/rpc_server.h`
2. Implement in `core-daemon/rpc/rpc_methods.cpp`
3. Add to CLI in `cli/commands/`
4. Update documentation

### Adding a GUI Feature

1. Add UI in `desktop/gui/`
2. Connect to RPC backend
3. Add tests
4. Update user documentation

## Troubleshooting

### Node won't start

- Check logs: `tail -f ~/.parthenon/debug.log`
- Verify port 8333 is available
- Check disk space

### Can't connect to RPC

- Verify `rpc.enabled=true`
- Check credentials
- Ensure connecting to correct port

### Slow sync

- Check internet speed
- Verify sufficient disk I/O
- Consider more connections

## Documentation

- [parthenond README](core-daemon/README.md)
- [parthenon-cli README](cli/README.md)
- [Desktop GUI README](desktop/README.md)
- [Mobile README](mobile/react-native/README.md)
- [Phase 9 Completion Report](../docs/PHASE9_COMPLETION.md)

## Support

For issues or questions:
- GitHub Issues: https://github.com/Tsoympet/PantheonChain/issues
- Main README: [../README.md](../README.md)

---

**Clients are under active development; see the implementation gap audit for readiness status.**
