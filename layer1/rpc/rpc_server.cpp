// ParthenonChain - JSON-RPC Server Implementation with HTTP Support

#include "rpc_server.h"

#include "primitives/transaction.h"

#include "node/node.h"
#include "common/monetary/units.h"
#include "validation.h"
#include "wallet/wallet.h"

#include <httplib.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <thread>

using json = nlohmann::json;

namespace {

std::string Base64Encode(const std::string& input) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((input.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= input.size()) {
        const uint32_t chunk = (static_cast<uint32_t>(static_cast<unsigned char>(input[i])) << 16) |
                               (static_cast<uint32_t>(static_cast<unsigned char>(input[i + 1])) << 8) |
                               static_cast<uint32_t>(static_cast<unsigned char>(input[i + 2]));
        encoded.push_back(table[(chunk >> 18) & 0x3F]);
        encoded.push_back(table[(chunk >> 12) & 0x3F]);
        encoded.push_back(table[(chunk >> 6) & 0x3F]);
        encoded.push_back(table[chunk & 0x3F]);
        i += 3;
    }

    const size_t remaining = input.size() - i;
    if (remaining == 1) {
        const uint32_t chunk = static_cast<uint32_t>(static_cast<unsigned char>(input[i])) << 16;
        encoded.push_back(table[(chunk >> 18) & 0x3F]);
        encoded.push_back(table[(chunk >> 12) & 0x3F]);
        encoded.push_back('=');
        encoded.push_back('=');
    } else if (remaining == 2) {
        const uint32_t chunk = (static_cast<uint32_t>(static_cast<unsigned char>(input[i])) << 16) |
                               (static_cast<uint32_t>(static_cast<unsigned char>(input[i + 1])) << 8);
        encoded.push_back(table[(chunk >> 18) & 0x3F]);
        encoded.push_back(table[(chunk >> 12) & 0x3F]);
        encoded.push_back(table[(chunk >> 6) & 0x3F]);
        encoded.push_back('=');
    }

    return encoded;
}

bool StartsWithCaseInsensitive(const std::string& value, const std::string& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }

    return std::equal(prefix.begin(), prefix.end(), value.begin(), [](char lhs, char rhs) {
        return std::tolower(static_cast<unsigned char>(lhs)) ==
               std::tolower(static_cast<unsigned char>(rhs));
    });
}



std::string TrimAsciiWhitespace(const std::string& value) {
    size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    size_t end = value.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

bool ConstantTimeEquals(const std::string& lhs, const std::string& rhs) {
    const size_t max_len = std::max(lhs.size(), rhs.size());
    unsigned char diff = static_cast<unsigned char>(lhs.size() ^ rhs.size());

    for (size_t i = 0; i < max_len; ++i) {
        const unsigned char l = i < lhs.size() ? static_cast<unsigned char>(lhs[i]) : 0;
        const unsigned char r = i < rhs.size() ? static_cast<unsigned char>(rhs[i]) : 0;
        diff |= static_cast<unsigned char>(l ^ r);
    }

    return diff == 0;
}

std::string ExtractAuthorizationHeader(const httplib::Request& req) {
#ifdef CPP_HTTPLIB_STUB_H
    (void)req;
    return "";
#else
    if (req.has_header("Authorization")) {
        return req.get_header_value("Authorization");
    }
    return "";
#endif
}

void SetAuthChallengeHeader(httplib::Response& res) {
#ifdef CPP_HTTPLIB_STUB_H
    (void)res;
#else
    res.set_header("WWW-Authenticate", "Basic realm=\"parthenon-rpc\"");
#endif
}

bool TryParseHexByte(char high, char low, uint8_t& out) {
    auto to_nibble = [](char c, uint8_t& nibble) {
        if (c >= '0' && c <= '9') {
            nibble = static_cast<uint8_t>(c - '0');
            return true;
        }
        if (c >= 'a' && c <= 'f') {
            nibble = static_cast<uint8_t>(10 + c - 'a');
            return true;
        }
        if (c >= 'A' && c <= 'F') {
            nibble = static_cast<uint8_t>(10 + c - 'A');
            return true;
        }
        return false;
    };

    uint8_t hi = 0;
    uint8_t lo = 0;
    if (!to_nibble(high, hi) || !to_nibble(low, lo)) {
        return false;
    }

    out = static_cast<uint8_t>((hi << 4) | lo);
    return true;
}

bool TryParseHexString(const std::string& hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }

    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t byte = 0;
        if (!TryParseHexByte(hex[i], hex[i + 1], byte)) {
            return false;
        }
        out.push_back(byte);
    }
    return true;
}

