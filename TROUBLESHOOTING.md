# Troubleshooting Guide

Common issues and solutions for ParthenonChain.

## Installation Issues

### Windows: "Windows protected your PC"

**Problem**: Windows SmartScreen blocks the installer.

**Solution**:
1. This is normal for new software without widespread download history
2. Click "More info"
3. Click "Run anyway"
4. Alternatively, right-click the installer → Properties → Check "Unblock" → OK

**Why this happens**: Windows SmartScreen doesn't recognize less-downloaded applications initially.

### macOS: "Cannot be opened because the developer cannot be verified"

**Problem**: macOS Gatekeeper blocks the application.

**Solution**:
1. Right-click (or Control+click) the ParthenonChain app
2. Select "Open"
3. Click "Open" in the confirmation dialog
4. This only needs to be done once

**Alternative**:
1. Go to System Preferences → Security & Privacy
2. Click the lock to make changes
3. Click "Open Anyway" next to the ParthenonChain message

**Why this happens**: macOS requires notarization for downloaded apps. Future releases will be notarized.

### Linux: dpkg/rpm errors during installation

**Problem**: Missing dependencies.

**Ubuntu/Debian**:
```bash
# Fix dependencies
sudo apt-get install -f

# Or install manually
sudo apt-get install libssl3 libboost-system1.74.0 libboost-filesystem1.74.0
```

**RHEL/Fedora**:
```bash
# Install dependencies
sudo dnf install openssl boost-system boost-filesystem
```

### Linux: Service won't start

**Check logs**:
```bash
sudo journalctl -u parthenond -f
```

**Common causes**:
1. Port 8333 already in use:
   ```bash
   sudo lsof -i :8333
   # Kill conflicting process or change port in config
   ```

2. Insufficient permissions:
   ```bash
   sudo chown -R parthenon:parthenon /var/lib/parthenon
   ```

3. Corrupted data directory:
   ```bash
   sudo systemctl stop parthenond
   sudo rm -rf /var/lib/parthenon/*
   sudo systemctl start parthenond
   ```

## Sync Issues

### Node won't sync / Stuck at specific block

**Check sync status**:
```bash
parthenon-cli getblockchaininfo
```

**Solutions**:

1. **Check internet connection**:
   ```bash
   ping 8.8.8.8
   ```

2. **Ensure P2P port is open**:
   ```bash
   # Check if port 8333 is accessible
   telnet your-public-ip 8333
   ```

3. **Add peers manually**:
   ```bash
   parthenon-cli addnode "seed.parthenonchain.org:8333" "add"
   parthenon-cli addnode "node.parthenonchain.org:8333" "add"
   ```

4. **Check firewall**:
   ```bash
   # Linux
   sudo ufw allow 8333/tcp
   
   # Windows
   # Open Windows Firewall → Allow app → Add parthenond.exe
   
   # macOS
   # System Preferences → Security & Privacy → Firewall → Firewall Options
   ```

5. **Restart with reindex** (if blockchain data is corrupted):
   ```bash
   parthenond -reindex
   ```

### Sync is very slow

**Possible causes and solutions**:

1. **Slow internet**: Check bandwidth usage and limit other downloads
2. **HDD instead of SSD**: Consider moving data directory to SSD
3. **Low RAM**: Close other applications, increase swap space
4. **Too few connections**:
   ```bash
   # Increase max connections in parthenond.conf
   network.max_connections=150
   ```

### "Verificationprogress stuck at 99.9%"

**This is normal!** The last blocks take longer to verify due to:
- More transactions per block
- Smart contract execution
- UTXO set validation

**Wait 15-30 minutes** before assuming there's a problem.

## Wallet Issues

### "Insufficient funds" when sending

**Possible causes**:

1. **Not enough balance**:
   ```bash
   parthenon-cli getbalance
   ```

