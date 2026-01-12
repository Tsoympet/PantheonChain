# parthenon-cli - ParthenonChain RPC Client

Command-line client for interacting with parthenond via RPC.

## Usage

### Interactive Mode

```bash
./parthenon-cli
```

This starts an interactive shell where you can type commands.

### Batch Mode

```bash
./parthenon-cli <command> [arguments...]
```

Execute a single command and exit.

## Commands

### getinfo
Get node information including version, block height, and connection count.

```bash
./parthenon-cli getinfo
```

### getblockcount
Get the current blockchain height.

```bash
./parthenon-cli getblockcount
```

### getbalance
Get wallet balance for an asset (default: TALN).

```bash
./parthenon-cli getbalance TALN
./parthenon-cli getbalance DRM
./parthenon-cli getbalance OBL
```

### sendtoaddress
Send a transaction to an address.

```bash
./parthenon-cli sendtoaddress TALN <address> <amount>
```

### stop
Shutdown the daemon.

```bash
./parthenon-cli stop
```

### help
Show available commands.

```bash
./parthenon-cli help
```

## Examples

```bash
# Get node info
./parthenon-cli getinfo

# Check TALN balance
./parthenon-cli getbalance TALN

# Send 10 TALN
./parthenon-cli sendtoaddress TALN addr1... 10.0

# Interactive mode
./parthenon-cli
parthenon> getinfo
parthenon> getbalance DRM
parthenon> quit
```
