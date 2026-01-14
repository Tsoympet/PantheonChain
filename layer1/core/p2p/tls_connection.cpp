// ParthenonChain - TLS Encryption Implementation

#include "tls_connection.h"

#include <cstring>
#include <iostream>

namespace parthenon {
namespace p2p {

// Static members
SSL_CTX* TLSConnection::server_ctx_ = nullptr;
SSL_CTX* TLSConnection::client_ctx_ = nullptr;
bool TLSConnection::initialized_ = false;

TLSConnection::TLSConnection() : ssl_(nullptr), connected_(false), socket_fd_(-1) {}

TLSConnection::~TLSConnection() {
    Close();
}

bool TLSConnection::InitializeGlobalContext() {
    if (initialized_)
        return true;

    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // Create server context
    const SSL_METHOD* server_method = TLS_server_method();
    server_ctx_ = SSL_CTX_new(server_method);
    if (!server_ctx_) {
        std::cerr << "Failed to create server SSL context\n";
        return false;
    }

    // Create client context
    const SSL_METHOD* client_method = TLS_client_method();
    client_ctx_ = SSL_CTX_new(client_method);
    if (!client_ctx_) {
        std::cerr << "Failed to create client SSL context\n";
        SSL_CTX_free(server_ctx_);
        server_ctx_ = nullptr;
        return false;
    }

    // Set security options
    SSL_CTX_set_options(server_ctx_,
                        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_set_options(client_ctx_,
                        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    // Set minimum protocol version to TLS 1.2
    SSL_CTX_set_min_proto_version(server_ctx_, TLS1_2_VERSION);
    SSL_CTX_set_min_proto_version(client_ctx_, TLS1_2_VERSION);

    // Set cipher suites (prefer strong ciphers)
    const char* cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
                              "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
                              "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256";
    SSL_CTX_set_cipher_list(server_ctx_, cipher_list);
    SSL_CTX_set_cipher_list(client_ctx_, cipher_list);

    initialized_ = true;
    return true;
}

void TLSConnection::CleanupGlobalContext() {
    if (!initialized_)
        return;

    if (server_ctx_) {
        SSL_CTX_free(server_ctx_);
        server_ctx_ = nullptr;
    }

    if (client_ctx_) {
        SSL_CTX_free(client_ctx_);
        client_ctx_ = nullptr;
    }

    EVP_cleanup();
    ERR_free_strings();

    initialized_ = false;
}

bool TLSConnection::LoadCertificate(const std::string& cert_file, const std::string& key_file) {
    if (!initialized_ || !server_ctx_)
        return false;

    // Load certificate
    if (SSL_CTX_use_certificate_file(server_ctx_, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load certificate file: " << cert_file << "\n";
        return false;
    }

    // Load private key
    if (SSL_CTX_use_PrivateKey_file(server_ctx_, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load private key file: " << key_file << "\n";
        return false;
    }

    // Verify private key matches certificate
    if (!SSL_CTX_check_private_key(server_ctx_)) {
        std::cerr << "Private key does not match certificate\n";
        return false;
    }

    return true;
}

bool TLSConnection::LoadTrustedCAs(const std::string& ca_file) {
    if (!initialized_ || !client_ctx_)
        return false;

    if (SSL_CTX_load_verify_locations(client_ctx_, ca_file.c_str(), nullptr) != 1) {
        std::cerr << "Failed to load trusted CA file: " << ca_file << "\n";
        return false;
    }

    // Enable certificate verification
    SSL_CTX_set_verify(client_ctx_, SSL_VERIFY_PEER, nullptr);

    return true;
}

bool TLSConnection::WrapSocket(int socket_fd, bool is_server) {
    if (!initialized_) {
        std::cerr << "TLS not initialized. Call InitializeGlobalContext() first.\n";
        return false;
    }

    socket_fd_ = socket_fd;

    // Create SSL object
    SSL_CTX* ctx = is_server ? server_ctx_ : client_ctx_;
    ssl_ = SSL_new(ctx);
    if (!ssl_) {
        std::cerr << "Failed to create SSL object\n";
        return false;
    }

    // Attach socket to SSL
    if (SSL_set_fd(ssl_, socket_fd) != 1) {
        std::cerr << "Failed to set SSL file descriptor\n";
        SSL_free(ssl_);
        ssl_ = nullptr;
        return false;
    }

    return true;
}

bool TLSConnection::PerformHandshake() {
    if (!ssl_)
        return false;

    int result = SSL_do_handshake(ssl_);
    if (result == 1) {
        connected_ = true;
        return true;
    }

    int ssl_error = SSL_get_error(ssl_, result);
    if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
        // Handshake in progress, needs more data
        return false;
    }

    // Handshake failed
    std::cerr << "SSL handshake failed: " << GetLastError() << "\n";
    return false;
}

void TLSConnection::Close() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    connected_ = false;
    socket_fd_ = -1;
}

int TLSConnection::Send(const void* data, size_t length) {
    if (!ssl_ || !connected_)
        return -1;

    int result = SSL_write(ssl_, data, static_cast<int>(length));
    if (result <= 0) {
        int ssl_error = SSL_get_error(ssl_, result);
        if (ssl_error == SSL_ERROR_WANT_WRITE || ssl_error == SSL_ERROR_WANT_READ) {
            return 0;  // Try again later
        }
        // Error occurred
        return -1;
    }

    return result;
}

int TLSConnection::Receive(void* buffer, size_t length) {
    if (!ssl_ || !connected_)
        return -1;

    int result = SSL_read(ssl_, buffer, static_cast<int>(length));
    if (result <= 0) {
        int ssl_error = SSL_get_error(ssl_, result);
        if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
            return 0;  // Try again later
        }
        // Error occurred or connection closed
        return -1;
    }

    return result;
}

std::string TLSConnection::GetLastError() {
    unsigned long err = ERR_get_error();
    if (err == 0)
        return "No error";

    char buffer[256];
    ERR_error_string_n(err, buffer, sizeof(buffer));
    return std::string(buffer);
}

}  // namespace p2p
}  // namespace parthenon
