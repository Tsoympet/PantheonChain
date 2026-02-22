// ParthenonChain - Hardware Wallet Implementation
// Support for Ledger, Trezor, and other hardware wallets

#include "hardware_wallet.h"

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <algorithm>
#include <sstream>

namespace parthenon {
namespace wallet {
namespace hardware {

// Parse derivation path from string
std::optional<DerivationPath> DerivationPath::Parse(const std::string& path_str) {
    if (path_str.empty() || path_str[0] != 'm') {
        return std::nullopt;
    }

    DerivationPath result;
    std::stringstream ss(path_str.substr(2));  // Skip "m/"
    std::string segment;

    while (std::getline(ss, segment, '/')) {
        bool hardened = false;
        if (!segment.empty() && segment.back() == '\'') {
            hardened = true;
            segment.pop_back();
        }

        try {
            uint32_t index = std::stoul(segment);
            if (hardened) {
                index |= 0x80000000;  // Set hardened bit
            }
            result.path.push_back(index);
        } catch (...) {
            return std::nullopt;
        }
    }

    return result;
}

// Convert derivation path to string
std::string DerivationPath::ToString() const {
    if (path.empty()) {
        return "m";
    }

    std::stringstream ss;
    ss << "m";

    for (uint32_t index : path) {
        ss << "/";
        if (index & 0x80000000) {
            // Hardened
            ss << (index & 0x7FFFFFFF) << "'";
        } else {
            ss << index;
        }
    }

    return ss.str();
}

/**
 * Generic hardware wallet implementation
 *
 * This is a reference implementation that can be extended for specific devices
 */
class GenericHardwareWallet : public HardwareWallet {
  public:
    GenericHardwareWallet(DeviceType type, const DeviceInfo& info)
        : type_(type), info_(info), status_(ConnectionStatus::DISCONNECTED) {}

    DeviceType GetType() const override { return type_; }

    DeviceInfo GetDeviceInfo() const override { return info_; }

    ConnectionStatus GetStatus() const override { return status_; }

    bool Connect() override {
        // In a real implementation, this would:
        // 1. Initialize USB/HID connection
        // 2. Send handshake to device
        // 3. Verify device firmware

        if (status_ == ConnectionStatus::CONNECTED) {
            return true;
        }

        // Simulate successful connection
        status_ = ConnectionStatus::CONNECTED;
        return true;
    }

    void Disconnect() override {
        if (status_ == ConnectionStatus::DISCONNECTED) {
            return;
        }

        // Close HID connection
        status_ = ConnectionStatus::DISCONNECTED;
    }

    bool UnlockWithPin(const std::string& pin) override {
        if (status_ != ConnectionStatus::CONNECTED && status_ != ConnectionStatus::LOCKED) {
            return false;
        }

        if (pin.empty() || pin.length() < 4) {
            return false;
        }

        // In a real implementation, send PIN to device
        // Device would verify and unlock if correct

        status_ = ConnectionStatus::CONNECTED;
        info_.pin_cached = true;
        return true;
    }

    std::optional<std::vector<uint8_t>> GetPublicKey(const DerivationPath& path) override {
        if (status_ != ConnectionStatus::CONNECTED) {
            return std::nullopt;
        }

        // In a real implementation:
        // 1. Send derivation path to device
        // 2. Device derives public key using BIP-32
        // 3. Return x-only public key

        // For now, return a mock public key
        // In production, this would communicate with actual hardware
        std::vector<uint8_t> pubkey(32);

        // Create deterministic but unique key based on path
        std::string path_str = path.ToString();
        auto hash = crypto::SHA256::Hash256(std::vector<uint8_t>(path_str.begin(), path_str.end()));
        std::copy(hash.begin(), hash.end(), pubkey.begin());

        return pubkey;
    }

