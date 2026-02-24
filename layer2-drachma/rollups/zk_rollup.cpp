// ParthenonChain - ZK-Rollup Implementation

#include "zk_rollup.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstring>

namespace parthenon {
namespace layer2 {
namespace rollups {

namespace {

constexpr size_t kDefaultCircuitSize = 1024;

void AppendUint32(std::vector<uint8_t>& out, uint32_t value) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(uint32_t));
}

void AppendUint64(std::vector<uint8_t>& out, uint64_t value) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(uint64_t));
}

bool ReadUint32(const std::vector<uint8_t>& data, size_t& offset, uint32_t& value) {
    if (offset + sizeof(uint32_t) > data.size()) {
        return false;
    }
    std::memcpy(&value, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return true;
}

bool ReadUint64(const std::vector<uint8_t>& data, size_t& offset, uint64_t& value) {
    if (offset + sizeof(uint64_t) > data.size()) {
        return false;
    }
    std::memcpy(&value, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return true;
}

void AppendArray(std::vector<uint8_t>& out, const std::array<uint8_t, 32>& value) {
    out.insert(out.end(), value.begin(), value.end());
}

std::array<uint8_t, 32> HashBytes(const uint8_t* data, size_t size) {
    crypto::SHA256 hasher;
    if (size > 0) {
        hasher.Write(data, size);
    }
    return hasher.Finalize();
}

std::array<uint8_t, 32> HashBytes(const std::vector<uint8_t>& data) {
    return HashBytes(data.data(), data.size());
}

std::array<uint8_t, 32> HashPair(const std::array<uint8_t, 32>& left,
                                  const std::array<uint8_t, 32>& right) {
    // Positional Merkle tree: left child always goes first, right child second.
    // The parent hash is SHA256(left || right) so the tree is index-aware and
    // compatible with standard ZK-rollup circuit verifiers.
    crypto::SHA256 hasher;
    hasher.Write(left.data(), left.size());
    hasher.Write(right.data(), right.size());
    return hasher.Finalize();
}

std::array<uint8_t, 32> HashAccount(const std::vector<uint8_t>& account) {
    return HashBytes(account);
}

std::array<uint8_t, 32>
ComputeMerkleRoot(const std::vector<std::array<uint8_t, 32>>& leaves) {
    if (leaves.empty()) {
        std::array<uint8_t, 32> empty{};
        empty.fill(0);
        return empty;
    }

    std::vector<std::array<uint8_t, 32>> layer = leaves;
    while (layer.size() > 1) {
        if (layer.size() % 2 != 0) {
            layer.push_back(layer.back());
        }
        std::vector<std::array<uint8_t, 32>> next;
        next.reserve(layer.size() / 2);
        for (size_t i = 0; i < layer.size(); i += 2) {
            next.push_back(HashPair(layer[i], layer[i + 1]));
        }
        layer = std::move(next);
    }
    return layer.front();
}

std::vector<std::pair<std::array<uint8_t, 32>, bool>>
BuildMerkleProof(const std::vector<std::array<uint8_t, 32>>& leaves, size_t index) {
    std::vector<std::pair<std::array<uint8_t, 32>, bool>> proof;
    if (leaves.empty() || index >= leaves.size()) {
        return proof;
    }

    std::vector<std::array<uint8_t, 32>> layer = leaves;
    size_t idx = index;
    while (layer.size() > 1) {
        if (layer.size() % 2 != 0) {
            layer.push_back(layer.back());
        }
        // For a positional tree, record both the sibling hash and whether it is
        // the RIGHT sibling (current node is left child, idx % 2 == 0) or the
        // LEFT sibling (current node is right child, idx % 2 == 1).
        // After padding, layer.size() is always even, so idx % 2 == 0 guarantees
        // idx + 1 < layer.size().
        if (idx % 2 == 0) {
            // current is left child → sibling is to the right (always valid after padding)
            if (idx + 1 < layer.size()) {
                proof.emplace_back(layer[idx + 1], /*is_right_sibling=*/true);
            }
        } else {
            // current is right child → sibling is to the left
            proof.emplace_back(layer[idx - 1], /*is_right_sibling=*/false);
        }
        std::vector<std::array<uint8_t, 32>> next;
        next.reserve(layer.size() / 2);
        for (size_t i = 0; i < layer.size(); i += 2) {
            next.push_back(HashPair(layer[i], layer[i + 1]));
        }
        idx /= 2;
        layer = std::move(next);
    }

    return proof;
}

std::vector<uint8_t> SerializeBatchInputs(const ZKRollupBatch& batch) {
    std::vector<uint8_t> inputs;
    AppendUint64(inputs, batch.batch_id);
    AppendUint64(inputs, batch.timestamp);
    AppendArray(inputs, batch.state_root_before);
    AppendArray(inputs, batch.state_root_after);
    AppendUint32(inputs, static_cast<uint32_t>(batch.transaction_hashes.size()));
    for (const auto& hash : batch.transaction_hashes) {
        AppendArray(inputs, hash);
    }
    inputs.insert(inputs.end(), batch.operator_signature.begin(), batch.operator_signature.end());
    return inputs;
}

std::vector<uint8_t> SerializeTransactionInputs(const ZKTransaction& tx) {
    std::vector<uint8_t> inputs;
    AppendArray(inputs, tx.tx_hash);
    AppendArray(inputs, tx.nullifier);
    AppendArray(inputs, tx.commitment);
    inputs.insert(inputs.end(), tx.encrypted_data.begin(), tx.encrypted_data.end());
    return inputs;
}

std::vector<uint8_t> SerializeExitInputs(const std::vector<uint8_t>& account, uint64_t amount) {
    std::vector<uint8_t> inputs = account;
    AppendUint64(inputs, amount);
    return inputs;
}

class RollupCircuit : public privacy::zksnark::Circuit {
  public:
    explicit RollupCircuit(const std::vector<uint8_t>& public_inputs)
        : public_inputs_(public_inputs) {}

    size_t GetConstraintCount() const override { return public_inputs_.size() + 1; }
    size_t GetInputCount() const override { return public_inputs_.size(); }
    bool Synthesize() override { return !public_inputs_.empty(); }

  private:
    std::vector<uint8_t> public_inputs_;
};

bool VerifyZkProof(const privacy::zksnark::ZKProof& proof,
                   const privacy::zksnark::ProofParameters& params,
                   const std::vector<uint8_t>& inputs) {
    if (!proof.public_inputs.empty() && proof.public_inputs != inputs) {
        return false;
    }
    privacy::zksnark::ZKVerifier verifier(params);
    return verifier.VerifyProof(proof, inputs);
}

}  // namespace

// ZKRollupState implementation
ZKRollupState::ZKRollupState() {
    state_root_.fill(0);
}

ZKRollupState::~ZKRollupState() {}

std::array<uint8_t, 32> ZKRollupState::GetStateRoot() const {
    return state_root_;
}

bool ZKRollupState::ApplyTransaction(const ZKTransaction& tx) {
    // Check nullifier hasn't been used
    if (used_nullifiers_[tx.nullifier]) {
        return false;
    }

    // Mark nullifier as used
    used_nullifiers_[tx.nullifier] = true;

    std::vector<uint8_t> account(tx.nullifier.begin(), tx.nullifier.end());
    balances_[account] = tx.commitment;

    std::vector<std::array<uint8_t, 32>> leaves;
    leaves.reserve(balances_.size());
    for (const auto& entry : balances_) {
        leaves.push_back(HashAccount(entry.first));
    }
    state_root_ = ComputeMerkleRoot(leaves);

    return true;
}

std::vector<std::pair<std::array<uint8_t, 32>, bool>>
ZKRollupState::GetMerkleProof(const std::vector<uint8_t>& account) const {
    if (account.empty()) {
        return {};
    }

    std::vector<std::array<uint8_t, 32>> leaves;
    leaves.reserve(balances_.size());
    size_t index = 0;
    bool found = false;
    size_t current = 0;
    for (const auto& entry : balances_) {
        leaves.push_back(HashAccount(entry.first));
        if (entry.first == account) {
            index = current;
            found = true;
        }
        ++current;
    }

    if (!found) {
        return {};
    }

    return BuildMerkleProof(leaves, index);
}

bool ZKRollupState::VerifyMerkleProof(
    const std::vector<uint8_t>& account,
    const std::vector<std::pair<std::array<uint8_t, 32>, bool>>& proof,
    const std::array<uint8_t, 32>& root) const {
    if (account.empty()) {
        return false;
    }

    auto current_hash = HashAccount(account);
    for (const auto& [sibling, is_right] : proof) {
        if (is_right) {
            // current node is the left child
            current_hash = HashPair(current_hash, sibling);
        } else {
            // current node is the right child
            current_hash = HashPair(sibling, current_hash);
        }
    }

    return current_hash == root;
}

std::optional<std::array<uint8_t, 32>>
ZKRollupState::GetBalance(const std::vector<uint8_t>& account) const {
    auto it = balances_.find(account);
    if (it == balances_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ZKRollup implementation
ZKRollup::ZKRollup()
    : current_batch_id_(0), current_block_height_(0),
      proof_params_(privacy::zksnark::ZKProver::Setup(kDefaultCircuitSize)) {}

ZKRollup::~ZKRollup() {}

bool ZKRollup::SubmitBatch(const ZKRollupBatch& batch) {
    if (batch.batch_id != current_batch_id_ + 1) {
        return false;
    }

    // Verify proof
    if (!VerifyBatchProof(batch)) {
        return false;
    }

    if (batch.state_root_after != state_.GetStateRoot()) {
        return false;
    }

    BatchInfo info;
    info.batch = batch;
    info.submission_block = current_block_height_;
    info.finalized = false;

    batches_[batch.batch_id] = info;
    current_batch_id_ = batch.batch_id;
    return true;
}

std::optional<ZKRollupBatch> ZKRollup::GetBatch(uint64_t batch_id) const {
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return std::nullopt;
    }
    return it->second.batch;
}

bool ZKRollup::AddTransaction(const ZKTransaction& tx) {
    auto inputs = SerializeTransactionInputs(tx);
    if (!VerifyZkProof(tx.transfer_proof, proof_params_, inputs)) {
        return false;
    }

    pending_transactions_.push_back(tx);
    return true;
}

ZKRollupBatch ZKRollup::CreateBatch() {
    ZKRollupBatch batch;
    batch.batch_id = current_batch_id_ + 1;
    batch.state_root_before = state_.GetStateRoot();

    // Process transactions
    for (const auto& tx : pending_transactions_) {
        state_.ApplyTransaction(tx);
        batch.transaction_hashes.push_back(tx.tx_hash);
    }

    batch.state_root_after = state_.GetStateRoot();
    batch.timestamp = current_block_height_;

    pending_transactions_.clear();
    return batch;
}

bool ZKRollup::VerifyBatchProof(const ZKRollupBatch& batch) const {
    auto inputs = SerializeBatchInputs(batch);
    return VerifyZkProof(batch.validity_proof, proof_params_, inputs);
}

bool ZKRollup::FinalizeBatch(uint64_t batch_id) {
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return false;
    }

    it->second.finalized = true;
    return true;
}

std::vector<ZKRollupBatch> ZKRollup::GetPendingBatches() const {
    std::vector<ZKRollupBatch> pending;
    for (const auto& [id, info] : batches_) {
        if (!info.finalized) {
            pending.push_back(info.batch);
        }
    }
    return pending;
}

std::vector<uint8_t> ZKRollup::CompressBatch(const ZKRollupBatch& batch) const {
    std::vector<uint8_t> data;
    data.reserve(128 + batch.transaction_hashes.size() * 32 +
                 batch.validity_proof.proof_data.size() +
                 batch.validity_proof.public_inputs.size() + batch.operator_signature.size());

    AppendUint64(data, batch.batch_id);
    AppendUint64(data, batch.timestamp);
    AppendArray(data, batch.state_root_before);
    AppendArray(data, batch.state_root_after);

    AppendUint32(data, static_cast<uint32_t>(batch.transaction_hashes.size()));
    for (const auto& hash : batch.transaction_hashes) {
        AppendArray(data, hash);
    }

    AppendUint32(data, static_cast<uint32_t>(batch.validity_proof.proof_data.size()));
    data.insert(data.end(), batch.validity_proof.proof_data.begin(),
                batch.validity_proof.proof_data.end());

    AppendUint32(data, static_cast<uint32_t>(batch.validity_proof.public_inputs.size()));
    data.insert(data.end(), batch.validity_proof.public_inputs.begin(),
                batch.validity_proof.public_inputs.end());

    AppendUint32(data, batch.validity_proof.proof_type);

    AppendUint32(data, static_cast<uint32_t>(batch.operator_signature.size()));
    data.insert(data.end(), batch.operator_signature.begin(), batch.operator_signature.end());

    return data;
}

std::optional<ZKRollupBatch>
ZKRollup::DecompressBatch(const std::vector<uint8_t>& data) const {
    ZKRollupBatch batch;
    size_t offset = 0;

    if (!ReadUint64(data, offset, batch.batch_id)) {
        return std::nullopt;
    }
    if (!ReadUint64(data, offset, batch.timestamp)) {
        return std::nullopt;
    }
    if (offset + 64 > data.size()) {
        return std::nullopt;
    }
    std::memcpy(batch.state_root_before.data(), data.data() + offset, 32);
    offset += 32;
    std::memcpy(batch.state_root_after.data(), data.data() + offset, 32);
    offset += 32;

    uint32_t tx_count = 0;
    if (!ReadUint32(data, offset, tx_count)) {
        return std::nullopt;
    }
    batch.transaction_hashes.reserve(tx_count);
    for (uint32_t i = 0; i < tx_count; ++i) {
        if (offset + 32 > data.size()) {
            return std::nullopt;
        }
        std::array<uint8_t, 32> hash{};
        std::memcpy(hash.data(), data.data() + offset, 32);
        offset += 32;
        batch.transaction_hashes.push_back(hash);
    }

    uint32_t proof_size = 0;
    if (!ReadUint32(data, offset, proof_size)) {
        return std::nullopt;
    }
    if (offset + proof_size > data.size()) {
        return std::nullopt;
    }
    batch.validity_proof.proof_data.assign(data.begin() + offset, data.begin() + offset + proof_size);
    offset += proof_size;

    uint32_t public_size = 0;
    if (!ReadUint32(data, offset, public_size)) {
        return std::nullopt;
    }
    if (offset + public_size > data.size()) {
        return std::nullopt;
    }
    batch.validity_proof.public_inputs.assign(data.begin() + offset,
                                              data.begin() + offset + public_size);
    offset += public_size;

    if (!ReadUint32(data, offset, batch.validity_proof.proof_type)) {
        return std::nullopt;
    }

    uint32_t sig_size = 0;
    if (!ReadUint32(data, offset, sig_size)) {
        return std::nullopt;
    }
    if (offset + sig_size > data.size()) {
        return std::nullopt;
    }
    batch.operator_signature.assign(data.begin() + offset, data.begin() + offset + sig_size);
    offset += sig_size;

    if (offset != data.size()) {
        return std::nullopt;
    }

    return batch;
}

// ZKRollupProver implementation
ZKRollupProver::ZKRollupProver() {
    SetupParameters(kDefaultCircuitSize);
}
ZKRollupProver::~ZKRollupProver() {}

privacy::zksnark::ZKProof
ZKRollupProver::GenerateBatchProof(const ZKRollupBatch& batch) {
    auto inputs = SerializeBatchInputs(batch);
    RollupCircuit circuit(inputs);
    privacy::zksnark::ZKProver prover(params_);
    auto proof_opt = prover.GenerateProof(circuit, inputs);
    if (!proof_opt) {
        return {};
    }
    return *proof_opt;
}

privacy::zksnark::ZKProof
ZKRollupProver::GenerateTransferProof(const ZKTransaction& tx,
                                      const std::vector<uint8_t>& witness) {
    auto inputs = SerializeTransactionInputs(tx);
    // Combine transaction inputs with the provided witness material
    std::vector<uint8_t> combined = inputs;
    combined.insert(combined.end(), witness.begin(), witness.end());
    RollupCircuit circuit(combined);
    privacy::zksnark::ZKProver prover(params_);
    auto proof_opt = prover.GenerateProof(circuit, combined);
    if (!proof_opt) {
        return {};
    }
    return *proof_opt;
}

bool ZKRollupProver::SetupParameters(size_t circuit_size) {
    params_ = privacy::zksnark::ZKProver::Setup(circuit_size);
    return !params_.proving_key.empty() && !params_.verification_key.empty();
}

// ZKRollupVerifier implementation
ZKRollupVerifier::ZKRollupVerifier(ZKRollup* rollup)
    : rollup_(rollup), params_(rollup ? rollup->GetProofParameters()
                                      : privacy::zksnark::ProofParameters()) {}
ZKRollupVerifier::~ZKRollupVerifier() {}

bool ZKRollupVerifier::VerifyBatchProof(const ZKRollupBatch& batch) const {
    auto inputs = SerializeBatchInputs(batch);
    return VerifyZkProof(batch.validity_proof, params_, inputs);
}

bool ZKRollupVerifier::VerifyTransactionProof(const ZKTransaction& tx) const {
    auto inputs = SerializeTransactionInputs(tx);
    return VerifyZkProof(tx.transfer_proof, params_, inputs);
}

bool ZKRollupVerifier::BatchVerifyProofs(const std::vector<ZKRollupBatch>& batches) const {
    for (const auto& batch : batches) {
        if (!VerifyBatchProof(batch)) {
            return false;
        }
    }
    return true;
}

// ZKRollupExitManager implementation
ZKRollupExitManager::ZKRollupExitManager()
    : proof_params_(privacy::zksnark::ZKProver::Setup(kDefaultCircuitSize)) {}

ZKRollupExitManager::ZKRollupExitManager(const privacy::zksnark::ProofParameters& params)
    : proof_params_(params) {}

bool ZKRollupExitManager::RequestExit(const ExitRequest& request) {
    if (!VerifyExitProof(request)) {
        return false;
    }

    auto it = pending_exits_.find(request.account);
    if (it != pending_exits_.end() && !it->second.processed) {
        return false;
    }

    pending_exits_[request.account] = request;
    return true;
}

bool ZKRollupExitManager::ProcessExit(const std::vector<uint8_t>& account) {
    auto it = pending_exits_.find(account);
    if (it == pending_exits_.end()) {
        return false;
    }

    it->second.processed = true;
    return true;
}

std::vector<ZKRollupExitManager::ExitRequest> ZKRollupExitManager::GetPendingExits() const {
    std::vector<ExitRequest> pending;
    for (const auto& [account, request] : pending_exits_) {
        if (!request.processed) {
            pending.push_back(request);
        }
    }
    return pending;
}

bool ZKRollupExitManager::VerifyExitProof(const ExitRequest& request) const {
    if (request.account.empty() || request.amount == 0) {
        return false;
    }

    auto current_hash = HashAccount(request.account);
    for (const auto& [sibling, is_right] : request.merkle_proof) {
        if (is_right) {
            current_hash = HashPair(current_hash, sibling);
        } else {
            current_hash = HashPair(sibling, current_hash);
        }
    }

    if (current_hash != request.merkle_root) {
        return false;
    }

    auto inputs = SerializeExitInputs(request.account, request.amount);
    return VerifyZkProof(request.ownership_proof, proof_params_, inputs);
}

}  // namespace rollups
}  // namespace layer2
}  // namespace parthenon
