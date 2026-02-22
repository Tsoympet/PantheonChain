// ParthenonChain - Merkle Patricia Trie Implementation

#include "mpt.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace evm {

MerklePatriciaTrie::MerklePatriciaTrie() : root_(std::make_shared<Node>()) {
    root_->type = NodeType::EMPTY;
}

MerklePatriciaTrie::~MerklePatriciaTrie() = default;

void MerklePatriciaTrie::Put(const Key& key, const Value& value) {
    if (value.empty()) {
        // Empty value = delete
        Delete(key);
        return;
    }

    auto nibbles = ToNibbles(key);
    root_ = Insert(root_, nibbles, 0, value);
}

std::optional<MerklePatriciaTrie::Value> MerklePatriciaTrie::Get(const Key& key) const {
    auto nibbles = ToNibbles(key);
    return Lookup(root_, nibbles, 0);
}

void MerklePatriciaTrie::Delete(const Key& key) {
    auto nibbles = ToNibbles(key);
    root_ = Remove(root_, nibbles, 0);
}

MerklePatriciaTrie::Hash MerklePatriciaTrie::GetRootHash() const {
    return HashNode(root_);
}

void MerklePatriciaTrie::Clear() {
    root_ = std::make_shared<Node>();
    root_->type = NodeType::EMPTY;
}

std::vector<uint8_t> MerklePatriciaTrie::ToNibbles(const Key& key) {
    std::vector<uint8_t> nibbles;
    nibbles.reserve(key.size() * 2);

    for (uint8_t byte : key) {
        nibbles.push_back(byte >> 4);    // High nibble
        nibbles.push_back(byte & 0x0F);  // Low nibble
    }

    return nibbles;
}

MerklePatriciaTrie::Key MerklePatriciaTrie::FromNibbles(const std::vector<uint8_t>& nibbles) {
    Key key;
    key.reserve((nibbles.size() + 1) / 2);

    for (size_t i = 0; i < nibbles.size(); i += 2) {
        uint8_t byte = nibbles[i] << 4;
        if (i + 1 < nibbles.size()) {
            byte |= nibbles[i + 1];
        }
        key.push_back(byte);
    }

    return key;
}

size_t MerklePatriciaTrie::CommonPrefixLength(const std::vector<uint8_t>& a, size_t a_start,
                                              const std::vector<uint8_t>& b, size_t b_start) {
    size_t length = 0;
    while (a_start + length < a.size() && b_start + length < b.size() &&
           a[a_start + length] == b[b_start + length]) {
        length++;
    }
    return length;
}

