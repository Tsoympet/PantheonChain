// ParthenonChain - Zero-Copy Networking
// High-performance networking using sendfile, splice, and memory mapping

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace parthenon {
namespace p2p {

/**
 * Zero-copy network operations for maximum throughput
 * Uses sendfile(), splice(), and mmap() on Linux
 */
class ZeroCopyNetwork {
  public:
    /**
     * Send file descriptor contents without copying to userspace
     * @param socket_fd Destination socket
     * @param file_fd Source file descriptor
     * @param offset Offset in file
     * @param count Bytes to send
     * @return Bytes sent, or -1 on error
     */
    static ssize_t SendFile(int socket_fd, int file_fd, off_t offset, size_t count);

    /**
     * Splice data between two file descriptors without userspace copy
     * @param fd_in Input file descriptor
     * @param fd_out Output file descriptor
     * @param len Number of bytes to splice
     * @return Bytes spliced, or -1 on error
     */
    static ssize_t Splice(int fd_in, int fd_out, size_t len);

    /**
     * Memory-map a file for zero-copy access
     * @param file_path Path to file
     * @param size File size
     * @return Pointer to mapped memory, or nullptr on error
     */
    static void* MemoryMapFile(const std::string& file_path, size_t& size);

    /**
     * Unmap memory-mapped file
     */
    static bool UnmapFile(void* addr, size_t size);

    /**
     * Check if zero-copy operations are available
     */
    static bool IsAvailable();

    /**
     * Send data using most efficient method available
     * Falls back to regular send() if zero-copy unavailable
     */
    static ssize_t OptimizedSend(int socket_fd, const void* data, size_t len);

    /**
     * Receive data using most efficient method available
     */
    static ssize_t OptimizedRecv(int socket_fd, void* buffer, size_t len);
};

/**
 * DPDK (Data Plane Development Kit) integration for kernel bypass
 * Direct userspace networking for maximum performance
 */
class DPDKNetwork {
  public:
    /**
     * Initialize DPDK with given configuration
     * @param config DPDK EAL parameters
     * @return true if initialized successfully
     */
    bool Init(const std::vector<std::string>& config);

    /**
     * Setup port for packet I/O
     * @param port_id Physical port ID
     * @param rx_queues Number of RX queues
     * @param tx_queues Number of TX queues
     */
    bool SetupPort(uint16_t port_id, uint16_t rx_queues, uint16_t tx_queues);

    /**
     * Send packet batch (zero-copy)
     * @param port_id Port to send on
     * @param queue_id TX queue ID
     * @param packets Array of packet pointers
     * @param count Number of packets
     * @return Number of packets sent
     */
    uint16_t SendBurst(uint16_t port_id, uint16_t queue_id, void** packets, uint16_t count);

    /**
     * Receive packet batch (zero-copy)
     * @param port_id Port to receive from
     * @param queue_id RX queue ID
     * @param packets Buffer for packet pointers
     * @param max_count Maximum packets to receive
     * @return Number of packets received
     */
    uint16_t ReceiveBurst(uint16_t port_id, uint16_t queue_id, void** packets, uint16_t max_count);

    /**
     * Check if DPDK is available
     */
    static bool IsAvailable();

    /**
     * Get statistics for port
     */
    std::string GetPortStats(uint16_t port_id);

    void Shutdown();

  private:
    bool initialized_ = false;
    uint16_t num_ports_ = 0;
};

}  // namespace p2p
}  // namespace parthenon
