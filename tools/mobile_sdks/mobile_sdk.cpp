// ParthenonChain Mobile SDK Implementation

#include "mobile_sdk.h"

#include "crypto/schnorr.h"
#include "crypto/sha256.h"

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <deque>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_set>

namespace parthenon {
namespace mobile {

namespace {

constexpr uint32_t kDefaultPollIntervalMs = 5000;
constexpr size_t kAddressHashBytes = 20;
constexpr size_t kMnemonicWordSize = 4;
constexpr size_t kStorageKeySize = 32;
constexpr size_t kStorageNonceSize = 12;
constexpr size_t kStorageTagSize = 16;
constexpr uint64_t kMaxTransactionScanBlocks = 1000;
constexpr size_t kMaxSeenTransactions = 10000;

using json = nlohmann::json;

struct ParsedEndpoint {
    std::string host;
    std::string port;
    std::string path;
};

std::atomic<uint64_t> g_request_id{1};
std::mutex g_storage_mutex;

std::string BytesToHex(const uint8_t* data, size_t size) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < size; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string BytesToHex(const std::vector<uint8_t>& data) {
    return BytesToHex(data.data(), data.size());
}

std::string FormatMnemonic(const std::vector<uint8_t>& entropy) {
    const auto hex = BytesToHex(entropy);
    std::ostringstream mnemonic;
    for (size_t i = 0; i < hex.size(); i += kMnemonicWordSize) {
        if (i > 0) {
            mnemonic << ' ';
        }
        mnemonic << hex.substr(i, kMnemonicWordSize);
    }
    return mnemonic.str();
}

bool ParseMnemonicEntropy(const std::string& mnemonic, std::vector<uint8_t>& entropy) {
    std::string hex;
    hex.reserve(mnemonic.size());
    for (char c : mnemonic) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            continue;
        }
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
        hex.push_back(c);
    }

    if (hex.empty() || (hex.size() % 2) != 0) {
        return false;
    }

    entropy.clear();
    entropy.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t high = 0;
        uint8_t low = 0;
        auto to_nibble = [](char value, uint8_t& nibble) {
            if (value >= '0' && value <= '9') {
                nibble = static_cast<uint8_t>(value - '0');
                return true;
            }
            if (value >= 'a' && value <= 'f') {
                nibble = static_cast<uint8_t>(value - 'a' + 10);
                return true;
            }
            if (value >= 'A' && value <= 'F') {
                nibble = static_cast<uint8_t>(value - 'A' + 10);
                return true;
            }
            return false;
        };

        if (!to_nibble(hex[i], high) || !to_nibble(hex[i + 1], low)) {
            return false;
        }
        entropy.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return true;
}

bool FillRandomBytes(uint8_t* data, size_t size) {
    if (size == 0) {
        return true;
    }
    return RAND_bytes(data, static_cast<int>(size)) == 1;
}

crypto::Schnorr::PrivateKey DerivePrivateKey(const std::vector<uint8_t>& entropy) {
    auto hash = crypto::SHA256::Hash256(entropy);
    crypto::Schnorr::PrivateKey privkey{};
    std::copy(hash.begin(), hash.end(), privkey.begin());
    return privkey;
}

bool PopulatePrivateKey(crypto::Schnorr::PrivateKey& privkey) {
    std::array<uint8_t, crypto::Schnorr::PRIVATE_KEY_SIZE> random_bytes{};
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (!FillRandomBytes(random_bytes.data(), random_bytes.size())) {
            break;
        }
        std::copy(random_bytes.begin(), random_bytes.end(), privkey.begin());
        if (crypto::Schnorr::ValidatePrivateKey(privkey)) {
            return true;
        }
    }
    return false;
}

std::string DeriveAddress(const std::vector<uint8_t>& pubkey) {
    auto hash = crypto::SHA256::Hash256(pubkey);
    std::ostringstream ss;
    ss << "ptn1q" << BytesToHex(hash.data(), kAddressHashBytes);
    return ss.str();
}

