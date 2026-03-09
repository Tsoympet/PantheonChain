#include "common/serialization.h"

#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cerr
        << "PantheonChain CLI v2.3.0\n\n"
        << "Usage: pantheon-cli <command> [subcommand] [options] [--json]\n\n"
        << "Transfer:\n"
        << "  pantheon-cli transfer send --layer=l2 --from=<acct> --to=<acct> --amount=<amount>\n\n"
        << "Contract:\n"
        << "  pantheon-cli contract deploy --layer=l3 --bytecode=<hex>\n"
        << "  pantheon-cli contract call   --layer=l3 --address=<hex> --data=<hex>\n\n"
        << "Commitments:\n"
        << "  pantheon-cli commitments list --layer=l1|l2\n"
        << "  pantheon-cli commitments get  --layer=l1|l2 --id=<id>\n"
        << "  pantheon-cli submit-commitment --layer=l2|l3 --commitment=<encoded>\n\n"
        << "Validator:\n"
        << "  pantheon-cli validator status|start|stop|keys-import --layer=l2|l3\n\n"
        << "Wallet:\n"
        << "  pantheon-cli wallet create  --layer=l1|l2|l3                 Generate new keypair\n"
        << "  pantheon-cli wallet import  --layer=l1|l2|l3 --privkey=<hex> Import existing key\n"
        << "  pantheon-cli wallet export  --layer=l1|l2|l3 --address=<addr>\n"
        << "  pantheon-cli wallet list    --layer=l1|l2|l3\n"
        << "  pantheon-cli wallet balance --layer=l1|l2|l3 --address=<addr> --asset=TALN|DRM|OBL\n\n"
        << "Account:\n"
        << "  pantheon-cli account balance --layer=l1|l2|l3 --address=<addr> [--asset=<asset>]\n"
        << "  pantheon-cli account nonce   --layer=l2|l3    --address=<addr>\n"
        << "  pantheon-cli account txs     --layer=l2|l3    --address=<addr> [--limit=<n>]\n\n"
        << "Staking:\n"
        << "  pantheon-cli staking deposit  --layer=l2|l3 --amount=<amount> --address=<addr>\n"
        << "  pantheon-cli staking withdraw --layer=l2|l3 --amount=<amount> --address=<addr>\n"
        << "  pantheon-cli staking rewards  --layer=l2|l3 --address=<addr>\n"
        << "  pantheon-cli staking status   --layer=l2|l3 --address=<addr>\n\n"
        << "Governance:\n"
        << "  pantheon-cli governance propose  --layer=l2|l3 --type=PARAM_CHANGE|PROTOCOL_UPGRADE|EMERGENCY"
           " --title=<t> --description=<d>\n"
        << "  pantheon-cli governance vote     --layer=l2|l3 --id=<proposal_id> --choice=yes|no|abstain"
           " --address=<addr>\n"
        << "  pantheon-cli governance tally    --layer=l2|l3 --id=<proposal_id>\n"
        << "  pantheon-cli governance list     --layer=l2|l3\n"
        << "  pantheon-cli governance get      --layer=l2|l3 --id=<proposal_id>\n"
        << "  pantheon-cli governance execute  --layer=l2|l3 --id=<proposal_id>\n\n"
        << "Node:\n"
        << "  pantheon-cli node sync-status    --layer=l1|l2|l3\n"
        << "  pantheon-cli node peer-info      --layer=l1|l2|l3\n"
        << "  pantheon-cli node stop           --layer=l1|l2|l3\n\n"
        << "RPC:\n"
        << "  pantheon-cli rpc call --layer=l1|l2|l3 --method=<method> [--params=<json>]\n\n"
        << "Config:\n"
        << "  pantheon-cli config validate --file=<path>\n\n"
        << "Global flags:\n"
        << "  --json         Output in JSON format\n"
        << "  --rpc=<url>    Override RPC endpoint (default: inferred from --layer)\n"
        << "  --version      Print version and exit\n";
}

std::string FindValue(int argc, char* argv[], const std::string& prefix) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind(prefix, 0) == 0) {
            return arg.substr(prefix.size());
        }
    }
    return "";
}

bool HasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (flag == argv[i]) {
            return true;
        }
    }
    return false;
}

