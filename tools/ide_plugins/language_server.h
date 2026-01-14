// ParthenonChain IDE Plugin - Language Server Protocol Implementation
// Provides IDE features for smart contract development

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

namespace parthenon {
namespace ide {

/**
 * Source code location
 */
struct Location {
    std::string file_path;
    uint32_t line;
    uint32_t column;
    
    Location() : line(0), column(0) {}
};

/**
 * Diagnostic severity
 */
enum class DiagnosticSeverity {
    ERROR,
    WARNING,
    INFO,
    HINT
};

/**
 * Code diagnostic (error/warning)
 */
struct Diagnostic {
    Location location;
    std::string message;
    DiagnosticSeverity severity;
    std::string code;
    
    Diagnostic() : severity(DiagnosticSeverity::ERROR) {}
};

/**
 * Code completion item
 */
struct CompletionItem {
    std::string label;
    std::string detail;
    std::string documentation;
    std::string insert_text;
    uint32_t kind;  // Variable, Function, Class, etc.
    
    CompletionItem() : kind(0) {}
};

/**
 * Function signature
 */
struct SignatureInfo {
    std::string label;
    std::string documentation;
    std::vector<std::string> parameters;
};

/**
 * Symbol information
 */
struct Symbol {
    std::string name;
    std::string type;
    Location location;
    std::string documentation;
};

/**
 * Hover information
 */
struct HoverInfo {
    std::string content;
    Location location;
};

/**
 * Gas estimate for function
 */
struct GasEstimate {
    uint64_t estimated_gas;
    uint64_t max_gas;
    std::string complexity;  // "low", "medium", "high"
    
    GasEstimate() : estimated_gas(0), max_gas(0) {}
};

/**
 * Smart Contract Analyzer
 * Analyzes Solidity code for errors and optimizations
 */
class ContractAnalyzer {
public:
    /**
     * Analyze contract source code
     */
    std::vector<Diagnostic> Analyze(const std::string& source_code);
    
    /**
     * Check for security vulnerabilities
     */
    std::vector<Diagnostic> CheckSecurity(const std::string& source_code);
    
    /**
     * Suggest optimizations
     */
    std::vector<Diagnostic> SuggestOptimizations(const std::string& source_code);
    
    /**
     * Estimate gas costs
     */
    std::map<std::string, GasEstimate> EstimateGas(const std::string& source_code);
    
private:
    bool CheckReentrancy(const std::string& source_code);
    bool CheckOverflow(const std::string& source_code);
    bool CheckAccessControl(const std::string& source_code);
};

/**
 * Code Completion Provider
 * Provides intelligent code completion
 */
class CompletionProvider {
public:
    /**
     * Get completion items at cursor position
     */
    std::vector<CompletionItem> GetCompletions(
        const std::string& source_code,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Get signature help for function call
     */
    std::optional<SignatureInfo> GetSignatureHelp(
        const std::string& source_code,
        uint32_t line,
        uint32_t column
    );
    
private:
    std::vector<CompletionItem> GetKeywordCompletions();
    std::vector<CompletionItem> GetTypeCompletions();
    std::vector<CompletionItem> GetFunctionCompletions();
};

/**
 * Definition Provider
 * Provides go-to-definition functionality
 */
class DefinitionProvider {
public:
    /**
     * Find definition of symbol at cursor
     */
    std::optional<Location> FindDefinition(
        const std::string& source_code,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Find all references to symbol
     */
    std::vector<Location> FindReferences(
        const std::string& source_code,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Find all symbols in document
     */
    std::vector<Symbol> FindSymbols(const std::string& source_code);
};

/**
 * Hover Provider
 * Provides hover documentation
 */
class HoverProvider {
public:
    /**
     * Get hover information at cursor
     */
    std::optional<HoverInfo> GetHoverInfo(
        const std::string& source_code,
        uint32_t line,
        uint32_t column
    );
    
private:
    std::string GetTypeDocumentation(const std::string& type);
    std::string GetFunctionDocumentation(const std::string& function);
};

/**
 * Formatting Provider
 * Provides code formatting
 */
class FormattingProvider {
public:
    /**
     * Format entire document
     */
    std::string FormatDocument(const std::string& source_code);
    
    /**
     * Format range in document
     */
    std::string FormatRange(
        const std::string& source_code,
        uint32_t start_line,
        uint32_t end_line
    );
    
    /**
     * Configure formatting options
     */
    struct FormatOptions {
        uint32_t tab_size;
        bool use_spaces;
        bool insert_final_newline;
        
        FormatOptions() : tab_size(4), use_spaces(true), insert_final_newline(true) {}
    };
    
    void SetOptions(const FormatOptions& options);
    
private:
    FormatOptions options_;
};

/**
 * Refactoring Provider
 * Provides refactoring operations
 */
class RefactoringProvider {
public:
    /**
     * Rename symbol
     */
    struct Edit {
        Location location;
        std::string old_text;
        std::string new_text;
    };
    
    std::vector<Edit> RenameSymbol(
        const std::string& source_code,
        uint32_t line,
        uint32_t column,
        const std::string& new_name
    );
    
    /**
     * Extract function
     */
    std::optional<std::string> ExtractFunction(
        const std::string& source_code,
        uint32_t start_line,
        uint32_t end_line,
        const std::string& function_name
    );
    
    /**
     * Extract variable
     */
    std::optional<std::string> ExtractVariable(
        const std::string& source_code,
        uint32_t line,
        uint32_t column,
        const std::string& variable_name
    );
};

/**
 * Language Server
 * Main entry point for IDE integration
 */
class LanguageServer {
public:
    LanguageServer();
    ~LanguageServer();
    
    /**
     * Initialize language server
     */
    bool Initialize(const std::string& workspace_root);
    
    /**
     * Open document
     */
    void OpenDocument(const std::string& file_path, const std::string& content);
    
    /**
     * Update document
     */
    void UpdateDocument(const std::string& file_path, const std::string& content);
    
    /**
     * Close document
     */
    void CloseDocument(const std::string& file_path);
    
    /**
     * Get diagnostics for document
     */
    std::vector<Diagnostic> GetDiagnostics(const std::string& file_path);
    
    /**
     * Get completions
     */
    std::vector<CompletionItem> GetCompletions(
        const std::string& file_path,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Get hover info
     */
    std::optional<HoverInfo> GetHover(
        const std::string& file_path,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Go to definition
     */
    std::optional<Location> GotoDefinition(
        const std::string& file_path,
        uint32_t line,
        uint32_t column
    );
    
    /**
     * Format document
     */
    std::string FormatDocument(const std::string& file_path);
    
private:
    std::string workspace_root_;
    std::map<std::string, std::string> open_documents_;
    
    ContractAnalyzer analyzer_;
    CompletionProvider completion_provider_;
    DefinitionProvider definition_provider_;
    HoverProvider hover_provider_;
    FormattingProvider formatting_provider_;
    RefactoringProvider refactoring_provider_;
};

} // namespace ide
} // namespace parthenon
