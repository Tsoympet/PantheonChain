# PantheonChain

A decentralized blockchain network built for scalability and security.

## Building from Source

To build PantheonChain from source:

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils

# Clone the repository
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Build
./autogen.sh
./configure
make
make install
```

### Running with Docker (Recommended)

The easiest way to run a ParthenonChain node is using Docker:

```bash
# Clone the repository
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Using Docker Compose (recommended)
docker-compose up -d

# Or build and run manually
docker build -t parthenon-node .
docker run -d \
  -p 8333:8333 \
  -p 8332:8332 \
  -v parthenon-data:/home/parthenon/.parthenon \
  --name parthenon-node \
  parthenon-node
```

**Docker commands:**
```bash
# View logs
docker-compose logs -f parthenond

# Stop node
docker-compose down

# Access CLI
docker exec -it parthenon-node parthenon-cli getinfo

# Restart node
docker-compose restart
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
