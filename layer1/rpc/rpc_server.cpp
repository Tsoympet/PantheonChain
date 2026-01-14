// ParthenonChain - JSON-RPC Server Implementation with HTTP Support

#include "rpc_server.h"

#include "primitives/transaction.h"

#include "node/node.h"
#include "wallet/wallet.h"

#include <httplib.h>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

using json = nlohmann::json;

namespace parthenon {
namespace rpc {

RPCServer::RPCServer(uint16_t port)
    : port_(port), running_(false), node_(nullptr), wallet_(nullptr) {
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
    auto server = std::make_shared<httplib::Server>();

    // Set up POST endpoint for JSON-RPC
    server->Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        try {
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
            RPCResponse rpc_res = HandleRequest(rpc_req);

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
    std::thread([server, this]() {
        std::cout << "RPC HTTP server listening on port " << port_ << std::endl;
        server->listen("127.0.0.1", port_);
    }).detach();

    running_ = true;
    std::cout << "RPC server started successfully" << std::endl;

    return true;
}

void RPCServer::Stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping RPC server..." << std::endl;
    // Note: cpp-httplib server will stop when server object is destroyed
    running_ = false;
    std::cout << "RPC server stopped" << std::endl;
}

void RPCServer::RegisterMethod(const std::string& method, RPCHandler handler) {
    methods_[method] = handler;
}

RPCResponse RPCServer::HandleRequest(const RPCRequest& request) {
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
            asset = params[0].get<std::string>();
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
        result["balance"] = balance;
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
            height = std::stoull(params[0].get<std::string>());
        }

        // Get block from node's chain
        auto block_opt = node_->GetBlockByHeight(height);

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
        tx_bytes.reserve(tx_hex.length() / 2);

        for (size_t i = 0; i < tx_hex.length(); i += 2) {
            std::string byte_str = tx_hex.substr(i, 2);
            try {
                uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
                tx_bytes.push_back(byte);
            } catch (...) {
                response.error = "Invalid hex character in transaction";
                return response;
            }
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
            amount = std::stoull(params[1].get<std::string>());
        }

        // Parse asset ID (default to TALANTON)
        primitives::AssetID asset_id = primitives::AssetID::TALANTON;
        if (params.size() >= 3) {
            int asset_int = params[2].get<int>();
            asset_id = static_cast<primitives::AssetID>(asset_int);
        }

        // Convert address hex to pubkey
        std::vector<uint8_t> recipient_pubkey;
        for (size_t i = 0; i < address_hex.length(); i += 2) {
            std::string byte_str = address_hex.substr(i, 2);
            recipient_pubkey.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
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

}  // namespace rpc
}  // namespace parthenon
