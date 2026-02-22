#ifndef PARTHENON_ENTERPRISE_PERMISSIONED_H
#define PARTHENON_ENTERPRISE_PERMISSIONED_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <set>

namespace parthenon {
namespace enterprise {

/**
 * Permissioned Mode
 * Private blockchain configuration for enterprise use
 */
class PermissionedMode {
public:
    enum class PermissionLevel {
        ADMIN,          // Full control
        VALIDATOR,      // Can validate blocks
        PARTICIPANT,    // Can submit transactions
        OBSERVER        // Read-only access
    };
    
    struct Participant {
        std::vector<uint8_t> address;
        PermissionLevel level;
        std::string organization;
        bool is_active;
    };
    
    PermissionedMode();
    ~PermissionedMode();
    
    /**
     * Add participant to network
     */
    bool AddParticipant(
        const std::vector<uint8_t>& address,
        PermissionLevel level,
        const std::string& organization
    );
    
    /**
     * Remove participant
     */
    bool RemoveParticipant(const std::vector<uint8_t>& address);
    
    /**
     * Check if address has permission
     */
    bool HasPermission(
        const std::vector<uint8_t>& address,
        PermissionLevel required_level
    ) const;
    
    /**
     * Get participant info
     */
    Participant GetParticipant(const std::vector<uint8_t>& address) const;
    
    /**
     * Get all participants
     */
    std::vector<Participant> GetParticipants() const;
    
    /**
     * Enable/disable permissioned mode
     */
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
private:
    bool enabled_;
    std::map<std::vector<uint8_t>, Participant> participants_;
};

/**
 * Consortium Support
 * Multi-organization governance
 */
class ConsortiumManager {
public:
    struct Organization {
        std::string name;
        std::vector<uint8_t> admin_address;
        std::vector<std::vector<uint8_t>> members;
        uint32_t voting_weight;
        bool is_active;
    };
    
    struct ConsortiumDecision {
        uint64_t decision_id;
        std::string description;
        std::map<std::string, bool> votes;  // org name -> vote
        bool approved;
        uint64_t timestamp;
    };
    
    ConsortiumManager();
    ~ConsortiumManager();
    
    /**
     * Register organization
     */
    bool RegisterOrganization(
        const std::string& name,
        const std::vector<uint8_t>& admin_address,
        uint32_t voting_weight
    );
    
    /**
     * Add member to organization
     */
    bool AddMember(
        const std::string& org_name,
        const std::vector<uint8_t>& member_address
    );
    
    /**
     * Propose consortium decision
     */
    uint64_t ProposeDecision(const std::string& description);
    
    /**
     * Vote on decision
     */
    bool VoteOnDecision(
        uint64_t decision_id,
        const std::string& org_name,
        bool approve
    );
    
    /**
     * Check if decision is approved
     */
    bool IsDecisionApproved(uint64_t decision_id) const;
    
    /**
     * Get organization
     */
    Organization GetOrganization(const std::string& name) const;
    
private:
    std::map<std::string, Organization> organizations_;
    std::map<uint64_t, ConsortiumDecision> decisions_;
    uint64_t next_decision_id_;
};

/**
 * KYC/AML Compliance
 * Know Your Customer and Anti-Money Laundering tools
 */
class ComplianceManager {
public:
    enum class RiskLevel {
        LOW,
        MEDIUM,
        HIGH,
        BLOCKED
    };
    
    struct KYCRecord {
        std::vector<uint8_t> address;
        std::string full_name;
        std::string jurisdiction;
        RiskLevel risk_level;
        uint64_t verification_date;
        bool is_verified;
        std::vector<std::string> documents;
    };
    
    struct TransactionAlert {
        std::vector<uint8_t> tx_hash;
        std::string alert_type;
        RiskLevel severity;
        std::string description;
        uint64_t timestamp;
        bool resolved;
    };
    
    ComplianceManager();
    ~ComplianceManager();
    
    /**
     * Register KYC information
     */
    bool RegisterKYC(const KYCRecord& record);
    