    std::optional<std::string> GetAddress(const DerivationPath& path,
                                          bool display_on_device) override {
        auto pubkey = GetPublicKey(path);
        if (!pubkey) {
            return std::nullopt;
        }

        // In a real implementation:
        // 1. Device derives address from public key
        // 2. If display_on_device, show on device screen
        // 3. Return address string

        // For now, create a mock address
        auto hash = crypto::SHA256::Hash256(*pubkey);

        std::stringstream ss;
        ss << "parthenon1";
        for (size_t i = 0; i < 20; ++i) {
            ss << std::hex << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    std::optional<primitives::Transaction>
    SignTransaction(const primitives::Transaction& tx,
                    const std::vector<DerivationPath>& input_paths) override {
        if (status_ != ConnectionStatus::CONNECTED) {
            return std::nullopt;
        }

        if (tx.inputs.size() != input_paths.size()) {
            return std::nullopt;
        }

        // Build a signed copy of the transaction
        primitives::Transaction signed_tx = tx;

        for (size_t i = 0; i < signed_tx.inputs.size(); ++i) {
            auto pubkey = GetPublicKey(input_paths[i]);
            if (!pubkey) {
                return std::nullopt;
            }

            // Derive the private key from the same path (matches GetPublicKey derivation).
            // NOTE: This is a software simulation. A production hardware wallet derives
            // keys using BIP-32 hierarchical deterministic derivation (HMAC-SHA512 with
            // chain codes). Real device communication replaces this derivation entirely.
            std::string path_str = input_paths[i].ToString();
            auto entropy = crypto::SHA256::Hash256(
                std::vector<uint8_t>(path_str.begin(), path_str.end()));
            crypto::Schnorr::PrivateKey privkey{};
            std::copy(entropy.begin(), entropy.end(), privkey.begin());

            // Get the consensus-defined signature hash for this input
            auto msg_hash = signed_tx.GetSignatureHash(i);

            auto sig_opt = crypto::Schnorr::Sign(privkey, msg_hash.data());
            if (!sig_opt) {
                return std::nullopt;
            }

            // Store signature + pubkey as the signature script
            signed_tx.inputs[i].signature_script.clear();
            signed_tx.inputs[i].signature_script.insert(
                signed_tx.inputs[i].signature_script.end(),
                sig_opt->begin(), sig_opt->end());
            signed_tx.inputs[i].signature_script.insert(
                signed_tx.inputs[i].signature_script.end(),
                pubkey->begin(), pubkey->end());
        }

        return signed_tx;
    }

    std::optional<std::vector<uint8_t>> SignMessage(const std::vector<uint8_t>& message,
                                                    const DerivationPath& path) override {
        if (status_ != ConnectionStatus::CONNECTED) {
            return std::nullopt;
        }

        // Derive the private key deterministically from the path.
        // NOTE: This is a software simulation. See SignTransaction for details on
        // production BIP-32 key derivation.
        std::string path_str = path.ToString();
        auto entropy = crypto::SHA256::Hash256(
            std::vector<uint8_t>(path_str.begin(), path_str.end()));
        crypto::Schnorr::PrivateKey privkey{};
        std::copy(entropy.begin(), entropy.end(), privkey.begin());

        // Hash the message before signing (BIP-340 signs a 32-byte hash)
        auto msg_hash = crypto::SHA256::Hash256(message);

        auto sig_opt = crypto::Schnorr::Sign(privkey, msg_hash.data());
        if (!sig_opt) {
            return std::nullopt;
        }

        return std::vector<uint8_t>(sig_opt->begin(), sig_opt->end());
    }

    bool VerifyAddress(const DerivationPath& path, const std::string& expected_address) override {
        if (status_ != ConnectionStatus::CONNECTED) {
            return false;
        }

        // In a real implementation:
        // 1. Derive address on device and show on screen
        // 2. User visually verifies it matches expected_address
        // 3. User confirms match on device
        // 4. Return user confirmation result

        auto derived = GetAddress(path, true);
        if (!derived) {
            return false;
        }

        return *derived == expected_address;
    }

  private:
    DeviceType type_;
    DeviceInfo info_;
    ConnectionStatus status_;
};

// HardwareWalletManager implementation
class HardwareWalletManager::Impl {
  public:
    Impl() {}

    std::vector<DeviceInfo> EnumerateDevices() {
        std::vector<DeviceInfo> devices;

        // In a real implementation:
        // 1. Enumerate USB HID devices
        // 2. Filter for known hardware wallet vendor/product IDs
        // 3. Query each device for info

        // For now, return empty list (no devices connected in simulation)
        // In production environment, this would detect real hardware wallets

        return devices;
    }

    std::unique_ptr<HardwareWallet> ConnectDevice(size_t device_index) {
        auto devices = EnumerateDevices();
        if (device_index >= devices.size()) {
            return nullptr;
        }

        const auto& info = devices[device_index];
        auto wallet = std::make_unique<GenericHardwareWallet>(info.type, info);

        if (!wallet->Connect()) {
            return nullptr;
        }

        return wallet;
    }

    std::unique_ptr<HardwareWallet> ConnectFirstDevice() {
        auto devices = EnumerateDevices();
        if (devices.empty()) {
            return nullptr;
        }

        return ConnectDevice(0);
    }
};

HardwareWalletManager::HardwareWalletManager() : impl_(std::make_unique<Impl>()) {}
HardwareWalletManager::~HardwareWalletManager() = default;

std::vector<DeviceInfo> HardwareWalletManager::EnumerateDevices() {
    return impl_->EnumerateDevices();
}

std::unique_ptr<HardwareWallet> HardwareWalletManager::ConnectDevice(size_t device_index) {
    return impl_->ConnectDevice(device_index);
}

std::unique_ptr<HardwareWallet> HardwareWalletManager::ConnectFirstDevice() {
    return impl_->ConnectFirstDevice();
}

}  // namespace hardware
}  // namespace wallet
}  // namespace parthenon
