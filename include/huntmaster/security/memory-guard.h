/**
 * @file memory-guard.h
 * @brief Memory Guard Security Header - Phase 3.4 Security Framework
 *
 * This header defines the MemoryGuard class and related structures
 * for advanced memory protection and runtime security monitoring.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

#ifndef HUNTMASTER_SECURITY_MEMORY_GUARD_H
#define HUNTMASTER_SECURITY_MEMORY_GUARD_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Guard Types
 */
enum class GuardType { StackGuard, HeapGuard, BufferGuard, OverflowGuard, UnderflowGuard };

/**
 * Violation Types
 */
enum class ViolationType {
    BufferOverflow,
    BufferUnderflow,
    UseAfterFree,
    DoubleFree,
    StackOverflow,
    HeapCorruption,
    InvalidAccess
};

/**
 * Guard Configuration
 */
struct GuardConfig {
    bool enableStackGuards = true;
    bool enableHeapGuards = true;
    bool enableBufferGuards = true;
    bool enableRuntimeChecks = true;
    size_t guardZoneSize = 4096;
    uint32_t canaryValue = 0xDEADBEEF;
    bool enableViolationLogging = true;
    bool terminateOnViolation = true;
};

/**
 * Memory Violation Event
 */
struct MemoryViolation {
    ViolationType type;
    void* address;
    size_t size;
    uint64_t timestamp;
    std::string stackTrace;
    std::string description;
};

/**
 * Memory Statistics
 */
struct MemoryStats {
    size_t totalAllocations = 0;
    size_t totalDeallocations = 0;
    size_t currentAllocations = 0;
    size_t peakAllocations = 0;
    size_t totalBytesAllocated = 0;
    size_t totalBytesFreed = 0;
    size_t currentBytesAllocated = 0;
    size_t peakBytesAllocated = 0;
    size_t violationCount = 0;
    size_t leakCount = 0;
    double fragmentationRatio = 0.0;
    uint64_t lastUpdate = 0;
};

/**
 * Memory Guard Class
 *
 * Provides advanced memory protection including stack guards,
 * heap guards, buffer overflow detection, and runtime monitoring.
 */
class MemoryGuard {
  public:
    explicit MemoryGuard(const GuardConfig& config = GuardConfig{});
    ~MemoryGuard();

    // Guard Installation
    bool installStackGuard();
    bool installHeapGuard();
    bool installBufferGuard(void* buffer, size_t size);
    bool removeBufferGuard(void* buffer);

    // Memory Validation
    bool validateMemoryAccess(const void* ptr, size_t size);
    bool validateStackIntegrity();
    bool validateHeapIntegrity();
    bool checkHeapIntegrity();
    bool detectStackOverflow();
    bool validatePointer(void* ptr);
    bool checkMemoryLeak();
    bool validateBufferIntegrity(const void* buffer);

    // Canary Management
    uint32_t generateCanary();
    bool validateCanary(uint32_t canary, void* location);
    bool updateCanary(void* location);

    // Violation Handling
    void reportViolation(const MemoryViolation& violation);
    std::vector<MemoryViolation> getViolationHistory();
    void clearViolationHistory();

    // Runtime Monitoring
    bool startRuntimeMonitoring();
    bool stopRuntimeMonitoring();
    bool isMonitoringActive();

    // Statistics
    size_t getGuardedAllocationsCount();
    size_t getTotalViolationsCount();
    double getViolationRate();

    // Guard Audit
    bool performGuardAudit();

    // Secure Memory Operations
    void secureAlloc(size_t size, void** ptr);
    void secureFree(void* ptr);
    void enableMemoryProtection();
    void disableMemoryProtection();

    // Memory Analysis
    MemoryStats getMemoryStats();
    bool performMemoryAudit();

  private:
    struct MemoryGuardImpl;
    std::unique_ptr<MemoryGuardImpl> impl_;
};

/**
 * RAII Memory Guard for automatic buffer protection
 */
class ScopedMemoryGuard {
  public:
    ScopedMemoryGuard(MemoryGuard& guard, void* buffer, size_t size);
    ~ScopedMemoryGuard();

    // Disable copy construction and assignment
    ScopedMemoryGuard(const ScopedMemoryGuard&) = delete;
    ScopedMemoryGuard& operator=(const ScopedMemoryGuard&) = delete;

  private:
    MemoryGuard& guard_;
    void* buffer_;
    size_t size_;
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_MEMORY_GUARD_H