2. **Wrong asset type**:
   ```bash
   # Make sure you're sending the right asset
   parthenon-cli getbalance TALANTON
   parthenon-cli getbalance DRACHMA
   parthenon-cli getbalance OBOLOS
   ```

3. **Need OBOLOS for transaction fees**:
   - All transactions require OBOLOS for fees
   - Acquire some OBOLOS first

4. **Funds are unconfirmed**:
   ```bash
   # Check unconfirmed balance
   parthenon-cli getunconfirmedbalance
   ```

### Can't create new address

**Check wallet status**:
```bash
parthenon-cli getwalletinfo
```

**Solutions**:

1. **Wallet is locked**:
   ```bash
   parthenon-cli walletpassphrase "your-password" 60
   ```

2. **Wallet not created**:
   ```bash
   parthenon-cli createwallet "my-wallet"
   ```

3. **Disk full**:
   ```bash
   df -h
   # Free up space if needed
   ```

### Lost wallet password

**Unfortunately, there is no recovery option for lost passwords.**

**Prevention**:
- Always backup your wallet seed phrase
- Store password securely (password manager)
- Test your backup before storing large amounts

**If you have the seed phrase**:
```bash
parthenon-cli restorewallet "seed phrase here"
```

### Wallet file corrupted

**Symptoms**: Errors when accessing wallet, missing transactions

**Recovery**:

1. **From backup**:
   ```bash
   # Stop daemon
   parthenon-cli stop
   
   # Replace wallet file
   cp /path/to/backup/wallet.dat ~/.parthenon/wallet.dat
   
   # Restart
   parthenond -daemon
   ```

2. **From seed phrase**:
   ```bash
   parthenon-cli restorewallet "your twelve or twenty four word seed phrase"
   ```

3. **Salvage existing wallet** (last resort):
   ```bash
   parthenond -salvagewallet
   ```

## Transaction Issues

### Transaction not confirming

**Check transaction status**:
```bash
parthenon-cli gettransaction <txid>
```

**Possible reasons**:

1. **Low fee**: Transaction may take longer to confirm
   - Wait longer (could be hours)
   - Consider using higher fee next time

2. **Network congestion**: Many transactions waiting
   - Check mempool: `parthenon-cli getmempoolinfo`
   - Wait for congestion to clear

3. **Transaction stuck in mempool**:
   ```bash
   # Check if still in mempool
   parthenon-cli getrawmempool
   
   # If very old, might need to resend with higher fee
   # (Use RBF - Replace By Fee if supported)
   ```

4. **Double-spend detected**: Transaction conflicts with another
   - Check wallet for conflicting transactions
   - Wait for one to confirm (the other will be rejected)

### "Transaction already in block chain" error

**This means the transaction already succeeded!**

Check blockchain explorer or:
```bash
parthenon-cli gettransaction <txid>
```

If confirmed, the transaction is complete.

### Invalid address error

**Check address format**:
- ParthenonChain addresses start with "PTH1" for mainnet
- Ensure you copied the full address
- No extra spaces or characters

**Verify address**:
```bash
parthenon-cli validateaddress <address>
```

## Mining Issues

### Mining not finding blocks

**This is normal for solo mining!**

**Expected time to find a block**:
- Depends on network hashrate and your hashrate
- Could be hours, days, or weeks for solo miners

**Solutions**:

1. **Join a mining pool**: Much more consistent rewards
2. **Increase hashrate**: Add more miners/GPUs
3. **Check mining status**:
   ```bash
   parthenon-cli getmininginfo
   ```

### GPU mining not working

**Check GPU support**:
```bash
parthenond --help-mining
```

**Common issues**:

1. **CUDA/OpenCL not installed**:
   - Install NVIDIA CUDA Toolkit or AMD OpenCL
   - Restart after installation

2. **Driver version too old**:
   - Update graphics drivers
   - NVIDIA: 450+ for CUDA 11
   - AMD: Adrenalin 20.4+

