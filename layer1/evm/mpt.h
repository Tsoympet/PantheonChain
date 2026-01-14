// ParthenonChain - Merkle Patricia Trie Implementation
// Production-grade MPT for EVM state root calculation

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace parthenon {
namespace evm {

/**
 * Merkle Patricia Trie implementation for Ethereum-compatible state roots
 *
 * This implements a modified Merkle Patricia Trie as specified in the
 * Ethereum Yellow Paper, using RLP encoding and Keccak-256 hashing.
 *
 * For now, we use SHA-256 instead of Keccak-256 for simplicity while
 * maintaining the MPT structure.
 */
class MerklePatriciaTrie {
  public:
    using Hash = std::array<uint8_t, 32>;
    using Key = std::vector<uint8_t>;
    using Value = std::vector<uint8_t>;

    MerklePatriciaTrie();
    ~MerklePatriciaTrie();

    /**
     * Insert or update a key-value pair
     */
    void Put(const Key& key, const Value& value);

    /**
     * Get a value by key
     */
    std::optional<Value> Get(const Key& key) const;

    /**
     * Delete a key
     */
    void Delete(const Key& key);

    /**
     * Calculate and return the root hash
     * This is the Merkle root of the trie
     */
    Hash GetRootHash() const;

    /**
     * Clear the trie
     */
    void Clear();

  private:
    struct Node;
    using NodePtr = std::shared_ptr<Node>;

    /**
     * Node types in the MPT
     */
    enum class NodeType {
        EMPTY,      // Empty node (null)
        LEAF,       // Leaf node (key path + value)
        EXTENSION,  // Extension node (shared path + child)
        BRANCH      // Branch node (16 children + optional value)
    };

    /**
     * MPT Node
     */
    struct Node {
        NodeType type;
        std::vector<uint8_t> path;      // Nibble path (for leaf/extension)
        std::vector<uint8_t> value;     // Value (for leaf/branch)
        std::vector<NodePtr> children;  // Children (for branch: 16 + 1 for value)

        Node() : type(NodeType::EMPTY) {}
    };

    // Root node
    NodePtr root_;

    // Helper functions
    NodePtr Insert(NodePtr node, const std::vector<uint8_t>& nibbles, size_t idx,
                   const Value& value);
    std::optional<Value> Lookup(const NodePtr& node, const std::vector<uint8_t>& nibbles,
                                size_t idx) const;
    NodePtr Remove(NodePtr node, const std::vector<uint8_t>& nibbles, size_t idx);

    // Convert byte key to nibbles (hex digits)
    static std::vector<uint8_t> ToNibbles(const Key& key);

    // Convert nibbles back to bytes
    static Key FromNibbles(const std::vector<uint8_t>& nibbles);

    // Hash a node
    Hash HashNode(const NodePtr& node) const;

    // Encode node to bytes (simplified RLP-like encoding)
    std::vector<uint8_t> EncodeNode(const NodePtr& node) const;

    // Find common prefix length
    static size_t CommonPrefixLength(const std::vector<uint8_t>& a, size_t a_start,
                                     const std::vector<uint8_t>& b, size_t b_start);
};

}  // namespace evm
}  // namespace parthenon