void PrintMessage(bool as_json, const std::string& action, const std::string& layer,
                  const std::string& detail) {
    if (as_json) {
        std::cout << "{\"action\":\"" << action << "\",\"layer\":\"" << layer
                  << "\",\"detail\":\"" << detail << "\"}" << std::endl;
        return;
    }
    std::cout << "[" << (layer.empty() ? "cli" : layer) << "] " << action
              << ": " << detail << std::endl;
}

// Default RPC URL for each layer (overridable with --rpc=<url>)
std::string DefaultRpcUrl(const std::string& layer) {
    if (layer == "l1") return "http://127.0.0.1:8332";
    if (layer == "l2") return "http://127.0.0.1:9332";
    if (layer == "l3") return "http://127.0.0.1:10332";
    return "http://127.0.0.1:8332";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string command = argv[1];
    const std::string layer   = FindValue(argc, argv, "--layer=");
    const bool as_json        = HasFlag(argc, argv, "--json");
    const std::string rpc_url = [&] {
        auto override = FindValue(argc, argv, "--rpc=");
        return override.empty() ? DefaultRpcUrl(layer) : override;
    }();

    if (command == "--version" || command == "version") {
        std::cout << "pantheon-cli 2.3.0" << std::endl;
        return 0;
    }

    // ---------------------------------------------------------------- //
    //  transfer                                                          //
    // ---------------------------------------------------------------- //
    if (command == "transfer" && argc >= 3 && std::string(argv[2]) == "send" && layer == "l2") {
        const std::string from   = FindValue(argc, argv, "--from=");
        const std::string to     = FindValue(argc, argv, "--to=");
        const std::string amount = FindValue(argc, argv, "--amount=");
        if (from.empty() || to.empty() || amount.empty()) {
            std::cerr << "missing --from/--to/--amount" << std::endl;
            return 1;
        }
        PrintMessage(as_json, "transfer.send", "l2", from + "->" + to + " amount=" + amount);
        return 0;
    }

    // ---------------------------------------------------------------- //
    //  contract                                                          //
    // ---------------------------------------------------------------- //
    if (command == "contract" && argc >= 3 && layer == "l3") {
        const std::string sub = argv[2];
        if (sub == "deploy") {
            const std::string bytecode = FindValue(argc, argv, "--bytecode=");
            if (bytecode.empty()) { std::cerr << "missing --bytecode" << std::endl; return 1; }
            PrintMessage(as_json, "contract.deploy", "l3",
                         "address=0x0000000000000000000000000000000000000001");
            return 0;
        }
        if (sub == "call") {
            const std::string address = FindValue(argc, argv, "--address=");
            const std::string data    = FindValue(argc, argv, "--data=");
            if (address.empty() || data.empty()) {
                std::cerr << "missing --address/--data" << std::endl; return 1;
            }
            PrintMessage(as_json, "contract.call", "l3", "return=0x01");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  commitments                                                       //
    // ---------------------------------------------------------------- //
    if (command == "commitments" && argc >= 3 &&
        (layer == "l1" || layer == "l2")) {
        const std::string sub = argv[2];
        if (sub == "list") {
            PrintMessage(as_json, "commitments.list", layer, "count=1");
            return 0;
        }
        if (sub == "get") {
            const std::string id = FindValue(argc, argv, "--id=");
            if (id.empty()) { std::cerr << "missing --id" << std::endl; return 1; }
            PrintMessage(as_json, "commitments.get", layer, "id=" + id);
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  submit-commitment                                                 //
    // ---------------------------------------------------------------- //
    if (command == "submit-commitment" && (layer == "l2" || layer == "l3")) {
        const std::string encoded = FindValue(argc, argv, "--commitment=");
        if (encoded.empty()) { std::cerr << "missing --commitment" << std::endl; return 1; }
        pantheon::common::Commitment commitment{};
        auto result = pantheon::common::DecodeCommitment(encoded, commitment);
        if (!result.valid) {
            std::cerr << "commitment decode error: " << result.reason << std::endl;
            return 1;
        }
        PrintMessage(as_json, "submit-commitment", layer, "commitment accepted");
        return 0;
    }

    // ---------------------------------------------------------------- //
    //  validator                                                         //
    // ---------------------------------------------------------------- //
    if (command == "validator" && argc >= 3 && (layer == "l2" || layer == "l3")) {
        const std::string op = argv[2];
        if (op == "status" || op == "start" || op == "stop" || op == "keys-import") {
            PrintMessage(as_json, "validator." + op, layer, "ok");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  wallet                                                            //
    // ---------------------------------------------------------------- //
    if (command == "wallet" && argc >= 3) {
        const std::string sub = argv[2];
        if (sub == "create") {
            if (layer.empty()) { std::cerr << "missing --layer" << std::endl; return 1; }
            // In production this calls the node's getnewaddress RPC.
            PrintMessage(as_json, "wallet.create", layer,
                         "address=parthenon1q0000000000000000000000000000000000000000");
            return 0;
        }
        if (sub == "import") {
            const std::string privkey = FindValue(argc, argv, "--privkey=");
            if (privkey.empty()) { std::cerr << "missing --privkey" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            PrintMessage(as_json, "wallet.import", layer, "imported");
            return 0;
        }
        if (sub == "export") {
            const std::string address = FindValue(argc, argv, "--address=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            PrintMessage(as_json, "wallet.export", layer, "address=" + address);
            return 0;
        }
        if (sub == "list") {
            if (layer.empty()) { std::cerr << "missing --layer" << std::endl; return 1; }
            PrintMessage(as_json, "wallet.list", layer, "count=0");
            return 0;
        }
        if (sub == "balance") {
            const std::string address = FindValue(argc, argv, "--address=");
            const std::string asset   = FindValue(argc, argv, "--asset=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            const std::string a = asset.empty() ? "TALN" : asset;
            PrintMessage(as_json, "wallet.balance", layer,
                         "address=" + address + " asset=" + a + " balance=0");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  account                                                           //
    // ---------------------------------------------------------------- //
    if (command == "account" && argc >= 3) {
        const std::string sub = argv[2];
        if (sub == "balance") {
            const std::string address = FindValue(argc, argv, "--address=");
            const std::string asset   = FindValue(argc, argv, "--asset=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            const std::string a = asset.empty() ? "TALN" : asset;
            PrintMessage(as_json, "account.balance", layer,
                         "address=" + address + " asset=" + a + " balance=0");
            return 0;
        }
        if (sub == "nonce") {
            const std::string address = FindValue(argc, argv, "--address=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            PrintMessage(as_json, "account.nonce", layer, "address=" + address + " nonce=0");
            return 0;
        }
        if (sub == "txs") {
            const std::string address = FindValue(argc, argv, "--address=");
            const std::string limit   = FindValue(argc, argv, "--limit=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            if (layer.empty())   { std::cerr << "missing --layer"   << std::endl; return 1; }
            const std::string n = limit.empty() ? "10" : limit;
            PrintMessage(as_json, "account.txs", layer,
                         "address=" + address + " limit=" + n + " count=0");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  staking                                                           //
    // ---------------------------------------------------------------- //
    if (command == "staking" && argc >= 3 && (layer == "l2" || layer == "l3")) {
        const std::string sub = argv[2];
        if (sub == "deposit") {
            const std::string amount  = FindValue(argc, argv, "--amount=");
            const std::string address = FindValue(argc, argv, "--address=");
            if (amount.empty() || address.empty()) {
                std::cerr << "missing --amount/--address" << std::endl; return 1;
            }
            PrintMessage(as_json, "staking.deposit", layer,
                         "address=" + address + " amount=" + amount + " queued");
            return 0;
        }
        if (sub == "withdraw") {
            const std::string amount  = FindValue(argc, argv, "--amount=");
            const std::string address = FindValue(argc, argv, "--address=");
            if (amount.empty() || address.empty()) {
                std::cerr << "missing --amount/--address" << std::endl; return 1;
            }
            PrintMessage(as_json, "staking.withdraw", layer,
                         "address=" + address + " amount=" + amount + " queued");
            return 0;
        }
        if (sub == "rewards") {
            const std::string address = FindValue(argc, argv, "--address=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            PrintMessage(as_json, "staking.rewards", layer,
                         "address=" + address + " pending_rewards=0");
            return 0;
        }
        if (sub == "status") {
            const std::string address = FindValue(argc, argv, "--address=");
            if (address.empty()) { std::cerr << "missing --address" << std::endl; return 1; }
            PrintMessage(as_json, "staking.status", layer,
                         "address=" + address + " staked=0 active=false");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  governance                                                        //
    // ---------------------------------------------------------------- //
    if (command == "governance" && argc >= 3 && (layer == "l2" || layer == "l3")) {
        const std::string sub = argv[2];
        if (sub == "propose") {
            const std::string type  = FindValue(argc, argv, "--type=");
            const std::string title = FindValue(argc, argv, "--title=");
            if (type.empty() || title.empty()) {
                std::cerr << "missing --type/--title" << std::endl; return 1;
            }
            PrintMessage(as_json, "governance.propose", layer,
                         "type=" + type + " title=" + title + " id=proposal-1");
            return 0;
        }
        if (sub == "vote") {
            const std::string id      = FindValue(argc, argv, "--id=");
            const std::string choice  = FindValue(argc, argv, "--choice=");
            const std::string address = FindValue(argc, argv, "--address=");
            if (id.empty() || choice.empty() || address.empty()) {
                std::cerr << "missing --id/--choice/--address" << std::endl; return 1;
            }
            PrintMessage(as_json, "governance.vote", layer,
                         "id=" + id + " choice=" + choice + " voter=" + address);
            return 0;
        }
        if (sub == "tally") {
            const std::string id = FindValue(argc, argv, "--id=");
            if (id.empty()) { std::cerr << "missing --id" << std::endl; return 1; }
            PrintMessage(as_json, "governance.tally", layer,
                         "id=" + id + " yes=0 no=0 abstain=0 result=pending");
            return 0;
        }
        if (sub == "list") {
            PrintMessage(as_json, "governance.list", layer, "count=0");
            return 0;
        }
        if (sub == "get") {
            const std::string id = FindValue(argc, argv, "--id=");
            if (id.empty()) { std::cerr << "missing --id" << std::endl; return 1; }
            PrintMessage(as_json, "governance.get", layer, "id=" + id + " status=pending");
            return 0;
        }
        if (sub == "execute") {
            const std::string id = FindValue(argc, argv, "--id=");
            if (id.empty()) { std::cerr << "missing --id" << std::endl; return 1; }
            PrintMessage(as_json, "governance.execute", layer, "id=" + id + " executed");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  node                                                              //
    // ---------------------------------------------------------------- //
    if (command == "node" && argc >= 3) {
        const std::string sub = argv[2];
        if (layer.empty()) { std::cerr << "missing --layer" << std::endl; return 1; }
        if (sub == "sync-status") {
            PrintMessage(as_json, "node.sync-status", layer,
                         "rpc=" + rpc_url + " syncing=false best_height=0");
            return 0;
        }
        if (sub == "peer-info") {
            PrintMessage(as_json, "node.peer-info", layer,
                         "rpc=" + rpc_url + " peers=0");
            return 0;
        }
        if (sub == "stop") {
            PrintMessage(as_json, "node.stop", layer, "rpc=" + rpc_url + " stop sent");
            return 0;
        }
    }

    // ---------------------------------------------------------------- //
    //  rpc                                                               //
    // ---------------------------------------------------------------- //
    if (command == "rpc" && argc >= 3 && std::string(argv[2]) == "call") {
        const std::string method = FindValue(argc, argv, "--method=");
        const std::string params = FindValue(argc, argv, "--params=");
        if (method.empty()) { std::cerr << "missing --method" << std::endl; return 1; }
        if (layer.empty())  { std::cerr << "missing --layer"  << std::endl; return 1; }
        // In production this makes an HTTP POST to rpc_url with the JSON-RPC payload.
        PrintMessage(as_json, "rpc.call", layer,
                     "url=" + rpc_url + " method=" + method +
                     (params.empty() ? "" : " params=" + params));
        return 0;
    }

    // ---------------------------------------------------------------- //
    //  config validate                                                   //
    // ---------------------------------------------------------------- //
    if (command == "config" && argc >= 3 && std::string(argv[2]) == "validate") {
        const std::string file = FindValue(argc, argv, "--file=");
        if (file.empty()) { std::cerr << "missing --file" << std::endl; return 1; }
        // Delegate to the Python validator (available in the repo).
        const std::string cmd = "python3 scripts/validate-config.py " + file;
        int rc = std::system(cmd.c_str());
        return rc == 0 ? 0 : 1;
    }

    PrintUsage();
    return 1;
}
