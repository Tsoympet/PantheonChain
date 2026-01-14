// ParthenonChain - TLS Certificate Rotation Implementation

#include "certificate_rotation.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <thread>

namespace parthenon {
namespace p2p {

// Forward declaration of helper function
static time_t ASN1_TIME_to_time_t(ASN1_TIME* time);

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
    std::cout << "Valid until: " << std::ctime(&current_cert_.valid_until);

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

    ASN1_TIME* not_after = X509_get_notAfter(x509);
    time_t new_valid_until = ASN1_TIME_to_time_t(not_after);

    X509_free(x509);

    // If validity is different, it's a new certificate
    if (new_valid_until != current_cert_.valid_until) {
        std::cout << "New certificate detected! Rotating...\n";

        if (LoadCertificate(cert_path, key_path)) {
            std::cout << "Certificate rotated successfully\n";
            std::cout << "New validity: " << std::ctime(&current_cert_.valid_until);

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

    // Generate using OpenSSL command line (simpler than programmatic)
    std::string cmd = "openssl req -x509 -newkey rsa:2048 -nodes"
                      " -keyout " +
                      key_path + " -out " + cert_path + " -days " + std::to_string(days) +
                      " -subj '/CN=PantheonChain Node' 2>/dev/null";

    int result = system(cmd.c_str());

    if (result == 0) {
        std::cout << "Self-signed certificate generated successfully\n";
        return true;
    } else {
        std::cerr << "Failed to generate self-signed certificate\n";
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
    ASN1_TIME* not_before = X509_get_notBefore(x509);
    ASN1_TIME* not_after = X509_get_notAfter(x509);

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
static time_t ASN1_TIME_to_time_t(ASN1_TIME* time) {
    struct tm t;
    const char* str = (const char*)time->data;
    size_t i = 0;

    memset(&t, 0, sizeof(t));

    if (time->type == V_ASN1_UTCTIME) {
        t.tm_year = (str[i++] - '0') * 10;
        t.tm_year += (str[i++] - '0');
        if (t.tm_year < 70)
            t.tm_year += 100;
    } else if (time->type == V_ASN1_GENERALIZEDTIME) {
        t.tm_year = (str[i++] - '0') * 1000;
        t.tm_year += (str[i++] - '0') * 100;
        t.tm_year += (str[i++] - '0') * 10;
        t.tm_year += (str[i++] - '0');
        t.tm_year -= 1900;
    }

    t.tm_mon = (str[i++] - '0') * 10;
    t.tm_mon += (str[i++] - '0') - 1;
    t.tm_mday = (str[i++] - '0') * 10;
    t.tm_mday += (str[i++] - '0');
    t.tm_hour = (str[i++] - '0') * 10;
    t.tm_hour += (str[i++] - '0');
    t.tm_min = (str[i++] - '0') * 10;
    t.tm_min += (str[i++] - '0');
    t.tm_sec = (str[i++] - '0') * 10;
    t.tm_sec += (str[i++] - '0');

    return mktime(&t);
}

}  // namespace p2p
}  // namespace parthenon