MerklePatriciaTrie::NodePtr MerklePatriciaTrie::Insert(NodePtr node,
                                                       const std::vector<uint8_t>& nibbles,
                                                       size_t idx, const Value& value) {
    // Base case: reached end of path
    if (idx == nibbles.size()) {
        if (node->type == NodeType::EMPTY) {
            // Create new leaf
            auto leaf = std::make_shared<Node>();
            leaf->type = NodeType::LEAF;
            leaf->path.clear();
            leaf->value = value;
            return leaf;
        } else if (node->type == NodeType::BRANCH) {
            // Update branch value
            node->value = value;
            return node;
        } else {
            // Convert to branch with value
            auto branch = std::make_shared<Node>();
            branch->type = NodeType::BRANCH;
            branch->children.resize(17);
            branch->value = value;
            return branch;
        }
    }

    if (node->type == NodeType::EMPTY) {
        // Create new leaf for remaining path
        auto leaf = std::make_shared<Node>();
        leaf->type = NodeType::LEAF;
        leaf->path.assign(nibbles.begin() + idx, nibbles.end());
        leaf->value = value;
        return leaf;
    }

    if (node->type == NodeType::LEAF || node->type == NodeType::EXTENSION) {
        size_t common = CommonPrefixLength(nibbles, idx, node->path, 0);

        if (common == node->path.size()) {
            // Path fully matches - continue down
            if (node->type == NodeType::LEAF) {
                // Convert leaf to extension+branch
                if (idx + common == nibbles.size()) {
                    // Exact match - update value
                    node->value = value;
                    return node;
                } else {
                    // Create branch
                    auto branch = std::make_shared<Node>();
                    branch->type = NodeType::BRANCH;
                    branch->children.resize(17);
                    branch->children[nibbles[idx + common]] =
                        Insert(std::make_shared<Node>(), nibbles, idx + common + 1, value);
                    branch->value = node->value;  // Old leaf value goes to branch

                    if (common > 0) {
                        // Create extension
                        auto ext = std::make_shared<Node>();
                        ext->type = NodeType::EXTENSION;
                        ext->path = node->path;
                        ext->children.resize(1);
                        ext->children[0] = branch;
                        return ext;
                    }
                    return branch;
                }
            } else {
                // Extension - continue to child
                node->children[0] = Insert(node->children[0], nibbles, idx + common, value);
                return node;
            }
        } else {
            // Partial match - need to split
            auto branch = std::make_shared<Node>();
            branch->type = NodeType::BRANCH;
            branch->children.resize(17);

            // Add old node's remaining path
            if (common < node->path.size()) {
                auto old_child = std::make_shared<Node>();
                old_child->type = node->type;
                old_child->path.assign(node->path.begin() + common + 1, node->path.end());
                old_child->value = node->value;
                old_child->children = node->children;
                branch->children[node->path[common]] = old_child;
            }

            // Add new value's remaining path
            if (idx + common < nibbles.size()) {
                branch->children[nibbles[idx + common]] =
                    Insert(std::make_shared<Node>(), nibbles, idx + common + 1, value);
            } else {
                branch->value = value;
            }

            // Add extension for common prefix if needed
            if (common > 0) {
                auto ext = std::make_shared<Node>();
                ext->type = NodeType::EXTENSION;
                ext->path.assign(node->path.begin(), node->path.begin() + common);
                ext->children.resize(1);
                ext->children[0] = branch;
                return ext;
            }

            return branch;
        }
    }

    if (node->type == NodeType::BRANCH) {
        uint8_t nibble = nibbles[idx];
        node->children[nibble] =
            Insert(node->children[nibble] ? node->children[nibble] : std::make_shared<Node>(),
                   nibbles, idx + 1, value);
        return node;
    }

    return node;
}

std::optional<MerklePatriciaTrie::Value>
MerklePatriciaTrie::Lookup(const NodePtr& node, const std::vector<uint8_t>& nibbles,
                           size_t idx) const {
    if (!node || node->type == NodeType::EMPTY) {
        return std::nullopt;
    }

    if (idx == nibbles.size()) {
        if (node->type == NodeType::LEAF || node->type == NodeType::BRANCH) {
            if (!node->value.empty()) {
                return node->value;
            }
        }
        return std::nullopt;
    }

    if (node->type == NodeType::LEAF) {
        // Check if path matches
        if (idx + node->path.size() == nibbles.size()) {
            if (std::equal(node->path.begin(), node->path.end(), nibbles.begin() + idx)) {
                return node->value;
            }
        }
        return std::nullopt;
    }

    if (node->type == NodeType::EXTENSION) {
        // Check if path matches
        if (idx + node->path.size() <= nibbles.size()) {
            if (std::equal(node->path.begin(), node->path.end(), nibbles.begin() + idx)) {
                return Lookup(node->children[0], nibbles, idx + node->path.size());
            }
        }
        return std::nullopt;
    }

    if (node->type == NodeType::BRANCH) {
        uint8_t nibble = nibbles[idx];
        if (nibble < 16 && node->children[nibble]) {
            return Lookup(node->children[nibble], nibbles, idx + 1);
        }
        return std::nullopt;
    }

    return std::nullopt;
}