std::optional<int> AssetIdFor(std::string asset) {
    std::transform(asset.begin(), asset.end(), asset.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (asset == "TALANTON" || asset == "TALN") {
        return 0;
    }
    if (asset == "DRACHMA" || asset == "DRM") {
        return 1;
    }
    if (asset == "OBOLOS" || asset == "OBL") {
        return 2;
    }
    return std::nullopt;
}

std::optional<ParsedEndpoint> ParseEndpoint(const std::string& endpoint, std::string& error) {
    std::string url = endpoint;
    if (url.empty()) {
        error = "RPC endpoint not configured";
        return std::nullopt;
    }

    constexpr const char* kHttpPrefix = "http://";
    constexpr const char* kHttpsPrefix = "https://";

    if (url.rfind(kHttpsPrefix, 0) == 0) {
        error = "HTTPS endpoints are not supported in the mobile SDK";
        return std::nullopt;
    }

    if (url.rfind(kHttpPrefix, 0) == 0) {
        url = url.substr(std::strlen(kHttpPrefix));
    }

    std::string path = "/";
    auto path_pos = url.find('/');
    if (path_pos != std::string::npos) {
        path = url.substr(path_pos);
        url = url.substr(0, path_pos);
    }

    std::string host = url;
    std::string port = "80";
    auto colon_pos = url.find(':');
    if (colon_pos != std::string::npos) {
        host = url.substr(0, colon_pos);
        port = url.substr(colon_pos + 1);
    }

    if (host.empty()) {
        error = "RPC endpoint host is missing";
        return std::nullopt;
    }

    return ParsedEndpoint{host, port, path};
}

std::optional<std::string> HttpPost(const ParsedEndpoint& endpoint, const std::string& body,
                                    std::string& error) {
    try {
        boost::asio::io_context io;
        boost::asio::ip::tcp::resolver resolver(io);
        boost::asio::ip::tcp::socket socket(io);

        auto endpoints = resolver.resolve(endpoint.host, endpoint.port);
        boost::asio::connect(socket, endpoints);

        std::ostringstream request;
        request << "POST " << endpoint.path << " HTTP/1.1\r\n";
        request << "Host: " << endpoint.host << "\r\n";
        request << "Content-Type: application/json\r\n";
        request << "Connection: close\r\n";
        request << "Content-Length: " << body.size() << "\r\n\r\n";
        request << body;

        boost::asio::write(socket, boost::asio::buffer(request.str()));

        boost::asio::streambuf response;
        boost::system::error_code ec;
        boost::asio::read(socket, response, ec);
        if (ec && ec != boost::asio::error::eof) {
            error = ec.message();
            return std::nullopt;
        }

        std::istream response_stream(&response);
        std::string http_version;
        unsigned int status_code = 0;
        response_stream >> http_version >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);

        if (!response_stream || http_version.rfind("HTTP/", 0) != 0) {
            error = "Invalid HTTP response";
            return std::nullopt;
        }

        if (status_code < 200 || status_code >= 300) {
            error = "HTTP error: " + std::to_string(status_code);
            return std::nullopt;
        }

        std::string header;
        while (std::getline(response_stream, header) && header != "\r") {
        }

        std::string response_body((std::istreambuf_iterator<char>(response_stream)),
                                  std::istreambuf_iterator<char>());
        return response_body;
    } catch (const std::exception& e) {
        error = e.what();
        return std::nullopt;
    }
}

std::optional<json> ParseRpcResult(const json& result) {
    if (result.is_string()) {
        auto parsed = json::parse(result.get<std::string>(), nullptr, false);
        if (!parsed.is_discarded()) {
            return parsed;
        }
    }
    return result;
}

std::optional<json> RpcRequest(const NetworkConfig& config, const std::string& method,
                               const json& params, std::string& error) {
    auto endpoint = ParseEndpoint(config.endpoint, error);
    if (!endpoint) {
        return std::nullopt;
    }

    json request;
    request["jsonrpc"] = "2.0";
    request["id"] = g_request_id.fetch_add(1);
    request["method"] = method;
    request["params"] = params;

    auto response_body = HttpPost(*endpoint, request.dump(), error);
    if (!response_body) {
        return std::nullopt;
    }

    auto response = json::parse(*response_body, nullptr, false);
    if (response.is_discarded()) {
        error = "Invalid JSON-RPC response";
        return std::nullopt;
    }

    if (response.contains("error") && !response["error"].is_null()) {
        if (response["error"].is_object() && response["error"].contains("message")) {
            error = response["error"]["message"].get<std::string>();
        } else if (response["error"].is_string()) {
            error = response["error"].get<std::string>();
        } else {
            error = response["error"].dump();
        }
        return std::nullopt;
    }

    if (!response.contains("result")) {
        error = "Missing JSON-RPC result";
        return std::nullopt;
    }

    return ParseRpcResult(response["result"]);
}

