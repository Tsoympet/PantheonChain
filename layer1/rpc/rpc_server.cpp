// ParthenonChain - JSON-RPC Server Implementation

#include "rpc_server.h"
#include "node/node.h"
#include "wallet/wallet.h"
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
    
    // TODO: Parse height/hash from params
    // TODO: Get block from node
    // TODO: Serialize block to JSON
    
    response.error = "Not fully implemented";
    return response;
}

RPCResponse RPCServer::HandleSendTransaction(const RPCRequest& req) {
    RPCResponse response;
    response.id = req.id;
    
    if (!node_) {
        response.error = "Node not initialized";
        return response;
    }
    
    // TODO: Parse transaction hex from params
    // TODO: Deserialize transaction
    // TODO: Submit to node
    
    response.error = "Not fully implemented";
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
    
    // TODO: Parse address, amount, asset from params
    // TODO: Create transaction using wallet
    // TODO: Submit to node
    
    response.error = "Not fully implemented";
    return response;
}

} // namespace rpc
} // namespace parthenon
