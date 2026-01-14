// ParthenonChain - Decentralized Identity (DID) System
// W3C DID standard compliant decentralized identifiers

#pragma once

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <array>

namespace parthenon {
namespace identity {

/**
 * DID Document
 * W3C compliant DID document
 */
struct DIDDocument {
    std::string id;  // did:parthenon:...
    std::vector<std::string> context;
    
    struct PublicKey {
        std::string id;
        std::string type;  // Ed25519, Secp256k1, etc.
        std::string controller;
        std::vector<uint8_t> public_key_bytes;
    };
    
    struct Service {
        std::string id;
        std::string type;
        std::string service_endpoint;
    };
    
    std::vector<PublicKey> public_keys;
    std::vector<std::string> authentication;
    std::vector<Service> services;
    uint64_t created;
    uint64_t updated;
    
    DIDDocument() : created(0), updated(0) {}
};

/**
 * Verifiable Credential
 * W3C Verifiable Credentials standard
 */
struct VerifiableCredential {
    std::string id;
    std::vector<std::string> type;
    std::string issuer;
    std::string issuance_date;
    std::string expiration_date;
    std::map<std::string, std::string> credential_subject;
    std::vector<uint8_t> proof;
    
    bool IsExpired(uint64_t current_time) const;
};

/**
 * DID Manager
 * Create and manage DIDs
 */
class DIDManager {
public:
    DIDManager();
    ~DIDManager();
    
    /**
     * Create new DID
     */
    std::string CreateDID(const std::vector<uint8_t>& public_key);
    
    /**
     * Resolve DID to document
     */
    std::optional<DIDDocument> ResolveDID(const std::string& did);
    
    /**
     * Update DID document
     */
    bool UpdateDIDDocument(
        const std::string& did,
        const DIDDocument& document,
        const std::vector<uint8_t>& signature
    );
    
    /**
     * Revoke DID
     */
    bool RevokeDID(
        const std::string& did,
        const std::vector<uint8_t>& signature
    );
    
    /**
     * Add public key to DID
     */
    bool AddPublicKey(
        const std::string& did,
        const DIDDocument::PublicKey& key
    );
    
    /**
     * Add service endpoint
     */
    bool AddService(
        const std::string& did,
        const DIDDocument::Service& service
    );
    
private:
    std::map<std::string, DIDDocument> did_registry_;
};

/**
 * Verifiable Credentials Manager
 */
class CredentialManager {
public:
    /**
     * Issue credential
     */
    VerifiableCredential IssueCredential(
        const std::string& issuer_did,
        const std::string& subject_did,
        const std::map<std::string, std::string>& claims,
        const std::vector<uint8_t>& issuer_signature
    );
    
    /**
     * Verify credential
     */
    bool VerifyCredential(const VerifiableCredential& credential);
    
    /**
     * Revoke credential
     */
    bool RevokeCredential(const std::string& credential_id);
    
    /**
     * Check if credential is revoked
     */
    bool IsRevoked(const std::string& credential_id) const;
    
private:
    std::map<std::string, bool> revocation_list_;
};

/**
 * Zero-Knowledge Proof Credentials
 * Selective disclosure without revealing full credential
 */
class ZKPCredentials {
public:
    /**
     * Create ZK proof for specific claims
     */
    std::vector<uint8_t> CreateProof(
        const VerifiableCredential& credential,
        const std::vector<std::string>& claims_to_prove
    );
    
    /**
     * Verify ZK proof
     */
    bool VerifyProof(
        const std::vector<uint8_t>& proof,
        const std::string& issuer_did
    );
};

} // namespace identity
} // namespace parthenon
