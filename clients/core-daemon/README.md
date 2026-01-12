# parthenond - ParthenonChain Node Daemon

The full node daemon for ParthenonChain blockchain.

## Usage

```bash
./parthenond [config_file]
```

If no config file is specified, `parthenond.conf` in the current directory will be used.

## Configuration

Edit `parthenond.conf` to configure the node:

### Network Settings
- `network.port` - P2P network port (default: 8333)
- `network.max_connections` - Maximum peer connections (default: 125)
- `network.timeout` - Network timeout in seconds (default: 60)

### RPC Settings
- `rpc.enabled` - Enable RPC server (default: true)
- `rpc.port` - RPC server port (default: 8332)
- `rpc.user` - RPC username
- `rpc.password` - RPC password (CHANGE THIS!)

### Data Settings
- `data_dir` - Blockchain data directory (default: ./data)
- `log_level` - Logging level: debug, info, warning, error (default: info)

### Mining
- `mining.enabled` - Enable mining (default: false)

## Shutdown

The daemon responds to SIGINT (Ctrl+C) and SIGTERM for graceful shutdown.

## Example

```bash
# Start with default config
./parthenond

# Start with custom config
./parthenond /etc/parthenon/parthenond.conf
```
