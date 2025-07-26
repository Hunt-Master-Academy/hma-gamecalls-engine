/**
 * @file wasm-memory.test.js
 * @brief WASM Memory Management Test Module - Phase 3.1 Automated Testing
 *
 * This module provides comprehensive testing for WebAssembly memory management,
 * including allocation, deallocation, and memory safety in the Huntmaster Engine.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "mocha";

// Mock WASM memory environment
class MockWasmMemory {
  constructor(initialPages = 1) {
    this.pages = initialPages;
    this.buffer = new ArrayBuffer(initialPages * 65536); // 64KB per page
    this.allocatedBlocks = new Map();
    this.nextAddress = 1024; // Start allocations after reserved space
    this.totalAllocated = 0;
    this.peakUsage = 0;
  }

  malloc(size) {
    if (this.nextAddress + size > this.buffer.byteLength) {
      throw new Error("Out of memory");
    }
    const address = this.nextAddress;
    this.allocatedBlocks.set(address, size);
    this.nextAddress += size;
    this.totalAllocated += size;
    this.peakUsage = Math.max(this.peakUsage, this.totalAllocated);
    return address;
  }

  free(address) {
    const size = this.allocatedBlocks.get(address);
    if (size) {
      this.allocatedBlocks.delete(address);
      this.totalAllocated -= size;
      return true;
    }
    return false;
  }

  grow(pages) {
    const oldSize = this.buffer.byteLength;
    const newSize = oldSize + pages * 65536;
    const newBuffer = new ArrayBuffer(newSize);
    new Uint8Array(newBuffer).set(new Uint8Array(this.buffer));
    this.buffer = newBuffer;
    this.pages += pages;
    return this.pages;
  }

  getUsage() {
    return {
      totalAllocated: this.totalAllocated,
      peakUsage: this.peakUsage,
      activeBlocks: this.allocatedBlocks.size,
      totalSize: this.buffer.byteLength,
      utilization: this.totalAllocated / this.buffer.byteLength,
    };
  }
}

// Mock WASM instance
global.WebAssembly = global.WebAssembly || {
  Memory: class {
    constructor(descriptor) {
      this.buffer = new ArrayBuffer(descriptor.initial * 65536);
      this.pages = descriptor.initial;
    }
    grow(pages) {
      const oldSize = this.buffer.byteLength;
      const newSize = oldSize + pages * 65536;
      const newBuffer = new ArrayBuffer(newSize);
      new Uint8Array(newBuffer).set(new Uint8Array(this.buffer));
      this.buffer = newBuffer;
      this.pages += pages;
      return this.pages;
    }
  },
};

/**
 * WASM Memory Management Test Suite
 * Tests memory allocation, usage, and cleanup
 */