std::optional<uint64_t> FetchBlockHeight(const NetworkConfig& config, std::string& error) {
    auto result = RpcRequest(config, "getblockcount", json::array(), error);
    if (!result) {
        return std::nullopt;
    }
    if (result->is_number_unsigned()) {
        return result->get<uint64_t>();
    }
    if (result->is_string()) {
        try {
            return static_cast<uint64_t>(std::stoull(result->get<std::string>()));
        } catch (...) {
            error = "Invalid block height response";
            return std::nullopt;
        }
    }
    error = "Unexpected block height response";
    return std::nullopt;
}

std::optional<json> FetchBlockInfo(const NetworkConfig& config, uint64_t height,
                                   std::string& error) {
    json params = json::array();
    params.push_back(height);
    return RpcRequest(config, "getblock", params, error);
}

std::filesystem::path StorageRoot() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = std::getenv("USERPROFILE");
    }
    std::filesystem::path root = home ? std::filesystem::path(home) : std::filesystem::temp_directory_path();
    return root / ".parthenon_mobile_sdk";
}

std::filesystem::path StorageKeyPath() {
    return StorageRoot() / "storage.key";
}

std::filesystem::path StorageDataPath(const std::string& key) {
    auto hash = crypto::SHA256::Hash256(reinterpret_cast<const uint8_t*>(key.data()), key.size());
    return StorageRoot() / (BytesToHex(hash.data(), hash.size()) + ".bin");
}

bool LoadOrCreateStorageKey(std::array<uint8_t, kStorageKeySize>& key) {
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    auto path = StorageKeyPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    if (std::filesystem::exists(path, ec)) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }
        in.read(reinterpret_cast<char*>(key.data()), key.size());
        return in.gcount() == static_cast<std::streamsize>(key.size());
    }

    if (!FillRandomBytes(key.data(), key.size())) {
        return false;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(key.data()), key.size());
    out.close();

    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, ec);
    return true;
}

bool EncryptStorage(const std::array<uint8_t, kStorageKeySize>& key,
                    const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& ciphertext) {
    std::array<uint8_t, kStorageNonceSize> nonce{};
    if (!FillRandomBytes(nonce.data(), nonce.size())) {
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool ok = false;
    int out_len = 0;
    int final_len = 0;
    std::array<uint8_t, kStorageTagSize> tag{};
    std::vector<uint8_t> encrypted(plaintext.size());

    do {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            break;
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1) {
            break;
        }
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
            break;
        }
        if (!plaintext.empty()) {
            if (EVP_EncryptUpdate(ctx,
                                  encrypted.data(),
                                  &out_len,
                                  plaintext.data(),
                                  static_cast<int>(plaintext.size())) != 1) {
                break;
            }
        }
        if (EVP_EncryptFinal_ex(ctx, encrypted.data() + out_len, &final_len) != 1) {
            break;
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data()) != 1) {
            break;
        }

        encrypted.resize(static_cast<size_t>(out_len + final_len));
        ciphertext.clear();
        ciphertext.reserve(nonce.size() + encrypted.size() + tag.size());
        ciphertext.insert(ciphertext.end(), nonce.begin(), nonce.end());
        ciphertext.insert(ciphertext.end(), encrypted.begin(), encrypted.end());
        ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());
        ok = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

bool DecryptStorage(const std::array<uint8_t, kStorageKeySize>& key,
                    const std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& plaintext) {
    if (ciphertext.size() < (kStorageNonceSize + kStorageTagSize)) {
        return false;
    }

    const uint8_t* nonce = ciphertext.data();
    const size_t encrypted_len = ciphertext.size() - kStorageNonceSize - kStorageTagSize;
    const uint8_t* encrypted = ciphertext.data() + kStorageNonceSize;
    const uint8_t* tag = ciphertext.data() + kStorageNonceSize + encrypted_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool ok = false;
    int out_len = 0;
    int final_len = 0;
    std::vector<uint8_t> decrypted(encrypted_len);

    do {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            break;
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(kStorageNonceSize), nullptr) !=
            1) {
            break;
        }
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce) != 1) {
            break;
        }
        if (encrypted_len > 0) {
            if (EVP_DecryptUpdate(ctx,
                                  decrypted.data(),
                                  &out_len,
                                  encrypted,
                                  static_cast<int>(encrypted_len)) != 1) {
                break;
            }
        }
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(kStorageTagSize),
                                const_cast<uint8_t*>(tag)) != 1) {
            break;
        }
        if (EVP_DecryptFinal_ex(ctx, decrypted.data() + out_len, &final_len) != 1) {
            break;
        }
        decrypted.resize(static_cast<size_t>(out_len + final_len));
        plaintext = std::move(decrypted);
        ok = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

}  // namespace