3. **GPU not detected**:
   ```bash
   # Check if GPU is visible
   nvidia-smi  # NVIDIA
   clinfo      # AMD/OpenCL
   ```

### High CPU/memory usage during mining

**This is expected behavior!** Mining is computationally intensive.

**Solutions**:
1. Reduce thread count:
   ```bash
   parthenon-cli setgenerate true 2  # Use only 2 threads
   ```

2. Use GPU mining instead of CPU

3. Limit mining hours (stop during high usage times)

## RPC Issues

### "Could not connect to server"

**Check if daemon is running**:
```bash
# Linux/macOS
ps aux | grep parthenond

# Windows
tasklist | findstr parthenond
```

**If not running**:
```bash
parthenond -daemon
```

**Check RPC credentials**:
```bash
# In parthenond.conf
rpc.enabled=true
rpc.user=yourusername
rpc.password=yourpassword
# Optional dev-only override:
# rpc.allow_unauthenticated=true
```

**Check RPC port**:
```bash
# Default is 8332
rpc.port=8332
```

**Test connection**:
```bash
curl --user username:password --data-binary '{"jsonrpc":"1.0","method":"getinfo"}' http://localhost:8332/
```

### "403 Forbidden" RPC error

**Problem**: RPC credentials incorrect.

**Solution**: Check `parthenond.conf` username and password match what you're using in the RPC call.

### "Refusing to start RPC server without credentials"

**Problem**: RPC is enabled but `rpc.user`/`rpc.password` are missing.

**Solution**: Set both values in `parthenond.conf`. For local development only, you can set `rpc.allow_unauthenticated=true`.

### "Connection refused" error

**Check**:
1. Is daemon running?
2. Is RPC enabled in config?
3. Is firewall blocking port 8332?
4. Are you connecting to the right IP/port?

## Network Issues

### "No connections" / "0 peers"

**Check network connectivity**:
```bash
parthenon-cli getnetworkinfo
parthenon-cli getpeerinfo
```

**Solutions**:

1. **Check internet connection**
2. **Open P2P port (8333)**:
   ```bash
   sudo ufw allow 8333/tcp
   ```

3. **Add seed nodes manually**:
   ```bash
   parthenon-cli addnode "seed1.parthenonchain.org:8333" "add"
   parthenon-cli addnode "seed2.parthenonchain.org:8333" "add"
   ```

4. **Check if behind NAT/firewall**:
   - Configure port forwarding for port 8333
   - Or run as outbound-only node (will connect but not accept connections)

### Too many orphan blocks

**Check peer connections**:
```bash
parthenon-cli getpeerinfo
```

**Solutions**:
1. Ensure you're fully synced
2. Connect to more peers
3. Check system time is accurate (NTP sync)

## Performance Issues

### High memory usage

**Normal ranges**:
- Full node: 2-4 GB RAM
- With many connections: 4-8 GB RAM

**Reduce memory usage**:
```bash
# In parthenond.conf
dbcache=1024        # Reduce database cache (MB)
maxconnections=50   # Reduce connections
```

### High disk usage / Disk full

**Check disk usage**:
```bash
du -sh ~/.parthenon/
```

**Solutions**:

1. **Prune old blocks** (reduces storage by ~80%):
   ```bash
   # In parthenond.conf
   prune=550  # Keep last 550 MB of blocks
   ```

2. **Move data directory to larger disk**:
   ```bash
   # Stop daemon
   parthenon-cli stop
   
   # Move data
   mv ~/.parthenon /mnt/bigdisk/parthenon
   
   # Create symlink
   ln -s /mnt/bigdisk/parthenon ~/.parthenon
   
   # Restart
   parthenond -daemon
   ```

3. **Clean up logs**:
   ```bash
   rm ~/.parthenon/debug.log.old
   ```

### Slow startup

**Normal**: First startup takes time to load blockchain

**If consistently slow**:
1. Use SSD instead of HDD
2. Increase `dbcache` in config
3. Reduce `maxconnections`

