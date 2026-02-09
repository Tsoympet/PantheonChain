// ParthenonChain RPC Client
// Copyright (c) 2024 ParthenonChain Developers
// Distributed under the MIT software license

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace parthenon {

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

        if (method == "getinfo") {
            result += "{\"version\": \"1.0.0\", \"blocks\": 12345, \"connections\": 8}";
        } else if (method == "getblockcount") {
            result += "12345";
        } else if (method == "getbalance") {
            std::string asset = params.empty() ? "TALN" : params[0];
            result += "{\"asset\": \"" + asset + "\", \"balance\": 1000.50}";
        } else if (method == "sendtoaddress") {
            if (params.size() < 3) {
                return "{\"error\": \"Usage: sendtoaddress <asset> <address> <amount>\"}";
            }
            result +=
                "{\"txid\": \"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\"}";
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
        std::cout << "  getinfo                              - Get node information\n";
        std::cout << "  getblockcount                        - Get current block height\n";
        std::cout << "  getbalance [asset]                   - Get wallet balance\n";
        std::cout << "  sendtoaddress <asset> <addr> <amt>   - Send transaction\n";
        std::cout << "  stop                                 - Stop the daemon\n";
        std::cout << "  help                                 - Show this help\n";
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