bool TryParseUint64Decimal(const std::string& value, uint64_t& out) {
    if (value.empty()) {
        return false;
    }

    uint64_t parsed = 0;
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (ec != std::errc{} || ptr != value.data() + value.size()) {
        return false;
    }

    out = parsed;
    return true;
}

}  // namespace

namespace parthenon {
namespace rpc {

RPCServer::RPCServer(uint16_t port)
    : port_(port),
      running_(false),
      node_(nullptr),
      wallet_(nullptr),
      rate_limiter_(std::make_unique<RateLimiter>(100, 60)) {
    if (!parthenon::common::monetary::ValidateMonetaryInvariants()) {
        throw std::runtime_error("Monetary constants invariant violation at startup");
    }
    InitializeStandardMethods();
}

RPCServer::~RPCServer() {
    Stop();
}

void RPCServer::SetNode(node::Node* node) {
    node_ = node;
}

void RPCServer::SetWallet(wallet::Wallet* wallet) {
    wallet_ = wallet;
}

bool RPCServer::Start() {
    if (running_) {
        return false;
    }

    std::cout << "Starting RPC server on port " << port_ << std::endl;

    // Create HTTP server
    http_server_ = std::make_shared<httplib::Server>();

    // Set up POST endpoint for JSON-RPC
#ifndef CPP_HTTPLIB_STUB_H
    http_server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        (void)req;
        json health = {{"status", running_ ? "ok" : "stopped"}, {"rpc_port", port_}};
        res.set_content(health.dump(), "application/json");
    });
#endif

    http_server_->Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            // Extract client IP for rate limiting
            std::string client_ip = req.remote_addr;

            // Check rate limit
            if (!rate_limiter_->AllowRequest(client_ip)) {
                json error_response;
                error_response["jsonrpc"] = "2.0";
                error_response["error"] = {
                    {"code", -32001}, {"message", "Rate limit exceeded. Please try again later."}};
                error_response["id"] = nullptr;
                res.status = 429;  // Too Many Requests
                res.set_content(error_response.dump(), "application/json");
                return;
            }

            if (IsAuthenticationEnabled()) {
                const auto auth_header = ExtractAuthorizationHeader(req);
                if (!IsAuthorized(auth_header)) {
                    json error_response;
                    error_response["jsonrpc"] = "2.0";
                    error_response["error"] = {{"code", -32600},
                                               {"message", "Authentication required"}};
                    error_response["id"] = nullptr;
                    res.status = 401;
                    SetAuthChallengeHeader(res);
                    res.set_content(error_response.dump(), "application/json");
                    return;
                }
            }

            // Parse JSON-RPC request
            auto j = json::parse(req.body);

            RPCRequest rpc_req;
            rpc_req.method = j.value("method", "");
            rpc_req.id = j.value("id", "");

            // Convert params to JSON string
            if (j.contains("params")) {
                rpc_req.params = j["params"].dump();
            }

            // Handle the request
            RPCResponse rpc_res = HandleRequest(rpc_req, client_ip);

            // Build JSON-RPC response
            json response;
            response["jsonrpc"] = "2.0";
            response["id"] = rpc_res.id;

            if (rpc_res.IsError()) {
                response["error"] = {{"code", -1}, {"message", rpc_res.error}};
            } else {
                // Parse result as JSON if possible
                try {
                    response["result"] = json::parse(rpc_res.result);
                } catch (...) {
                    response["result"] = rpc_res.result;
                }
            }

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            json error_response;
            error_response["jsonrpc"] = "2.0";
            error_response["error"] = {{"code", -32700},
                                       {"message", "Parse error: " + std::string(e.what())}};
            error_response["id"] = nullptr;
            res.set_content(error_response.dump(), "application/json");
        }
    });

    // Start server in background thread
    running_ = true;
    server_thread_ = std::thread([this]() {
        std::cout << "RPC HTTP server listening on port " << port_ << std::endl;
        if (http_server_) {
            http_server_->listen("127.0.0.1", port_);
        }
    });
    std::cout << "RPC server started successfully" << std::endl;

    return true;
}