// Wallet implementation
Wallet::Wallet() {}

std::unique_ptr<Wallet> Wallet::Generate() {
    auto wallet = std::unique_ptr<Wallet>(new Wallet());
    std::vector<uint8_t> entropy(32);
    if (!FillRandomBytes(entropy.data(), entropy.size())) {
        return nullptr;
    }
    wallet->mnemonic_ = FormatMnemonic(entropy);

    auto privkey = DerivePrivateKey(entropy);
    if (!crypto::Schnorr::ValidatePrivateKey(privkey)) {
        return nullptr;
    }

    auto pubkey_opt = crypto::Schnorr::GetPublicKey(privkey);
    if (!pubkey_opt) {
        return nullptr;
    }

    wallet->private_key_.assign(privkey.begin(), privkey.end());
    wallet->public_key_.assign(pubkey_opt->begin(), pubkey_opt->end());
    wallet->address_ = DeriveAddress(wallet->public_key_);
    return wallet;
}

std::unique_ptr<Wallet> Wallet::FromMnemonic(const std::string& mnemonic) {
    auto wallet = std::unique_ptr<Wallet>(new Wallet());
    wallet->mnemonic_ = mnemonic;
    std::vector<uint8_t> entropy;
    if (!ParseMnemonicEntropy(mnemonic, entropy)) {
        return nullptr;
    }

    auto privkey = DerivePrivateKey(entropy);
    if (!crypto::Schnorr::ValidatePrivateKey(privkey)) {
        return nullptr;
    }

    auto pubkey_opt = crypto::Schnorr::GetPublicKey(privkey);
    if (!pubkey_opt) {
        return nullptr;
    }

    wallet->private_key_.assign(privkey.begin(), privkey.end());
    wallet->public_key_.assign(pubkey_opt->begin(), pubkey_opt->end());
    wallet->address_ = DeriveAddress(wallet->public_key_);
    return wallet;
}

std::unique_ptr<Wallet> Wallet::FromPrivateKey(const std::vector<uint8_t>& private_key) {
    auto wallet = std::unique_ptr<Wallet>(new Wallet());
    crypto::Schnorr::PrivateKey privkey{};
    if (private_key.size() != privkey.size()) {
        return nullptr;
    }
    std::copy(private_key.begin(), private_key.end(), privkey.begin());

    if (!crypto::Schnorr::ValidatePrivateKey(privkey)) {
        return nullptr;
    }

    auto pubkey_opt = crypto::Schnorr::GetPublicKey(privkey);
    if (!pubkey_opt) {
        return nullptr;
    }

    wallet->private_key_.assign(privkey.begin(), privkey.end());
    wallet->public_key_.assign(pubkey_opt->begin(), pubkey_opt->end());
    wallet->address_ = DeriveAddress(wallet->public_key_);
    return wallet;
}

std::string Wallet::GetAddress() const {
    return address_;
}

std::vector<uint8_t> Wallet::GetPublicKey() const {
    return public_key_;
}

std::vector<uint8_t> Wallet::SignTransaction(const Transaction& tx) {
    std::ostringstream payload;
    payload << tx.from << "|" << tx.to << "|" << tx.amount << "|" << tx.asset << "|" << tx.memo << "|" << tx.fee;
    auto hash = crypto::SHA256::Hash256(
        reinterpret_cast<const uint8_t*>(payload.str().data()), payload.str().size());

    crypto::Schnorr::PrivateKey privkey{};
    if (private_key_.size() != privkey.size()) {
        return {};
    }
    std::copy(private_key_.begin(), private_key_.end(), privkey.begin());

    std::array<uint8_t, 32> aux_rand{};
    const uint8_t* aux_ptr = nullptr;
    if (FillRandomBytes(aux_rand.data(), aux_rand.size())) {
        aux_ptr = aux_rand.data();
    }

    auto signature_opt = crypto::Schnorr::Sign(privkey, hash.data(), aux_ptr);
    if (!signature_opt) {
        return {};
    }
    return std::vector<uint8_t>(signature_opt->begin(), signature_opt->end());
}

