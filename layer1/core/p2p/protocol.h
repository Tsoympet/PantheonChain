// ParthenonChain - Network Protocol
// P2P message types and protocol constants

#ifndef PARTHENON_P2P_PROTOCOL_H
#define PARTHENON_P2P_PROTOCOL_H

#include <cstdint>
#include <string>

namespace parthenon {
namespace p2p {

/**
 * Protocol version
 */
static constexpr uint32_t PROTOCOL_VERSION = 70001;

/**
 * Minimum supported protocol version
 */
static constexpr uint32_t MIN_PROTOCOL_VERSION = 70001;

/**
 * Network magic bytes (identifies the network)
 * Different for mainnet, testnet, regtest
 */
struct NetworkMagic {
    static constexpr uint32_t MAINNET = 0xD9B4BEF9;
    static constexpr uint32_t TESTNET = 0x0B110907;
    static constexpr uint32_t REGTEST = 0xDAB5BFFA;
};

/**
 * Message types
 */
enum class MessageType : uint32_t {
    // Handshake
    VERSION = 0x76657273,  // "vers"
    VERACK = 0x76657261,   // "vera"

    // Connectivity
    PING = 0x70696E67,     // "ping"
    PONG = 0x706F6E67,     // "pong"
    ADDR = 0x61646472,     // "addr"
    GETADDR = 0x67657461,  // "geta"

    // Inventory
    INV = 0x696E7600,       // "inv\0"
    GETDATA = 0x67657464,   // "getd"
    NOTFOUND = 0x6E6F7466,  // "notf"

    // Blocks
    GETBLOCKS = 0x67657462,   // "getb"
    GETHEADERS = 0x67657468,  // "geth"
    BLOCK = 0x626C6F63,       // "bloc"
    HEADERS = 0x68656164,     // "head"

    // Transactions
    TX = 0x74780000,       // "tx\0\0"
    MEMPOOL = 0x6D656D70,  // "memp"

    // Other
    REJECT = 0x72656A65,  // "reje"
    ALERT = 0x616C6572,   // "aler"
};

/**
 * Inventory types
 */
enum class InvType : uint32_t {
    ERROR = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
    MSG_FILTERED_BLOCK = 3,
};

/**
 * Service flags (what services a node provides)
 */
enum class ServiceFlags : uint64_t {
    NODE_NONE = 0,
    NODE_NETWORK = (1 << 0),           // Can provide full blocks
    NODE_GETUTXO = (1 << 1),           // Can respond to UTXO queries
    NODE_BLOOM = (1 << 2),             // Can filter blocks/txs using bloom filters
    NODE_WITNESS = (1 << 3),           // Supports witness data
    NODE_NETWORK_LIMITED = (1 << 10),  // Provides last 288 blocks
};

/**
 * Network constants
 */
static constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;  // 32 MB
static constexpr size_t MAX_HEADERS_COUNT = 2000;
static constexpr size_t MAX_INV_SIZE = 50000;
static constexpr size_t MAX_ADDR_TO_SEND = 1000;
static constexpr size_t MAX_PROTOCOL_MESSAGE_LENGTH = 4 * 1024 * 1024;  // 4 MB

/**
 * Timeouts (in seconds)
 */
static constexpr uint32_t TIMEOUT_INTERVAL = 20 * 60;  // 20 minutes
static constexpr uint32_t PING_INTERVAL = 2 * 60;      // 2 minutes
static constexpr uint32_t FEELER_INTERVAL = 2 * 60;    // 2 minutes

/**
 * Connection limits
 */
static constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;
static constexpr size_t MAX_INBOUND_CONNECTIONS = 117;
static constexpr size_t MAX_CONNECTIONS = MAX_OUTBOUND_CONNECTIONS + MAX_INBOUND_CONNECTIONS;

/**
 * DoS protection
 */
static constexpr size_t MAX_ORPHAN_TRANSACTIONS = 100;
static constexpr uint32_t ORPHAN_TX_EXPIRE_TIME = 20 * 60;  // 20 minutes
static constexpr size_t MAX_REJECT_MESSAGE_LENGTH = 111;

/**
 * Network address structure
 */
struct NetAddr {
    uint64_t services;
    uint8_t ip[16];  // IPv6 address (IPv4 mapped)
    uint16_t port;
    uint32_t time;  // Last seen time

    NetAddr() : services(0), ip{}, port(0), time(0) {}

    /**
     * Check if this is an IPv4 address
     */
    bool IsIPv4() const;

    /**
     * Check if this is a valid routable address
     */
    bool IsRoutable() const;
};

}  // namespace p2p
}  // namespace parthenon

#endif  // PARTHENON_P2P_PROTOCOL_H
