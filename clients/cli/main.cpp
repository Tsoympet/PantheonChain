// ParthenonChain RPC Client
// Copyright (c) 2024 ParthenonChain Developers
// Distributed under the MIT software license

#include "common/monetary/units.h"
#include "primitives/asset.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace parthenon {

namespace {

std::string AmountToRaw(const std::string& amount, const std::string& unit_flag) {
    primitives::AssetID asset = primitives::AssetID::TALANTON;
    if (unit_flag == "--in-dr") {
        asset = primitives::AssetID::DRACHMA;
    } else if (unit_flag == "--in-ob") {
        asset = primitives::AssetID::OBOLOS;
    }

    const auto parsed = common::monetary::ParseDisplayAmount(amount, asset);
    if (!parsed) {
        return "invalid";
    }
    return std::to_string(*parsed);
}

void PrintDualAmount(uint64_t raw, primitives::AssetID asset) {
    const auto view = common::monetary::BuildAmountView(raw, asset);
    std::cout << "amount_raw=" << view.amount_raw << " amount=" << view.amount << " token=" << view.token
              << std::endl;
}

}  // namespace

class RPCClient {
  private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;

  public:
    RPCClient(const std::string& host, int port, const std::string& user,
              const std::string& password)
        : host_(host), port_(port), user_(user), password_(password) {}

    std::string Call(const std::string& method, const std::vector<std::string>& params) {
        // Simulate RPC call
        (void)host_;      // Reserved for future networked RPC integration
        (void)port_;      // Reserved for future networked RPC integration
        (void)user_;      // Reserved for future RPC authentication integration
        (void)password_;  // Reserved for future RPC authentication integration

        std::string result = "{\"result\": ";

        if (method == "getinfo" || method == "chain/info") {
            result += "{\"version\": \"1.0.0\", \"blocks\": 12345, \"connections\": 8}";
        } else if (method == "chain/monetary_spec") {
            result += "{\"spec_hash\": \"" + common::monetary::MonetarySpecHash() +
                      "\", \"ratio_dr_per_tal\": 6000, \"ratio_ob_per_dr\": 6, \"ratio_ob_per_tal\": 36000}";
        } else if (method == "staking/deposit") {
            if (params.empty()) {
                return "{\"error\": \"Usage: stake deposit --layer=l2|l3\"}";
            }
            result += "{\"status\":\"accepted\",\"module\":\"staking\",\"layer\":\"" +
                      params[0] + "\",\"fee_token\":\"DRACHMA\"}";
        } else if (method == "evm/deploy") {
            if (params.empty()) {
                return "{\"error\": \"Usage: deploy-contract --layer=l3\"}";
            }
            result += "{\"status\":\"accepted\",\"module\":\"evm\",\"layer\":\"" +
                      params[0] + "\",\"fee_token\":\"OBOLOS\"}";
        } else if (method == "commitments/submit") {
            if (params.empty()) {
                return "{\"error\": \"Usage: submit-commitment --layer=l2|l3\"}";
            }
            result +=
                "{\"status\":\"queued\",\"module\":\"commitments\",\"layer\":\"" +
                params[0] + "\"}";
        } else if (method == "getblockcount") {
            result += "12345";
        } else if (method == "getbalance") {
            std::string asset = params.empty() ? "TALANTON" : params[0];
            uint64_t raw = 100050000000ULL;
            primitives::AssetID id = primitives::AssetID::TALANTON;
            if (asset == "DRACHMA") {
                id = primitives::AssetID::DRACHMA;
            } else if (asset == "OBOLOS") {
                id = primitives::AssetID::OBOLOS;
            }
            auto view = common::monetary::BuildAmountView(raw, id);
            result += "{\"asset\": \"" + asset + "\", \"balance\": " + std::to_string(raw) +
                      ", \"amount_raw\": \"" + std::to_string(view.amount_raw) +
                      "\", \"amount\": \"" + view.amount + "\", \"token\": \"" + view.token + "\"}";
        } else if (method == "sendtoaddress") {
            if (params.size() < 3) {
                return "{\"error\": \"Usage: sendtoaddress <asset> <address> <amount_raw>\"}";
            }
            result +=
                "{\"txid\": \"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\",\"amount_raw\":\"" +
                params[2] + "\"}";
        } else if (method == "stop") {
            result += "\"ParthenonChain server stopping\"";
        } else {
            return "{\"error\": \"Unknown command: " + method + "\"}";
        }

        result += "}";
        return result;
    }
};

class CLI {
  private:
    RPCClient rpc_;

