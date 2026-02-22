// ParthenonChain - Key Generation Tool
// Utility for generating and managing cryptographic keys

#include <iostream>
#include <fstream>
#include <iomanip>
#include <openssl/rand.h>
#include "crypto/schnorr.h"
#include "crypto/sha256.h"

using namespace parthenon;

/**
 * Generate a new key pair
 */
void GenerateKeyPair() {
    // Generate cryptographically secure random private key
    crypto::Schnorr::PrivateKey privkey;

    // Retry until we get a valid private key
    do {
        RAND_bytes(privkey.data(), static_cast<int>(privkey.size()));
    } while (!crypto::Schnorr::ValidatePrivateKey(privkey));
    
    // Derive public key
    auto pubkey_opt = crypto::Schnorr::GetPublicKey(privkey);
    if (!pubkey_opt) {
        std::cerr << "Failed to derive public key!" << std::endl;
        return;
    }
    
    auto pubkey = *pubkey_opt;
    
    // Print keys
    std::cout << "Private Key: ";
    for (uint8_t byte : privkey) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;
    
    std::cout << "Public Key:  ";
    for (uint8_t byte : pubkey) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;
    
    // Save to files
    std::ofstream privkey_file("private.key", std::ios::binary);
    privkey_file.write(reinterpret_cast<const char*>(privkey.data()), privkey.size());
    privkey_file.close();
    
    std::ofstream pubkey_file("public.key", std::ios::binary);
    pubkey_file.write(reinterpret_cast<const char*>(pubkey.data()), pubkey.size());
    pubkey_file.close();
    
    std::cout << std::endl;
    std::cout << "Keys saved to private.key and public.key" << std::endl;
    std::cout << "WARNING: Keep private.key secure!" << std::endl;
}

/**
 * Sign a message
 */
void SignMessage(const std::string& message_file, const std::string& privkey_file) {
    // Read private key
    std::ifstream key_file(privkey_file, std::ios::binary);
    if (!key_file) {
        std::cerr << "Failed to open private key file!" << std::endl;
        return;
    }
    
    crypto::Schnorr::PrivateKey privkey;
    key_file.read(reinterpret_cast<char*>(privkey.data()), privkey.size());
    key_file.close();
    
    // Read message
    std::ifstream msg_file(message_file, std::ios::binary);
    if (!msg_file) {
        std::cerr << "Failed to open message file!" << std::endl;
        return;
    }
    
    std::vector<uint8_t> message(
        (std::istreambuf_iterator<char>(msg_file)),
        std::istreambuf_iterator<char>()
    );
    msg_file.close();
    
    // Hash message
    crypto::SHA256 hasher;
    hasher.Write(message.data(), message.size());
    auto msg_hash = hasher.Finalize();
    
    // Sign
    auto signature_opt = crypto::Schnorr::Sign(msg_hash, privkey);
    if (!signature_opt) {
        std::cerr << "Failed to sign message!" << std::endl;
        return;
    }
    
    auto signature = *signature_opt;
    
    // Print signature
    std::cout << "Signature: ";
    for (uint8_t byte : signature) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;
    
    // Save signature
    std::ofstream sig_file("message.sig", std::ios::binary);
    sig_file.write(reinterpret_cast<const char*>(signature.data()), signature.size());
    sig_file.close();
    
    std::cout << "Signature saved to message.sig" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "ParthenonChain - Key Tools" << std::endl;
    std::cout << "==========================" << std::endl << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " generate - Generate new key pair" << std::endl;
        std::cout << "  " << argv[0] << " sign <message_file> <private_key_file> - Sign a message" << std::endl;
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "generate") {
        GenerateKeyPair();
    } else if (command == "sign" && argc >= 4) {
        SignMessage(argv[2], argv[3]);
    } else {
        std::cerr << "Unknown command or missing arguments" << std::endl;
        return 1;
    }
    
    return 0;
}
