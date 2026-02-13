// ParthenonChain - Hardware Wallet Firmware Verification
// Verify authenticity and integrity of hardware wallet firmware

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace parthenon {
namespace wallet {
namespace hardware {

/**
 * Firmware information
 */
struct FirmwareInfo {
    std::string vendor;              // e.g., "Ledger", "Trezor"
    std::string model;               // e.g., "Nano S", "Model T"
    std::string version;             // e.g., "2.1.0"
    std::vector<uint8_t> hash;       // SHA-256 hash of firmware
    std::vector<uint8_t> signature;  // Vendor signature
    std::vector<uint8_t> image;      // Optional firmware image blob
    uint64_t build_timestamp;
    std::string release_notes_url;

    FirmwareInfo() : build_timestamp(0) {}
};

/**
 * Vendor public keys for signature verification
 */
struct VendorKeys {
    std::string vendor_name;
    std::vector<std::vector<uint8_t>> public_keys;  // Multiple keys for key rotation
    std::string certificate_url;                    // URL to vendor certificate
};

/**
 * Firmware verification result
 */
enum class VerificationStatus {
    VALID,              // Firmware is authentic and unmodified
    INVALID_SIGNATURE,  // Signature verification failed
    UNKNOWN_VENDOR,     // Vendor not recognized
    HASH_MISMATCH,      // Firmware hash doesn't match expected
    EXPIRED,            // Firmware is too old (security risk)
    UNTRUSTED,          // Firmware not from official source
    ERROR               // Verification error
};

struct VerificationResult {
    VerificationStatus status;
    std::string message;
    FirmwareInfo firmware_info;
    bool is_latest_version;
    std::vector<std::string> security_advisories;  // Known vulnerabilities

    VerificationResult() : is_latest_version(false) {}
};

/**
 * Hardware Wallet Firmware Verifier
 *
 * Verifies firmware authenticity using:
 * - Vendor digital signatures
 * - Hash verification
 * - Known firmware database
 * - Security advisory checking
 */
class FirmwareVerifier {
  public:
    FirmwareVerifier();
    ~FirmwareVerifier();

    /**
     * Verify firmware from device
     * @param device_firmware Firmware data read from device
     * @param vendor Vendor name (e.g., "Ledger", "Trezor")
     * @return Verification result
     */
    VerificationResult VerifyFirmware(const std::vector<uint8_t>& device_firmware,
                                      const std::string& vendor);

    /**
     * Verify firmware update and enforce anti-rollback checks
     * @param device_firmware Firmware data read from device
     * @param vendor Vendor name (e.g., "Ledger", "Trezor")
     * @param current_version Current firmware version installed on device
     * @return Verification result
     */
    VerificationResult VerifyFirmwareUpdate(const std::vector<uint8_t>& device_firmware,
                                            const std::string& vendor,
                                            const std::string& current_version);

    /**
     * Verify firmware hash against known database
     * @param firmware_hash SHA-256 hash of firmware
     * @param vendor Vendor name
     * @param version Firmware version
     * @return true if hash matches official firmware
     */
    bool VerifyHash(const std::vector<uint8_t>& firmware_hash, const std::string& vendor,
                    const std::string& version);

    /**
     * Verify vendor signature on firmware
     * @param firmware Firmware data
     * @param signature Vendor signature
     * @param vendor Vendor name
     * @return true if signature is valid
     */
    bool VerifySignature(const std::vector<uint8_t>& firmware,
                         const std::vector<uint8_t>& signature, const std::string& vendor);

    /**
     * Check if firmware version is latest
     * @param vendor Vendor name
     * @param current_version Current firmware version
     * @return Latest available version
     */
    std::optional<std::string> CheckLatestVersion(const std::string& vendor,
                                                  const std::string& current_version);

    /**
     * Get security advisories for firmware
     * @param vendor Vendor name
     * @param version Firmware version
     * @return List of known security issues
     */
    std::vector<std::string> GetSecurityAdvisories(const std::string& vendor,
                                                   const std::string& version);

    /**
     * Add vendor public key for verification
     * @param vendor_keys Vendor name and public keys
     */
    void AddVendorKeys(const VendorKeys& vendor_keys);

    /**
     * Revoke a vendor public key (key rotation)
     * @param vendor Vendor name
     * @param public_key Vendor public key to revoke
     */
    void RevokeVendorKey(const std::string& vendor, const std::vector<uint8_t>& public_key);

    /**
     * Add known firmware to database
     * @param firmware_info Firmware information
     */
    void AddKnownFirmware(const FirmwareInfo& firmware_info);

    /**
     * Add security advisory for a specific firmware version
     * @param vendor Vendor name
     * @param version Firmware version
     * @param advisory Advisory description or ID
     */
    void AddSecurityAdvisory(const std::string& vendor, const std::string& version,
                             const std::string& advisory);

    /**
     * Get firmware info by hash
     * @param firmware_hash SHA-256 hash
     * @return Firmware info if found
     */
    std::optional<FirmwareInfo> GetFirmwareInfo(const std::vector<uint8_t>& firmware_hash);

    /**
     * Get firmware info by vendor and version
     * @param vendor Vendor name
     * @param version Firmware version
     * @return Firmware info if found
     */
    std::optional<FirmwareInfo> GetFirmwareInfo(const std::string& vendor,
                                                const std::string& version);

    /**
     * Load vendor keys from file
     * @param filename Path to vendor keys file
     * @return true if loaded successfully
     */
    bool LoadVendorKeys(const std::string& filename);

    /**
     * Load firmware database from file
     * @param filename Path to firmware database
     * @return true if loaded successfully
     */
    bool LoadFirmwareDatabase(const std::string& filename);

    /**
     * Update firmware database from remote source
     * @param url URL to firmware database
     * @return true if updated successfully
     */
    bool UpdateFirmwareDatabase(const std::string& url);

  private:
    std::map<std::string, VendorKeys> vendor_keys_;
    std::map<std::vector<uint8_t>, FirmwareInfo> known_firmware_;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> security_advisories_;
    std::map<std::string, std::set<std::vector<uint8_t>>> revoked_vendor_keys_;

    std::vector<uint8_t> ComputeHash(const std::vector<uint8_t>& data);
    bool VerifySchnorrSignature(const std::vector<uint8_t>& message,
                                const std::vector<uint8_t>& signature,
                                const std::vector<uint8_t>& public_key);
    bool IsVendorKeyRevoked(const std::string& vendor,
                            const std::vector<uint8_t>& public_key) const;
};

/**
 * Firmware Update Manager
 * Safely manage firmware updates for hardware wallets
 */
class FirmwareUpdateManager {
  public:
    /**
     * Check for available updates
     * @param vendor Vendor name
     * @param current_version Current firmware version
     * @return Available update info
     */
    std::optional<FirmwareInfo> CheckForUpdates(const std::string& vendor,
                                                const std::string& current_version);

    /**
     * Download firmware update
     * @param vendor Vendor name
     * @param version Version to download
     * @return Firmware data
     */
    std::optional<std::vector<uint8_t>> DownloadFirmware(const std::string& vendor,
                                                         const std::string& version);

    /**
     * Verify downloaded firmware before update
     * @param firmware Firmware data
     * @param vendor Vendor name
     * @return Verification result
     */
    VerificationResult VerifyUpdate(const std::vector<uint8_t>& firmware,
                                    const std::string& vendor);

    /**
     * Verify downloaded firmware before update with anti-rollback checks
     * @param firmware Firmware data
     * @param vendor Vendor name
     * @param current_version Current firmware version installed on device
     * @return Verification result
     */
    VerificationResult VerifyUpdate(const std::vector<uint8_t>& firmware, const std::string& vendor,
                                    const std::string& current_version);

    /**
     * Install firmware update to device
     * @param device_id Device identifier
     * @param firmware Verified firmware data
     * @return true if installation successful
     */
    bool InstallUpdate(const std::vector<uint8_t>& device_id, const std::vector<uint8_t>& firmware);

  private:
    FirmwareVerifier verifier_;
};

/**
 * Bootloader Verification
 * Verify device bootloader is authentic
 */
class BootloaderVerifier {
  public:
    /**
     * Verify bootloader signature
     * @param bootloader_data Bootloader data
     * @param vendor Vendor name
     * @return true if bootloader is authentic
     */
    static bool VerifyBootloader(const std::vector<uint8_t>& bootloader_data,
                                 const std::string& vendor);

    /**
     * Check bootloader version
     * @param vendor Vendor name
     * @param version Current bootloader version
     * @return Latest bootloader version
     */
    static std::optional<std::string> CheckBootloaderVersion(const std::string& vendor,
                                                             const std::string& version);

    /**
     * Verify secure boot is enabled
     * @param device_id Device identifier
     * @return true if secure boot is enabled
     */
    static bool VerifySecureBoot(const std::vector<uint8_t>& device_id);
};

/**
 * Supply Chain Verification
 * Verify device hasn't been tampered with during shipping
 */
class SupplyChainVerifier {
  public:
    /**
     * Verify device seals and packaging
     * @param device_serial Device serial number
     * @param vendor Vendor name
     * @return Verification status
     */
    static VerificationStatus VerifyDeviceSeals(const std::string& device_serial,
                                                const std::string& vendor);

    /**
     * Check device against vendor registry
     * @param device_serial Device serial number
     * @param vendor Vendor name
     * @return true if device is registered
     */
    static bool CheckDeviceRegistry(const std::string& device_serial, const std::string& vendor);

    /**
     * Verify device has not been reported stolen
     * @param device_serial Device serial number
     * @return true if device is not stolen
     */
    static bool CheckStolenRegistry(const std::string& device_serial);
};

}  // namespace hardware
}  // namespace wallet
}  // namespace parthenon
