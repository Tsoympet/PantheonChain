// ParthenonChain - Block Primitives
// Consensus-critical: Block structure and Merkle tree

#ifndef PARTHENON_PRIMITIVES_BLOCK_H
#define PARTHENON_PRIMITIVES_BLOCK_H

#include "transaction.h"

#include <array>
#include <vector>

namespace parthenon {
namespace primitives {

/**
 * BlockHeader contains the consensus-critical block metadata
 * All consensus rules are enforced based on this header
 */
struct BlockHeader {
    uint32_t version;                         // Block version
    std::array<uint8_t, 32> prev_block_hash;  // Previous block hash
    std::array<uint8_t, 32> merkle_root;      // Merkle root of transactions
    uint32_t timestamp;                       // Block timestamp (Unix epoch)
    uint32_t bits;                            // Difficulty target (compact format)
    uint32_t nonce;                           // Proof-of-work nonce
    uint64_t base_fee_per_gas;                // EIP-1559 base fee (for EVM transactions)
    uint64_t gas_used;                        // Total gas used by EVM transactions
    uint64_t gas_limit;                       // Gas limit for EVM transactions

    BlockHeader()
        : version(1),
          prev_block_hash{},
          merkle_root{},
          timestamp(0),
          bits(0),
          nonce(0),
          base_fee_per_gas(1000000000),  // 1 Gwei initial base fee
          gas_used(0),
          gas_limit(30000000) {}  // 30M gas limit

    /**
     * Get block hash (SHA-256d of serialized header)
     * This is the canonical block identifier
     */
    std::array<uint8_t, 32> GetHash() const;

    /**
     * Serialize block header (104 bytes: 80 bytes Bitcoin-like + 24 bytes EVM fields)
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize block header
     */
    static BlockHeader Deserialize(const uint8_t* data);

    /**
     * Check if this block hash meets the difficulty target
     */
    bool MeetsDifficultyTarget() const;
};

/**
 * Block contains the header and all transactions
 */
class Block {
  public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    Block() = default;

    /**
     * Get block hash (from header)
     */
    std::array<uint8_t, 32> GetHash() const { return header.GetHash(); }

    /**
     * Calculate merkle root from transactions
     */
    std::array<uint8_t, 32> CalculateMerkleRoot() const;

    /**
     * Serialize complete block
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize complete block
     */
    static std::optional<Block> Deserialize(const uint8_t* data, size_t len);

    /**
     * Validate block structure
     * - Must have at least one transaction (coinbase)
     * - First transaction must be coinbase
     * - Only first transaction can be coinbase
     * - Merkle root must match calculated value
     * - All transactions must be valid
     * - Block hash must meet difficulty target
     */
    bool IsValid() const;

    /**
     * Check if this is the genesis block
     */
    bool IsGenesis() const { return header.prev_block_hash == std::array<uint8_t, 32>{}; }
};

/**
 * Merkle tree operations
 */
class MerkleTree {
  public:
    /**
     * Calculate merkle root from transaction IDs
     */
    static std::array<uint8_t, 32>
    CalculateRoot(const std::vector<std::array<uint8_t, 32>>& hashes);

    /**
     * Calculate merkle root from transactions
     */
    static std::array<uint8_t, 32> CalculateRoot(const std::vector<Transaction>& transactions);

  private:
    /**
     * Hash two nodes together
     */
    static std::array<uint8_t, 32> HashNodes(const std::array<uint8_t, 32>& left,
                                             const std::array<uint8_t, 32>& right);
};

}  // namespace primitives
}  // namespace parthenon

#endif  // PARTHENON_PRIMITIVES_BLOCK_H
