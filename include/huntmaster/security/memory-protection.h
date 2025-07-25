/**
 * @file memory-protection.h
 * @brief Memory Protection Security Header - Phase 3.3 Security Framework
 *
 * This header defines the MemoryProtection class and related structures
 * for comprehensive memory protection and secure memory management.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

#ifndef HUNTMASTER_SECURITY_MEMORY_PROTECTION_H
#define HUNTMASTER_SECURITY_MEMORY_PROTECTION_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Memory Configuration
 */
struct MemoryConfig {
    bool enableStackProtection = true;
    bool enableHeapProtection = true;
    bool enableGuardPages = true;
    bool enableCanaries = true;
    bool enableASLR = true;
    size_t maxAllocationSize = 67108864;  // 64MB
    size_t guardPageSize = 4096;
    uint32_t canaryPattern = 0xDEADBEEF;
    bool enableSecureClear = true;
};

/**
 * Access Types
 */
enum class AccessType { Read, Write, Execute, ReadWrite, ReadExecute };

/**
 * Memory Protection Types
 */
enum class ProtectionType { None, Read, Write, Execute, Guard };

/**
 * Memory Violation Types
 */
enum class ViolationType {
    BufferOverflow,
    UseAfterFree,
    DoubleFree,
    StackOverflow,
    HeapCorruption,
    InvalidAccess,
    GuardPageViolation
};

/**
 * Data Structures
 */
struct AllocationInfo {
    void* address;
    size_t size;
    uint64_t timestamp;
    std::string source;
    bool isSecure;
};

struct MemoryReport {
    uint64_t totalAllocations;
    uint64_t activeAllocations;
    size_t totalMemoryUsed;
    size_t peakMemoryUsage;
    uint64_t violationCount;
    double fragmentationRatio;
    uint64_t guardPageViolations;
};

struct ProtectionPolicies {
    bool enforceStackCanaries;
    bool enforceHeapGuards;
    bool enforceExecutionPrevention;
    bool enforceAddressRandomization;
    size_t minimumAllocationSize;
    size_t maximumAllocationSize;
    uint32_t auditFrequency;
};

struct MemoryViolation {
    ViolationType type;
    void* address;
    size_t size;
    uint64_t timestamp;
    std::string description;
    uint32_t severity;
};

struct SecureBuffer {
    void* data;
    size_t size;
    size_t capacity;
    uint32_t canary;
    bool isLocked;
};

/**
 * MemoryProtection Class
 * Provides comprehensive memory protection and secure management
 */
class MemoryProtection {
  public:
    explicit MemoryProtection(const MemoryConfig& config = MemoryConfig{});
    ~MemoryProtection();

    // Disable copy construction and assignment
    MemoryProtection(const MemoryProtection&) = delete;
    MemoryProtection& operator=(const MemoryProtection&) = delete;

    // Secure Memory Allocation
    bool allocateSecureBuffer(size_t size, void** buffer);
    bool deallocateSecureBuffer(void* buffer);
    bool reallocateSecureBuffer(void** buffer, size_t oldSize, size_t newSize);

    // Memory Access Validation
    bool validateMemoryAccess(const void* ptr, size_t size, AccessType type);
    bool checkBufferBounds(const void* buffer, size_t offset, size_t size);

    // Protection Mechanisms
    bool enableStackProtection();
    bool enableHeapProtection();
    bool enableExecutionPrevention();
    bool enableAddressRandomization();

    // Memory Clearing and Sanitization
    void clearSensitiveData(void* data, size_t size);
    bool sanitizeMemoryRegion(void* start, size_t size);

    // Monitoring and Auditing
    MemoryReport generateMemoryReport() const;
    bool performMemoryAudit();
    std::vector<MemoryViolation> getRecentViolations() const;

    // Policy Management
    void updateProtectionPolicies(const ProtectionPolicies& policies);
    const ProtectionPolicies& getProtectionPolicies() const {
        return policies_;
    }

    // Violation Handling
    bool handleMemoryViolation(const MemoryViolation& violation);
    void registerViolationHandler(std::function<void(const MemoryViolation&)> handler);

    // Buffer Management
    std::unique_ptr<SecureBuffer> createSecureBuffer(size_t size);
    bool lockBuffer(SecureBuffer* buffer);
    bool unlockBuffer(SecureBuffer* buffer);

    // Configuration
    void updateConfig(const MemoryConfig& config) {
        config_ = config;
    }
    const MemoryConfig& getConfig() const {
        return config_;
    }

  private:
    MemoryConfig config_;
    ProtectionPolicies policies_;

    // TODO: Add private memory management members
    // TODO: Add allocation tracking structures
    // TODO: Add violation tracking
    // TODO: Add memory statistics
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_MEMORY_PROTECTION_H
