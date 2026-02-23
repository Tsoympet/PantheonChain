// ParthenonChain - TLS Certificate Rotation Implementation

#include "certificate_rotation.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)  // Suppress unsafe POSIX function warnings (fopen)
#endif

#include <cstring>
#include <fstream>
#include <iostream>
#include <ctime>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <thread>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace parthenon {
namespace p2p {

// Forward declaration of helper function
static time_t ASN1_TIME_to_time_t(const ASN1_TIME* time);
static std::string FormatTime(time_t time_value);

bool CertificateInfo::IsExpired() const {
    return std::time(nullptr) >= valid_until;
}

bool CertificateInfo::IsExpiringSoon(time_t days) const {
    time_t threshold = std::time(nullptr) + (days * 86400);
    return valid_until <= threshold;
}

bool CertificateRotation::Init(const std::string& cert_dir, uint32_t check_interval_seconds) {
    cert_dir_ = cert_dir;
    check_interval_ = check_interval_seconds;

    // Try to load initial certificate
    std::string cert_path = cert_dir_ + "/server.crt";
    std::string key_path = cert_dir_ + "/server.key";

    if (!LoadCertificate(cert_path, key_path)) {
        std::cerr << "Failed to load initial certificate from " << cert_dir_ << "\n";
        return false;
    }

    std::cout << "Certificate loaded: " << current_cert_.subject << "\n";
    std::cout << "Valid until: " << FormatTime(current_cert_.valid_until) << "\n";

    if (current_cert_.IsExpired()) {
        std::cerr << "WARNING: Certificate is already expired!\n";
    } else if (current_cert_.IsExpiringSoon(30)) {
        std::cout << "WARNING: Certificate expires soon!\n";
    }

    return true;
}

void CertificateRotation::SetRotationCallback(
    std::function<void(const CertificateInfo&)> callback) {
    callback_ = callback;
}

void CertificateRotation::Start() {
    if (running_) {
        return;
    }

    running_ = true;

    // Start rotation checker thread
    std::thread rotation_thread([this]() { this->RotationLoop(); });
    rotation_thread.detach();

    std::cout << "Certificate rotation checker started (interval: " << check_interval_ << "s)\n";
}

void CertificateRotation::Stop() {
    running_ = false;
}

bool CertificateRotation::CheckAndRotate() {
    std::string cert_path = cert_dir_ + "/server.crt";
    std::string key_path = cert_dir_ + "/server.key";

    // Check if files have been updated
    std::ifstream cert_file(cert_path, std::ios::binary | std::ios::ate);
    if (!cert_file.is_open()) {
        return false;
    }

    // Simple check: Try to load and compare validity dates
    CertificateInfo new_cert;
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        return false;
    }

    X509* x509 = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);

    if (!x509) {
        return false;
    }

    const ASN1_TIME* not_before = X509_get_notBefore(x509);
    const ASN1_TIME* not_after = X509_get_notAfter(x509);
    new_cert.cert_path = cert_path;
    new_cert.key_path = key_path;
    new_cert.valid_from = ASN1_TIME_to_time_t(not_before);
    new_cert.valid_until = ASN1_TIME_to_time_t(not_after);

    X509_free(x509);

    // If validity is different, it's a new certificate
    if (new_cert.valid_until != current_cert_.valid_until) {
        std::cout << "New certificate detected (valid from: " << FormatTime(new_cert.valid_from)
                  << ")\n";
        std::cout << "  cert: " << new_cert.cert_path << " key: " << new_cert.key_path << "\n";

        if (LoadCertificate(cert_path, key_path)) {
            std::cout << "Certificate rotated successfully\n";
            std::cout << "New validity: " << FormatTime(new_cert.valid_until) << "\n";

            if (callback_) {
                callback_(current_cert_);
            }

            return true;
        }
    }

    return false;
}

CertificateInfo CertificateRotation::GetCurrentCertificate() {
    return current_cert_;
}