std::vector<uint8_t> Wallet::SignMessage(const std::string& message) {
    auto hash =
        crypto::SHA256::Hash256(reinterpret_cast<const uint8_t*>(message.data()), message.size());

    crypto::Schnorr::PrivateKey privkey{};
    if (private_key_.size() != privkey.size()) {
        return {};
    }
    std::copy(private_key_.begin(), private_key_.end(), privkey.begin());

    std::array<uint8_t, 32> aux_rand{};
    const uint8_t* aux_ptr = nullptr;
    if (FillRandomBytes(aux_rand.data(), aux_rand.size())) {
        aux_ptr = aux_rand.data();
    }

    auto signature_opt = crypto::Schnorr::Sign(privkey, hash.data(), aux_ptr);
    if (!signature_opt) {
        return {};
    }
    return std::vector<uint8_t>(signature_opt->begin(), signature_opt->end());
}

std::string Wallet::ExportMnemonic() const {
    return mnemonic_;
}

std::vector<uint8_t> Wallet::ExportPrivateKey() const {
    return private_key_;
}

// MobileClient implementation
class MobileClient::Impl {
public:
    explicit Impl(const NetworkConfig& config) : config_(config), running_(true) {}

    ~Impl() {
        running_ = false;
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        for (auto& thread : subscriptions_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    NetworkConfig config_;
    std::atomic<bool> running_;
    std::mutex subscription_mutex_;
    std::vector<std::thread> subscriptions_;
};

MobileClient::MobileClient(const NetworkConfig& config)
    : impl_(std::make_unique<Impl>(config)) {
}

MobileClient::~MobileClient() = default;

void MobileClient::GetBalance(const std::string& address, BalanceCallback callback) {
    if (!address.empty()) {
        callback(std::nullopt,
                 "Address filtering is not supported; pass an empty string to query wallet balances");
        return;
    }
    std::string error;
    Balance balance;
    const std::vector<std::pair<std::string, uint64_t*>> assets = {
        {"TALANTON", &balance.taln},
        {"DRACHMA", &balance.drm},
        {"OBOLOS", &balance.obl}};

    for (const auto& entry : assets) {
        auto result = RpcRequest(impl_->config_, "getbalance", json::array({entry.first}), error);
        if (!result) {
            callback(std::nullopt, error);
            return;
        }

        if (!result->is_object() || !result->contains("balance")) {
            callback(std::nullopt, "Unexpected balance response");
            return;
        }

        if ((*result)["balance"].is_number_unsigned()) {
            *entry.second = (*result)["balance"].get<uint64_t>();
        } else if ((*result)["balance"].is_string()) {
            try {
                *entry.second = static_cast<uint64_t>(std::stoull((*result)["balance"].get<std::string>()));
            } catch (...) {
                callback(std::nullopt, "Invalid balance value");
                return;
            }
        }
    }

    callback(balance, std::nullopt);
}

void MobileClient::SendTransaction(const Transaction& tx, TransactionCallback callback) {
    std::string error;
    if (!tx.from.empty()) {
        callback(std::nullopt, "Sender selection is managed by the node wallet; 'from' is not supported");
        return;
    }
    if (tx.fee > 0) {
        callback(std::nullopt, "Custom fees are not supported by the RPC endpoint");
        return;
    }

    auto asset_id = AssetIdFor(tx.asset);
    if (!asset_id) {
        callback(std::nullopt, "Unsupported asset type");
        return;
    }

    json params = json::array({tx.to, tx.amount, *asset_id});

    auto result = RpcRequest(impl_->config_, "sendtoaddress", params, error);
    if (!result) {
        callback(std::nullopt, error);
        return;
    }

    if (result->is_string()) {
        callback(result->get<std::string>(), std::nullopt);
        return;
    }

    callback(std::nullopt, "Unexpected transaction response");
}

void MobileClient::GetTransactionHistory(const std::string& address, uint32_t limit, HistoryCallback callback) {
    if (!address.empty()) {
        callback({}, "Address filtering is not supported; pass an empty string for wallet history");
        return;
    }
    std::string error;
    std::vector<TransactionHistory> history;
    if (limit == 0) {
        callback(history, std::nullopt);
        return;
    }

    auto height_opt = FetchBlockHeight(impl_->config_, error);
    if (!height_opt) {
        callback({}, error);
        return;
    }

    uint64_t height = *height_opt;
    uint64_t scanned = 0;
    for (uint64_t current = height;
         current > 0 && history.size() < limit && scanned < kMaxTransactionScanBlocks;
         --current, ++scanned) {
        auto block_info = FetchBlockInfo(impl_->config_, current, error);
        if (!block_info || !block_info->is_object() || !block_info->contains("tx")) {
            continue;
        }

        const uint64_t confirmations = height - current + 1;
        const uint64_t timestamp = block_info->value("timestamp", 0ULL);

        for (const auto& txid : (*block_info)["tx"]) {
            if (!txid.is_string()) {
                continue;
            }
            TransactionHistory entry;
            entry.txid = txid.get<std::string>();
            entry.timestamp = timestamp;
            entry.status = "confirmed";
            entry.confirmations = static_cast<uint32_t>(confirmations);
            entry.asset = "UNKNOWN";
            history.push_back(entry);
            if (history.size() >= limit) {
                break;
            }
        }
    }

    callback(history, std::nullopt);
}

void MobileClient::GetTransaction(const std::string& txid, TxInfoCallback callback) {
    std::string error;
    auto height_opt = FetchBlockHeight(impl_->config_, error);
    if (!height_opt) {
        callback(std::nullopt, error);
        return;
    }

    const uint64_t max_scan = kMaxTransactionScanBlocks;
    uint64_t height = *height_opt;
    uint64_t scanned = 0;
    for (uint64_t current = height; current > 0 && scanned < max_scan; --current, ++scanned) {
        auto block_info = FetchBlockInfo(impl_->config_, current, error);
        if (!block_info || !block_info->is_object() || !block_info->contains("tx")) {
            continue;
        }

        const uint64_t confirmations = height - current + 1;
        const uint64_t timestamp = block_info->value("timestamp", 0ULL);

        for (const auto& entry : (*block_info)["tx"]) {
            if (!entry.is_string()) {
                continue;
            }
            if (entry.get<std::string>() == txid) {
                TransactionHistory info;
                info.txid = txid;
                info.timestamp = timestamp;
                info.status = "confirmed";
                info.confirmations = static_cast<uint32_t>(confirmations);
                info.asset = "UNKNOWN";
                callback(info, std::nullopt);
                return;
            }
        }
    }

    callback(std::nullopt, "Transaction not found");
}

void MobileClient::CallContract(const ContractCall& call, ContractCallCallback callback) {
    [[maybe_unused]] const ContractCall& c = call;
    callback(std::nullopt, "Contract calls are not supported by the current SDK implementation");
}

void MobileClient::DeployContract(const std::vector<uint8_t>& bytecode, TransactionCallback callback) {
    [[maybe_unused]] const std::vector<uint8_t>& code = bytecode;
    callback(std::nullopt, "Contract deployment is not supported by the current SDK implementation");
}

void MobileClient::SubscribeToBlocks(BlockCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->subscription_mutex_);
    impl_->subscriptions_.emplace_back([config = impl_->config_, running = &impl_->running_, callback]() {
        std::string error;
        uint64_t last_height = 0;
        auto height_opt = FetchBlockHeight(config, error);
        if (height_opt) {
            last_height = *height_opt;
        }

        while (running->load()) {
            auto current_height_opt = FetchBlockHeight(config, error);
            if (current_height_opt) {
                for (uint64_t height = last_height + 1; height <= *current_height_opt; ++height) {
                    auto block_info = FetchBlockInfo(config, height, error);
                    if (block_info && block_info->is_object()) {
                        std::string hash = block_info->value("hash", "");
                        try {
                            callback(height, hash);
                        } catch (...) {
                        }
                    }
                }
                last_height = *current_height_opt;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultPollIntervalMs));
        }
    });
}

