// ParthenonChain - RPC Input Validation Utilities
// Comprehensive input validation and sanitization for RPC methods

#ifndef PARTHENON_RPC_VALIDATION_H
#define PARTHENON_RPC_VALIDATION_H

#include <cstdint>
#include <cctype>
#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace parthenon {
namespace rpc {

/**
 * Input validation utilities for RPC methods
 * Prevents injection attacks and validates data types
 */
class InputValidator {
  public:
    /**
     * Validate block height parameter
     * @param height Block height to validate
     * @param max_height Maximum allowed height
     * @return true if valid
     */
    static bool ValidateBlockHeight(uint64_t height, uint64_t max_height) {
        return height <= max_height;
    }

    /**
     * Validate amount parameter (prevents negative or excessive values)
     * @param amount Amount to validate
     * @param max_amount Maximum allowed amount
     * @return true if valid
     */
    static bool ValidateAmount(uint64_t amount, uint64_t max_amount) {
        return amount > 0 && amount <= max_amount;
    }

    /**
     * Validate address format (basic hex validation)
     * @param address Address string to validate
     * @return true if format is valid
     */
    static bool ValidateAddress(const std::string& address) {
        if (address.empty() || address.length() > 100) {
            return false;
        }
        // Check for valid hex characters
        std::regex hex_pattern("^[0-9a-fA-F]+$");
        return std::regex_match(address, hex_pattern);
    }

    /**
     * Validate asset name
     * @param asset Asset name string
     * @return true if valid asset name
     */
    static bool ValidateAssetName(const std::string& asset) {
        return asset == "TALANTON" || asset == "DRACHMA" || asset == "OBOLOS";
    }

    /**
     * Validate transaction hash format
     * @param hash Transaction hash (hex string)
     * @return true if valid format
     */
    static bool ValidateTxHash(const std::string& hash) {
        if (hash.length() != 64) {  // SHA256 = 32 bytes = 64 hex chars
            return false;
        }
        std::regex hex_pattern("^[0-9a-fA-F]{64}$");
        return std::regex_match(hash, hex_pattern);
    }

    /**
     * Sanitize string input (remove potentially dangerous characters)
     * @param input Input string
     * @return Sanitized string
     */
    static std::string SanitizeString(const std::string& input) {
        std::string output;
        output.reserve(input.length());

        for (char c : input) {
            // Allow alphanumeric, spaces, and basic punctuation
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc) || c == ' ' || c == '-' || c == '_' || c == '.') {
                output += c;
            }
        }

        // Limit length
        if (output.length() > 256) {
            output = output.substr(0, 256);
        }

        return output;
    }

    /**
     * Validate and parse uint64_t from string
     * @param str String to parse
     * @return Parsed value or std::nullopt if invalid
     */
    static std::optional<uint64_t> ParseUint64(const std::string& str) {
        if (str.empty() || str.length() > 20) {  // max uint64_t is 20 digits
            return std::nullopt;
        }

        size_t pos = 0;
        for (; pos < str.length(); ++pos) {
            unsigned char uc = static_cast<unsigned char>(str[pos]);
            if (!std::isdigit(uc)) {
                return std::nullopt;
            }
        }

        // Manual conversion with overflow check (avoids exception-based parsing).
        uint64_t value = 0;
        for (char c : str) {
            uint64_t digit = static_cast<uint64_t>(c - '0');
            if (value > (UINT64_MAX - digit) / 10ULL) {
                return std::nullopt;
            }
            value = value * 10ULL + digit;
        }

        return value;
    }

    /**
     * Validate fee rate parameter
     * @param fee_rate Fee rate in basis points
     * @return true if valid (0-10000 basis points = 0-100%)
     */
    static bool ValidateFeeRate(uint64_t fee_rate) {
        return fee_rate <= 10000;  // Max 100%
    }

    /**
     * Validate array parameter length
     * @param length Array length
     * @param max_length Maximum allowed length
     * @return true if valid
     */
    static bool ValidateArrayLength(size_t length, size_t max_length = 1000) {
        return length > 0 && length <= max_length;
    }

    /**
     * Validate public key format
     * @param pubkey Public key bytes
     * @return true if valid format (33 or 65 bytes for compressed/uncompressed)
     */
    static bool ValidatePubKey(const std::vector<uint8_t>& pubkey) {
        return pubkey.size() == 33 || pubkey.size() == 65;
    }

    /**
     * Validate signature format
     * @param signature Signature bytes
     * @return true if valid format (64 bytes for Schnorr)
     */
    static bool ValidateSignature(const std::vector<uint8_t>& signature) {
        return signature.size() == 64;
    }
};

}  // namespace rpc
}  // namespace parthenon

#endif  // PARTHENON_RPC_VALIDATION_H
