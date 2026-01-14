// ParthenonChain - Hardware Wallet Interface
// Support for Ledger, Trezor, and other hardware wallets

#pragma once

#include "crypto/schnorr.h"
#include "primitives/transaction.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace wallet {
namespace hardware {

/**
 * Hardware wallet device types
 */
enum class DeviceType {
    LEDGER,   // Ledger Nano S/X
    TREZOR,   // Trezor One/Model T
    KEEPKEY,  // KeepKey
    GENERIC   // Generic HID device
};

/**
 * Hardware wallet connection status
 */
enum class ConnectionStatus { DISCONNECTED, CONNECTED, LOCKED, ERROR };

/**
 * Hardware wallet device information
 */
struct DeviceInfo {
    DeviceType type;
    std::string model;
    std::string version;
    std::string serial;
    bool initialized;
    bool pin_cached;
};

/**
 * BIP-32 derivation path
 */
struct DerivationPath {
    std::vector<uint32_t> path;  // e.g., [44', 0', 0', 0, 0] for m/44'/0'/0'/0/0

    DerivationPath() = default;
    explicit DerivationPath(const std::vector<uint32_t>& p) : path(p) {}

    // Parse from string like "m/44'/0'/0'/0/0"
    static std::optional<DerivationPath> Parse(const std::string& path_str);

    // Convert to string
    std::string ToString() const;
};

/**
 * Hardware wallet interface
 *
 * Provides abstraction for hardware wallet operations:
 * - Device discovery and connection
 * - Public key derivation
 * - Transaction signing
 * - Address generation
 */
class HardwareWallet {
  public:
    virtual ~HardwareWallet() = default;

    /**
     * Get device type
     */
    virtual DeviceType GetType() const = 0;

    /**
     * Get device information
     */
    virtual DeviceInfo GetDeviceInfo() const = 0;

    /**
     * Get connection status
     */
    virtual ConnectionStatus GetStatus() const = 0;

    /**
     * Connect to device
     * @return true if connection successful
     */
    virtual bool Connect() = 0;

    /**
     * Disconnect from device
     */
    virtual void Disconnect() = 0;

    /**
     * Unlock device with PIN
     * @param pin Device PIN code
     * @return true if unlock successful
     */
    virtual bool UnlockWithPin(const std::string& pin) = 0;

    /**
     * Get public key at derivation path
     * @param path BIP-32 derivation path
     * @return 32-byte x-only public key or nullopt on error
     */
    virtual std::optional<std::vector<uint8_t>> GetPublicKey(const DerivationPath& path) = 0;

    /**
     * Get address at derivation path
     * @param path BIP-32 derivation path
     * @param display_on_device Show address on device screen for verification
     * @return Address string or nullopt on error
     */
    virtual std::optional<std::string> GetAddress(const DerivationPath& path,
                                                  bool display_on_device = false) = 0;

    /**
     * Sign transaction on device
     * @param tx Transaction to sign
     * @param input_paths Derivation paths for each input
     * @return Signed transaction or nullopt on error
     */
    virtual std::optional<primitives::Transaction>
    SignTransaction(const primitives::Transaction& tx,
                    const std::vector<DerivationPath>& input_paths) = 0;

    /**
     * Sign message with device
     * @param message Message to sign
     * @param path Derivation path for signing key
     * @return Schnorr signature or nullopt on error
     */
    virtual std::optional<std::vector<uint8_t>> SignMessage(const std::vector<uint8_t>& message,
                                                            const DerivationPath& path) = 0;

    /**
     * Verify address on device screen
     * @param path Derivation path
     * @param expected_address Expected address to verify
     * @return true if user confirmed address matches
     */
    virtual bool VerifyAddress(const DerivationPath& path, const std::string& expected_address) = 0;
};

/**
 * Hardware wallet manager
 *
 * Manages hardware wallet device discovery and connection
 */
class HardwareWalletManager {
  public:
    HardwareWalletManager();
    ~HardwareWalletManager();

    /**
     * Enumerate connected hardware wallet devices
     * @return List of device information
     */
    std::vector<DeviceInfo> EnumerateDevices();

    /**
     * Connect to specific device
     * @param device_index Index from EnumerateDevices
     * @return Hardware wallet instance or nullptr on error
     */
    std::unique_ptr<HardwareWallet> ConnectDevice(size_t device_index);

    /**
     * Connect to first available device
     * @return Hardware wallet instance or nullptr if no device found
     */
    std::unique_ptr<HardwareWallet> ConnectFirstDevice();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace hardware
}  // namespace wallet
}  // namespace parthenon
