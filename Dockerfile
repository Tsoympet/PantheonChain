# Multi-stage Dockerfile for PantheonChain
# Stage 1: Builder
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libboost-program-options-dev \
    pkg-config \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Initialize and update submodules
RUN git submodule update --init --recursive || true

# Create build directory and compile
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Run tests
RUN cd build && ctest --output-on-failure || true

# Stage 2: Runtime
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-thread1.74.0 \
    libboost-program-options1.74.0 \
    && rm -rf /var/lib/apt/lists/*

# Create parthenon user
RUN useradd -m -u 1000 -s /bin/bash parthenon

# Create data directories
RUN mkdir -p /home/parthenon/.parthenon/data && \
    chown -R parthenon:parthenon /home/parthenon/.parthenon

# Copy binaries from builder
COPY --from=builder /build/build/clients/core-daemon/parthenond /usr/local/bin/
COPY --from=builder /build/build/clients/cli/parthenon-cli /usr/local/bin/
COPY --from=builder /build/build/relayers/pantheon-relayer-l2 /usr/local/bin/
COPY --from=builder /build/build/relayers/pantheon-relayer-l3 /usr/local/bin/

# Copy configuration
COPY --from=builder /build/build/clients/core-daemon/parthenond.conf /etc/parthenon/

# Copy documentation
COPY README.md WHITEPAPER.md EULA.md LICENSE /usr/share/doc/parthenon/

# Set permissions
RUN chmod +x /usr/local/bin/parthenond /usr/local/bin/parthenon-cli /usr/local/bin/pantheon-relayer-l2 /usr/local/bin/pantheon-relayer-l3

# Switch to parthenon user
USER parthenon
WORKDIR /home/parthenon

# Expose ports
# P2P port
EXPOSE 8333
# RPC port
EXPOSE 8332

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD parthenon-cli getinfo || exit 1

# Default command
CMD ["parthenond"]