## GUI Issues

### Desktop wallet won't launch

**Check error logs**:
```bash
# Linux/macOS
~/.parthenon/debug.log

# Windows
%APPDATA%\ParthenonChain\debug.log
```

**Common solutions**:

1. **Missing Qt libraries**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install qt6-base qt6-tools
   ```

2. **OpenGL issues**:
   ```bash
   # Try software rendering
   QT_XCB_GL_INTEGRATION=none parthenon-qt
   ```

3. **Display server issues** (Linux):
   ```bash
   # Try with Wayland or X11
   QT_QPA_PLATFORM=wayland parthenon-qt
   QT_QPA_PLATFORM=xcb parthenon-qt
   ```

### GUI crashes on startup

**Try**:
1. Delete Qt settings:
   ```bash
   rm ~/.config/ParthenonChain/ParthenonChain-Qt.conf
   ```

2. Reset window state:
   ```bash
   parthenon-qt -resetguisettings
   ```

3. Run in debug mode:
   ```bash
   parthenon-qt -debug=qt
   ```

## Smart Contract Issues

### Contract deployment fails

**Check**:
1. **Sufficient OBOLOS for gas**:
   ```bash
   parthenon-cli getbalance OBOLOS
   ```

2. **Gas limit too low**: Increase gas limit
3. **Invalid bytecode**: Verify contract compilation
4. **Node not synced**: Wait for full sync

### Contract call reverts

**Common reasons**:
1. Incorrect function parameters
2. Insufficient gas
3. Contract logic rejects call (require/assert failed)
4. Contract state doesn't allow operation

**Debug**:
```bash
parthenon-cli callcontract <address> <data> <gas> --debug
```

## Getting Help

### Check Documentation
- [README.md](README.md) - Main documentation
- [QUICK_START.md](QUICK_START.md) - Getting started guide
- [docs/INSTALLATION.md](docs/INSTALLATION.md) - Installation details
- [clients/README.md](clients/README.md) - Client documentation

### Enable Debug Logging

```bash
# In parthenond.conf
debug=1
debug=net  # Network debugging
debug=rpc  # RPC debugging
debug=db   # Database debugging
```

### Collect Diagnostic Information

When asking for help, include:

1. **Version**:
   ```bash
   parthenond --version
   ```

2. **OS and architecture**:
   ```bash
   uname -a
   ```

3. **Error messages** from:
   - Terminal output
   - `~/.parthenon/debug.log`
   - System logs (`journalctl -u parthenond`)

4. **Configuration** (remove passwords!):
   ```bash
   cat ~/.parthenon/parthenond.conf
   ```

5. **Network status**:
   ```bash
   parthenon-cli getnetworkinfo
   parthenon-cli getpeerinfo
   ```

### Contact Support

- 💬 [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions) - Ask questions
- 🐛 [GitHub Issues](https://github.com/Tsoympet/PantheonChain/issues) - Report bugs
- 📧 Email: support@parthenonchain.org

### Before Reporting a Bug

1. ✅ Search existing issues
2. ✅ Check if fixed in latest version
3. ✅ Reproduce on clean install if possible
4. ✅ Gather diagnostic information (see above)
5. ✅ Write clear steps to reproduce

---

## Quick Fixes Checklist

- [ ] Restart the daemon
- [ ] Check internet connection
- [ ] Verify disk space
- [ ] Check firewall settings
- [ ] Update to latest version
- [ ] Review debug.log for errors
- [ ] Verify configuration file
- [ ] Test with default config
- [ ] Check system time is correct
- [ ] Ensure no other instances running

---

<p align="center">
  <strong>Still having issues? We're here to help!</strong><br/>
  <a href="https://github.com/Tsoympet/PantheonChain/discussions">Ask in Discussions</a> •
  <a href="https://github.com/Tsoympet/PantheonChain/issues">Report a Bug</a>
</p>