void RPCServer::Stop() {
    if (!running_ && !server_thread_.joinable()) {
        return;
    }

    std::cout << "Stopping RPC server..." << std::endl;
    running_ = false;

    if (http_server_) {
        http_server_->stop();
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    http_server_.reset();
    std::cout << "RPC server stopped" << std::endl;
}

void RPCServer::RegisterMethod(const std::string& method, RPCHandler handler) {
    methods_[method] = handler;
}

void RPCServer::ConfigureRateLimit(uint32_t requests_per_window, uint32_t window_seconds) {
    rate_limiter_ = std::make_unique<RateLimiter>(requests_per_window, window_seconds);
}

void RPCServer::ConfigureBasicAuth(const std::string& user, const std::string& password) {
    auth_user_ = user;
    auth_password_ = password;
}

bool RPCServer::IsAuthenticationEnabled() const {
    return !auth_user_.empty() && !auth_password_.empty();
}

bool RPCServer::IsAuthorized(const std::string& authorization_header) const {
    if (!IsAuthenticationEnabled()) {
        return true;
    }

    if (!StartsWithCaseInsensitive(authorization_header, "Basic ")) {
        return false;
    }

    const auto provided_token = TrimAsciiWhitespace(authorization_header.substr(6));
    const auto expected_token = Base64Encode(auth_user_ + ":" + auth_password_);
    return ConstantTimeEquals(provided_token, expected_token);
}

RPCResponse RPCServer::HandleRequest(const RPCRequest& request,
                                     const std::string& /* client_ip */) {
    RPCResponse response;
    response.id = request.id;

    auto it = methods_.find(request.method);
    if (it == methods_.end()) {
        response.error = "Method not found: " + request.method;
        return response;
    }

    return it->second(request);
}

void RPCServer::InitializeStandardMethods() {
    RegisterMethod("getinfo", [this](const RPCRequest& req) { return HandleGetInfo(req); });
    RegisterMethod("getbalance", [this](const RPCRequest& req) { return HandleGetBalance(req); });
    RegisterMethod("getblockcount",
                   [this](const RPCRequest& req) { return HandleGetBlockCount(req); });
    RegisterMethod("getblock", [this](const RPCRequest& req) { return HandleGetBlock(req); });
    RegisterMethod("sendrawtransaction",
                   [this](const RPCRequest& req) { return HandleSendTransaction(req); });
    RegisterMethod("getnewaddress",
                   [this](const RPCRequest& req) { return HandleGetNewAddress(req); });
    RegisterMethod("sendtoaddress",
                   [this](const RPCRequest& req) { return HandleSendToAddress(req); });
    RegisterMethod("stop", [this](const RPCRequest& req) { return HandleStop(req); });
    RegisterMethod("chain/info", [this](const RPCRequest& req) { return HandleChainInfo(req); });
    RegisterMethod("chain/monetary_spec", [this](const RPCRequest& req) { return HandleMonetarySpec(req); });
    RegisterMethod("staking/deposit", [this](const RPCRequest& req) { return HandleStakingDeposit(req); });
    RegisterMethod("commitments/submit", [this](const RPCRequest& req) { return HandleCommitmentSubmit(req); });
    RegisterMethod("commitments/list", [this](const RPCRequest& req) { return HandleCommitmentList(req); });
    RegisterMethod("evm/deploy", [this](const RPCRequest& req) { return HandleEvmDeploy(req); });
}

RPCResponse RPCServer::HandleGetInfo(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }

    json info;
    info["version"] = 100;
    info["protocolversion"] = 70015;
    info["blocks"] = node_->GetHeight();
    info["connections"] = node_->GetPeers().size();

    auto sync_status = node_->GetSyncStatus();
    info["syncing"] = sync_status.is_syncing;
    if (sync_status.is_syncing) {
        info["sync_progress"] = sync_status.progress_percent;
    }

    response.result = info.dump();
    return response;
}

