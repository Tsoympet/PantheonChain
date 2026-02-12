// ParthenonChain - TLS Certificate Rotation
// Automatic certificate renewal and rotation without downtime

#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <string>

namespace parthenon {
namespace p2p {

/**
 * Certificate information
 */
struct CertificateInfo {
    std::string cert_path;
    std::string key_path;
    time_t valid_from;
    time_t valid_until;
    std::string issuer;
    std::string subject;

    bool IsExpired() const;
    bool IsExpiringSoon(time_t days = 30) const;
};

/**
 * Automatic TLS certificate rotation
 */
class CertificateRotation {
  public:
    /**
     * Initialize certificate rotation
     * @param cert_dir Directory containing certificates
     * @param check_interval_seconds How often to check for new certs
     */
    bool Init(const std::string& cert_dir, uint32_t check_interval_seconds = 3600);

    /**
     * Register callback for when certificate is rotated
     * @param callback Function to call on rotation
     */
    void SetRotationCallback(std::function<void(const CertificateInfo&)> callback);

    /**
     * Start rotation checker thread
     */
    void Start();

    /**
     * Stop rotation checker
     */
    void Stop();

    /**
     * Manually check for new certificate
     * @return true if new certificate loaded
     */
    bool CheckAndRotate();

    /**
     * Get current certificate info
     */
    CertificateInfo GetCurrentCertificate();

    /**
     * Generate self-signed certificate for testing
     */
    static bool GenerateSelfSigned(const std::string& cert_path, const std::string& key_path,
                                   uint32_t days = 365);

  private:
    std::string cert_dir_;
    uint32_t check_interval_;
    CertificateInfo current_cert_;
    std::function<void(const CertificateInfo&)> callback_;
    bool running_ = false;
    void* rotation_thread_ = nullptr;

    void RotationLoop();
    bool LoadCertificate(const std::string& cert_path, const std::string& key_path);
};

}  // namespace p2p
}  // namespace parthenon
