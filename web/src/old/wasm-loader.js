// web/js/wasm-loader.js
// Helper to load and initialize the WASM module

class WASMLoader {
  constructor() {
    this.module = null;
    this.ready = false;
  }

  async load() {
    console.log("[WASMLoader] Loading Huntmaster WASM module...");

    try {
      // Load the Emscripten-generated module
      const HuntmasterEngine = await import("../../dist/huntmaster_engine.js");

      // Initialize the module
      this.module = await HuntmasterEngine.default();

      // Wait for runtime to be ready
      if (this.module.ready) {
        await this.module.ready;
      }

      // Verify critical functions exist
      const requiredFunctions = [
        "_createEngine",
        "_destroyEngine",
        "_loadMasterCall",
        "_startSession",
        "_processAudioChunk",
        "_getSimilarityScore",
        "_endSession",
        "_malloc",
        "_free",
      ];

      for (const func of requiredFunctions) {
        if (typeof this.module[func] !== "function") {
          throw new Error(`Required function ${func} not found in WASM module`);
        }
      }

      console.log("[WASMLoader] Module loaded successfully");
      console.log(
        "[WASMLoader] Available heap size:",
        this.module.HEAP8.length
      );

      this.ready = true;
      return this.module;
    } catch (error) {
      console.error("[WASMLoader] Failed to load module:", error);
      throw error;
    }
  }

  // Helper methods for memory management
  allocateFloat32Array(size) {
    const bytesPerElement = 4;
    const totalBytes = size * bytesPerElement;
    const ptr = this.module._malloc(totalBytes);

    if (!ptr) {
      throw new Error("Failed to allocate memory");
    }

    return {
      ptr: ptr,
      size: size,
      totalBytes: totalBytes,
      set: (data) => {
        this.module.HEAPF32.set(data, ptr / bytesPerElement);
      },
      free: () => {
        this.module._free(ptr);
      },
    };
  }
}

// Export for use in other modules
window.WASMLoader = WASMLoader;