    /**
     * Verify address KYC status
     */
    bool IsKYCVerified(const std::vector<uint8_t>& address) const;
    
    /**
     * Get KYC record
     */
    KYCRecord GetKYCRecord(const std::vector<uint8_t>& address) const;
    
    /**
     * Screen transaction for AML
     */
    std::vector<TransactionAlert> ScreenTransaction(
        const std::vector<uint8_t>& from,
        const std::vector<uint8_t>& to,
        uint64_t amount
    );
    
    /**
     * Report suspicious activity
     */
    void ReportSuspiciousActivity(const TransactionAlert& alert);
    
    /**
     * Get all alerts
     */
    std::vector<TransactionAlert> GetAlerts(bool unresolved_only = true) const;
    
private:
    std::map<std::vector<uint8_t>, KYCRecord> kyc_records_;
    std::vector<TransactionAlert> alerts_;
    
    bool CheckLargeTransaction(uint64_t amount) const;
    bool CheckHighRiskJurisdiction(const std::string& jurisdiction) const;
};

/**
 * Audit Logging
 * Comprehensive audit trail
 */
class AuditLogger {
public:
    enum class AuditEventType {
        TRANSACTION,
        BLOCK_CREATED,
        PERMISSION_CHANGE,
        CONFIGURATION_CHANGE,
        ACCESS_ATTEMPT,
        SECURITY_EVENT
    };
    
    struct AuditEvent {
        uint64_t event_id;
        AuditEventType type;
        uint64_t timestamp;
        std::vector<uint8_t> actor;
        std::string action;
        std::map<std::string, std::string> details;
        bool success;
    };
    
    /**
     * Log audit event
     */
    static void LogEvent(const AuditEvent& event);
    
    /**
     * Query audit log
     */
    static std::vector<AuditEvent> QueryLog(
        AuditEventType type,
        uint64_t start_time,
        uint64_t end_time
    );
    
    /**
     * Export audit log
     */
    static bool ExportLog(
        const std::string& filename,
        uint64_t start_time,
        uint64_t end_time
    );
    
    /**
     * Get events by actor
     */
    static std::vector<AuditEvent> GetEventsByActor(
        const std::vector<uint8_t>& actor
    );
    
private:
    static std::vector<AuditEvent> events_;
    static uint64_t next_event_id_;
};

/**
 * SLA Monitoring
 * Service Level Agreement tracking
 */
class SLAMonitor {
public:
    struct SLAMetrics {
        double uptime_percentage;
        uint64_t avg_block_time_ms;
        uint64_t avg_tx_confirmation_time_ms;
        uint64_t failed_transactions;
        uint64_t total_transactions;
        double success_rate;
    };
    
    struct SLAThresholds {
        double min_uptime;
        uint64_t max_block_time_ms;
        uint64_t max_confirmation_time_ms;
        double min_success_rate;
    };
    
    SLAMonitor();
    ~SLAMonitor();
    
    /**
     * Record metrics
     */
    void RecordBlockTime(uint64_t time_ms);
    void RecordTransactionConfirmation(uint64_t time_ms);
    void RecordTransactionResult(bool success);
    void RecordDowntime(uint64_t duration_ms);
    
    /**
     * Get current metrics
     */
    SLAMetrics GetMetrics() const;
    
    /**
     * Set SLA thresholds
     */
    void SetThresholds(const SLAThresholds& thresholds);
    
    /**
     * Check SLA compliance
     */
    bool IsSLACompliant() const;
    
    /**
     * Get SLA violations
     */
    std::vector<std::string> GetViolations() const;
    
private:
    SLAMetrics current_metrics_;
    SLAThresholds thresholds_;
    uint64_t total_uptime_ms_;
    uint64_t total_downtime_ms_;
    uint64_t block_time_count_ = 0;
    uint64_t tx_confirmation_count_ = 0;
};

} // namespace enterprise
} // namespace parthenon

#endif // PARTHENON_ENTERPRISE_PERMISSIONED_H