RPCResponse RPCServer::HandleGetBalance(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!wallet_) {
        response.error = "Wallet not initialized";
        return response;
    }

    // Parse parameters
    try {
        auto params = json::parse(req.params);
        std::string asset = "TALANTON";  // default

        if (params.is_array() && params.size() > 0) {
            asset = InputValidator::SanitizeString(params[0].get<std::string>());
        }

        // Validate asset name
        if (!InputValidator::ValidateAssetName(asset)) {
            response.error = "Invalid asset name. Must be TALANTON, DRACHMA, or OBOLOS";
            return response;
        }

        // Get balance from wallet
        if (!wallet_) {
            response.error = "Wallet not attached";
            return response;
        }

        // Map asset name to AssetID
        primitives::AssetID asset_id = primitives::AssetID::TALANTON;
        if (asset == "DRACHMA") {
            asset_id = primitives::AssetID::DRACHMA;
        } else if (asset == "OBOLOS") {
            asset_id = primitives::AssetID::OBOLOS;
        }

        uint64_t balance = wallet_->GetBalance(asset_id);

        json result;
        const auto view = parthenon::common::monetary::BuildAmountView(balance, asset_id);
        result["balance"] = balance;
        result["amount_raw"] = std::to_string(view.amount_raw);
        result["amount"] = view.amount;
        result["token"] = view.token;
        result["asset"] = asset;

        response.result = result.dump();
    } catch (const std::exception& e) {
        response.error = "Invalid parameters: " + std::string(e.what());
    }

    return response;
}

RPCResponse RPCServer::HandleGetBlockCount(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }

    response.result = std::to_string(node_->GetHeight());
    return response;
}

RPCResponse RPCServer::HandleGetBlock(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }

    try {
        auto params = json::parse(req.params);

        if (!params.is_array() || params.empty()) {
            response.error = "Missing block height or hash parameter";
            return response;
        }

        uint64_t height = 0;
        if (params[0].is_number()) {
            height = params[0].get<uint64_t>();
        } else {
            auto height_opt = InputValidator::ParseUint64(params[0].get<std::string>());
            if (!height_opt) {
                response.error = "Invalid block height format";
                return response;
            }
            height = *height_opt;
        }

        // Validate block height
        if (!InputValidator::ValidateBlockHeight(height, node_->GetHeight())) {
            response.error = "Block height exceeds chain height";
            return response;
        }

        // Get block from node's chain
        auto block_opt = node_->GetBlockByHeight(static_cast<uint32_t>(height));

        if (!block_opt) {
            response.error = "Block not found at height " + std::to_string(height);
            return response;
        }

        const auto& block = *block_opt;

        // Build block info JSON
        json block_info;

        // Convert block hash to hex
        auto block_hash = block.GetHash();
        std::ostringstream hash_hex;
        hash_hex << std::hex << std::setfill('0');
        for (uint8_t byte : block_hash) {
            hash_hex << std::setw(2) << static_cast<int>(byte);
        }
        block_info["hash"] = hash_hex.str();

        block_info["height"] = height;
        block_info["version"] = block.header.version;
        block_info["timestamp"] = block.header.timestamp;
        block_info["nonce"] = block.header.nonce;

        // Convert previous hash to hex
        std::ostringstream prev_hex;
        prev_hex << std::hex << std::setfill('0');
        for (uint8_t byte : block.header.prev_block_hash) {
            prev_hex << std::setw(2) << static_cast<int>(byte);
        }
        block_info["previousblockhash"] = prev_hex.str();

        // Convert merkle root to hex
        std::ostringstream merkle_hex;
        merkle_hex << std::hex << std::setfill('0');
        for (uint8_t byte : block.header.merkle_root) {
            merkle_hex << std::setw(2) << static_cast<int>(byte);
        }
        block_info["merkleroot"] = merkle_hex.str();

        // Add transactions
        json tx_array = json::array();
        for (const auto& tx : block.transactions) {
            auto tx_hash = tx.GetTxID();
            std::ostringstream tx_hex;
            tx_hex << std::hex << std::setfill('0');
            for (uint8_t byte : tx_hash) {
                tx_hex << std::setw(2) << static_cast<int>(byte);
            }
            tx_array.push_back(tx_hex.str());
        }
        block_info["tx"] = tx_array;
        block_info["size"] = block.transactions.size();

        response.result = block_info.dump();

    } catch (const std::exception& e) {
        response.error = "Invalid parameters: " + std::string(e.what());
    }

    return response;
}

