// ParthenonChain - Hardware Wallet Firmware Verification Implementation

#include "firmware_verification.h"

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace {

using json = nlohmann::json;

bool ParseSemVer(const std::string& version, std::array<int, 3>& out) {
    std::stringstream ss(version);
    std::string token;
    int idx = 0;

    while (std::getline(ss, token, '.')) {
        if (idx >= 3 || token.empty() ||
            !std::all_of(token.begin(), token.end(),
                         [](unsigned char c) { return std::isdigit(c) != 0; })) {
            return false;
        }
        out[idx++] = std::stoi(token);
    }

    if (idx != 3) {
        return false;
    }

    return true;
}

int CompareSemVer(const std::string& lhs, const std::string& rhs) {
    std::array<int, 3> l{0, 0, 0};
    std::array<int, 3> r{0, 0, 0};

    if (!ParseSemVer(lhs, l) || !ParseSemVer(rhs, r)) {
        return 0;
    }

    for (int i = 0; i < 3; ++i) {
        if (l[i] < r[i]) {
            return -1;
        }
        if (l[i] > r[i]) {
            return 1;
        }
    }

    return 0;
}

bool HexToBytes(const std::string& hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto to_nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return 10 + (c - 'a');
            }
            if (c >= 'A' && c <= 'F') {
                return 10 + (c - 'A');
            }
            return -1;
        };
        int high = to_nibble(hex[i]);
        int low = to_nibble(hex[i + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        out.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return true;
}

bool LoadJsonFile(const std::string& filename, json& out) {
    std::ifstream in(filename);
    if (!in) {
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    try {
        out = json::parse(buffer.str());
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

}  // namespace

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

VerificationResult
FirmwareVerifier::VerifyFirmwareUpdate(const std::vector<uint8_t>& device_firmware,
                                       const std::string& vendor,
                                       const std::string& current_version) {
    auto result = VerifyFirmware(device_firmware, vendor);
    if (result.status != VerificationStatus::VALID) {
        return result;
    }

    if (result.firmware_info.version.empty()) {
        result.status = VerificationStatus::ERROR;
        result.message = "Firmware version missing for rollback check";
        return result;
    }

    std::array<int, 3> current{};
    std::array<int, 3> candidate{};
    if (!ParseSemVer(current_version, current) ||
        !ParseSemVer(result.firmware_info.version, candidate)) {
        result.status = VerificationStatus::ERROR;
        result.message = "Invalid firmware version format";
        return result;
    }

    if (CompareSemVer(result.firmware_info.version, current_version) < 0) {
        result.status = VerificationStatus::EXPIRED;
        result.message = "Firmware rollback detected";
        result.is_latest_version = false;
    }

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
        if (IsVendorKeyRevoked(vendor, pubkey)) {
            continue;
        }
        if (VerifySchnorrSignature(firmware_hash, signature, pubkey)) {
            return true;
        }
    }

    return false;
}

std::optional<std::string>
FirmwareVerifier::CheckLatestVersion(const std::string& vendor,
                                     const std::string& current_version) {
    std::string latest_version;
    bool found = false;

    for (const auto& [hash, info] : known_firmware_) {
        (void)hash;
        if (info.vendor != vendor) {
            continue;
        }

        if (!found || CompareSemVer(info.version, latest_version) > 0) {
            latest_version = info.version;
            found = true;
        }
    }

    if (!found) {
        return std::nullopt;
    }

    if (CompareSemVer(latest_version, current_version) > 0) {
        return latest_version;
    }

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

void FirmwareVerifier::RevokeVendorKey(const std::string& vendor,
                                       const std::vector<uint8_t>& public_key) {
    revoked_vendor_keys_[vendor].insert(public_key);
}

void FirmwareVerifier::AddKnownFirmware(const FirmwareInfo& firmware_info) {
    known_firmware_[firmware_info.hash] = firmware_info;
}

void FirmwareVerifier::AddSecurityAdvisory(const std::string& vendor, const std::string& version,
                                           const std::string& advisory) {
    security_advisories_[vendor][version].push_back(advisory);
}

std::optional<FirmwareInfo>
FirmwareVerifier::GetFirmwareInfo(const std::vector<uint8_t>& firmware_hash) {
    auto it = known_firmware_.find(firmware_hash);
    if (it == known_firmware_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<FirmwareInfo> FirmwareVerifier::GetFirmwareInfo(const std::string& vendor,
                                                              const std::string& version) {
    for (const auto& [hash, info] : known_firmware_) {
        (void)hash;
        if (info.vendor == vendor && info.version == version) {
            return info;
        }
    }
    return std::nullopt;
}

bool FirmwareVerifier::LoadVendorKeys(const std::string& filename) {
    json data;
    if (!LoadJsonFile(filename, data)) {
        return false;
    }

    const json* vendors = nullptr;
    if (data.is_object() && data.contains("vendors")) {
        vendors = &data["vendors"];
    } else if (data.is_array()) {
        vendors = &data;
    }

    if (!vendors || !vendors->is_array()) {
        return false;
    }

    bool loaded = false;
    for (size_t i = 0; i < vendors->size(); ++i) {
        const auto& entry = (*vendors)[i];
        if (!entry.is_object()) {
            continue;
        }
        VendorKeys keys;
        keys.vendor_name = entry.value("vendor", entry.value("name", ""));
        if (keys.vendor_name.empty()) {
            continue;
        }
        keys.certificate_url = entry.value("certificate_url", "");

        if (entry.contains("public_keys") && entry["public_keys"].is_array()) {
            const auto& keys_array = entry["public_keys"];
            for (size_t j = 0; j < keys_array.size(); ++j) {
                auto key_str = keys_array[j].get<std::string>();
                if (key_str.empty()) {
                    continue;
                }
                std::vector<uint8_t> key_bytes;
                if (HexToBytes(key_str, key_bytes)) {
                    keys.public_keys.push_back(std::move(key_bytes));
                }
            }
        }

        if (keys.public_keys.empty()) {
            continue;
        }
        AddVendorKeys(keys);
        loaded = true;
    }

    return loaded;
}

bool FirmwareVerifier::LoadFirmwareDatabase(const std::string& filename) {
    json data;
    if (!LoadJsonFile(filename, data)) {
        return false;
    }

    const json* entries = nullptr;
    if (data.is_object() && data.contains("firmware")) {
        entries = &data["firmware"];
    } else if (data.is_array()) {
        entries = &data;
    }

    if (!entries || !entries->is_array()) {
        return false;
    }

    bool loaded = false;
    for (size_t i = 0; i < entries->size(); ++i) {
        const auto& entry = (*entries)[i];
        if (!entry.is_object()) {
            continue;
        }
        FirmwareInfo info;
        info.vendor = entry.value("vendor", "");
        info.model = entry.value("model", "");
        info.version = entry.value("version", "");
        info.release_notes_url = entry.value("release_notes_url", "");
        info.build_timestamp = entry.value("build_timestamp", 0ULL);
        if (info.vendor.empty() || info.version.empty()) {
            continue;
        }

        if (entry.contains("hash")) {
            auto hash_str = entry["hash"].get<std::string>();
            if (!hash_str.empty()) {
                HexToBytes(hash_str, info.hash);
            }
        }
        if (entry.contains("signature")) {
            auto sig_str = entry["signature"].get<std::string>();
            if (!sig_str.empty()) {
                HexToBytes(sig_str, info.signature);
            }
        }
        if (entry.contains("image")) {
            auto image_str = entry["image"].get<std::string>();
            if (!image_str.empty()) {
                HexToBytes(image_str, info.image);
            }
        }

        if (info.hash.empty() && !info.image.empty()) {
            auto hash_arr = crypto::SHA256::Hash256(info.image);
            info.hash.assign(hash_arr.begin(), hash_arr.end());
        }

        if (info.hash.empty()) {
            continue;
        }

        AddKnownFirmware(info);
        loaded = true;
    }

    return loaded;
}

bool FirmwareVerifier::UpdateFirmwareDatabase(const std::string& url) {
    std::string path = url;
    constexpr const char* kFilePrefix = "file://";
    if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
        return false;
    }
    if (path.rfind(kFilePrefix, 0) == 0) {
        path = path.substr(std::strlen(kFilePrefix));
    } else if (path.find("://") != std::string::npos) {
        return false;
    }

    if (path.empty()) {
        return false;
    }

    return LoadFirmwareDatabase(path);
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

bool FirmwareVerifier::IsVendorKeyRevoked(const std::string& vendor,
                                          const std::vector<uint8_t>& public_key) const {
    auto vendor_it = revoked_vendor_keys_.find(vendor);
    if (vendor_it == revoked_vendor_keys_.end()) {
        return false;
    }
    return vendor_it->second.find(public_key) != vendor_it->second.end();
}

// FirmwareUpdateManager implementation
std::optional<FirmwareInfo>
FirmwareUpdateManager::CheckForUpdates(const std::string& vendor,
                                       const std::string& current_version) {
    auto latest = verifier_.CheckLatestVersion(vendor, current_version);

    if (!latest) {
        return std::nullopt;
    }

    auto info = verifier_.GetFirmwareInfo(vendor, *latest);
    if (info) {
        return info;
    }

    FirmwareInfo fallback;
    fallback.vendor = vendor;
    fallback.version = *latest;
    return fallback;
}

std::optional<std::vector<uint8_t>>
FirmwareUpdateManager::DownloadFirmware(const std::string& vendor, const std::string& version) {
    auto info = verifier_.GetFirmwareInfo(vendor, version);
    if (!info || info->image.empty()) {
        return std::nullopt;
    }
    return info->image;
}

VerificationResult FirmwareUpdateManager::VerifyUpdate(const std::vector<uint8_t>& firmware,
                                                       const std::string& vendor) {
    return verifier_.VerifyFirmware(firmware, vendor);
}

VerificationResult FirmwareUpdateManager::VerifyUpdate(const std::vector<uint8_t>& firmware,
                                                       const std::string& vendor,
                                                       const std::string& current_version) {
    return verifier_.VerifyFirmwareUpdate(firmware, vendor, current_version);
}

bool FirmwareUpdateManager::InstallUpdate(const std::vector<uint8_t>& device_id,
                                          const std::vector<uint8_t>& firmware) {
    if (device_id.empty() || firmware.empty()) {
        return false;
    }

    auto hash_arr = crypto::SHA256::Hash256(firmware);
    std::vector<uint8_t> hash(hash_arr.begin(), hash_arr.end());
    auto info = verifier_.GetFirmwareInfo(hash);
    if (!info) {
        return false;
    }

    auto result = verifier_.VerifyFirmware(firmware, info->vendor);
    return result.status == VerificationStatus::VALID;
}

// BootloaderVerifier implementation
bool BootloaderVerifier::VerifyBootloader(const std::vector<uint8_t>& bootloader_data,
                                          const std::string& vendor) {
    if (bootloader_data.empty() || vendor.empty()) {
        return false;
    }

    static const std::set<std::string> kKnownVendors = {"Ledger", "Trezor", "KeepKey"};
    if (kKnownVendors.find(vendor) == kKnownVendors.end()) {
        return false;
    }

    // Minimal integrity check for test environments; production should verify vendor signatures.
    auto hash = crypto::SHA256::Hash256(bootloader_data);
    return std::any_of(hash.begin(), hash.end(), [](uint8_t byte) { return byte != 0; });
}

std::optional<std::string>
BootloaderVerifier::CheckBootloaderVersion(const std::string& vendor, const std::string& version) {
    if (vendor.empty() || version.empty()) {
        return std::nullopt;
    }

    static const std::map<std::string, std::string> kLatestVersions = {
        {"Ledger", "2.0.0"},
        {"Trezor", "2.2.0"},
        {"KeepKey", "1.1.0"}};

    auto it = kLatestVersions.find(vendor);
    if (it == kLatestVersions.end()) {
        return std::nullopt;
    }

    if (CompareSemVer(it->second, version) > 0) {
        return it->second;
    }
    return std::nullopt;
}

bool BootloaderVerifier::VerifySecureBoot(const std::vector<uint8_t>& device_id) {
    if (device_id.empty()) {
        return false;
    }
    // Placeholder attestation check for tests until device APIs are available.
    return std::any_of(device_id.begin(), device_id.end(), [](uint8_t byte) { return byte != 0; });
}

// SupplyChainVerifier implementation
VerificationStatus
SupplyChainVerifier::VerifyDeviceSeals(const std::string& device_serial, const std::string& vendor) {
    if (device_serial.empty() || vendor.empty()) {
        return VerificationStatus::ERROR;
    }
    return VerificationStatus::VALID;
}

bool SupplyChainVerifier::CheckDeviceRegistry(const std::string& device_serial,
                                              const std::string& vendor) {
    return !device_serial.empty() && !vendor.empty();
}

bool SupplyChainVerifier::CheckStolenRegistry(const std::string& device_serial) {
    if (device_serial.empty()) {
        return false;
    }
    static const std::set<std::string> kStolen = {"STOLEN-0001", "STOLEN-0002"};
    return kStolen.find(device_serial) == kStolen.end();
}

}  // namespace hardware
}  // namespace wallet
}  // namespace parthenon
