// ParthenonChain - JSON-RPC Server Implementation

#include "rpc_server.h"
#include "node/node.h"
#include "wallet/wallet.h"
#include "primitives/transaction.h"
#include <sstream>
#include <iomanip>

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
    
    // TODO: Implement HTTP server initialization
    // Would use a library like libmicrohttpd or cpp-httplib
    // For now, just mark as running
    running_ = true;
    
    return true;
}

void RPCServer::Stop() {
    if (!running_) {
        return;
    }
    
    // TODO: Implement HTTP server shutdown
    running_ = false;
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
    
    try {
        return it->second(request);
    } catch (const std::exception& e) {
        response.error = std::string("Internal error: ") + e.what();
        return response;
    }
}

void RPCServer::InitializeStandardMethods() {
    RegisterMethod("getinfo", [this](const RPCRequest& req) { 
        return HandleGetInfo(req); 
    });
    RegisterMethod("getbalance", [this](const RPCRequest& req) { 
        return HandleGetBalance(req); 
    });
    RegisterMethod("getblockcount", [this](const RPCRequest& req) { 
        return HandleGetBlockCount(req); 
    });
    RegisterMethod("getblock", [this](const RPCRequest& req) { 
        return HandleGetBlock(req); 
    });
    RegisterMethod("sendtransaction", [this](const RPCRequest& req) { 
        return HandleSendTransaction(req); 
    });
    RegisterMethod("getnewaddress", [this](const RPCRequest& req) {
        return HandleGetNewAddress(req);
    });
    RegisterMethod("sendtoaddress", [this](const RPCRequest& req) {
        return HandleSendToAddress(req);
    });
}

RPCResponse RPCServer::HandleGetInfo(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    std::ostringstream json;
    json << "{"
         << "\"version\":\"1.0.0\","
         << "\"protocolversion\":1,";
    
    if (node_) {
        json << "\"blocks\":" << node_->GetHeight() << ","
             << "\"connections\":" << node_->GetPeers().size() << ",";
        
        auto sync_status = node_->GetSyncStatus();
        json << "\"syncing\":" << (sync_status.is_syncing ? "true" : "false") << ","
             << "\"syncprogress\":" << std::fixed << std::setprecision(2) 
             << sync_status.progress_percent << ",";
    } else {
        json << "\"blocks\":0,"
             << "\"connections\":0,"
             << "\"syncing\":false,"
             << "\"syncprogress\":0,";
    }
    
    json << "\"difficulty\":1.0,"
         << "\"testnet\":true"
         << "}";
    
    response.result = json.str();
    return response;
}

RPCResponse RPCServer::HandleGetBalance(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (!wallet_) {
        response.error = "Wallet not initialized";
        return response;
    }
    
    auto balances = wallet_->GetBalances();
    
    std::ostringstream json;
    json << "{"
         << "\"TALANTON\":" << balances[primitives::AssetID::TALANTON] << ","
         << "\"DRACHMA\":" << balances[primitives::AssetID::DRACHMA] << ","
         << "\"OBOLOS\":" << balances[primitives::AssetID::OBOLOS]
         << "}";
    
    response.result = json.str();
    return response;
}

RPCResponse RPCServer::HandleGetBlockCount(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (node_) {
        response.result = std::to_string(node_->GetHeight());
    } else {
        response.result = "0";
    }
    
    return response;
}