    void ShowHelp() {
        std::cout << "ParthenonChain RPC Client v1.0.0\n\n";
        std::cout << "Available commands:\n";
        std::cout << "  getinfo                                              - Get node information\n";
        std::cout << "  getblockcount                                        - Get current block height\n";
        std::cout << "  getbalance [asset]                                   - Get wallet balance\n";
        std::cout << "  sendtoaddress <asset> <addr> <amt> [--in-tal|--in-dr|--in-ob] - Send transaction\n";
        std::cout << "  chain/monetary_spec                                  - Get monetary unit spec\n";
        std::cout << "  stop                                                 - Stop the daemon\n";
        std::cout << "  stake deposit --layer=l2|l3                          - Submit staking deposit\n";
        std::cout << "  deploy-contract --layer=l3                           - Deploy EVM contract on OBOLOS\n";
        std::cout << "  submit-commitment --layer=l2|l3                      - Submit L2/L3 commitment\n";
        std::cout << "  help                                                 - Show this help\n";
        std::cout << std::endl;
    }

  public:
    CLI(const std::string& host, int port, const std::string& user, const std::string& password)
        : rpc_(host, port, user, password) {}

    void ExecuteCommand(const std::string& cmd, const std::vector<std::string>& args) {
        if (cmd == "help") {
            ShowHelp();
            return;
        }

        if (cmd == "stake" && !args.empty() && args[0] == "deposit") {
            std::string layer = "l2";
            for (const auto& arg : args) {
                if (arg.rfind("--layer=", 0) == 0) {
                    layer = arg.substr(8);
                }
            }
            std::cout << rpc_.Call("staking/deposit", {layer}) << std::endl;
            return;
        }

        if (cmd == "deploy-contract") {
            std::string layer = "l3";
            for (const auto& arg : args) {
                if (arg.rfind("--layer=", 0) == 0) {
                    layer = arg.substr(8);
                }
            }
            std::cout << rpc_.Call("evm/deploy", {layer}) << std::endl;
            return;
        }

        if (cmd == "submit-commitment") {
            std::string layer = "l2";
            for (const auto& arg : args) {
                if (arg.rfind("--layer=", 0) == 0) {
                    layer = arg.substr(8);
                }
            }
            std::cout << rpc_.Call("commitments/submit", {layer}) << std::endl;
            return;
        }

        if (cmd == "sendtoaddress" && args.size() >= 3) {
            std::string unit_flag = "--in-tal";
            for (const auto& arg : args) {
                if (arg == "--in-tal" || arg == "--in-dr" || arg == "--in-ob") {
                    unit_flag = arg;
                }
            }
            std::string raw = AmountToRaw(args[2], unit_flag);
            if (raw == "invalid") {
                std::cout << "{\"error\":\"invalid amount for denomination\"}" << std::endl;
                return;
            }
            std::vector<std::string> params = {args[0], args[1], raw};
            std::cout << rpc_.Call(cmd, params) << std::endl;
            return;
        }

        if (cmd == "getbalance") {
            std::string asset = args.empty() ? "TALANTON" : args[0];
            std::cout << rpc_.Call(cmd, {asset}) << std::endl;
            if (asset == "DRACHMA") {
                auto ob = common::monetary::ConvertDrToOb(100050000000ULL);
                if (ob) {
                    PrintDualAmount(*ob, primitives::AssetID::OBOLOS);
                }
            }
            return;
        }

        std::string response = rpc_.Call(cmd, args);
        std::cout << response << std::endl;
    }

    void InteractiveMode() {
        std::cout << "ParthenonChain RPC Client (interactive mode)\n";
        std::cout << "Type 'help' for available commands, 'quit' to exit\n\n";

        std::string line;
        while (true) {
            std::cout << "parthenon> ";
            if (!std::getline(std::cin, line))
                break;

            if (line.empty())
                continue;
            if (line == "quit" || line == "exit")
                break;

            // Parse command and arguments
            std::vector<std::string> tokens;
            size_t start = 0;
            while (start < line.size()) {
                size_t end = line.find(' ', start);
                if (end == std::string::npos) {
                    tokens.push_back(line.substr(start));
                    break;
                }
                tokens.push_back(line.substr(start, end - start));
                start = end + 1;
            }

            if (tokens.empty())
                continue;

            std::string cmd = tokens[0];
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());

            ExecuteCommand(cmd, args);
        }
    }
};

}  // namespace parthenon

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8332;
    std::string user = "parthenon";
    std::string password = "changeme";

    parthenon::CLI cli(host, port, user, password);

    if (argc == 1) {
        // Interactive mode
        cli.InteractiveMode();
    } else {
        // Batch mode
        std::string cmd = argv[1];
        std::vector<std::string> args;
        for (int i = 2; i < argc; i++) {
            args.push_back(argv[i]);
        }
        cli.ExecuteCommand(cmd, args);
    }

    return 0;
}
