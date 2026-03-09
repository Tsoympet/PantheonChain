// ParthenonChain - GraphQL API Implementation
// GraphQL endpoint for blockchain queries

#include "graphql_api.h"

#include <algorithm>
#include <sstream>
#include <cctype>

namespace parthenon {
namespace layer2 {
namespace apis {

namespace {

// ---------------------------------------------------------------------------
// Minimal GraphQL document parser
// ---------------------------------------------------------------------------
// Extracts the top-level operation type ("query"/"mutation"/"subscription")
// and the first root-level field name from a GraphQL document.  This avoids
// full lexer complexity while correctly handling:
//   - Anonymous queries:  { blocks { ... } }
//   - Named queries:      query GetBlocks { blocks { ... } }
//   - Whitespace / comments (single-line # ... )
// Returns the root field name in lower-case, or "" if none found.
static std::string ExtractRootField(const std::string& doc) {
    std::string s = doc;

    // Strip single-line comments
    {
        std::string stripped;
        stripped.reserve(s.size());
        bool in_comment = false;
        for (char c : s) {
            if (c == '#') { in_comment = true; }
            if (c == '\n') { in_comment = false; }
            if (!in_comment) stripped += c;
        }
        s = std::move(stripped);
    }

    // Skip optional operation keyword and name: query/mutation/subscription [Name]
    // so we can find the opening '{'.
    size_t pos = 0;
    auto skip_ws = [&]() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
    };

    skip_ws();
    // Peek at keyword
    if (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos]))) {
        // Read the first identifier
        size_t word_start = pos;
        while (pos < s.size() && (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_'))
            ++pos;
        std::string word = s.substr(word_start, pos - word_start);
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        if (word == "query" || word == "mutation" || word == "subscription") {
            // Skip optional operation name
            skip_ws();
            if (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos]))) {
                while (pos < s.size() &&
                       (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_'))
                    ++pos;
            }
            // Skip optional variable definitions  ( ... )
            skip_ws();
            if (pos < s.size() && s[pos] == '(') {
                int depth = 1;
                ++pos;
                while (pos < s.size() && depth > 0) {
                    if (s[pos] == '(') ++depth;
                    else if (s[pos] == ')') --depth;
                    ++pos;
                }
            }
        }
        // else: anonymous shorthand query starting with a field name — rewind
        else {
            pos = word_start;
        }
    }

    // Expect '{'
    skip_ws();
    if (pos >= s.size() || s[pos] != '{') return "";
    ++pos;

    // Read first field name inside the selection set
    skip_ws();
    if (pos >= s.size() || !std::isalpha(static_cast<unsigned char>(s[pos]))) return "";

    size_t field_start = pos;
    while (pos < s.size() && (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_'))
        ++pos;

    std::string field = s.substr(field_start, pos - field_start);
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);
    return field;
}

// Extract a string argument value from a GraphQL field call.
// e.g.  extractArg("hash", "block(hash: \"0xabc\") { ... }")  → "0xabc"
static std::string ExtractArg(const std::string& arg_name, const std::string& query) {
    std::string key = arg_name + ":";
    auto pos = query.find(key);
    if (pos == std::string::npos) return "";
    pos += key.size();
    // skip whitespace
    while (pos < query.size() && std::isspace(static_cast<unsigned char>(query[pos]))) ++pos;
    if (pos >= query.size()) return "";

    if (query[pos] == '"') {
        // quoted string
        ++pos;
        std::string val;
        while (pos < query.size() && query[pos] != '"') val += query[pos++];
        return val;
    }
    // unquoted (e.g. number)
    std::string val;
    while (pos < query.size() && !std::isspace(static_cast<unsigned char>(query[pos])) &&
           query[pos] != ',' && query[pos] != ')' && query[pos] != '}')
        val += query[pos++];
    return val;
}

}  // namespace

class GraphQLAPI::Impl {
  public:
    Impl(uint16_t port) : port_(port), running_(false) {}

    bool Start() {
        if (running_) {
            return false;
        }

        if (port_ == 0) {
            return false;
        }

        running_ = true;
        return true;
    }

    void Stop() {
        if (!running_) {
            return;
        }

        running_ = false;
    }

    bool IsRunning() const { return running_; }

