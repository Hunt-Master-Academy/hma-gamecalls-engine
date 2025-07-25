/**
 * @file audit-logger.h
 * @brief Security Audit Logger Header - Phase 3.3 Security Framework
 *
 * This header defines the AuditLogger class and related structures
 * for comprehensive security audit logging and monitoring.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

#ifndef HUNTMASTER_SECURITY_AUDIT_LOGGER_H
#define HUNTMASTER_SECURITY_AUDIT_LOGGER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Audit Configuration
 */
struct AuditConfig {
    bool enableEncryption = true;
    bool enableCompression = true;
    bool enableRemoteLogging = false;
    bool enableRealTimeAlerts = true;
    std::string logDirectory = "./logs/audit";
    size_t maxLogFileSize = 104857600;  // 100MB
    uint32_t maxLogFiles = 10;
    uint32_t retentionDays = 365;
    std::string encryptionKey;
};

/**
 * Event Types and Severities
 */
enum class EventType {
    Authentication,
    Authorization,
    DataAccess,
    SystemChange,
    SecurityViolation,
    ComplianceEvent,
    AdminAction,
    UserActivity
};

enum class EventSeverity { Info, Warning, Error, Critical, Fatal };

enum class AccessResult { Success, Failure, Denied, Timeout };

/**
 * Data Structures
 */
struct SecurityEvent {
    uint64_t eventId;
    EventType type;
    EventSeverity severity;
    uint64_t timestamp;
    std::string source;
    std::string description;
    std::string userId;
    std::string sessionId;
    std::vector<std::pair<std::string, std::string>> metadata;
};

struct AccessAttempt {
    std::string userId;
    std::string resource;
    std::string action;
    AccessResult result;
    uint64_t timestamp;
    std::string sourceIP;
    std::string userAgent;
    std::string sessionId;
};

struct DataAccessEvent {
    std::string userId;
    std::string dataType;
    std::string action;
    std::string resource;
    size_t dataSize;
    uint64_t timestamp;
    std::string classification;
    bool isAuthorized;
};

struct SystemChangeEvent {
    std::string userId;
    std::string component;
    std::string changeType;
    std::string oldValue;
    std::string newValue;
    uint64_t timestamp;
    std::string approvalId;
    bool isAuthorized;
};

struct ComplianceEvent {
    std::string regulation;
    std::string requirement;
    std::string action;
    std::string result;
    uint64_t timestamp;
    std::string evidence;
    bool isCompliant;
    std::string assessor;
};

struct AuditReport {
    uint64_t reportId;
    uint64_t generatedAt;
    std::string reportType;
    uint64_t totalEvents;
    uint64_t criticalEvents;
    uint64_t securityViolations;
    std::vector<SecurityEvent> keyFindings;
    std::string summary;
};

struct QueryCriteria {
    uint64_t startTime;
    uint64_t endTime;
    std::vector<EventType> eventTypes;
    std::vector<EventSeverity> severities;
    std::string userId;
    std::string source;
    size_t maxResults;
};

struct ReportCriteria {
    uint64_t startTime;
    uint64_t endTime;
    std::string reportType;
    std::vector<EventType> includeTypes;
    std::vector<EventSeverity> includeSeverities;
    bool includeCompliance;
    bool includeMetrics;
};

struct AlertConfig {
    bool enableRealTimeAlerts;
    std::vector<EventType> alertTypes;
    std::vector<EventSeverity> alertSeverities;
    std::string alertEndpoint;
    uint32_t alertThreshold;
    uint32_t alertWindow;
};

struct ExportCriteria {
    uint64_t startTime;
    uint64_t endTime;
    std::vector<EventType> eventTypes;
    std::string destination;
    bool includeMetadata;
    bool encryptExport;
};

/**
 * AuditLogger Class
 * Provides comprehensive security audit logging and monitoring
 */
class AuditLogger {
  public:
    explicit AuditLogger(const AuditConfig& config = AuditConfig{});
    ~AuditLogger();

    // Disable copy construction and assignment
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger& operator=(const AuditLogger&) = delete;

    // Event Logging
    void logSecurityEvent(const SecurityEvent& event);
    void logAccessAttempt(const AccessAttempt& attempt);
    void logDataAccess(const DataAccessEvent& access);
    void logSystemChange(const SystemChangeEvent& change);
    void logComplianceEvent(const ComplianceEvent& event);

    // Reporting and Analysis
    AuditReport generateAuditReport(const ReportCriteria& criteria);
    std::vector<SecurityEvent> querySecurityEvents(const QueryCriteria& criteria);

    // Log Management
    void archiveOldLogs(uint32_t retentionDays);
    bool validateLogIntegrity();
    void rotateLogFiles();

    // Configuration
    void updateConfig(const AuditConfig& config) {
        config_ = config;
    }
    const AuditConfig& getConfig() const {
        return config_;
    }

    // Alerting
    void configureAlerts(const AlertConfig& alertConfig);
    void registerAlertHandler(std::function<void(const SecurityEvent&)> handler);

    // Import/Export
    bool exportAuditData(const ExportCriteria& criteria, const std::string& format);
    bool importAuditData(const std::string& source, const std::string& format);

    // Statistics
    uint64_t getTotalEvents() const;
    uint64_t getEventsByType(EventType type) const;
    uint64_t getEventsBySeverity(EventSeverity severity) const;

  private:
    AuditConfig config_;
    AlertConfig alertConfig_;

    // TODO: Add private audit logging members
    // TODO: Add log file management
    // TODO: Add encryption handling
    // TODO: Add alert management
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_AUDIT_LOGGER_H