void MobileClient::SubscribeToAddress(const std::string& address, AddressTxCallback callback) {
    if (address.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->subscription_mutex_);
    impl_->subscriptions_.emplace_back([config = impl_->config_, running = &impl_->running_, callback]() {
        std::string error;
        uint64_t last_height = 0;
        auto height_opt = FetchBlockHeight(config, error);
        if (height_opt) {
            last_height = *height_opt;
        }

        std::unordered_set<std::string> seen_tx;
        std::deque<std::string> seen_order;

        while (running->load()) {
            auto current_height_opt = FetchBlockHeight(config, error);
            if (current_height_opt) {
                for (uint64_t height = last_height + 1; height <= *current_height_opt; ++height) {
                    auto block_info = FetchBlockInfo(config, height, error);
                    if (!block_info || !block_info->is_object() || !block_info->contains("tx")) {
                        continue;
                    }

                    const uint64_t confirmations = *current_height_opt - height + 1;
                    const uint64_t timestamp = block_info->value("timestamp", 0ULL);

                    for (const auto& txid : (*block_info)["tx"]) {
                        if (!txid.is_string()) {
                            continue;
                        }
                        const auto id = txid.get<std::string>();
                        if (seen_tx.insert(id).second) {
                            seen_order.push_back(id);
                            if (seen_order.size() > kMaxSeenTransactions) {
                                seen_tx.erase(seen_order.front());
                                seen_order.pop_front();
                            }
                            TransactionHistory entry;
                            entry.txid = id;
                            entry.timestamp = timestamp;
                            entry.status = "confirmed";
                            entry.confirmations = static_cast<uint32_t>(confirmations);
                            entry.asset = "UNKNOWN";
                            try {
                                callback(entry);
                            } catch (...) {
                            }
                        }
                    }
                }
                last_height = *current_height_opt;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kDefaultPollIntervalMs));
        }
    });
}