RPCResponse RPCServer::HandleSendTransaction(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }

    try {
        auto params = json::parse(req.params);

        if (!params.is_array() || params.empty()) {
            response.error = "Missing transaction hex parameter";
            return response;
        }

        std::string tx_hex = params[0].get<std::string>();

        // Deserialize transaction from hex
        // Remove "0x" prefix if present
        if (tx_hex.substr(0, 2) == "0x" || tx_hex.substr(0, 2) == "0X") {
            tx_hex = tx_hex.substr(2);
        }

        // Convert hex string to bytes
        if (tx_hex.length() % 2 != 0) {
            response.error = "Invalid hex string (odd length)";
            return response;
        }

        std::vector<uint8_t> tx_bytes;
        if (!TryParseHexString(tx_hex, tx_bytes)) {
            response.error = "Invalid hex character in transaction";
            return response;
        }

        // Deserialize transaction from bytes
        auto tx_opt = primitives::Transaction::Deserialize(tx_bytes.data(), tx_bytes.size());

        if (!tx_opt) {
            response.error = "Failed to deserialize transaction";
            return response;
        }

        primitives::Transaction tx = *tx_opt;

        // Submit transaction to node
        bool success = node_->SubmitTransaction(tx);

        if (success) {
            auto tx_hash = tx.GetTxID();
            std::ostringstream hex;
            hex << std::hex << std::setfill('0');
            for (uint8_t byte : tx_hash) {
                hex << std::setw(2) << static_cast<int>(byte);
            }
            response.result = "\"" + hex.str() + "\"";
        } else {
            response.error = "Transaction rejected by mempool";
        }

    } catch (const std::exception& e) {
        response.error = "Invalid parameters: " + std::string(e.what());
    }

    return response;
}

RPCResponse RPCServer::HandleGetNewAddress(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!wallet_) {
        response.error = "Wallet not initialized";
        return response;
    }

    try {
        auto params = json::parse(req.params);
        std::string label = "";

        if (params.is_array() && params.size() > 0) {
            label = params[0].get<std::string>();
        }

        auto addr = wallet_->GenerateAddress(label);

        // Convert pubkey to hex
        std::ostringstream hex;
        hex << std::hex << std::setfill('0');
        for (uint8_t byte : addr.pubkey) {
            hex << std::setw(2) << static_cast<int>(byte);
        }

        response.result = "\"" + hex.str() + "\"";

    } catch (const std::exception& e) {
        response.error = "Failed to generate address: " + std::string(e.what());
    }

    return response;
}

RPCResponse RPCServer::HandleSendToAddress(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!wallet_ || !node_) {
        response.error = "Wallet or node not initialized";
        return response;
    }

    try {
        auto params = json::parse(req.params);

        if (!params.is_array() || params.size() < 2) {
            response.error = "Missing required parameters: address, amount";
            return response;
        }

        std::string address_hex = params[0].get<std::string>();
        uint64_t amount = 0;

        if (params[1].is_number()) {
            amount = params[1].get<uint64_t>();
        } else {
            if (!TryParseUint64Decimal(params[1].get<std::string>(), amount)) {
                response.error = "Invalid amount";
                return response;
            }
        }

        // Parse asset ID (default to TALANTON)
        primitives::AssetID asset_id = primitives::AssetID::TALANTON;
        if (params.size() >= 3) {
            int asset_int = params[2].get<int>();
            if (asset_int < static_cast<int>(primitives::AssetID::TALANTON) ||
                asset_int > static_cast<int>(primitives::AssetID::OBOLOS)) {
                response.error = "Invalid asset ID";
                return response;
            }
            asset_id = static_cast<primitives::AssetID>(asset_int);
        }

        // Convert address hex to pubkey
        std::vector<uint8_t> recipient_pubkey;
        if (!TryParseHexString(address_hex, recipient_pubkey)) {
            response.error = "Invalid recipient address hex";
            return response;
        }

        // Create output
        primitives::TxOutput output;
        output.value = primitives::AssetAmount(asset_id, amount);
        output.pubkey_script = recipient_pubkey;

        // Create transaction using wallet
        auto tx_result = wallet_->CreateTransaction({output}, asset_id, 1000);  // 1000 sat fee

        if (!tx_result.has_value()) {
            response.error = "Failed to create transaction (insufficient funds?)";
            return response;
        }

        auto tx = tx_result.value();

        // Submit to node
        bool success = node_->SubmitTransaction(tx);

        if (success) {
            auto tx_hash = tx.GetTxID();
            std::ostringstream hex;
            hex << std::hex << std::setfill('0');
            for (uint8_t byte : tx_hash) {
                hex << std::setw(2) << static_cast<int>(byte);
            }
            response.result = "\"" + hex.str() + "\"";
        } else {
            response.error = "Transaction rejected by mempool";
        }

    } catch (const std::exception& e) {
        response.error = "Failed to send: " + std::string(e.what());
    }

    return response;
}