bool CertificateRotation::GenerateSelfSigned(const std::string& cert_path,
                                             const std::string& key_path, uint32_t days) {
    std::cout << "Generating self-signed certificate...\n";

    // Validate paths to prevent path traversal
    // Only allow alphanumeric, dash, underscore, dot, and forward slash
    auto is_safe_path = [](const std::string& path) -> bool {
        if (path.empty() || path.length() > 4096) {
            return false;
        }
        for (char c : path) {
            if (!std::isalnum(c) && c != '-' && c != '_' && c != '.' && c != '/') {
                return false;
            }
        }
        // Prevent path traversal
        if (path.find("..") != std::string::npos) {
            return false;
        }
        return true;
    };

    if (!is_safe_path(cert_path) || !is_safe_path(key_path)) {
        std::cerr << "Invalid certificate or key path (contains unsafe characters)\n";
        return false;
    }

    // Validate days parameter
    if (days == 0 || days > 36500) {  // Max ~100 years
        std::cerr << "Invalid certificate validity period\n";
        return false;
    }

    // Generate RSA key pair using OpenSSL API (avoids system() call)
    EVP_PKEY* pkey = EVP_RSA_gen(2048);
    if (!pkey) {
        std::cerr << "Failed to generate RSA key pair\n";
        return false;
    }

    // Create X509 certificate
    X509* x509 = X509_new();
    if (!x509) {
        EVP_PKEY_free(pkey);
        std::cerr << "Failed to create X509 certificate\n";
        return false;
    }

    // Set version to X.509v3
    X509_set_version(x509, 2);

    // Set serial number
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    // Set validity period
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), static_cast<long>(days) * 86400L);

    // Set public key
    X509_set_pubkey(x509, pkey);

    // Set subject name
    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>("PantheonChain Node"),
                               -1, -1, 0);
    // Self-signed: issuer == subject
    X509_set_issuer_name(x509, name);

    // Sign the certificate with SHA-256
    if (!X509_sign(x509, pkey, EVP_sha256())) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        std::cerr << "Failed to sign certificate\n";
        return false;
    }

    // Write private key to file (unencrypted; secure with restrictive permissions below)
    FILE* key_fp = fopen(key_path.c_str(), "w");
    if (!key_fp) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        std::cerr << "Failed to open key file for writing\n";
        return false;
    }
    bool key_ok = PEM_write_PrivateKey(key_fp, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1;
    fclose(key_fp);

    // Restrict private key file permissions to owner-read/write only (0600)
#if !defined(_WIN32)
    chmod(key_path.c_str(), S_IRUSR | S_IWUSR);
#endif

    // Write certificate to file
    FILE* cert_fp = fopen(cert_path.c_str(), "w");
    if (!cert_fp) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        std::cerr << "Failed to open certificate file for writing\n";
        return false;
    }
    bool cert_ok = PEM_write_X509(cert_fp, x509) == 1;
    fclose(cert_fp);

    X509_free(x509);
    EVP_PKEY_free(pkey);

    if (key_ok && cert_ok) {
        std::cout << "Self-signed certificate generated successfully\n";
        return true;
    } else {
        std::cerr << "Failed to write certificate or key to disk\n";
        return false;
    }
}

void CertificateRotation::RotationLoop() {
    while (running_) {
        // Sleep for check interval
        std::this_thread::sleep_for(std::chrono::seconds(check_interval_));

        if (!running_) {
            break;
        }

        // Check for certificate rotation
        CheckAndRotate();

        // Warn if expiring soon
        if (current_cert_.IsExpiringSoon(30)) {
            std::cout << "WARNING: Certificate expires in less than 30 days!\n";
        }
    }
}

bool CertificateRotation::LoadCertificate(const std::string& cert_path,
                                          const std::string& key_path) {
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        return false;
    }

    X509* x509 = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);

    if (!x509) {
        return false;
    }

    // Extract certificate information
    const ASN1_TIME* not_before = X509_get_notBefore(x509);
    const ASN1_TIME* not_after = X509_get_notAfter(x509);

    current_cert_.cert_path = cert_path;
    current_cert_.key_path = key_path;
    current_cert_.valid_from = ASN1_TIME_to_time_t(not_before);
    current_cert_.valid_until = ASN1_TIME_to_time_t(not_after);

    // Get subject
    char* subject = X509_NAME_oneline(X509_get_subject_name(x509), nullptr, 0);
    if (subject) {
        current_cert_.subject = subject;
        OPENSSL_free(subject);
    }

    // Get issuer
    char* issuer = X509_NAME_oneline(X509_get_issuer_name(x509), nullptr, 0);
    if (issuer) {
        current_cert_.issuer = issuer;
        OPENSSL_free(issuer);
    }

    X509_free(x509);

    return true;
}

// Helper function to convert ASN1_TIME to time_t
static time_t ASN1_TIME_to_time_t(const ASN1_TIME* time) {
    if (!time) {
        return 0;
    }

    std::tm t {};
    if (ASN1_TIME_to_tm(time, &t) != 1) {
        return 0;
    }

#if defined(_WIN32)
    return _mkgmtime(&t);
#else
    return timegm(&t);
#endif
}

static std::string FormatTime(time_t time_value) {
    std::tm tm_snapshot {};
#if defined(_WIN32)
    gmtime_s(&tm_snapshot, &time_value);
#else
    gmtime_r(&time_value, &tm_snapshot);
#endif
    char buffer[64];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_snapshot) == 0) {
        return "Unable to format time";
    }
    return std::string(buffer) + " UTC";
}

}  // namespace p2p
}  // namespace parthenon
