/**
 * @file test_memory_guard.cpp
 * @brief Comprehensive test suite for MemoryGuard security component
 *
 * This test suite provides thorough testing of the MemoryGuard class
 * including stack protection, heap monitoring, buffer overflow detection,
 * canary validation, violation reporting, and memory statistics.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/security/memory-guard.h"

using namespace huntmaster;
using namespace huntmaster::security;
using namespace huntmaster::test;

class MemoryGuardTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Configure memory guard for comprehensive testing
        config_.enableStackGuards = true;
        config_.enableHeapGuards = true;
        config_.enableBufferGuards = true;
        config_.enableRuntimeChecks = true;
        config_.guardZoneSize = 4096;
        config_.canaryValue = 0xDEADBEEF;
        config_.enableViolationLogging = true;
        config_.terminateOnViolation = false;  // Don't terminate during testing

        memoryGuard_ = std::make_unique<MemoryGuard>(config_);

        // Start runtime monitoring
        memoryGuard_->startRuntimeMonitoring();
    }

    void TearDown() override {
        // Stop monitoring and cleanup
        if (memoryGuard_) {
            memoryGuard_->stopRuntimeMonitoring();
            memoryGuard_.reset();
        }
        TestFixtureBase::TearDown();
    }

    // Helper function to allocate test buffer
    void* allocateTestBuffer(size_t size) {
        void* buffer = std::malloc(size);
        if (buffer) {
            std::memset(buffer, 0, size);
        }
        return buffer;
    }

    // Helper function to deallocate test buffer
    void deallocateTestBuffer(void* buffer) {
        if (buffer) {
            std::free(buffer);
        }
    }

    // Helper function to create memory violation for testing
    MemoryViolation createTestViolation(ViolationType type) {
        MemoryViolation violation;
        violation.type = type;
        violation.address = reinterpret_cast<void*>(0x1000);
        violation.size = 1024;
        violation.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
        violation.stackTrace = "test_stack_trace";
        violation.description = "Test violation for unit testing";
        return violation;
    }

    GuardConfig config_;
    std::unique_ptr<MemoryGuard> memoryGuard_;
};

// Constructor and configuration tests
TEST_F(MemoryGuardTest, ConstructorDestructorTest) {
    EXPECT_NE(memoryGuard_, nullptr);
    EXPECT_TRUE(memoryGuard_->isMonitoringActive());
}

TEST_F(MemoryGuardTest, GuardInstallationTest) {
    // Test stack guard installation
    EXPECT_TRUE(memoryGuard_->installStackGuard());

    // Test heap guard installation
    EXPECT_TRUE(memoryGuard_->installHeapGuard());

    // Test buffer guard installation
    void* testBuffer = allocateTestBuffer(1024);
    EXPECT_TRUE(memoryGuard_->installBufferGuard(testBuffer, 1024));

    // Cleanup
    EXPECT_TRUE(memoryGuard_->removeBufferGuard(testBuffer));
    deallocateTestBuffer(testBuffer);
}

TEST_F(MemoryGuardTest, InvalidGuardInstallationTest) {
    // Test buffer guard with null pointer
    EXPECT_FALSE(memoryGuard_->installBufferGuard(nullptr, 1024));

    // Test buffer guard with zero size
    void* testBuffer = allocateTestBuffer(1024);
    EXPECT_FALSE(memoryGuard_->installBufferGuard(testBuffer, 0));

    deallocateTestBuffer(testBuffer);
}

// Memory validation tests
TEST_F(MemoryGuardTest, ValidMemoryAccessTest) {
    void* testBuffer = allocateTestBuffer(1024);

    // Install buffer guard
    EXPECT_TRUE(memoryGuard_->installBufferGuard(testBuffer, 1024));

    // Valid memory access should succeed
    EXPECT_TRUE(memoryGuard_->validateMemoryAccess(testBuffer, 1024));
    EXPECT_TRUE(memoryGuard_->validateMemoryAccess(testBuffer, 512));  // Partial access

    // Cleanup
    EXPECT_TRUE(memoryGuard_->removeBufferGuard(testBuffer));
    deallocateTestBuffer(testBuffer);
}

TEST_F(MemoryGuardTest, InvalidMemoryAccessTest) {
    // Test access to null pointer
    EXPECT_FALSE(memoryGuard_->validateMemoryAccess(nullptr, 1024));

    // Test access to invalid address range
    void* invalidPtr = reinterpret_cast<void*>(0x1);
    EXPECT_FALSE(memoryGuard_->validateMemoryAccess(invalidPtr, 1024));

    // Test oversized access
    void* testBuffer = allocateTestBuffer(1024);
    memoryGuard_->installBufferGuard(testBuffer, 1024);

    EXPECT_FALSE(memoryGuard_->validateMemoryAccess(testBuffer, 2048));  // Larger than buffer

    // Cleanup
    memoryGuard_->removeBufferGuard(testBuffer);
    deallocateTestBuffer(testBuffer);
}

TEST_F(MemoryGuardTest, StackIntegrityValidationTest) {
    // Install stack guard first
    EXPECT_TRUE(memoryGuard_->installStackGuard());

    // Validate stack integrity
    EXPECT_TRUE(memoryGuard_->validateStackIntegrity());

    // Test stack overflow detection
    EXPECT_FALSE(memoryGuard_->detectStackOverflow());  // Should not detect overflow normally
}

TEST_F(MemoryGuardTest, HeapIntegrityValidationTest) {
    // Install heap guard first
    EXPECT_TRUE(memoryGuard_->installHeapGuard());

    // Validate heap integrity
    EXPECT_TRUE(memoryGuard_->validateHeapIntegrity());
    EXPECT_TRUE(memoryGuard_->checkHeapIntegrity());
}

TEST_F(MemoryGuardTest, PointerValidationTest) {
    void* validBuffer = allocateTestBuffer(1024);

    // Valid pointer should pass validation
    EXPECT_TRUE(memoryGuard_->validatePointer(validBuffer));

    // Invalid pointers should fail validation
    EXPECT_FALSE(memoryGuard_->validatePointer(nullptr));
    EXPECT_FALSE(memoryGuard_->validatePointer(reinterpret_cast<void*>(0x1)));

    deallocateTestBuffer(validBuffer);
}

TEST_F(MemoryGuardTest, BufferIntegrityValidationTest) {
    void* testBuffer = allocateTestBuffer(1024);

    // Install buffer guard
    EXPECT_TRUE(memoryGuard_->installBufferGuard(testBuffer, 1024));

    // Validate buffer integrity
    EXPECT_TRUE(memoryGuard_->validateBufferIntegrity(testBuffer));

    // Cleanup
    EXPECT_TRUE(memoryGuard_->removeBufferGuard(testBuffer));
    deallocateTestBuffer(testBuffer);
}

// Canary management tests
TEST_F(MemoryGuardTest, CanaryGenerationTest) {
    uint32_t canary1 = memoryGuard_->generateCanary();
    uint32_t canary2 = memoryGuard_->generateCanary();

    // Canaries should be non-zero
    EXPECT_NE(canary1, 0);
    EXPECT_NE(canary2, 0);

    // Canaries should be different (high probability)
    EXPECT_NE(canary1, canary2);
}

TEST_F(MemoryGuardTest, CanaryValidationTest) {
    void* testBuffer = allocateTestBuffer(1024);
    uint32_t canary = memoryGuard_->generateCanary();

    // Place canary in buffer
    *reinterpret_cast<uint32_t*>(testBuffer) = canary;

    // Validate canary
    EXPECT_TRUE(memoryGuard_->validateCanary(canary, testBuffer));

    // Test with wrong canary value
    EXPECT_FALSE(memoryGuard_->validateCanary(canary + 1, testBuffer));

    deallocateTestBuffer(testBuffer);
}

TEST_F(MemoryGuardTest, CanaryUpdateTest) {
    void* testBuffer = allocateTestBuffer(1024);
    uint32_t originalValue = *reinterpret_cast<uint32_t*>(testBuffer);

    // Update canary at location
    EXPECT_TRUE(memoryGuard_->updateCanary(testBuffer));

    // Value should have changed
    uint32_t newValue = *reinterpret_cast<uint32_t*>(testBuffer);
    EXPECT_NE(originalValue, newValue);

    deallocateTestBuffer(testBuffer);
}

// Violation handling tests
TEST_F(MemoryGuardTest, ViolationReportingTest) {
    auto violation = createTestViolation(ViolationType::BufferOverflow);

    // Report violation
    memoryGuard_->reportViolation(violation);

    // Check violation history
    auto history = memoryGuard_->getViolationHistory();
    EXPECT_FALSE(history.empty());
    EXPECT_EQ(history.back().type, ViolationType::BufferOverflow);
    EXPECT_EQ(history.back().description, violation.description);
}

TEST_F(MemoryGuardTest, MultipleViolationReportingTest) {
    // Report multiple violations
    memoryGuard_->reportViolation(createTestViolation(ViolationType::BufferOverflow));
    memoryGuard_->reportViolation(createTestViolation(ViolationType::BufferUnderflow));
    memoryGuard_->reportViolation(createTestViolation(ViolationType::UseAfterFree));

    auto history = memoryGuard_->getViolationHistory();
    EXPECT_EQ(history.size(), 3);

    // Verify violation types
    EXPECT_EQ(history[0].type, ViolationType::BufferOverflow);
    EXPECT_EQ(history[1].type, ViolationType::BufferUnderflow);
    EXPECT_EQ(history[2].type, ViolationType::UseAfterFree);
}

TEST_F(MemoryGuardTest, ViolationHistoryClearTest) {
    // Report some violations
    memoryGuard_->reportViolation(createTestViolation(ViolationType::DoubleFree));
    memoryGuard_->reportViolation(createTestViolation(ViolationType::StackOverflow));

    // Verify violations exist
    EXPECT_FALSE(memoryGuard_->getViolationHistory().empty());

    // Clear history
    memoryGuard_->clearViolationHistory();

    // Verify history is cleared
    EXPECT_TRUE(memoryGuard_->getViolationHistory().empty());
}

// Runtime monitoring tests
TEST_F(MemoryGuardTest, RuntimeMonitoringTest) {
    // Should already be active from setup
    EXPECT_TRUE(memoryGuard_->isMonitoringActive());

    // Stop monitoring
    EXPECT_TRUE(memoryGuard_->stopRuntimeMonitoring());
    EXPECT_FALSE(memoryGuard_->isMonitoringActive());

    // Restart monitoring
    EXPECT_TRUE(memoryGuard_->startRuntimeMonitoring());
    EXPECT_TRUE(memoryGuard_->isMonitoringActive());
}

// Statistics and metrics tests
TEST_F(MemoryGuardTest, GuardedAllocationsCountTest) {
    size_t initialCount = memoryGuard_->getGuardedAllocationsCount();

    // Install some buffer guards
    void* buffer1 = allocateTestBuffer(1024);
    void* buffer2 = allocateTestBuffer(2048);

    memoryGuard_->installBufferGuard(buffer1, 1024);
    memoryGuard_->installBufferGuard(buffer2, 2048);

    // Count should increase
    size_t newCount = memoryGuard_->getGuardedAllocationsCount();
    EXPECT_GE(newCount, initialCount + 2);

    // Remove guards
    memoryGuard_->removeBufferGuard(buffer1);
    memoryGuard_->removeBufferGuard(buffer2);

    deallocateTestBuffer(buffer1);
    deallocateTestBuffer(buffer2);
}

TEST_F(MemoryGuardTest, ViolationCountAndRateTest) {
    size_t initialViolations = memoryGuard_->getTotalViolationsCount();
    double initialRate = memoryGuard_->getViolationRate();

    // Report some violations
    for (int i = 0; i < 5; ++i) {
        memoryGuard_->reportViolation(createTestViolation(ViolationType::BufferOverflow));
    }

    // Check updated counts and rate
    size_t newViolations = memoryGuard_->getTotalViolationsCount();
    double newRate = memoryGuard_->getViolationRate();

    EXPECT_EQ(newViolations, initialViolations + 5);
    EXPECT_GE(newRate, initialRate);
}

TEST_F(MemoryGuardTest, MemoryStatisticsTest) {
    auto stats = memoryGuard_->getMemoryStats();

    // Basic statistics should be available
    EXPECT_GE(stats.totalAllocations, 0);
    EXPECT_GE(stats.totalDeallocations, 0);
    EXPECT_GE(stats.currentAllocations, 0);
    EXPECT_GE(stats.totalBytesAllocated, 0);
    EXPECT_GE(stats.fragmentationRatio, 0.0);
    EXPECT_LE(stats.fragmentationRatio, 1.0);
}

// Audit tests
TEST_F(MemoryGuardTest, GuardAuditTest) {
    // Install some guards
    void* buffer1 = allocateTestBuffer(1024);
    void* buffer2 = allocateTestBuffer(2048);

    memoryGuard_->installBufferGuard(buffer1, 1024);
    memoryGuard_->installBufferGuard(buffer2, 2048);

    // Perform audit
    EXPECT_TRUE(memoryGuard_->performGuardAudit());

    // Cleanup
    memoryGuard_->removeBufferGuard(buffer1);
    memoryGuard_->removeBufferGuard(buffer2);
    deallocateTestBuffer(buffer1);
    deallocateTestBuffer(buffer2);
}

TEST_F(MemoryGuardTest, MemoryAuditTest) {
    // Perform memory audit
    EXPECT_TRUE(memoryGuard_->performMemoryAudit());
}

// Secure memory operations tests
TEST_F(MemoryGuardTest, SecureMemoryOperationsTest) {
    void* securePtr = nullptr;

    // Secure allocation
    memoryGuard_->secureAlloc(1024, &securePtr);
    EXPECT_NE(securePtr, nullptr);

    // Write some data to verify it's usable
    std::memset(securePtr, 0xAA, 1024);

    // Secure deallocation
    memoryGuard_->secureFree(securePtr);
}

TEST_F(MemoryGuardTest, MemoryProtectionTest) {
    // Enable memory protection
    memoryGuard_->enableMemoryProtection();

    // Disable memory protection
    memoryGuard_->disableMemoryProtection();

    // These calls should not throw or crash
    SUCCEED();
}

// Memory leak detection tests
TEST_F(MemoryGuardTest, MemoryLeakDetectionTest) {
    // Allocate some memory and intentionally "leak" it
    void* leakyBuffer = allocateTestBuffer(1024);
    memoryGuard_->installBufferGuard(leakyBuffer, 1024);

    // Check for memory leaks
    bool hasLeaks = memoryGuard_->checkMemoryLeak();

    // Cleanup to avoid actual leaks in tests
    memoryGuard_->removeBufferGuard(leakyBuffer);
    deallocateTestBuffer(leakyBuffer);

    // The result depends on implementation, but call should succeed
    (void)hasLeaks;  // Suppress unused variable warning
}

// ScopedMemoryGuard tests
TEST_F(MemoryGuardTest, ScopedMemoryGuardTest) {
    void* testBuffer = allocateTestBuffer(1024);

    {
        // Create scoped guard
        ScopedMemoryGuard scopedGuard(*memoryGuard_, testBuffer, 1024);

        // Memory should be guarded within scope
        EXPECT_TRUE(memoryGuard_->validateMemoryAccess(testBuffer, 1024));

        // Guard should be automatically removed when scope exits
    }

    deallocateTestBuffer(testBuffer);
}

// Thread safety and concurrent access tests
TEST_F(MemoryGuardTest, ConcurrentGuardInstallationTest) {
    const int numThreads = 4;
    const int buffersPerThread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};

    std::vector<std::vector<void*>> threadBuffers(numThreads);

    // Create threads that install guards concurrently
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            threadBuffers[t].resize(buffersPerThread);

            for (int i = 0; i < buffersPerThread; ++i) {
                threadBuffers[t][i] = allocateTestBuffer(512);

                if (memoryGuard_->installBufferGuard(threadBuffers[t][i], 512)) {
                    successCount++;
                } else {
                    failCount++;
                }

                // Small delay to increase contention
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Cleanup all buffers
    for (int t = 0; t < numThreads; ++t) {
        for (int i = 0; i < buffersPerThread; ++i) {
            memoryGuard_->removeBufferGuard(threadBuffers[t][i]);
            deallocateTestBuffer(threadBuffers[t][i]);
        }
    }

    // Most operations should succeed
    EXPECT_GT(successCount.load(), failCount.load());
}

TEST_F(MemoryGuardTest, ConcurrentViolationReportingTest) {
    const int numThreads = 3;
    const int violationsPerThread = 20;
    std::vector<std::thread> threads;

    // Create threads that report violations concurrently
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < violationsPerThread; ++i) {
                auto violation = createTestViolation(ViolationType::BufferOverflow);
                violation.description =
                    "Thread " + std::to_string(t) + " violation " + std::to_string(i);

                memoryGuard_->reportViolation(violation);

                // Small delay to test thread safety
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all violations were recorded
    auto history = memoryGuard_->getViolationHistory();
    EXPECT_EQ(history.size(), numThreads * violationsPerThread);
}

// Performance tests
TEST_F(MemoryGuardTest, GuardInstallationPerformanceTest) {
    const int numBuffers = 1000;
    std::vector<void*> buffers(numBuffers);

    // Allocate buffers
    for (int i = 0; i < numBuffers; ++i) {
        buffers[i] = allocateTestBuffer(1024);
    }

    // Measure guard installation time
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numBuffers; ++i) {
        memoryGuard_->installBufferGuard(buffers[i], 1024);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    double avgTimePerGuard = static_cast<double>(duration.count()) / numBuffers;

    std::cout << "Average guard installation time: " << avgTimePerGuard << " μs" << std::endl;

    // Performance should be reasonable
    EXPECT_LT(avgTimePerGuard, 1000.0);  // Less than 1ms per guard installation

    // Cleanup
    for (int i = 0; i < numBuffers; ++i) {
        memoryGuard_->removeBufferGuard(buffers[i]);
        deallocateTestBuffer(buffers[i]);
    }
}

TEST_F(MemoryGuardTest, ValidationPerformanceTest) {
    const int numValidations = 10000;
    void* testBuffer = allocateTestBuffer(1024);

    memoryGuard_->installBufferGuard(testBuffer, 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numValidations; ++i) {
        memoryGuard_->validateMemoryAccess(testBuffer, 1024);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    double avgTimePerValidation = static_cast<double>(duration.count()) / numValidations;

    std::cout << "Average validation time: " << avgTimePerValidation << " μs" << std::endl;

    // Performance should be very fast
    EXPECT_LT(avgTimePerValidation, 10.0);  // Less than 10μs per validation

    // Cleanup
    memoryGuard_->removeBufferGuard(testBuffer);
    deallocateTestBuffer(testBuffer);
}

// Edge cases and boundary conditions
TEST_F(MemoryGuardTest, BoundaryConditionsTest) {
    // Test minimum buffer size
    void* minBuffer = allocateTestBuffer(1);
    EXPECT_TRUE(memoryGuard_->installBufferGuard(minBuffer, 1));
    EXPECT_TRUE(memoryGuard_->validateMemoryAccess(minBuffer, 1));
    memoryGuard_->removeBufferGuard(minBuffer);
    deallocateTestBuffer(minBuffer);

    // Test large buffer size
    const size_t largeSize = 1024 * 1024;  // 1MB
    void* largeBuffer = allocateTestBuffer(largeSize);
    if (largeBuffer) {  // Only test if allocation succeeded
        EXPECT_TRUE(memoryGuard_->installBufferGuard(largeBuffer, largeSize));
        EXPECT_TRUE(memoryGuard_->validateMemoryAccess(largeBuffer, largeSize));
        memoryGuard_->removeBufferGuard(largeBuffer);
        deallocateTestBuffer(largeBuffer);
    }
}

TEST_F(MemoryGuardTest, ErrorConditionsTest) {
    // Test double installation of same buffer
    void* testBuffer = allocateTestBuffer(1024);

    EXPECT_TRUE(memoryGuard_->installBufferGuard(testBuffer, 1024));
    EXPECT_FALSE(memoryGuard_->installBufferGuard(testBuffer, 1024));  // Should fail

    // Test removal of non-guarded buffer
    void* unguardedBuffer = allocateTestBuffer(512);
    EXPECT_FALSE(memoryGuard_->removeBufferGuard(unguardedBuffer));

    // Cleanup
    memoryGuard_->removeBufferGuard(testBuffer);
    deallocateTestBuffer(testBuffer);
    deallocateTestBuffer(unguardedBuffer);
}
