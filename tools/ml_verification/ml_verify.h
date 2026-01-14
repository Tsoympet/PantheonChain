// ParthenonChain - Machine Learning On-Chain Verification
// Verify ML model inference on blockchain

#pragma once

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <array>

namespace parthenon {
namespace ml {

/**
 * ML Model Hash
 */
using ModelHash = std::array<uint8_t, 32>;

/**
 * Inference Proof
 * Proof that model inference was computed correctly
 */
struct InferenceProof {
    ModelHash model_hash;
    std::vector<float> input_data;
    std::vector<float> output_data;
    std::vector<uint8_t> zkproof;  // ZK proof of correct computation
    
    bool IsValid() const { return !zkproof.empty(); }
};

/**
 * ML Model Registry
 * On-chain registry of ML models
 */
class MLModelRegistry {
public:
    struct ModelInfo {
        ModelHash hash;
        std::string name;
        std::string framework;  // TensorFlow, PyTorch, ONNX
        std::vector<uint32_t> input_shape;
        std::vector<uint32_t> output_shape;
        std::string ipfs_cid;  // Model stored on IPFS
        uint64_t registered_block;
    };
    
    /**
     * Register ML model
     */
    bool RegisterModel(const ModelInfo& model);
    
    /**
     * Get model info
     */
    std::optional<ModelInfo> GetModel(const ModelHash& hash);
    
    /**
     * Verify model hash
     */
    bool VerifyModelHash(
        const ModelHash& hash,
        const std::vector<uint8_t>& model_weights
    );
    
private:
    std::map<ModelHash, ModelInfo> registry_;
};

/**
 * Zero-Knowledge ML Inference
 * Prove inference without revealing model or data
 */
class ZKMLInference {
public:
    /**
     * Generate proof of inference
     */
    InferenceProof GenerateProof(
        const ModelHash& model_hash,
        const std::vector<float>& input,
        const std::vector<float>& output,
        const std::vector<uint8_t>& model_weights
    );
    
    /**
     * Verify inference proof
     */
    bool VerifyProof(const InferenceProof& proof);
    
    /**
     * Batch verify multiple inferences
     */
    bool BatchVerify(const std::vector<InferenceProof>& proofs);
};

/**
 * Optimistic ML Rollup
 * Rollup for ML inference with fraud proofs
 */
class MLRollup {
public:
    struct MLBatch {
        uint64_t batch_id;
        std::vector<InferenceProof> inferences;
        std::array<uint8_t, 32> state_root;
        uint64_t timestamp;
    };
    
    /**
     * Submit batch of inferences
     */
    bool SubmitBatch(const MLBatch& batch);
    
    /**
     * Challenge incorrect inference
     */
    bool ChallengeInference(
        uint64_t batch_id,
        uint32_t inference_index,
        const std::vector<uint8_t>& fraud_proof
    );
    
    /**
     * Finalize batch after challenge period
     */
    bool FinalizeBatch(uint64_t batch_id);
    
private:
    std::map<uint64_t, MLBatch> batches_;
};

/**
 * On-Chain ML Applications
 */
class MLApplications {
public:
    /**
     * Spam detection
     */
    struct SpamDetectionResult {
        float spam_score;
        bool is_spam;
        InferenceProof proof;
    };
    
    SpamDetectionResult DetectSpam(const std::string& message);
    
    /**
     * Credit scoring
     */
    struct CreditScore {
        uint32_t score;  // 300-850
        InferenceProof proof;
    };
    
    CreditScore CalculateCreditScore(
        const std::map<std::string, float>& features
    );
    
    /**
     * Fraud detection
     */
    struct FraudDetectionResult {
        float fraud_probability;
        bool is_fraudulent;
        InferenceProof proof;
    };
    
    FraudDetectionResult DetectFraud(
        const std::vector<float>& transaction_features
    );
    
private:
    ZKMLInference zkml_;
    MLModelRegistry registry_;
};

/**
 * Federated Learning On-Chain
 * Coordinate federated learning without revealing data
 */
class FederatedLearning {
public:
    struct ModelUpdate {
        std::vector<uint8_t> encrypted_gradients;
        std::vector<uint8_t> zkproof;  // Proof of correct training
        std::vector<uint8_t> contributor;
    };
    
    /**
     * Submit model update
     */
    bool SubmitUpdate(const ModelUpdate& update);
    
    /**
     * Aggregate updates
     */
    std::vector<uint8_t> AggregateUpdates();
    
    /**
     * Verify update is valid
     */
    bool VerifyUpdate(const ModelUpdate& update);
    
private:
    std::vector<ModelUpdate> updates_;
};

} // namespace ml
} // namespace parthenon