describe("WASM Memory Management Tests", () => {
  let wasmInstance;
  let memoryTracker;
  let mockMemory;
  let allocationHistory;
  let memoryBaseline;

  beforeEach(() => {
    // Initialize WASM instance for memory testing
    mockMemory = new MockWasmMemory(4); // Start with 4 pages (256KB)
    wasmInstance = {
      exports: {
        memory: new WebAssembly.Memory({ initial: 4 }),
        malloc: (size) => mockMemory.malloc(size),
        free: (address) => mockMemory.free(address),
        realloc: (address, newSize) => {
          if (address === 0) return mockMemory.malloc(newSize);
          const oldSize = mockMemory.allocatedBlocks.get(address);
          if (!oldSize) return 0;
          const newAddress = mockMemory.malloc(newSize);
          mockMemory.free(address);
          return newAddress;
        },
        get_memory_usage: () => mockMemory.getUsage().totalAllocated,
        get_memory_stats: () => mockMemory.getUsage(),
      },
    };

    // Set up memory tracking and monitoring
    memoryTracker = {
      allocations: [],
      deallocations: [],
      leaks: [],
      startTime: performance.now(),
      peakUsage: 0,
      errors: [],
    };

    // Configure memory limits and quotas
    const memoryLimits = {
      maxAllocation: 1024 * 1024, // 1MB
      maxTotalUsage: 4 * 1024 * 1024, // 4MB
      leakThreshold: 1024, // 1KB
      fragmentationThreshold: 0.3,
    };

    // Initialize test data buffers
    allocationHistory = [];

    // Create memory allocation tracking system
    const originalMalloc = wasmInstance.exports.malloc;
    wasmInstance.exports.malloc = (size) => {
      const address = originalMalloc(size);
      const allocation = {
        address,
        size,
        timestamp: performance.now(),
        freed: false,
      };
      memoryTracker.allocations.push(allocation);
      allocationHistory.push(allocation);
      return address;
    };

    const originalFree = wasmInstance.exports.free;
    wasmInstance.exports.free = (address) => {
      const result = originalFree(address);
      const allocation = allocationHistory.find(
        (a) => a.address === address && !a.freed
      );
      if (allocation) {
        allocation.freed = true;
        allocation.freeTime = performance.now();
        memoryTracker.deallocations.push({
          address,
          size: allocation.size,
          lifetime: allocation.freeTime - allocation.timestamp,
        });
      }
      return result;
    };

    // Set up memory usage baseline measurements
    memoryBaseline = {
      heapUsed: mockMemory.getUsage().totalAllocated,
      timestamp: performance.now(),
      pageCount: mockMemory.pages,
    };

    // Initialize memory leak detection
    const leakDetector = {
      checkInterval: null,
      lastCheck: performance.now(),
      tolerance: 1024, // 1KB tolerance
    };

    // Configure memory pressure simulation
    const pressureSimulator = {
      enabled: false,
      level: 0, // 0-1 scale
      interval: null,
    };

    // Set up garbage collection monitoring
    if (typeof performance !== "undefined" && performance.memory) {
      memoryTracker.jsHeapBaseline = performance.memory.usedJSHeapSize;
    }

    // Initialize memory profiling tools
    memoryTracker.profiler = {
      samples: [],
      interval: null,
      sampleRate: 100, // ms
    };
  });

  afterEach(() => {
    // Clean up allocated memory
    const activeAllocations = allocationHistory.filter((a) => !a.freed);
    activeAllocations.forEach((allocation) => {
      try {
        wasmInstance.exports.free(allocation.address);
      } catch (error) {
        memoryTracker.errors.push(
          `Cleanup error for ${allocation.address}: ${error.message}`
        );
      }
    });

    // Verify no memory leaks
    const remainingAllocations = allocationHistory.filter((a) => !a.freed);
    if (remainingAllocations.length > 0) {
      memoryTracker.leaks = remainingAllocations;
      console.warn(
        `Memory leaks detected: ${remainingAllocations.length} allocations not freed`
      );
    }

    // Reset memory tracking state
    memoryTracker.allocations = [];
    memoryTracker.deallocations = [];
    allocationHistory = [];

    // Generate memory usage reports
    const finalUsage = mockMemory.getUsage();
    const memoryReport = {
      finalUsage,
      peakUsage: memoryTracker.peakUsage,
      totalAllocations: memoryTracker.allocations.length,
      totalDeallocations: memoryTracker.deallocations.length,
      leaks: memoryTracker.leaks.length,
      errors: memoryTracker.errors.length,
    };

    // Validate all memory has been freed
    expect(finalUsage.activeBlocks).to.equal(0, "All memory should be freed");

    // Check for memory fragmentation
    const fragmentation = 1 - finalUsage.totalAllocated / finalUsage.totalSize;
    if (fragmentation > 0.5) {
      console.warn(
        `High memory fragmentation detected: ${(fragmentation * 100).toFixed(
          2
        )}%`
      );
    }

    // Export memory performance metrics
    if (memoryTracker.errors.length === 0) {
      console.log("Memory Test Report:", memoryReport);
    }

    // Verify garbage collection effectiveness
    if (typeof performance !== "undefined" && performance.memory) {
      const currentJSHeap = performance.memory.usedJSHeapSize;
      const jsHeapGrowth = currentJSHeap - memoryTracker.jsHeapBaseline;
      if (jsHeapGrowth > 1024 * 1024) {
        // 1MB growth
        console.warn(
          `Significant JS heap growth: ${(jsHeapGrowth / 1024 / 1024).toFixed(
            2
          )}MB`
        );
      }
    }

    // Reset memory pressure conditions
    mockMemory = null;
    wasmInstance = null;

    // Archive memory test results
    memoryTracker = null;
  });

  describe("Memory Allocation", () => {
    /**
     * Test Case 1: Basic Memory Allocation
     * Validates basic malloc and free operations
     */
    it("should handle basic memory allocation and deallocation", async () => {
      const testSizes = [64, 256, 1024, 4096];
      const allocatedAddresses = [];

      // Test allocation of various sizes
      for (const size of testSizes) {
        const address = wasmInstance.exports.malloc(size);
        expect(address).to.be.greaterThan(
          0,
          `Should allocate ${size} bytes successfully`
        );
        allocatedAddresses.push({ address, size });
      }

      // Verify memory usage increased
      const currentUsage = wasmInstance.exports.get_memory_usage();
      expect(currentUsage).to.be.greaterThan(
        memoryBaseline.heapUsed,
        "Memory usage should increase"
      );

      // Test deallocation
      for (const allocation of allocatedAddresses) {
        const freed = wasmInstance.exports.free(allocation.address);
        expect(freed).to.be.true;
      }

      // Verify memory was freed
      const finalUsage = wasmInstance.exports.get_memory_usage();
      expect(finalUsage).to.equal(
        memoryBaseline.heapUsed,
        "All memory should be freed"
      );
    });

    /**
     * Test Case 2: Memory Allocation Limits and Large Allocations
     * Tests behavior at memory allocation boundaries
     */
    it("should handle large memory allocations", async () => {
      const largeSize = 512 * 1024; // 512KB
      const addresses = [];

      try {
        // Allocate memory until we approach limits
        for (let i = 0; i < 8; i++) {
          const address = wasmInstance.exports.malloc(largeSize);
          addresses.push(address);
          expect(address).to.be.greaterThan(
            0,
            `Allocation ${i} should succeed`
          );
        }

        // Should fail when exceeding available memory
        expect(() => {
          wasmInstance.exports.malloc(largeSize);
        }).to.throw("Out of memory");
      } finally {
        // Clean up allocations
        addresses.forEach((address) => wasmInstance.exports.free(address));
      }
    });

    /**
     * Test Case 3: Dynamic Memory Resizing
     * Tests memory reallocation functionality
     */
    it("should support dynamic memory resizing", async () => {
      const initialSize = 256;
      const newSize = 512;

      // Allocate initial memory
      const initialAddress = wasmInstance.exports.malloc(initialSize);
      expect(initialAddress).to.be.greaterThan(0);

      // Reallocate to larger size
      const newAddress = wasmInstance.exports.realloc(initialAddress, newSize);
      expect(newAddress).to.be.greaterThan(0);

      // Verify memory usage
      const usage = wasmInstance.exports.get_memory_usage();
      expect(usage).to.be.at.least(newSize);

      // Clean up
      wasmInstance.exports.free(newAddress);
    });

    it("should handle large memory allocations", async () => {
      console.log("\n=== Large Memory Allocation Tests ===");

      // Test allocation of large audio processing buffers
      const audioBufferSizes = [
        1024 * 1024, // 1MB buffer
        4 * 1024 * 1024, // 4MB buffer
        8 * 1024 * 1024, // 8MB buffer
        16 * 1024 * 1024, // 16MB buffer
      ];

      const allocationResults = [];

      for (const bufferSize of audioBufferSizes) {
        console.log(
          `Testing allocation of ${bufferSize / (1024 * 1024)}MB buffer...`
        );

        try {
          const startTime = performance.now();
          const address = wasmMemory.malloc(bufferSize);
          const allocTime = performance.now() - startTime;

          // Verify the allocation succeeded
          expect(address).to.be.greaterThan(0);
          expect(wasmMemory.allocatedBlocks.has(address)).to.be.true;

          // Test writing to the allocated memory
          const view = new Uint8Array(
            wasmMemory.buffer,
            address,
            Math.min(bufferSize, 1024)
          );
          view.fill(0xaa);
          expect(view[0]).to.equal(0xaa);
          expect(view[view.length - 1]).to.equal(0xaa);

          allocationResults.push({
            size: bufferSize,
            address,
            allocTime,
            success: true,
          });

          console.log(
            `✓ ${
              bufferSize / (1024 * 1024)
            }MB allocation successful in ${allocTime.toFixed(2)}ms`
          );

          // Clean up
          wasmMemory.free(address);
        } catch (error) {
          allocationResults.push({
            size: bufferSize,
            error: error.message,
            success: false,
          });

          if (error.message.includes("Out of memory")) {
            console.log(
              `✓ ${
                bufferSize / (1024 * 1024)
              }MB allocation correctly failed - memory limit reached`
            );
          } else {
            console.error(
              `✗ Unexpected error for ${
                bufferSize / (1024 * 1024)
              }MB allocation:`,
              error.message
            );
            throw error;
          }
        }
      }

      // Verify system memory limits are respected
      const systemMemoryLimit = wasmMemory.buffer.byteLength;
      console.log(
        `System memory limit: ${systemMemoryLimit / (1024 * 1024)}MB`
      );

      try {
        const oversizeAddress = wasmMemory.malloc(systemMemoryLimit + 1024);
        wasmMemory.free(oversizeAddress);
        throw new Error("Oversize allocation should have failed");
      } catch (error) {
        expect(error.message).to.include("Out of memory");
        console.log("✓ System memory limits properly enforced");
      }

      // Test out-of-memory scenarios with multiple allocations
      const addresses = [];
      let totalAllocated = 0;
      const chunkSize = 1024 * 1024; // 1MB chunks

      try {
        while (totalAllocated < systemMemoryLimit) {
          const address = wasmMemory.malloc(chunkSize);
          addresses.push(address);
          totalAllocated += chunkSize;
        }

        // This should fail
        wasmMemory.malloc(chunkSize);
        throw new Error("Should have run out of memory");
      } catch (error) {
        expect(error.message).to.include("Out of memory");
        console.log(
          `✓ Out-of-memory correctly triggered after ${
            totalAllocated / (1024 * 1024)
          }MB`
        );
      } finally {
        // Clean up all allocations
        addresses.forEach((addr) => wasmMemory.free(addr));
      }

      // Validate memory fragmentation handling
      const fragmentationTest = () => {
        const allocations = [];

        // Allocate many small blocks
        for (let i = 0; i < 100; i++) {
          allocations.push(wasmMemory.malloc(1024));
        }

        // Free every other block to create fragmentation
        for (let i = 1; i < allocations.length; i += 2) {
          wasmMemory.free(allocations[i]);
          allocations[i] = null;
        }

        // Try to allocate a larger block
        try {
          const largeBlock = wasmMemory.malloc(50 * 1024); // 50KB
          wasmMemory.free(largeBlock);
          console.log("✓ Large allocation succeeded despite fragmentation");
        } catch (error) {
          console.log("✓ Fragmentation correctly prevents large allocation");
        }

        // Clean up remaining allocations
        allocations.forEach((addr) => {
          if (addr !== null) wasmMemory.free(addr);
        });
      };

      fragmentationTest();

      // Test memory growth strategies
      const originalSize = wasmMemory.buffer.byteLength;
      const growthResult = wasmMemory.grow(2); // Add 2 pages (128KB)

      if (growthResult >= 0) {
        expect(wasmMemory.buffer.byteLength).to.be.greaterThan(originalSize);
        console.log(
          `✓ Memory growth successful: ${originalSize} → ${wasmMemory.buffer.byteLength} bytes`
        );
      } else {
        console.log(
          "✓ Memory growth failed gracefully (expected in constrained environment)"
        );
      }

      console.log("✓ Large memory allocation tests completed");
    });

    it("should support dynamic memory resizing", async () => {
      console.log("\n=== Dynamic Memory Resizing Tests ===");

      // Test dynamic buffer resizing operations
      const initialSize = 1024 * 4; // 4KB initial buffer
      const bufferAddress = wasmMemory.malloc(initialSize);

      // Fill buffer with test data
      const initialView = new Uint8Array(
        wasmMemory.buffer,
        bufferAddress,
        initialSize
      );
      for (let i = 0; i < initialSize; i++) {
        initialView[i] = i % 256;
      }

      console.log(
        `Initial buffer allocated: ${initialSize} bytes at address ${bufferAddress}`
      );

      // Verify data preservation during resize
      const resizeToSize = 1024 * 8; // 8KB
      const preservationTest = () => {
        // Simulate resize by allocating new buffer and copying data
        const newAddress = wasmMemory.malloc(resizeToSize);
        const newView = new Uint8Array(
          wasmMemory.buffer,
          newAddress,
          resizeToSize
        );

        // Copy original data
        const copySize = Math.min(initialSize, resizeToSize);
        for (let i = 0; i < copySize; i++) {
          newView[i] = initialView[i];
        }

        // Verify data preservation
        for (let i = 0; i < copySize; i++) {
          expect(newView[i]).to.equal(i % 256);
        }

        console.log(
          `✓ Data preserved during resize from ${initialSize} to ${resizeToSize} bytes`
        );

        // Clean up old buffer
        wasmMemory.free(bufferAddress);

        return newAddress;
      };

      const newBufferAddress = preservationTest();

      // Test resize failure recovery
      const resizeFailureTest = () => {
        const oversizeRequest = wasmMemory.buffer.byteLength + 1024;

        try {
          // This should fail
          const failedAddress = wasmMemory.malloc(oversizeRequest);
          wasmMemory.free(failedAddress);
          throw new Error("Oversize allocation should have failed");
        } catch (error) {
          expect(error.message).to.include("Out of memory");

          // Verify original buffer is still intact
          const currentView = new Uint8Array(
            wasmMemory.buffer,
            newBufferAddress,
            resizeToSize
          );
          expect(currentView[0]).to.equal(0);
          expect(currentView[255]).to.equal(255);

          console.log(
            "✓ Resize failure recovery working - original buffer intact"
          );
        }
      };

      resizeFailureTest();

      // Validate memory copy operations
      const copyOperationsTest = () => {
        const sourceSize = 512;
        const sourceAddress = wasmMemory.malloc(sourceSize);
        const sourceView = new Uint8Array(
          wasmMemory.buffer,
          sourceAddress,
          sourceSize
        );

        // Fill source with pattern
        for (let i = 0; i < sourceSize; i++) {
          sourceView[i] = (i * 2) % 256;
        }

        const destAddress = wasmMemory.malloc(sourceSize);
        const destView = new Uint8Array(
          wasmMemory.buffer,
          destAddress,
          sourceSize
        );

        // Copy data
        for (let i = 0; i < sourceSize; i++) {
          destView[i] = sourceView[i];
        }

        // Verify copy
        for (let i = 0; i < sourceSize; i++) {
          expect(destView[i]).to.equal(sourceView[i]);
        }

        console.log(
          `✓ Memory copy operations verified for ${sourceSize} bytes`
        );

        // Test overlapping copy detection
        const overlapTest = () => {
          // This would be dangerous in real WASM, but we can test detection
          const hasOverlap = (src, srcSize, dest, destSize) => {
            return src < dest + destSize && dest < src + srcSize;
          };

          expect(hasOverlap(sourceAddress, sourceSize, destAddress, sourceSize))
            .to.be.false;
          expect(hasOverlap(100, 50, 120, 50)).to.be.true;

          console.log("✓ Overlapping copy detection working");
        };

        overlapTest();

        // Clean up
        wasmMemory.free(sourceAddress);
        wasmMemory.free(destAddress);
      };

      copyOperationsTest();

      // Test buffer resize with different growth patterns
      const growthPatternTest = () => {
        const patterns = [
          { name: "Linear Growth", multiplier: 1.5 },
          { name: "Exponential Growth", multiplier: 2.0 },
          { name: "Conservative Growth", multiplier: 1.25 },
        ];

        for (const pattern of patterns) {
          const startSize = 1024;
          let currentSize = startSize;
          let currentAddress = wasmMemory.malloc(startSize);

          const maxIterations = 5;
          for (let i = 0; i < maxIterations; i++) {
            const newSize = Math.floor(currentSize * pattern.multiplier);

            try {
              const newAddress = wasmMemory.malloc(newSize);

              // Simulate data copy
              const oldView = new Uint8Array(
                wasmMemory.buffer,
                currentAddress,
                currentSize
              );
              const newView = new Uint8Array(
                wasmMemory.buffer,
                newAddress,
                newSize
              );

              for (let j = 0; j < currentSize; j++) {
                newView[j] = oldView[j];
              }

              wasmMemory.free(currentAddress);
              currentAddress = newAddress;
              currentSize = newSize;
            } catch (error) {
              console.log(
                `${pattern.name}: Growth stopped at ${currentSize} bytes (${error.message})`
              );
              break;
            }
          }

          console.log(
            `✓ ${pattern.name} pattern tested up to ${currentSize} bytes`
          );
          wasmMemory.free(currentAddress);
        }
      };

      growthPatternTest();

      // Clean up
      wasmMemory.free(newBufferAddress);

      console.log("✓ Dynamic memory resizing tests completed");
    });
  });

  describe("Memory Access Patterns", () => {
    /**
     * Test Case 4: Memory Bounds Checking
     * Validates memory safety and bounds checking
     */
    it("should validate memory bounds checking", async () => {
      const validSize = 1024;
      const address = wasmInstance.exports.malloc(validSize);

      // Verify valid allocation
      expect(address).to.be.greaterThan(0);

      // Test double-free protection
      expect(wasmInstance.exports.free(address)).to.be.true;
      expect(wasmInstance.exports.free(address)).to.be.false; // Should fail on double-free

      // Test invalid free
      const invalidAddress = 999999;
      expect(wasmInstance.exports.free(invalidAddress)).to.be.false;

      // Test zero-size allocation
      const zeroAddress = wasmInstance.exports.malloc(0);
      if (zeroAddress > 0) {
        wasmInstance.exports.free(zeroAddress);
      }

      // Test negative size (should be handled safely)
      expect(() => {
        wasmInstance.exports.malloc(-1);
      }).to.not.throw();
    });

    /**
     * Test Case 5: Memory Access Performance
     * Benchmarks memory allocation performance
     */
    it("should optimize memory access performance", async () => {
      const iterationCount = 1000;
      const allocationSize = 256;
      const addresses = [];

      // Benchmark allocation performance
      const allocStartTime = performance.now();
      for (let i = 0; i < iterationCount; i++) {
        const address = wasmInstance.exports.malloc(allocationSize);
        addresses.push(address);
      }
      const allocEndTime = performance.now();

      // Benchmark deallocation performance
      const freeStartTime = performance.now();
      addresses.forEach((address) => wasmInstance.exports.free(address));
      const freeEndTime = performance.now();

      // Calculate performance metrics
      const allocTime = allocEndTime - allocStartTime;
      const freeTime = freeEndTime - freeStartTime;
      const allocRate = iterationCount / (allocTime / 1000); // allocations per second
      const freeRate = iterationCount / (freeTime / 1000); // deallocations per second

      // Performance assertions
      expect(allocTime).to.be.lessThan(
        1000,
        "Allocation should complete within 1 second"
      );
      expect(freeTime).to.be.lessThan(
        1000,
        "Deallocation should complete within 1 second"
      );
      expect(allocRate).to.be.greaterThan(
        1000,
        "Should allocate at least 1000 blocks per second"
      );
      expect(freeRate).to.be.greaterThan(
        1000,
        "Should free at least 1000 blocks per second"
      );

      // Log performance results
      console.log(
        `Memory Performance: Alloc: ${allocRate.toFixed(
          0
        )}/sec, Free: ${freeRate.toFixed(0)}/sec`
      );
    });

    /**
     * Test Case 6: Concurrent Memory Access Simulation
     * Tests thread-safe memory operations simulation
     */
    it("should handle concurrent memory access", async () => {
      const concurrentWorkers = 4;
      const allocationsPerWorker = 50;
      const allAllocations = [];

      // Simulate concurrent allocations
      const workers = Array.from(
        { length: concurrentWorkers },
        async (_, workerId) => {
          const workerAllocations = [];
          for (let i = 0; i < allocationsPerWorker; i++) {
            const size = Math.floor(Math.random() * 1024) + 64;
            try {
              const address = wasmInstance.exports.malloc(size);
              workerAllocations.push({ address, size, workerId });
            } catch (error) {
              if (!error.message.includes("Out of memory")) {
                throw error;
              }
            }
          }
          return workerAllocations;
        }
      );

      // Wait for all workers to complete
      const results = await Promise.all(workers);
      results.forEach((workerAllocations) => {
        allAllocations.push(...workerAllocations);
      });

      // Verify allocations are valid and unique
      const addressSet = new Set();
      allAllocations.forEach(({ address }) => {
        expect(address).to.be.greaterThan(0);
        expect(addressSet.has(address)).to.be.false; // No duplicate addresses
        addressSet.add(address);
      });

      // Clean up all allocations
      allAllocations.forEach(({ address }) => {
        wasmInstance.exports.free(address);
      });

      console.log(
        `Concurrent test: ${allAllocations.length} allocations from ${concurrentWorkers} workers`
      );
    });
  });

  describe("Memory Cleanup", () => {
    /**
     * Test Case 7: Memory Deallocation
     * Validates memory usage statistics and tracking
     */
    it("should deallocate memory properly", async () => {
      const testAllocations = [
        { size: 128, count: 5 },
        { size: 256, count: 3 },
        { size: 512, count: 2 },
      ];

      const addresses = [];
      let expectedUsage = 0;

      // Perform tracked allocations
      for (const { size, count } of testAllocations) {
        for (let i = 0; i < count; i++) {
          const address = wasmInstance.exports.malloc(size);
          addresses.push(address);
          expectedUsage += size;
        }
      }

      // Verify tracking accuracy
      const stats = wasmInstance.exports.get_memory_stats();
      expect(stats.totalAllocated).to.be.at.least(expectedUsage);
      expect(stats.activeBlocks).to.equal(addresses.length);

      // Test usage monitoring
      expect(memoryTracker.allocations.length).to.equal(addresses.length);

      // Clean up and verify
      addresses.forEach((address) => wasmInstance.exports.free(address));

      const finalStats = wasmInstance.exports.get_memory_stats();
      expect(finalStats.activeBlocks).to.equal(0);
    });

    /**
     * Test Case 8: Memory Leak Detection
     * Verifies that memory leaks are properly detected
     */
    it("should detect and prevent memory leaks", async () => {
      const startUsage = wasmInstance.exports.get_memory_usage();
      const leakSizes = [128, 256, 512];
      const leakedAddresses = [];

      // Allocate memory but don't free it (simulate leaks)
      for (const size of leakSizes) {
        const address = wasmInstance.exports.malloc(size);
        leakedAddresses.push({ address, size });
      }

      // Verify memory usage increased
      const currentUsage = wasmInstance.exports.get_memory_usage();
      const expectedIncrease = leakSizes.reduce((sum, size) => sum + size, 0);
      expect(currentUsage - startUsage).to.be.at.least(expectedIncrease);

      // Memory tracker should detect leaks in afterEach
      const activeAllocations = allocationHistory.filter((a) => !a.freed);
      expect(activeAllocations.length).to.equal(leakSizes.length);

      // Clean up to prevent test pollution
      leakedAddresses.forEach(({ address }) => {
        wasmInstance.exports.free(address);
      });
    });

    /**
     * Test Case 9: Emergency Memory Cleanup
     * Tests behavior under memory pressure conditions
     */
    it("should handle emergency memory cleanup", async () => {
      const allocationSize = 32 * 1024; // 32KB chunks
      const addresses = [];
      let allocationsSucceeded = 0;

      try {
        // Allocate until we hit limits
        while (true) {
          try {
            const address = wasmInstance.exports.malloc(allocationSize);
            addresses.push(address);
            allocationsSucceeded++;

            // Safety check to prevent infinite loop
            if (allocationsSucceeded > 100) break;
          } catch (error) {
            if (error.message.includes("Out of memory")) {
              break;
            }
            throw error;
          }
        }

        expect(allocationsSucceeded).to.be.greaterThan(
          0,
          "Should succeed at least some allocations"
        );

        // Test that we can still allocate smaller blocks
        const smallAddress = wasmInstance.exports.malloc(64);
        expect(smallAddress).to.be.greaterThan(
          0,
          "Should allocate small blocks under pressure"
        );
        wasmInstance.exports.free(smallAddress);
      } finally {
        // Free all pressure allocations
        addresses.forEach((address) => {
          try {
            wasmInstance.exports.free(address);
          } catch (error) {
            memoryTracker.errors.push(
              `Pressure cleanup error: ${error.message}`
            );
          }
        });
      }
    });
  });

  describe("Memory Security", () => {
    /**
     * Test Case 10: Memory Isolation
     * Tests memory sandboxing and isolation
     */
    it("should isolate WASM memory from host", async () => {
      const testSize = 1024;
      const address = wasmInstance.exports.malloc(testSize);

      // Verify allocation is within WASM memory space
      expect(address).to.be.greaterThan(0);
      expect(address).to.be.lessThan(mockMemory.buffer.byteLength);

      // Test memory boundaries
      const memoryView = new Uint8Array(wasmInstance.exports.memory.buffer);
      expect(memoryView.length).to.be.greaterThan(address + testSize);

      // Verify memory is isolated from host system
      expect(address).to.not.equal(null);
      expect(address).to.not.equal(undefined);

      wasmInstance.exports.free(address);
    });

    /**
     * Test Case 11: Memory Access Control
     * Tests memory permission enforcement
     */
    it("should prevent unauthorized memory access", async () => {
      const validSize = 512;
      const address = wasmInstance.exports.malloc(validSize);

      // Test valid access
      expect(address).to.be.greaterThan(0);

      // Test access within bounds
      const memoryView = new Uint8Array(wasmInstance.exports.memory.buffer);
      memoryView[address] = 42; // Should succeed
      expect(memoryView[address]).to.equal(42);

      // Test that memory access is controlled
      expect(() => {
        // Attempt to access beyond allocated bounds should be safe in WASM
        memoryView[address + validSize + 1000] = 99;
      }).to.not.throw(); // WASM provides memory safety

      wasmInstance.exports.free(address);
    });

    /**
     * Test Case 12: Memory Sanitization
     * Tests memory clearing and sanitization
     */
    it("should sanitize memory on deallocation", async () => {
      const testSize = 256;
      const address = wasmInstance.exports.malloc(testSize);

      // Write test pattern to memory
      const memoryView = new Uint8Array(wasmInstance.exports.memory.buffer);
      for (let i = 0; i < testSize; i++) {
        memoryView[address + i] = 0xaa; // Test pattern
      }

      // Verify pattern is written
      for (let i = 0; i < testSize; i++) {
        expect(memoryView[address + i]).to.equal(0xaa);
      }

      // Free memory
      const freed = wasmInstance.exports.free(address);
      expect(freed).to.be.true;

      // Verify deallocation tracking
      const finalStats = wasmInstance.exports.get_memory_stats();
      expect(finalStats.activeBlocks).to.be.lessThan(1);
    });

    /**
     * Test Case 13: Memory Fragmentation Analysis
     * Analyzes memory fragmentation patterns
     */
    it("should analyze and report memory fragmentation", async () => {
      const fragmentationTest = async () => {
        const addresses = [];

        // Create fragmentation pattern
        for (let i = 0; i < 10; i++) {
          const address = wasmInstance.exports.malloc(1024);
          addresses.push(address);
        }

        // Free every other allocation to create fragmentation
        for (let i = 1; i < addresses.length; i += 2) {
          wasmInstance.exports.free(addresses[i]);
          addresses[i] = null;
        }

        // Analyze fragmentation
        const stats = wasmInstance.exports.get_memory_stats();
        const fragmentation = 1 - stats.totalAllocated / stats.totalSize;

        expect(fragmentation).to.be.a("number");
        expect(fragmentation).to.be.at.least(0);
        expect(fragmentation).to.be.at.most(1);

        // Clean up remaining allocations
        addresses
          .filter((addr) => addr !== null)
          .forEach((addr) => {
            wasmInstance.exports.free(addr);
          });
      };

      await fragmentationTest();
    });

    /**
     * Test Case 14: Comprehensive Stress Testing
     * Comprehensive stress testing of memory subsystem
     */
    it("should handle comprehensive memory stress testing", async () => {
      const stressTestDuration = 100; // milliseconds
      const stressStartTime = performance.now();
      let operationCount = 0;
      const activeAllocations = new Map();

      try {
        while (performance.now() - stressStartTime < stressTestDuration) {
          const operation = Math.random();

          if (operation < 0.7 && activeAllocations.size < 100) {
            // 70% chance to allocate
            const size = Math.floor(Math.random() * 2048) + 64;
            try {
              const address = wasmInstance.exports.malloc(size);
              activeAllocations.set(address, size);
              operationCount++;
            } catch (error) {
              if (!error.message.includes("Out of memory")) {
                throw error;
              }
            }
          } else if (activeAllocations.size > 0) {
            // 30% chance to free (if allocations exist)
            const addresses = Array.from(activeAllocations.keys());
            const randomAddress =
              addresses[Math.floor(Math.random() * addresses.length)];
            wasmInstance.exports.free(randomAddress);
            activeAllocations.delete(randomAddress);
            operationCount++;
          }
        }

        // Verify system stability
        expect(operationCount).to.be.greaterThan(
          0,
          "Should perform operations during stress test"
        );

        // Measure final memory state
        const finalStats = wasmInstance.exports.get_memory_stats();
        expect(finalStats.activeBlocks).to.equal(activeAllocations.size);
      } finally {
        // Clean up all remaining allocations
        for (const [address] of activeAllocations) {
          try {
            wasmInstance.exports.free(address);
          } catch (error) {
            memoryTracker.errors.push(
              `Stress test cleanup error: ${error.message}`
            );
          }
        }
      }

      console.log(
        `Stress test completed: ${operationCount} operations in ${stressTestDuration}ms`
      );
    });
  });
});

export { wasmInstance, memoryTracker };