void MobileClient::EstimateGas(const Transaction& tx, GasEstimateCallback callback) {
    [[maybe_unused]] const Transaction& transaction = tx;
    callback(std::nullopt, "Gas estimation is not supported by the current SDK implementation");
}

void MobileClient::GetNetworkStatus(NetworkStatusCallback callback) {
    std::string error;
    auto result = RpcRequest(impl_->config_, "getinfo", json::array(), error);
    if (!result || !result->is_object()) {
        callback(std::nullopt, error.empty() ? "Unexpected network status response" : error);
        return;
    }

    NetworkStatus status;
    status.block_height = result->value("blocks", 0ULL);
    status.peer_count = result->value("connections", 0ULL);
    status.syncing = result->value("syncing", false);
    status.network_id = impl_->config_.network_id;
    callback(status, std::nullopt);
}

// QRCodeHelper implementation
std::string QRCodeHelper::GeneratePaymentURI(
    const std::string& address,
    uint64_t amount,
    const std::string& asset,
    const std::string& memo) {
    
    std::ostringstream uri;
    uri << "pantheon:" << address;
    uri << "?amount=" << amount;
    uri << "&asset=" << asset;
    if (!memo.empty()) {
        uri << "&memo=" << memo;
    }
    return uri.str();
}

std::optional<QRCodeHelper::PaymentRequest> QRCodeHelper::ParsePaymentURI(const std::string& uri) {
    // In production: parse URI with proper URL decoding
    if (uri.substr(0, 9) != "pantheon:") {
        return std::nullopt;
    }
    
    PaymentRequest req;
    // Simplified parsing
    req.address = "ptn1q" + std::string(39, '0');
    req.amount = 1000;
    req.asset = "TALN";
    return req;
}

// SecureStorage implementation
bool SecureStorage::Store([[maybe_unused]] const std::string& key, [[maybe_unused]] const std::vector<uint8_t>& data) {
    std::array<uint8_t, kStorageKeySize> storage_key{};
    if (!LoadOrCreateStorageKey(storage_key)) {
        return false;
    }

    std::vector<uint8_t> encrypted;
    if (!EncryptStorage(storage_key, data, encrypted)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_storage_mutex);
    auto path = StorageDataPath(key);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(encrypted.data()), static_cast<std::streamsize>(encrypted.size()));
    out.close();
    return true;
}

std::optional<std::vector<uint8_t>> SecureStorage::Retrieve([[maybe_unused]] const std::string& key) {
    std::array<uint8_t, kStorageKeySize> storage_key{};
    if (!LoadOrCreateStorageKey(storage_key)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(g_storage_mutex);
    auto path = StorageDataPath(key);
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }

    std::vector<uint8_t> encrypted((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
    std::vector<uint8_t> plaintext;
    if (!DecryptStorage(storage_key, encrypted, plaintext)) {
        return std::nullopt;
    }
    return plaintext;
}

bool SecureStorage::Delete([[maybe_unused]] const std::string& key) {
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    auto path = StorageDataPath(key);
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

bool SecureStorage::Exists([[maybe_unused]] const std::string& key) {
    std::lock_guard<std::mutex> lock(g_storage_mutex);
    auto path = StorageDataPath(key);
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

} // namespace mobile
} // namespace parthenon
