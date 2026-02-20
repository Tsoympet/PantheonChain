// ParthenonChain - TLS Encryption Wrapper
// SSL/TLS support for P2P connections using OpenSSL

#pragma once

#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

namespace parthenon {
namespace p2p {

/**
 * TLS/SSL wrapper for secure peer connections
 */
class TLSConnection {
  public:
    TLSConnection();
    ~TLSConnection();

    // Initialize TLS context (call once per application)
    static bool InitializeGlobalContext();
    static void CleanupGlobalContext();

    // Connection setup
    bool WrapSocket(int socket_fd, bool is_server);
    bool PerformHandshake();
    void Close();

    // I/O operations
    int Send(const void* data, size_t length);
    int Receive(void* buffer, size_t length);

    // Status
    bool IsConnected() const { return ssl_ != nullptr && connected_; }
    std::string GetLastError();

    // Certificate management
    static bool LoadCertificate(const std::string& cert_file, const std::string& key_file);
    static bool LoadTrustedCAs(const std::string& ca_file);

  private:
    SSL* ssl_;
    bool connected_;
    int socket_fd_;

    static SSL_CTX* server_ctx_;
    static SSL_CTX* client_ctx_;
    static bool initialized_;
};

}  // namespace p2p
}  // namespace parthenon