RPCResponse RPCServer::HandleStop(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }

    node_->Stop();
    response.result = "\"Node stopping\"";
    return response;
}

}  // namespace rpc
}  // namespace parthenon

namespace parthenon {
namespace rpc {

RPCResponse RPCServer::HandleChainInfo(const RPCRequest& req) {
    auto response = HandleGetInfo(req);
    if (response.IsError()) {
        return response;
    }

    auto info = json::parse(response.result);
    info["monetary_spec_hash"] = parthenon::common::monetary::MonetarySpecHash();
    response.result = info.dump();
    return response;
}

RPCResponse RPCServer::HandleMonetarySpec(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;

    if (!parthenon::common::monetary::ValidateMonetaryInvariants()) {
        response.error = "Monetary constants invariant violation";
        return response;
    }

    json result;
    result["spec_hash"] = parthenon::common::monetary::MonetarySpecHash();
    result["payload"] = parthenon::common::monetary::MonetarySpecPayload();
    result["ratios"] = {
        {"dr_per_tal", parthenon::common::monetary::RATIO_DR_PER_TAL},
        {"ob_per_dr", parthenon::common::monetary::RATIO_OB_PER_DR},
        {"ob_per_tal", parthenon::common::monetary::RATIO_OB_PER_TAL},
    };
    result["decimals"] = {
        {"tal", parthenon::common::monetary::TAL_DECIMALS},
        {"dr", parthenon::common::monetary::DR_DECIMALS},
        {"ob", parthenon::common::monetary::OB_DECIMALS},
    };
    result["unit_table"] = {
        {"1 DRACHMA", "6 OBOLOS"},
        {"1 TALANTON", "6000 DRACHMA"},
        {"1 TALANTON (OB)", "36000 OBOLOS"},
    };

    response.result = result.dump();
    return response;
}

RPCResponse RPCServer::HandleStakingDeposit(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    json result;
    result["status"] = "accepted";
    result["module"] = "staking";
    result["fee_token"] = "DRACHMA";
    result["fee_note"] = "L2 fees are paid in DRACHMA; optional OBOLOS equivalent is informational.";
    result["params"] = req.params.empty() ? json::array() : json::parse(req.params, nullptr, false);
    response.result = result.dump();
    return response;
}

RPCResponse RPCServer::HandleCommitmentSubmit(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    const std::string payload = req.params.empty() ? "[]" : req.params;
    {
        std::lock_guard<std::mutex> lock(commitment_mutex_);
        commitment_log_.push_back(payload);
    }
    json result;
    result["status"] = "queued";
    result["count"] = commitment_log_.size();
    response.result = result.dump();
    return response;
}

RPCResponse RPCServer::HandleCommitmentList(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    json result;
    result["commitments"] = json::array();
    {
        std::lock_guard<std::mutex> lock(commitment_mutex_);
        for (const auto& entry : commitment_log_) {
            result["commitments"].push_back(entry);
        }
        result["count"] = commitment_log_.size();
    }
    response.result = result.dump();
    return response;
}

RPCResponse RPCServer::HandleEvmDeploy(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    json result;
    result["status"] = "accepted";
    result["module"] = "evm";
    result["fee_token"] = "OBOLOS";
    result["fee_note"] = "L3 gas is paid in OBOLOS; DRACHMA/TALANTON equivalents are reporting-only.";
    result["params"] = req.params.empty() ? json::array() : json::parse(req.params, nullptr, false);
    response.result = result.dump();
    return response;
}

}  // namespace rpc
}  // namespace parthenon