MerklePatriciaTrie::NodePtr
MerklePatriciaTrie::Remove(NodePtr node, const std::vector<uint8_t>& nibbles, size_t idx) {
    if (!node || node->type == NodeType::EMPTY) {
        return node;
    }

    if (idx == nibbles.size()) {
        // Clear the value at this node
        node->value.clear();
        // If a LEAF has no value, replace with EMPTY node
        if (node->type == NodeType::LEAF) {
            return std::make_shared<Node>();  // EMPTY
        }
        return node;
    }

    if (node->type == NodeType::LEAF) {
        // Check if the remaining path matches this leaf
        if (node->path.size() == nibbles.size() - idx &&
            std::equal(node->path.begin(), node->path.end(), nibbles.begin() + idx)) {
            node->value.clear();
            return std::make_shared<Node>();  // EMPTY
        }
        return node;  // Different path, no change
    }

    if (node->type == NodeType::BRANCH) {
        uint8_t nibble = nibbles[idx];
        if (nibble < 16 && node->children[nibble]) {
            node->children[nibble] = Remove(node->children[nibble], nibbles, idx + 1);
        }
        // Collapse branch with a single remaining child into extension/leaf
        int remaining = 0;
        int remaining_idx = -1;
        for (int i = 0; i < 16; ++i) {
            if (node->children[i] && node->children[i]->type != NodeType::EMPTY) {
                ++remaining;
                remaining_idx = i;
            }
        }
        if (remaining == 0 && node->value.empty()) {
            return std::make_shared<Node>();  // EMPTY
        }
        if (remaining == 1 && node->value.empty()) {
            // Collapse: merge remaining child into this node as extension
            auto child = node->children[remaining_idx];
            auto ext = std::make_shared<Node>();
            ext->type = NodeType::EXTENSION;
            ext->path.push_back(static_cast<uint8_t>(remaining_idx));
            ext->path.insert(ext->path.end(), child->path.begin(), child->path.end());
            ext->value = child->value;
            ext->children = child->children;
            return ext;
        }
        return node;
    }

    if (node->type == NodeType::EXTENSION) {
        if (idx + node->path.size() <= nibbles.size() &&
            std::equal(node->path.begin(), node->path.end(), nibbles.begin() + idx)) {
            node->children[0] = Remove(node->children[0], nibbles, idx + node->path.size());
            if (!node->children[0] || node->children[0]->type == NodeType::EMPTY) {
                return std::make_shared<Node>();  // EMPTY
            }
        }
        return node;
    }

    return node;
}

std::vector<uint8_t> MerklePatriciaTrie::EncodeNode(const NodePtr& node) const {
    std::vector<uint8_t> encoded;

    if (!node || node->type == NodeType::EMPTY) {
        return encoded;  // Empty encoding
    }

    // Encode node type
    encoded.push_back(static_cast<uint8_t>(node->type));

    // Encode path (for leaf/extension)
    if (node->type == NodeType::LEAF || node->type == NodeType::EXTENSION) {
        encoded.push_back(static_cast<uint8_t>(node->path.size()));
        encoded.insert(encoded.end(), node->path.begin(), node->path.end());
    }

    // Encode value
    if (!node->value.empty()) {
        encoded.push_back(static_cast<uint8_t>(node->value.size() >> 8));
        encoded.push_back(static_cast<uint8_t>(node->value.size() & 0xFF));
        encoded.insert(encoded.end(), node->value.begin(), node->value.end());
    } else {
        encoded.push_back(0);
        encoded.push_back(0);
    }

    // Encode children (for branch)
    if (node->type == NodeType::BRANCH) {
        for (size_t i = 0; i < 16; i++) {
            if (node->children[i]) {
                auto child_hash = HashNode(node->children[i]);
                encoded.insert(encoded.end(), child_hash.begin(), child_hash.end());
            } else {
                // Empty child
                std::array<uint8_t, 32> zero{};
                encoded.insert(encoded.end(), zero.begin(), zero.end());
            }
        }
    } else if (node->type == NodeType::EXTENSION) {
        // Extension has one child
        if (node->children.size() > 0 && node->children[0]) {
            auto child_hash = HashNode(node->children[0]);
            encoded.insert(encoded.end(), child_hash.begin(), child_hash.end());
        }
    }

    return encoded;
}

MerklePatriciaTrie::Hash MerklePatriciaTrie::HashNode(const NodePtr& node) const {
    if (!node || node->type == NodeType::EMPTY) {
        // Empty node hash
        return Hash{};
    }

    auto encoded = EncodeNode(node);

    crypto::SHA256 hasher;
    hasher.Write(encoded.data(), encoded.size());
    auto hash_result = hasher.Finalize();

    Hash result;
    std::memcpy(result.data(), hash_result.data(), 32);
    return result;
}

}  // namespace evm
}  // namespace parthenon
