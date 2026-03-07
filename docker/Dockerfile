FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build \
    libssl-dev \
    libboost-system-dev libboost-filesystem-dev \
    libboost-thread-dev libboost-program-options-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja \
    && cmake --build build -j"$(nproc)"

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.83.0 libboost-filesystem1.83.0 \
    libboost-thread1.83.0 libboost-program-options1.83.0 \
    curl \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /build/build/pantheon-l1-talanton-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-l2-drachma-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-l3-obolos-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-cli /usr/local/bin/
COPY --from=builder /build/build/pantheon-relayer-l2 /usr/local/bin/
COPY --from=builder /build/build/pantheon-relayer-l3 /usr/local/bin/
CMD ["pantheon-l1-talanton-node","--layer=l1"]
