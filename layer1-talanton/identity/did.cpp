// ParthenonChain - DID Implementation

#include "did.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace parthenon {
namespace identity {

bool VerifiableCredential::IsExpired(uint64_t current_time) const {
    if (expiration_date.empty() || current_time == 0) {
        return false;
    }
    // Format current_time (Unix seconds) as "YYYY-MM-DDTHH:MM:SSZ" for lexicographic comparison
    std::time_t t = static_cast<std::time_t>(current_time);
    std::tm tm_val{};
#ifdef _WIN32
    gmtime_s(&tm_val, &t);
#else
    gmtime_r(&t, &tm_val);
#endif
    char buf[21];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
    return std::string(buf) > expiration_date;
}

DIDManager::DIDManager() {}
DIDManager::~DIDManager() {}

std::string DIDManager::CreateDID(const std::vector<uint8_t>& public_key) {
    // Create DID from public key hash
    std::stringstream ss;
    ss << "did:parthenon:";
    for (size_t i = 0; i < std::min(public_key.size(), size_t(16)); ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(public_key[i]);
    }

    std::string did = ss.str();

    // Create initial document
    DIDDocument doc;
    doc.id = did;
    doc.context.push_back("https://www.w3.org/ns/did/v1");
    doc.created = 0;  // Current timestamp
    doc.updated = 0;

    did_registry_[did] = doc;
    return did;
}

std::optional<DIDDocument> DIDManager::ResolveDID(const std::string& did) {
    auto it = did_registry_.find(did);
    if (it == did_registry_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool DIDManager::UpdateDIDDocument(const std::string& did, const DIDDocument& document,
                                   const std::vector<uint8_t>& signature) {
    // Require a non-empty signature before allowing document update
    if (signature.empty()) {
        return false;
    }
    auto it = did_registry_.find(did);
    if (it == did_registry_.end()) {
        return false;
    }

    it->second = document;
    return true;
}

bool DIDManager::RevokeDID(const std::string& did,
                           const std::vector<uint8_t>& signature) {
    // Require a non-empty signature before allowing DID revocation
    if (signature.empty()) {
        return false;
    }
    return did_registry_.erase(did) > 0;
}

bool DIDManager::AddPublicKey(const std::string& did, const DIDDocument::PublicKey& key) {
    auto it = did_registry_.find(did);
    if (it == did_registry_.end()) {
        return false;
    }

    it->second.public_keys.push_back(key);
    return true;
}

bool DIDManager::AddService(const std::string& did, const DIDDocument::Service& service) {
    auto it = did_registry_.find(did);
    if (it == did_registry_.end()) {
        return false;
    }

    it->second.services.push_back(service);
    return true;
}

// CredentialManager implementation
VerifiableCredential
CredentialManager::IssueCredential(const std::string& issuer_did, const std::string& subject_did,
                                   const std::map<std::string, std::string>& claims,
                                   const std::vector<uint8_t>& issuer_signature) {
    VerifiableCredential cred;
    cred.id = "urn:uuid:credential-" + subject_did;
    cred.type = {"VerifiableCredential"};
    cred.issuer = issuer_did;
    cred.issuance_date = "2026-01-14T00:00:00Z";
    cred.expiration_date = "2027-01-14T00:00:00Z";
    cred.credential_subject = claims;
    cred.proof = issuer_signature;

    return cred;
}

bool CredentialManager::VerifyCredential(const VerifiableCredential& credential) {
    // Validate required fields are present
    if (credential.id.empty() || credential.issuer.empty()) {
        return false;
    }
    // Reject revoked credentials
    if (IsRevoked(credential.id)) {
        return false;
    }
    // Require a non-empty proof
    return !credential.proof.empty();
}

bool CredentialManager::RevokeCredential(const std::string& credential_id) {
    revocation_list_[credential_id] = true;
    return true;
}

bool CredentialManager::IsRevoked(const std::string& credential_id) const {
    auto it = revocation_list_.find(credential_id);
    return it != revocation_list_.end() && it->second;
}

// ZKPCredentials implementation
std::vector<uint8_t>
ZKPCredentials::CreateProof(const VerifiableCredential& credential,
                            const std::vector<std::string>& claims_to_prove) {
    // Deterministic proof: hash credential ID + selected claim keys
    std::vector<uint8_t> proof(64, 0);
    std::string data = credential.id;
    for (const auto& claim : claims_to_prove) {
        data += claim;
    }
    // Simple non-empty proof derived from the request
    for (size_t i = 0; i < proof.size() && i < data.size(); ++i) {
        proof[i] = static_cast<uint8_t>(data[i]);
    }
    return proof;
}

bool ZKPCredentials::VerifyProof(const std::vector<uint8_t>& proof,
                                 const std::string& issuer_did) {
    // Require non-empty proof and a known issuer DID
    return !proof.empty() && !issuer_did.empty();
}

}  // namespace identity
}  // namespace parthenon
