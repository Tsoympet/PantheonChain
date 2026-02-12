// ParthenonChain - Hardware Wallet Firmware Verification Implementation

#include "firmware_verification.h"

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <algorithm>
#include <ctime>

namespace parthenon {
namespace wallet {
namespace hardware {

// FirmwareVerifier implementation
FirmwareVerifier::FirmwareVerifier() {
    // Initialize with known vendor keys (in production, load from secure storage)

    // Ledger public keys
    VendorKeys ledger_keys;
    ledger_keys.vendor_name = "Ledger";
    ledger_keys.certificate_url = "https://www.ledger.com/certificates";
    // In production, these would be real public keys
    ledger_keys.public_keys.push_back(std::vector<uint8_t>(32, 0x01));
    vendor_keys_["Ledger"] = ledger_keys;

    // Trezor public keys
    VendorKeys trezor_keys;
    trezor_keys.vendor_name = "Trezor";
    trezor_keys.certificate_url = "https://trezor.io/security";
    trezor_keys.public_keys.push_back(std::vector<uint8_t>(32, 0x02));
    vendor_keys_["Trezor"] = trezor_keys;

    // KeepKey public keys
    VendorKeys keepkey_keys;
    keepkey_keys.vendor_name = "KeepKey";
    keepkey_keys.certificate_url = "https://shapeshift.com/keepkey/security";
    keepkey_keys.public_keys.push_back(std::vector<uint8_t>(32, 0x03));
    vendor_keys_["KeepKey"] = keepkey_keys;
}

FirmwareVerifier::~FirmwareVerifier() {}

VerificationResult FirmwareVerifier::VerifyFirmware(const std::vector<uint8_t>& device_firmware,
                                                    const std::string& vendor) {
    VerificationResult result;

    // Check if vendor is known
    if (vendor_keys_.find(vendor) == vendor_keys_.end()) {
        result.status = VerificationStatus::UNKNOWN_VENDOR;
        result.message = "Vendor not recognized: " + vendor;
        return result;
    }

    // Compute firmware hash
    auto firmware_hash = ComputeHash(device_firmware);

    // Check if firmware is in known database
    auto firmware_info_opt = GetFirmwareInfo(firmware_hash);

    if (firmware_info_opt) {
        result.firmware_info = *firmware_info_opt;

        // Verify signature
        bool sig_valid = VerifySignature(device_firmware, result.firmware_info.signature, vendor);

        if (!sig_valid) {
            result.status = VerificationStatus::INVALID_SIGNATURE;
            result.message = "Firmware signature verification failed";
            return result;
        }

        // Check for latest version
        auto latest = CheckLatestVersion(vendor, result.firmware_info.version);
        result.is_latest_version = !latest.has_value();

        // Get security advisories
        result.security_advisories = GetSecurityAdvisories(vendor, result.firmware_info.version);

        if (!result.security_advisories.empty()) {
            result.status = VerificationStatus::EXPIRED;
            result.message = "Firmware has known security vulnerabilities";
            return result;
        }

        result.status = VerificationStatus::VALID;
        result.message = "Firmware verified successfully";
        return result;
    }

    // Firmware not in database - untrusted
    result.status = VerificationStatus::UNTRUSTED;
    result.message = "Firmware not found in official database";
    return result;
}

bool FirmwareVerifier::VerifyHash(const std::vector<uint8_t>& firmware_hash,
                                  const std::string& vendor, const std::string& version) {
    auto firmware_info_opt = GetFirmwareInfo(firmware_hash);

    if (!firmware_info_opt) {
        return false;
    }

    auto& info = *firmware_info_opt;
    return info.vendor == vendor && info.version == version;
}

bool FirmwareVerifier::VerifySignature(const std::vector<uint8_t>& firmware,
                                       const std::vector<uint8_t>& signature,
                                       const std::string& vendor) {
    auto it = vendor_keys_.find(vendor);
    if (it == vendor_keys_.end()) {
        return false;
    }

    auto firmware_hash = ComputeHash(firmware);

    // Try all vendor public keys (for key rotation support)
    for (const auto& pubkey : it->second.public_keys) {
        if (VerifySchnorrSignature(firmware_hash, signature, pubkey)) {
            return true;
        }
    }

    return false;
}

std::optional<std::string>
FirmwareVerifier::CheckLatestVersion(const std::string& vendor,
                                     const std::string& current_version) {
    // In production, query remote API for latest version
    // For now, return nullopt indicating current is latest
    [[maybe_unused]] const std::string& v = vendor;
    [[maybe_unused]] const std::string& cv = current_version;

    return std::nullopt;
}

std::vector<std::string> FirmwareVerifier::GetSecurityAdvisories(const std::string& vendor,
                                                                 const std::string& version) {
    auto vendor_it = security_advisories_.find(vendor);
    if (vendor_it == security_advisories_.end()) {
        return {};
    }

    auto version_it = vendor_it->second.find(version);
    if (version_it == vendor_it->second.end()) {
        return {};
    }

    return version_it->second;
}

void FirmwareVerifier::AddVendorKeys(const VendorKeys& vendor_keys) {
    vendor_keys_[vendor_keys.vendor_name] = vendor_keys;
}

void FirmwareVerifier::AddKnownFirmware(const FirmwareInfo& firmware_info) {
    known_firmware_[firmware_info.hash] = firmware_info;
}

std::optional<FirmwareInfo>
FirmwareVerifier::GetFirmwareInfo(const std::vector<uint8_t>& firmware_hash) {
    auto it = known_firmware_.find(firmware_hash);
    if (it == known_firmware_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool FirmwareVerifier::LoadVendorKeys([[maybe_unused]] const std::string& filename) {
    // In production, load vendor keys from file
    return true;
}

bool FirmwareVerifier::LoadFirmwareDatabase([[maybe_unused]] const std::string& filename) {
    // In production, load firmware database from file
    return true;
}

bool FirmwareVerifier::UpdateFirmwareDatabase([[maybe_unused]] const std::string& url) {
    // In production, download and update firmware database from URL
    return true;
}

std::vector<uint8_t> FirmwareVerifier::ComputeHash(const std::vector<uint8_t>& data) {
    auto hash_arr = crypto::SHA256::Hash256(data);
    return std::vector<uint8_t>(hash_arr.begin(), hash_arr.end());
}

bool FirmwareVerifier::VerifySchnorrSignature(const std::vector<uint8_t>& message,
                                              const std::vector<uint8_t>& signature,
                                              const std::vector<uint8_t>& public_key) {
    if (message.size() != crypto::Schnorr::PRIVATE_KEY_SIZE ||
        signature.size() != crypto::Schnorr::SIGNATURE_SIZE ||
        public_key.size() != crypto::Schnorr::PUBLIC_KEY_SIZE) {
        return false;
    }

    crypto::Schnorr::PublicKey schnorr_pubkey{};
    std::copy(public_key.begin(), public_key.end(), schnorr_pubkey.begin());

    crypto::Schnorr::Signature schnorr_signature{};
    std::copy(signature.begin(), signature.end(), schnorr_signature.begin());

    return crypto::Schnorr::Verify(schnorr_pubkey, message.data(), schnorr_signature);
}

// FirmwareUpdateManager implementation
std::optional<FirmwareInfo>
FirmwareUpdateManager::CheckForUpdates(const std::string& vendor,
                                       const std::string& current_version) {
    auto latest = verifier_.CheckLatestVersion(vendor, current_version);

    if (!latest) {
        return std::nullopt;
    }

    // In production, fetch firmware info for latest version
    FirmwareInfo info;
    info.vendor = vendor;
    info.version = *latest;
    return info;
}

std::optional<std::vector<uint8_t>>
FirmwareUpdateManager::DownloadFirmware([[maybe_unused]] const std::string& vendor,
                                        [[maybe_unused]] const std::string& version) {
    // In production, download firmware from vendor's official server
    return std::nullopt;
}

VerificationResult FirmwareUpdateManager::VerifyUpdate(const std::vector<uint8_t>& firmware,
                                                       const std::string& vendor) {
    return verifier_.VerifyFirmware(firmware, vendor);
}

bool FirmwareUpdateManager::InstallUpdate([[maybe_unused]] const std::vector<uint8_t>& device_id,
                                          [[maybe_unused]] const std::vector<uint8_t>& firmware) {
    // In production, use device-specific protocol to install firmware
    return false;
}

// BootloaderVerifier implementation
bool BootloaderVerifier::VerifyBootloader(
    [[maybe_unused]] const std::vector<uint8_t>& bootloader_data,
    [[maybe_unused]] const std::string& vendor) {
    // In production, verify bootloader signature and hash
    return true;
}

std::optional<std::string>
BootloaderVerifier::CheckBootloaderVersion([[maybe_unused]] const std::string& vendor,
                                           [[maybe_unused]] const std::string& version) {
    // In production, check against known bootloader versions
    return std::nullopt;
}

bool BootloaderVerifier::VerifySecureBoot([[maybe_unused]] const std::vector<uint8_t>& device_id) {
    // In production, query device for secure boot status
    return true;
}

// SupplyChainVerifier implementation
VerificationStatus
SupplyChainVerifier::VerifyDeviceSeals([[maybe_unused]] const std::string& device_serial,
                                       [[maybe_unused]] const std::string& vendor) {
    // In production, verify packaging seals and anti-tamper stickers
    return VerificationStatus::VALID;
}

bool SupplyChainVerifier::CheckDeviceRegistry([[maybe_unused]] const std::string& device_serial,
                                              [[maybe_unused]] const std::string& vendor) {
    // In production, query vendor's device registry
    return true;
}

bool SupplyChainVerifier::CheckStolenRegistry([[maybe_unused]] const std::string& device_serial) {
    // In production, check against stolen device database
    return true;
}

}  // namespace hardware
}  // namespace wallet
}  // namespace parthenon
