FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y build-essential cmake && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)"

FROM ubuntu:24.04
COPY --from=builder /build/build/pantheon-l1-talanton-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-l2-drachma-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-l3-obolos-node /usr/local/bin/
COPY --from=builder /build/build/pantheon-cli /usr/local/bin/
COPY --from=builder /build/build/pantheon-relayer-l2 /usr/local/bin/
COPY --from=builder /build/build/pantheon-relayer-l3 /usr/local/bin/
CMD ["pantheon-l1-talanton-node","--layer=l1"]