    std::string HandleQuery(const std::string& query) {
        // Use the proper field-extraction router instead of a raw substring scan.
        // This correctly handles both anonymous shorthand { blocks { ... } } and
        // named queries (query GetBlocks { blocks { ... } }).
        std::string root = ExtractRootField(query);

        if (root == "blocks" || root == "block") {
            return HandleBlocksQuery(query);
        }
        if (root == "transactions" || root == "transaction" || root == "tx") {
            return HandleTransactionsQuery(query);
        }
        if (root == "contract" || root == "contracts") {
            return HandleContractQuery(query);
        }
        if (root == "balance") {
            return HandleBalanceQuery(query);
        }

        // Fallback: legacy substring match for clients sending partial fragments
        if (query.find("blocks") != std::string::npos) {
            return HandleBlocksQuery(query);
        }
        if (query.find("transactions") != std::string::npos) {
            return HandleTransactionsQuery(query);
        }
        if (query.find("contract") != std::string::npos) {
            return HandleContractQuery(query);
        }

        return R"({"errors": [{"message": "Unknown query type"}]})";
    }

    void SetBlockCallback(std::function<std::string(const std::string&)> callback) {
        block_callback_ = callback;
    }

    void SetTransactionCallback(std::function<std::string(const std::string&)> callback) {
        tx_callback_ = callback;
    }

    void SetContractCallback(std::function<std::string(const std::string&)> callback) {
        contract_callback_ = callback;
    }

  private:
    std::string HandleBlocksQuery(const std::string& query) {
        if (block_callback_) {
            return block_callback_(query);
        }

        // Extract optional filter arguments
        std::string hash   = ExtractArg("hash",   query);
        std::string height = ExtractArg("height", query);
        std::string limit  = ExtractArg("limit",  query);

        std::ostringstream oss;
        oss << R"({"data":{"blocks":[],"filter":{)";
        if (!hash.empty())   oss << R"("hash":")"   << hash   << R"(",)";
        if (!height.empty()) oss << R"("height":")" << height << R"(",)";
        if (!limit.empty())  oss << R"("limit":")"  << limit  << R"(",)";
        oss << R"("applied":true}}})";
        return oss.str();
    }

    std::string HandleTransactionsQuery(const std::string& query) {
        if (tx_callback_) {
            return tx_callback_(query);
        }

        std::string txid  = ExtractArg("txid",    query);
        std::string addr  = ExtractArg("address", query);
        std::string limit = ExtractArg("limit",   query);

        std::ostringstream oss;
        oss << R"({"data":{"transactions":[],"filter":{)";
        if (!txid.empty())  oss << R"("txid":")"    << txid  << R"(",)";
        if (!addr.empty())  oss << R"("address":")" << addr  << R"(",)";
        if (!limit.empty()) oss << R"("limit":")"   << limit << R"(",)";
        oss << R"("applied":true}}})";
        return oss.str();
    }

    std::string HandleContractQuery(const std::string& query) {
        if (contract_callback_) {
            return contract_callback_(query);
        }

        std::string addr = ExtractArg("address", query);

        std::ostringstream oss;
        oss << R"({"data":{"contract":null,"filter":{)";
        if (!addr.empty()) oss << R"("address":")" << addr << R"(",)";
        oss << R"("applied":true}}})";
        return oss.str();
    }

    std::string HandleBalanceQuery(const std::string& query) {
        std::string addr  = ExtractArg("address", query);
        std::string asset = ExtractArg("asset",   query);

        std::ostringstream oss;
        oss << R"({"data":{"balance":{"amount":"0","asset":")";
        oss << (asset.empty() ? "TALN" : asset);
        oss << R"(","address":")";
        oss << addr;
        oss << R"("}})";
        oss << "}";
        return oss.str();
    }

    uint16_t port_;
    bool running_;
    std::function<std::string(const std::string&)> block_callback_;
    std::function<std::string(const std::string&)> tx_callback_;
    std::function<std::string(const std::string&)> contract_callback_;
};

GraphQLAPI::GraphQLAPI(uint16_t port) : impl_(std::make_unique<Impl>(port)) {}

GraphQLAPI::~GraphQLAPI() {
    Stop();
}

bool GraphQLAPI::Start() {
    return impl_->Start();
}

void GraphQLAPI::Stop() {
    impl_->Stop();
}

std::string GraphQLAPI::HandleQuery(const std::string& query) {
    return impl_->HandleQuery(query);
}

bool GraphQLAPI::IsRunning() const {
    return impl_->IsRunning();
}

void GraphQLAPI::SetBlockCallback(std::function<std::string(const std::string&)> callback) {
    impl_->SetBlockCallback(callback);
}

void GraphQLAPI::SetTransactionCallback(std::function<std::string(const std::string&)> callback) {
    impl_->SetTransactionCallback(callback);
}

void GraphQLAPI::SetContractCallback(std::function<std::string(const std::string&)> callback) {
    impl_->SetContractCallback(callback);
}

}  // namespace apis
}  // namespace layer2
}  // namespace parthenon
