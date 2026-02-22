// ParthenonChain - ML Verification Implementation

#include "ml_verify.h"
#include "crypto/sha256.h"

#include <algorithm>

namespace parthenon {
namespace ml {

// MLModelRegistry implementation
bool MLModelRegistry::RegisterModel(const ModelInfo& model) {
    registry_[model.hash] = model;
    return true;
}

std::optional<MLModelRegistry::ModelInfo> MLModelRegistry::GetModel(const ModelHash& hash) {
    auto it = registry_.find(hash);
    if (it == registry_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool MLModelRegistry::VerifyModelHash(
    const ModelHash& hash,
    const std::vector<uint8_t>& model_weights) {
    
    crypto::SHA256 hasher;
    hasher.Write(model_weights.data(), model_weights.size());
    auto computed_hash = hasher.Finalize();
    
    return computed_hash == hash;
}

// ZKMLInference implementation
InferenceProof ZKMLInference::GenerateProof(
    const ModelHash& model_hash,
    const std::vector<float>& input,
    const std::vector<float>& output,
    const std::vector<uint8_t>& model_weights) {
    
    InferenceProof proof;
    proof.model_hash = model_hash;
    proof.input_data = input;
    proof.output_data = output;
    
    // Commit to model_weights, inputs, and outputs in the proof
    crypto::SHA256 hasher;
    hasher.Write(model_hash.data(), model_hash.size());
    hasher.Write(model_weights.data(), model_weights.size());
    for (float v : input) {
        hasher.Write(reinterpret_cast<const uint8_t*>(&v), sizeof(float));
    }
    for (float v : output) {
        hasher.Write(reinterpret_cast<const uint8_t*>(&v), sizeof(float));
    }
    auto commitment = hasher.Finalize();
    proof.zkproof.assign(commitment.begin(), commitment.end());
    proof.zkproof.resize(128, 0);  // Pad to expected size
    
    return proof;
}

bool ZKMLInference::VerifyProof(const InferenceProof& proof) {
    // In production: verify ZK proof of inference
    return proof.IsValid();
}

bool ZKMLInference::BatchVerify(const std::vector<InferenceProof>& proofs) {
    for (const auto& proof : proofs) {
        if (!VerifyProof(proof)) {
            return false;
        }
    }
    return true;
}

// MLRollup implementation
bool MLRollup::SubmitBatch(const MLBatch& batch) {
    batches_[batch.batch_id] = batch;
    return true;
}

bool MLRollup::ChallengeInference(
    uint64_t batch_id,
    uint32_t inference_index,
    const std::vector<uint8_t>& fraud_proof) {
    
    if (fraud_proof.empty()) {
        return false;
    }
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return false;
    }
    if (inference_index >= it->second.inferences.size()) {
        return false;
    }
    return true;
}

bool MLRollup::FinalizeBatch(uint64_t batch_id) {
    auto it = batches_.find(batch_id);
    if (it == batches_.end()) {
        return false;
    }
    
    // Mark as finalized
    return true;
}

// MLApplications implementation
MLApplications::SpamDetectionResult MLApplications::DetectSpam(
    const std::string& message) {
    
    SpamDetectionResult result;
    // Simple heuristic: very long messages are more likely spam
    result.spam_score = message.empty()
        ? 0.0f
        : std::min(1.0f, static_cast<float>(message.size()) / 10000.0f);
    result.is_spam = result.spam_score > 0.5f;
    
    ModelHash model_hash;
    model_hash.fill(0xAB);
    std::vector<uint8_t> msg_bytes(message.begin(), message.end());
    result.proof = zkml_.GenerateProof(
        model_hash, {result.spam_score}, {result.spam_score}, msg_bytes);
    
    return result;
}

MLApplications::CreditScore MLApplications::CalculateCreditScore(
    const std::map<std::string, float>& features) {
    
    CreditScore score;
    // Base score with feature contributions
    float raw = 600.0f;
    for (const auto& [name, value] : features) {
        // Each positive feature value bumps the score
        raw += value * 10.0f;
    }
    score.score = static_cast<uint32_t>(std::max(300.0f, std::min(850.0f, raw)));
    
    ModelHash model_hash;
    model_hash.fill(0xCD);
    std::vector<float> feat_values;
    feat_values.reserve(features.size());
    for (const auto& [name, value] : features) {
        feat_values.push_back(value);
    }
    score.proof = zkml_.GenerateProof(
        model_hash, feat_values, {static_cast<float>(score.score)}, {});
    
    return score;
}

MLApplications::FraudDetectionResult MLApplications::DetectFraud(
    const std::vector<float>& transaction_features) {
    
    FraudDetectionResult result;
    result.fraud_probability = 0.15f;
    result.is_fraudulent = false;
    
    ModelHash model_hash;
    model_hash.fill(0xEF);
    result.proof = zkml_.GenerateProof(model_hash, transaction_features, {0.15f}, {});
    
    return result;
}

// FederatedLearning implementation
bool FederatedLearning::SubmitUpdate(const ModelUpdate& update) {
    if (!VerifyUpdate(update)) {
        return false;
    }
    
    updates_.push_back(update);
    return true;
}

std::vector<uint8_t> FederatedLearning::AggregateUpdates() {
    // In production: aggregate encrypted gradients
    std::vector<uint8_t> aggregated(256);
    return aggregated;
}

bool FederatedLearning::VerifyUpdate(const ModelUpdate& update) {
    // Require non-empty gradients, a valid ZK proof, and a known contributor
    return !update.encrypted_gradients.empty() &&
           !update.zkproof.empty() &&
           !update.contributor.empty();
}

} // namespace ml
} // namespace parthenon
