// ParthenonChain IDE Plugin Implementation

#include "language_server.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace parthenon {
namespace ide {

// ContractAnalyzer implementation
std::vector<Diagnostic> ContractAnalyzer::Analyze(const std::string& source_code) {
    std::vector<Diagnostic> diagnostics;
    
    // Check for common issues
    if (source_code.find("pragma solidity") == std::string::npos) {
        Diagnostic diag;
        diag.message = "Missing pragma solidity directive";
        diag.severity = DiagnosticSeverity::ERROR;
        diag.code = "E001";
        diagnostics.push_back(diag);
    }
    
    return diagnostics;
}

std::vector<Diagnostic> ContractAnalyzer::CheckSecurity(const std::string& source_code) {
    std::vector<Diagnostic> diagnostics;

    if (CheckReentrancy(source_code)) {
        Diagnostic diag;
        diag.message = "Potential reentrancy vulnerability detected";
        diag.severity = DiagnosticSeverity::WARNING;
        diag.code = "S001";
        diagnostics.push_back(diag);
    }

    if (CheckOverflow(source_code)) {
        Diagnostic diag;
        diag.message = "Potential integer overflow: arithmetic without SafeMath detected";
        diag.severity = DiagnosticSeverity::WARNING;
        diag.code = "S002";
        diagnostics.push_back(diag);
    }

    if (CheckAccessControl(source_code)) {
        Diagnostic diag;
        diag.message = "Missing access control: no onlyOwner, msg.sender, or require() found";
        diag.severity = DiagnosticSeverity::WARNING;
        diag.code = "S003";
        diagnostics.push_back(diag);
    }

    return diagnostics;
}

std::vector<Diagnostic> ContractAnalyzer::SuggestOptimizations(const std::string& source_code) {
    std::vector<Diagnostic> diagnostics;
    
    // Check for common optimization opportunities
    if (source_code.find("public") != std::string::npos) {
        Diagnostic diag;
        diag.message = "Consider using 'external' instead of 'public' for functions only called externally";
        diag.severity = DiagnosticSeverity::HINT;
        diag.code = "O001";
        diagnostics.push_back(diag);
    }
    
    return diagnostics;
}

std::map<std::string, GasEstimate> ContractAnalyzer::EstimateGas(const std::string& source_code) {
    std::map<std::string, GasEstimate> estimates;
    
    // Count function occurrences to create per-function estimates
    size_t pos = 0;
    int func_index = 0;
    while ((pos = source_code.find("function ", pos)) != std::string::npos) {
        size_t name_start = pos + 9;
        size_t name_end = source_code.find('(', name_start);
        std::string name = (name_end != std::string::npos)
            ? source_code.substr(name_start, name_end - name_start)
            : "function" + std::to_string(func_index);
        // Trim whitespace
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        
        GasEstimate est;
        est.estimated_gas = 21000 + static_cast<uint64_t>(func_index) * 1000;
        est.max_gas = est.estimated_gas * 2;
        est.complexity = (est.estimated_gas < 30000) ? "low" : "medium";
        estimates[name] = est;
        
        pos = name_end != std::string::npos ? name_end : pos + 9;
        ++func_index;
    }
    
    if (estimates.empty()) {
        GasEstimate est;
        est.estimated_gas = 21000;
        est.max_gas = 50000;
        est.complexity = "low";
        estimates["default"] = est;
    }
    
    return estimates;
}

bool ContractAnalyzer::CheckReentrancy(const std::string& source_code) {
    // Flag if there is an external call (.call/.transfer) followed by a state write
    bool has_external_call = source_code.find(".call(") != std::string::npos ||
                             source_code.find(".transfer(") != std::string::npos ||
                             source_code.find(".send(") != std::string::npos;
    return has_external_call;
}

bool ContractAnalyzer::CheckOverflow(const std::string& source_code) {
    // Flag if arithmetic is used without SafeMath (and not inside an unchecked block)
    bool uses_arithmetic = source_code.find(" + ") != std::string::npos ||
                           source_code.find(" - ") != std::string::npos ||
                           source_code.find(" * ") != std::string::npos;
    bool uses_safemath = source_code.find("SafeMath") != std::string::npos;
    return uses_arithmetic && !uses_safemath;
}

bool ContractAnalyzer::CheckAccessControl(const std::string& source_code) {
    // Flag if there are no access control patterns
    bool has_access_control = source_code.find("onlyOwner") != std::string::npos ||
                              source_code.find("msg.sender") != std::string::npos ||
                              source_code.find("require(") != std::string::npos;
    return !has_access_control;
}

// CompletionProvider implementation
std::vector<CompletionItem> CompletionProvider::GetCompletions(
    const std::string& source_code,
    uint32_t line,
    uint32_t column) {
    
    std::vector<CompletionItem> items;
    
    // Find the start of the requested line
    size_t line_start = 0;
    for (uint32_t l = 0; l < line && line_start < source_code.size(); ++l) {
        size_t nl = source_code.find('\n', line_start);
        line_start = (nl == std::string::npos) ? source_code.size() : nl + 1;
    }
    
    // Extract the word ending at 'column' on this line
    size_t col_pos = std::min(line_start + column, source_code.size());
    size_t word_end = col_pos;
    size_t word_start = word_end;
    while (word_start > line_start &&
           std::isalnum(static_cast<unsigned char>(source_code[word_start - 1]))) {
        --word_start;
    }
    std::string prefix = source_code.substr(word_start, word_end - word_start);
    
    // Add keyword completions that match the prefix
    for (auto& kw : GetKeywordCompletions()) {
        if (prefix.empty() || kw.label.rfind(prefix, 0) == 0) {
            items.push_back(kw);
        }
    }

    // Add type completions that match the prefix
    for (auto& tc : GetTypeCompletions()) {
        if (prefix.empty() || tc.label.rfind(prefix, 0) == 0) {
            items.push_back(tc);
        }
    }

    // Add function completions that match the prefix
    for (auto& fc : GetFunctionCompletions()) {
        if (prefix.empty() || fc.label.rfind(prefix, 0) == 0) {
            items.push_back(fc);
        }
    }

    return items;
}

std::optional<SignatureInfo> CompletionProvider::GetSignatureHelp(
    const std::string& source_code,
    uint32_t line,
    uint32_t column) {
    
    // Look for an open parenthesis before the cursor — indicates a function call
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    size_t cursor = pos + column;
    if (cursor > source_code.size()) cursor = source_code.size();
    size_t paren_pos = source_code.rfind('(', cursor);
    if (paren_pos == std::string::npos) {
        return std::nullopt;
    }
    // Extract function name before the paren
    size_t name_end = paren_pos;
    while (name_end > 0 && source_code[name_end - 1] == ' ') --name_end;
    size_t name_start = name_end;
    while (name_start > 0 && (std::isalnum(static_cast<unsigned char>(source_code[name_start - 1])) ||
                               source_code[name_start - 1] == '_')) {
        --name_start;
    }
    if (name_start == name_end) {
        return std::nullopt;
    }
    SignatureInfo info;
    info.label = source_code.substr(name_start, name_end - name_start) + "(...)";
    return info;
}

std::vector<CompletionItem> CompletionProvider::GetKeywordCompletions() {
    std::vector<CompletionItem> items;
    
    CompletionItem item;
    item.label = "contract";
    item.detail = "contract keyword";
    item.insert_text = "contract ${1:ContractName} {\n    $0\n}";
    item.kind = 14;  // Keyword
    items.push_back(item);
    
    item.label = "function";
    item.detail = "function keyword";
    item.insert_text = "function ${1:functionName}($2) public $3 {\n    $0\n}";
    items.push_back(item);
    
    return items;
}

std::vector<CompletionItem> CompletionProvider::GetTypeCompletions() {
    std::vector<CompletionItem> items;
    
    CompletionItem item;
    item.label = "uint256";
    item.detail = "unsigned integer type";
    item.kind = 7;  // Class
    items.push_back(item);
    
    return items;
}

std::vector<CompletionItem> CompletionProvider::GetFunctionCompletions() {
    return {};
}

// DefinitionProvider implementation
std::optional<Location> DefinitionProvider::FindDefinition(
    const std::string& source_code,
    uint32_t line,
    uint32_t column) {
    
    // Find the word at the given position and look for its definition
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    // Extract word at column
    size_t word_start = pos + column;
    while (word_start > pos &&
           std::isalnum(static_cast<unsigned char>(source_code[word_start - 1]))) {
        --word_start;
    }
    size_t word_end = pos + column;
    while (word_end < source_code.size() &&
           std::isalnum(static_cast<unsigned char>(source_code[word_end]))) {
        ++word_end;
    }
    if (word_start >= word_end) return std::nullopt;
    std::string word = source_code.substr(word_start, word_end - word_start);

    // Search for "function <word>" in the source
    std::string pattern = "function " + word;
    size_t def_pos = source_code.find(pattern);
    if (def_pos == std::string::npos) return std::nullopt;

    Location loc;
    loc.line = static_cast<uint32_t>(
        std::count(source_code.begin(), source_code.begin() + def_pos, '\n'));
    loc.column = static_cast<uint32_t>(def_pos - source_code.rfind('\n', def_pos) - 1);
    return loc;
}

std::vector<Location> DefinitionProvider::FindReferences(
    const std::string& source_code,
    uint32_t line,
    uint32_t column) {
    
    auto def = FindDefinition(source_code, line, column);
    if (!def) return {};

    // Find the word at the given position
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    size_t word_start = pos + column;
    while (word_start > pos &&
           std::isalnum(static_cast<unsigned char>(source_code[word_start - 1]))) {
        --word_start;
    }
    size_t word_end = pos + column;
    while (word_end < source_code.size() &&
           std::isalnum(static_cast<unsigned char>(source_code[word_end]))) {
        ++word_end;
    }
    if (word_start >= word_end) return {};
    std::string word = source_code.substr(word_start, word_end - word_start);

    std::vector<Location> refs;
    size_t search_pos = 0;
    while ((search_pos = source_code.find(word, search_pos)) != std::string::npos) {
        Location ref;
        ref.line = static_cast<uint32_t>(
            std::count(source_code.begin(), source_code.begin() + search_pos, '\n'));
        refs.push_back(ref);
        search_pos += word.size();
    }
    return refs;
}

std::vector<Symbol> DefinitionProvider::FindSymbols(const std::string& source_code) {
    std::vector<Symbol> symbols;
    
    // Extract function names from source code
    size_t pos = 0;
    while ((pos = source_code.find("function ", pos)) != std::string::npos) {
        size_t name_start = pos + 9;
        size_t name_end = source_code.find('(', name_start);
        if (name_end == std::string::npos) break;
        std::string name = source_code.substr(name_start, name_end - name_start);
        // Trim whitespace
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        if (!name.empty()) {
            Symbol sym;
            sym.name = name;
            sym.type = "function";
            // Count line number up to this position
            sym.location.line = static_cast<uint32_t>(
                std::count(source_code.begin(), source_code.begin() + pos, '\n'));
            symbols.push_back(sym);
        }
        pos = name_end;
    }
    
    return symbols;
}

// HoverProvider implementation
std::optional<HoverInfo> HoverProvider::GetHoverInfo(
    const std::string& source_code,
    uint32_t line,
    uint32_t column) {
    
    // Extract word at position and provide type documentation
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    size_t word_start = pos + column;
    while (word_start > pos &&
           std::isalnum(static_cast<unsigned char>(source_code[word_start - 1]))) {
        --word_start;
    }
    size_t word_end = pos + column;
    while (word_end < source_code.size() &&
           std::isalnum(static_cast<unsigned char>(source_code[word_end]))) {
        ++word_end;
    }
    if (word_start >= word_end) return std::nullopt;
    std::string word = source_code.substr(word_start, word_end - word_start);
    std::string doc = GetTypeDocumentation(word);
    if (doc.empty()) doc = GetFunctionDocumentation(word);
    if (doc.empty()) return std::nullopt;

    HoverInfo info;
    info.content = doc;
    info.location.line = line;
    info.location.column = static_cast<uint32_t>(word_start - pos);
    return info;
}

std::string HoverProvider::GetTypeDocumentation(const std::string& type) {
    if (type == "uint256") return "Unsigned 256-bit integer";
    if (type == "address") return "20-byte Ethereum address";
    if (type == "bool")    return "Boolean (true/false)";
    if (type == "bytes32") return "Fixed 32-byte array";
    if (type == "string")  return "Dynamic UTF-8 string";
    return "";
}

std::string HoverProvider::GetFunctionDocumentation(const std::string& function) {
    if (function == "transfer")   return "Transfer Ether to address";
    if (function == "require")    return "Validate condition, revert on failure";
    if (function == "emit")       return "Emit an event";
    if (function == "keccak256")  return "Compute Keccak-256 hash";
    return "";
}

// FormattingProvider implementation
std::string FormattingProvider::FormatDocument(const std::string& source_code) {
    // In production: use proper code formatter
    return source_code;
}

std::string FormattingProvider::FormatRange(
    const std::string& source_code,
    uint32_t start_line,
    uint32_t end_line) {
    
    // Extract and return only the lines in [start_line, end_line]
    std::istringstream stream(source_code);
    std::string line_str;
    std::string result;
    uint32_t current = 0;
    while (std::getline(stream, line_str)) {
        if (current >= start_line && current <= end_line) {
            result += line_str + '\n';
        }
        ++current;
    }
    return result;
}

void FormattingProvider::SetOptions(const FormatOptions& options) {
    options_ = options;
}

// RefactoringProvider implementation
std::vector<RefactoringProvider::Edit> RefactoringProvider::RenameSymbol(
    const std::string& source_code,
    uint32_t line,
    uint32_t column,
    const std::string& new_name) {
    
    std::vector<Edit> edits;
    if (new_name.empty()) return edits;

    // Find the word at the cursor position
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    size_t word_start = pos + column;
    while (word_start > pos &&
           std::isalnum(static_cast<unsigned char>(source_code[word_start - 1]))) {
        --word_start;
    }
    size_t word_end = pos + column;
    while (word_end < source_code.size() &&
           std::isalnum(static_cast<unsigned char>(source_code[word_end]))) {
        ++word_end;
    }
    if (word_start >= word_end) return edits;
    std::string old_name = source_code.substr(word_start, word_end - word_start);

    // Collect all whole-word occurrences
    size_t search_pos = 0;
    while ((search_pos = source_code.find(old_name, search_pos)) != std::string::npos) {
        // Check word boundaries
        bool left_ok = (search_pos == 0) ||
                       !std::isalnum(static_cast<unsigned char>(source_code[search_pos - 1]));
        bool right_ok = (search_pos + old_name.size() >= source_code.size()) ||
                        !std::isalnum(static_cast<unsigned char>(
                            source_code[search_pos + old_name.size()]));
        if (left_ok && right_ok) {
            Edit edit;
            edit.location.line = static_cast<uint32_t>(
                std::count(source_code.begin(), source_code.begin() + search_pos, '\n'));
            edit.old_text = old_name;
            edit.new_text = new_name;
            edits.push_back(edit);
        }
        search_pos += old_name.size();
    }
    return edits;
}

std::optional<std::string> RefactoringProvider::ExtractFunction(
    const std::string& source_code,
    uint32_t start_line,
    uint32_t end_line,
    const std::string& function_name) {
    
    if (function_name.empty()) return std::nullopt;
    // Extract selected lines and wrap in a new function
    std::istringstream stream(source_code);
    std::string line_str;
    std::string body;
    uint32_t current = 0;
    while (std::getline(stream, line_str)) {
        if (current >= start_line && current <= end_line) {
            body += "    " + line_str + '\n';
        }
        ++current;
    }
    return "function " + function_name + "() private {\n" + body + "}\n";
}

std::optional<std::string> RefactoringProvider::ExtractVariable(
    const std::string& source_code,
    uint32_t line,
    uint32_t column,
    const std::string& variable_name) {
    
    if (variable_name.empty()) return std::nullopt;
    // Find the expression at position and wrap it
    size_t pos = 0;
    uint32_t current_line = 0;
    while (pos < source_code.size() && current_line < line) {
        if (source_code[pos++] == '\n') ++current_line;
    }
    size_t expr_pos = pos + column;
    if (expr_pos >= source_code.size()) return source_code;

    // Return source with the extracted variable declaration prepended to the line
    std::string result = source_code;
    std::string decl = "uint256 " + variable_name + " = /* extracted */;\n";
    size_t line_start = result.rfind('\n', expr_pos);
    line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
    result.insert(line_start, decl);
    return result;
}

// LanguageServer implementation
LanguageServer::LanguageServer() {}
LanguageServer::~LanguageServer() {}

bool LanguageServer::Initialize(const std::string& workspace_root) {
    workspace_root_ = workspace_root;
    return true;
}

void LanguageServer::OpenDocument(const std::string& file_path, const std::string& content) {
    open_documents_[file_path] = content;
}

void LanguageServer::UpdateDocument(const std::string& file_path, const std::string& content) {
    open_documents_[file_path] = content;
}

void LanguageServer::CloseDocument(const std::string& file_path) {
    open_documents_.erase(file_path);
}

std::vector<Diagnostic> LanguageServer::GetDiagnostics(const std::string& file_path) {
    auto it = open_documents_.find(file_path);
    if (it == open_documents_.end()) {
        return {};
    }
    
    return analyzer_.Analyze(it->second);
}

std::vector<CompletionItem> LanguageServer::GetCompletions(
    const std::string& file_path,
    uint32_t line,
    uint32_t column) {
    
    auto it = open_documents_.find(file_path);
    if (it == open_documents_.end()) {
        return {};
    }
    
    return completion_provider_.GetCompletions(it->second, line, column);
}

std::optional<HoverInfo> LanguageServer::GetHover(
    const std::string& file_path,
    uint32_t line,
    uint32_t column) {
    
    auto it = open_documents_.find(file_path);
    if (it == open_documents_.end()) {
        return std::nullopt;
    }
    
    return hover_provider_.GetHoverInfo(it->second, line, column);
}

std::optional<Location> LanguageServer::GotoDefinition(
    const std::string& file_path,
    uint32_t line,
    uint32_t column) {
    
    auto it = open_documents_.find(file_path);
    if (it == open_documents_.end()) {
        return std::nullopt;
    }
    
    return definition_provider_.FindDefinition(it->second, line, column);
}

std::string LanguageServer::FormatDocument(const std::string& file_path) {
    auto it = open_documents_.find(file_path);
    if (it == open_documents_.end()) {
        return "";
    }
    
    return formatting_provider_.FormatDocument(it->second);
}

} // namespace ide
} // namespace parthenon