RPCResponse RPCServer::HandleGetBlock(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }
    
    // Parse block height/hash from params
    if (req.params.empty()) {
        response.error = "Missing block height or hash parameter";
        return response;
    }
    
    // For now, treat param as height
    uint64_t height = 0;
    try {
        height = std::stoull(req.params[0]);
    } catch (...) {
        response.error = "Invalid height parameter";
        return response;
    }
    
    // TODO: Get block from node's chain (would need Chain::GetBlockByHeight)
    // For now, create a mock response structure
    std::ostringstream json;
    json << "{"
         << "\"hash\":\"0000000000000000000000000000000000000000000000000000000000000000\","
         << "\"height\":" << height << ","
         << "\"version\":1,"
         << "\"previousblockhash\":\"0000000000000000000000000000000000000000000000000000000000000000\","
         << "\"merkleroot\":\"0000000000000000000000000000000000000000000000000000000000000000\","
         << "\"time\":0,"
         << "\"bits\":\"1d00ffff\","
         << "\"nonce\":0,"
         << "\"tx\":[]"
         << "}";
    
    response.result = json.str();
    return response;
}

RPCResponse RPCServer::HandleSendTransaction(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }
    
    // Parse transaction hex from params
    if (req.params.empty()) {
        response.error = "Missing transaction hex parameter";
        return response;
    }
    
    std::string tx_hex = req.params[0];
    
    // Deserialize transaction from hex
    std::vector<uint8_t> tx_data;
    for (size_t i = 0; i < tx_hex.length(); i += 2) {
        std::string byte_str = tx_hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        tx_data.push_back(byte);
    }
    
    // TODO: Proper transaction deserialization
    // For now, create a mock transaction
    primitives::Transaction tx;
    
    // Submit transaction to node
    bool success = node_->SubmitTransaction(tx);
    
    if (success) {
        // Return transaction ID (hash)
        auto tx_hash = tx.GetHash();
        std::ostringstream hex;
        hex << std::hex << std::setfill('0');
        for (uint8_t byte : tx_hash) {
            hex << std::setw(2) << static_cast<int>(byte);
        }
        response.result = "\"" + hex.str() + "\"";
    } else {
        response.error = "Transaction rejected by mempool";
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
    
    // Generate new address
    auto address = wallet_->GenerateAddress("rpc");
    
    // Convert pubkey to hex string
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (uint8_t byte : address.pubkey) {
        hex << std::setw(2) << static_cast<int>(byte);
    }
    
    std::ostringstream json;
    json << "\""  << hex.str() << "\"";
    
    response.result = json.str();
    return response;
}

RPCResponse RPCServer::HandleSendToAddress(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (!wallet_) {
        response.error = "Wallet not initialized";
        return response;
    }
    
    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }
    
    // Parse parameters: address, amount, asset_id (optional)
    if (req.params.size() < 2) {
        response.error = "Missing required parameters: address, amount";
        return response;
    }
    
    std::string address_hex = req.params[0];
    uint64_t amount = 0;
    try {
        amount = std::stoull(req.params[1]);
    } catch (...) {
        response.error = "Invalid amount parameter";
        return response;
    }
    
    // Parse asset ID (default to TALANTON)
    primitives::AssetID asset_id = primitives::AssetID::TALANTON;
    if (req.params.size() >= 3) {
        int asset_int = std::stoi(req.params[2]);
        asset_id = static_cast<primitives::AssetID>(asset_int);
    }
    
    // Convert address hex to pubkey
    std::array<uint8_t, 32> recipient_pubkey{};
    for (size_t i = 0; i < 32 && i * 2 < address_hex.length(); i++) {
        std::string byte_str = address_hex.substr(i * 2, 2);
        recipient_pubkey[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }
    
    // Create output
    primitives::TxOutput output;
    output.value = primitives::AssetAmount(asset_id, amount);
    output.pubkey_script = recipient_pubkey;
    
    // Create transaction using wallet
    auto tx_result = wallet_->CreateTransaction({output}, asset_id, 1000); // 1000 sat fee
    
    if (!tx_result.has_value()) {
        response.error = "Failed to create transaction (insufficient funds?)";
        return response;
    }
    
    auto tx = tx_result.value();
    
    // Submit to node
    bool success = node_->SubmitTransaction(tx);
    
    if (success) {
        // Return transaction ID
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
    
    return response;
}

} // namespace rpc
} // namespace parthenon
