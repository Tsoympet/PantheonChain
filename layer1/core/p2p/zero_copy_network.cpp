// ParthenonChain - Zero-Copy Networking Implementation

#include "zero_copy_network.h"

#include <cstring>
#include <iostream>

// Platform-specific headers
#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

namespace parthenon {
namespace p2p {

// ============================================================================
// Zero-Copy Network Operations
// ============================================================================

ssize_t ZeroCopyNetwork::SendFile(int socket_fd, int file_fd, off_t offset, size_t count) {
#ifdef __linux__
    // Use sendfile() system call for zero-copy transfer
    off_t off = offset;
    ssize_t sent = sendfile(socket_fd, file_fd, &off, count);

    if (sent < 0) {
        std::cerr << "sendfile() failed: " << strerror(errno) << "\n";
        return -1;
    }

    return sent;
#else
    std::cerr << "sendfile() not available on this platform\n";
    return -1;
#endif
}

ssize_t ZeroCopyNetwork::Splice(int fd_in, int fd_out, size_t len) {
#ifdef __linux__
    // Use splice() for zero-copy pipe transfer
    ssize_t spliced = splice(fd_in, nullptr, fd_out, nullptr, len, SPLICE_F_MOVE | SPLICE_F_MORE);

    if (spliced < 0) {
        std::cerr << "splice() failed: " << strerror(errno) << "\n";
        return -1;
    }

    return spliced;
#else
    std::cerr << "splice() not available on this platform\n";
    return -1;
#endif
}

void* ZeroCopyNetwork::MemoryMapFile(const std::string& file_path, size_t& size) {
#ifdef __linux__
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << file_path << "\n";
        return nullptr;
    }

    // Get file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        close(fd);
        return nullptr;
    }

    size = static_cast<size_t>(file_size);

    // Memory map the file
    void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // Can close FD after mmap

    if (addr == MAP_FAILED) {
        std::cerr << "mmap() failed: " << strerror(errno) << "\n";
        return nullptr;
    }

    return addr;
#else
    std::cerr << "mmap() not available on this platform\n";
    return nullptr;
#endif
}

bool ZeroCopyNetwork::UnmapFile(void* addr, size_t size) {
#ifdef __linux__
    if (munmap(addr, size) < 0) {
        std::cerr << "munmap() failed: " << strerror(errno) << "\n";
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool ZeroCopyNetwork::IsAvailable() {
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

ssize_t ZeroCopyNetwork::OptimizedSend(int socket_fd, const void* data, size_t len) {
    // Try MSG_ZEROCOPY flag (Linux 4.14+)
#ifdef MSG_ZEROCOPY
    ssize_t sent = send(socket_fd, data, len, MSG_ZEROCOPY | MSG_DONTWAIT);
    if (sent >= 0) {
        return sent;
    }
#endif

    // Fallback to regular send
    return send(socket_fd, data, len, MSG_DONTWAIT);
}

ssize_t ZeroCopyNetwork::OptimizedRecv(int socket_fd, void* buffer, size_t len) {
    return recv(socket_fd, buffer, len, MSG_DONTWAIT);
}

// ============================================================================
// DPDK Network Implementation
// ============================================================================

bool DPDKNetwork::Init(const std::vector<std::string>& config) {
    if (initialized_) {
        return true;
    }

    if (!IsAvailable()) {
        std::cerr << "DPDK not available on this system\n";
        std::cout << "NOTE: Install DPDK library for kernel bypass networking\n";
        std::cout << "      Continuing with standard socket API\n";
        return false;
    }

    // TODO: Initialize DPDK EAL (Environment Abstraction Layer)
    // Convert config to argc/argv
    // rte_eal_init(argc, argv);

    // For now, mark as unavailable
    std::cout << "DPDK initialization skipped (library not linked)\n";
    std::cout << "To enable: Install DPDK and link with -lrte_eal -lrte_ethdev\n";

    return false;
}

bool DPDKNetwork::SetupPort(uint16_t port_id, uint16_t rx_queues, uint16_t tx_queues) {
    if (!initialized_) {
        return false;
    }

    // TODO: Configure port
    // rte_eth_dev_configure(port_id, rx_queues, tx_queues, &port_conf);
    // Setup RX queues
    // Setup TX queues
    // rte_eth_dev_start(port_id);

    return false;
}

uint16_t DPDKNetwork::SendBurst(uint16_t port_id, uint16_t queue_id, void** packets,
                                uint16_t count) {
    if (!initialized_) {
        return 0;
    }

    // TODO: Send packet burst
    // return rte_eth_tx_burst(port_id, queue_id, (struct rte_mbuf**)packets, count);

    return 0;
}

uint16_t DPDKNetwork::ReceiveBurst(uint16_t port_id, uint16_t queue_id, void** packets,
                                   uint16_t max_count) {
    if (!initialized_) {
        return 0;
    }

    // TODO: Receive packet burst
    // return rte_eth_rx_burst(port_id, queue_id, (struct rte_mbuf**)packets, max_count);

    return 0;
}

bool DPDKNetwork::IsAvailable() {
    // Check if DPDK library is available
    // TODO: Try to dlopen librte_eal.so
    return false;
}

std::string DPDKNetwork::GetPortStats(uint16_t port_id) {
    if (!initialized_) {
        return "DPDK not initialized";
    }

    // TODO: Get port statistics
    // struct rte_eth_stats stats;
    // rte_eth_stats_get(port_id, &stats);

    return "Port " + std::to_string(port_id) + " stats unavailable";
}

void DPDKNetwork::Shutdown() {
    if (initialized_) {
        // TODO: Stop all ports and cleanup
        // rte_eal_cleanup();
        initialized_ = false;
    }
}

}  // namespace p2p
}  // namespace parthenon
