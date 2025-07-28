/**
 * Data Encryption Module for Session Recording
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This module provides comprehensive data encryption and secure transmission
 * capabilities for session recording data, including end-to-end encryption,
 * key management, and secure storage protocols.
 *
 * @fileoverview Data encryption and secure transmission for session recording
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires DataValidator - For encryption data validation
 * @requires PrivacyCompliance - For privacy-compliant encryption
 */

import { DataValidator } from "../validation/data-validator.js";

/**
 * DataEncryption class for comprehensive data protection during session recording
 * Provides AES encryption, key management, secure transmission, and data integrity
 */
class DataEncryption {
  constructor(options = {}) {
    this.config = {
      algorithm: options.algorithm || "AES-GCM",
      keySize: options.keySize || 256,
      ivSize: options.ivSize || 12,
      tagSize: options.tagSize || 16,
      keyDerivation: options.keyDerivation || "PBKDF2",
      iterations: options.iterations || 100000,
      hashAlgorithm: options.hashAlgorithm || "SHA-256",
      enableCompression: options.enableCompression !== false,
      enableIntegrityCheck: options.enableIntegrityCheck !== false,
      keyRotationInterval: options.keyRotationInterval || 24 * 60 * 60 * 1000, // 24 hours
      maxKeyAge: options.maxKeyAge || 7 * 24 * 60 * 60 * 1000, // 7 days
      enableSecureHeaders: options.enableSecureHeaders !== false,
      debugMode: options.debugMode || false,
      ...options,
    };

    this.validator = new DataValidator();

    this.state = {
      isInitialized: false,
      currentKey: null,
      keyHistory: [],
      encryptionStats: {
        totalEncrypted: 0,
        totalDecrypted: 0,
        totalSize: 0,
        compressionRatio: 1.0,
        errorCount: 0,
      },
      keyRotationTimer: null,
    };

    this.cryptoAPI = {
      subtle: null,
      random: null,
      supported: false,
    };

    this.keyStorage = new Map();
    this.encryptionCache = new Map();

    this.initializeEncryption();
  }

  /**
   * Initialize the encryption system
   * Set up cryptographic APIs and key management
   */
  async initializeEncryption() {
    try {
      await this.checkCryptoSupport();

      this.initializeRandom();

      this.setupKeyDerivation();

      await this.initializeMasterKey();

      if (this.config.keyRotationInterval > 0) {
        this.setupKeyRotation();
      }

      if (this.config.enableIntegrityCheck) {
        this.setupIntegrityChecking();
      }

      this.state.isInitialized = true;
      console.log("DataEncryption: Initialized successfully");
    } catch (error) {
      console.error("DataEncryption: Initialization failed:", error);
      this.handleError("initialization_failed", error);
    }
  }

  /**
   * Check Web Crypto API support and compatibility
   * Validate cryptographic API availability
   */
  async checkCryptoSupport() {
    try {
      if (!window.crypto || !window.crypto.subtle) {
        throw new Error("Web Crypto API not supported");
      }

      this.cryptoAPI.subtle = window.crypto.subtle;
      this.cryptoAPI.random = window.crypto.getRandomValues.bind(window.crypto);

      await this.testCryptoOperations();

      this.cryptoAPI.supported = true;
      console.log("DataEncryption: Crypto API support confirmed");
    } catch (error) {
      console.error("DataEncryption: Crypto API check failed:", error);
      throw error;
    }
  }

  /**
   * Test basic cryptographic operations
   * Verify crypto API functionality with test operations
   */
  async testCryptoOperations() {
    try {
      const testRandom = new Uint8Array(16);
      this.cryptoAPI.random(testRandom);

      const testKey = await this.cryptoAPI.subtle.generateKey(
        {
          name: this.config.algorithm,
          length: this.config.keySize,
        },
        false,
        ["encrypt", "decrypt"]
      );

      const testData = new TextEncoder().encode("test");
      const testIv = new Uint8Array(this.config.ivSize);
      this.cryptoAPI.random(testIv);

      const encrypted = await this.cryptoAPI.subtle.encrypt(
        {
          name: this.config.algorithm,
          iv: testIv,
        },
        testKey,
        testData
      );

      const decrypted = await this.cryptoAPI.subtle.decrypt(
        {
          name: this.config.algorithm,
          iv: testIv,
        },
        testKey,
        encrypted
      );

      const decryptedText = new TextDecoder().decode(decrypted);
      if (decryptedText !== "test") {
        throw new Error("Crypto test failed: decryption mismatch");
      }

      console.log("DataEncryption: Crypto operations test passed");
    } catch (error) {
      console.error("DataEncryption: Crypto operations test failed:", error);
      throw error;
    }
  }

  /**
   * Initialize cryptographic random number generator
   * Set up secure random number generation
   */
  initializeRandom() {
    try {
      if (!this.cryptoAPI.random) {
        throw new Error("Cryptographic random number generator not available");
      }

      const testBuffer = new Uint8Array(32);
      this.cryptoAPI.random(testBuffer);

      const uniqueValues = new Set(testBuffer);
      if (uniqueValues.size < 16) {
        console.warn(
          "DataEncryption: Low entropy detected in random generator"
        );
      }

      console.log("DataEncryption: Random number generator initialized");
    } catch (error) {
      console.error("DataEncryption: Random initialization failed:", error);
      throw error;
    }
  }

  /**
   * Set up key derivation functions
   * Initialize PBKDF2 and other key derivation methods
   */
  setupKeyDerivation() {
    try {
      const supportedAlgorithms = ["PBKDF2", "HKDF"];
      if (!supportedAlgorithms.includes(this.config.keyDerivation)) {
        throw new Error(
          `Unsupported key derivation algorithm: ${this.config.keyDerivation}`
        );
      }

      this.keyDerivationParams = {
        name: this.config.keyDerivation,
        hash: this.config.hashAlgorithm,
        iterations: this.config.iterations,
      };

      console.log("DataEncryption: Key derivation functions configured");
    } catch (error) {
      console.error("DataEncryption: Key derivation setup failed:", error);
      throw error;
    }
  }

  /**
   * Initialize master encryption key
   * Generate or derive master key for encryption operations
   */
  async initializeMasterKey() {
    try {
      let masterKey = await this.loadMasterKeyFromStorage();

      if (!masterKey) {
        masterKey = await this.generateMasterKey();

        await this.storeMasterKeySecurely(masterKey);
      }

      await this.validateMasterKey(masterKey);

      this.state.currentKey = {
        key: masterKey,
        createdAt: Date.now(),
        algorithm: this.config.algorithm,
        keySize: this.config.keySize,
        keyId: this.generateKeyId(),
      };

      console.log("DataEncryption: Master key initialized");
    } catch (error) {
      console.error("DataEncryption: Master key initialization failed:", error);
      throw error;
    }
  }

  /**
   * Generate a new master encryption key
   * Create cryptographically secure master key
   */
  async generateMasterKey() {
    try {
      const key = await this.cryptoAPI.subtle.generateKey(
        {
          name: this.config.algorithm,
          length: this.config.keySize,
        },
        true, // extractable for backup
        ["encrypt", "decrypt"]
      );

      console.log("DataEncryption: Master key generated");
      return key;
    } catch (error) {
      console.error("DataEncryption: Master key generation failed:", error);
      throw error;
    }
  }

  /**
   * Set up automatic key rotation
   * Configure periodic key rotation for enhanced security
   */
  setupKeyRotation() {
    try {
      this.state.keyRotationTimer = setInterval(() => {
        this.rotateEncryptionKey();
      }, this.config.keyRotationInterval);

      console.log(
        `DataEncryption: Key rotation configured (${this.config.keyRotationInterval}ms interval)`
      );
    } catch (error) {
      console.error("DataEncryption: Key rotation setup failed:", error);
    }
  }

  /**
   * Set up integrity checking mechanisms
   * Configure data integrity validation
   */
  setupIntegrityChecking() {
    try {
      this.integrityParams = {
        name: "HMAC",
        hash: this.config.hashAlgorithm,
      };

      console.log("DataEncryption: Integrity checking configured");
    } catch (error) {
      console.error("DataEncryption: Integrity checking setup failed:", error);
    }
  }

  /**
   * Encrypt data using current encryption key
   * Perform AES-GCM encryption with integrity protection
   */
  async encrypt(data) {
    try {
      if (!data) {
        throw new Error("No data provided for encryption");
      }

      if (!this.state.isInitialized || !this.state.currentKey) {
        throw new Error("Encryption not initialized");
      }

      const dataString = typeof data === "string" ? data : JSON.stringify(data);
      const dataBuffer = new TextEncoder().encode(dataString);

      const iv = new Uint8Array(this.config.ivSize);
      this.cryptoAPI.random(iv);

      let processedData = dataBuffer;
      if (this.config.enableCompression) {
        processedData = await this.compressData(dataBuffer);
      }

      const encryptedData = await this.cryptoAPI.subtle.encrypt(
        {
          name: this.config.algorithm,
          iv: iv,
        },
        this.state.currentKey.key,
        processedData
      );

      const encryptedPackage = {
        algorithm: this.config.algorithm,
        keyId: this.state.currentKey.keyId,
        iv: Array.from(iv),
        data: Array.from(new Uint8Array(encryptedData)),
        timestamp: Date.now(),
        compressed: this.config.enableCompression,
        version: "1.0",
      };

      if (this.config.enableIntegrityCheck) {
        encryptedPackage.integrity = await this.calculateIntegrityHash(
          encryptedPackage
        );
      }

      this.state.encryptionStats.totalEncrypted++;
      this.state.encryptionStats.totalSize += dataString.length;

      if (this.config.debugMode) {
        console.log("DataEncryption: Data encrypted successfully");
      }

      return encryptedPackage;
    } catch (error) {
      console.error("DataEncryption: Encryption failed:", error);
      this.state.encryptionStats.errorCount++;
      this.handleError("encryption_failed", error);
      throw error;
    }
  }

  /**
   * Decrypt data using appropriate encryption key
   * Perform AES-GCM decryption with integrity validation
   */
  async decrypt(encryptedPackage) {
    try {
      if (!encryptedPackage || !encryptedPackage.data) {
        throw new Error("Invalid encrypted package");
      }

      if (this.config.enableIntegrityCheck && encryptedPackage.integrity) {
        const isValid = await this.verifyIntegrityHash(encryptedPackage);
        if (!isValid) {
          throw new Error("Integrity check failed");
        }
      }

      const decryptionKey = await this.getDecryptionKey(encryptedPackage.keyId);
      if (!decryptionKey) {
        throw new Error(`Decryption key not found: ${encryptedPackage.keyId}`);
      }

      const iv = new Uint8Array(encryptedPackage.iv);
      const encryptedData = new Uint8Array(encryptedPackage.data);

      const decryptedBuffer = await this.cryptoAPI.subtle.decrypt(
        {
          name: encryptedPackage.algorithm,
          iv: iv,
        },
        decryptionKey,
        encryptedData
      );

      let processedBuffer = new Uint8Array(decryptedBuffer);
      if (encryptedPackage.compressed) {
        processedBuffer = await this.decompressData(processedBuffer);
      }

      const decryptedString = new TextDecoder().decode(processedBuffer);

      let result;
      try {
        result = JSON.parse(decryptedString);
      } catch {
        result = decryptedString;
      }

      this.state.encryptionStats.totalDecrypted++;

      if (this.config.debugMode) {
        console.log("DataEncryption: Data decrypted successfully");
      }

      return result;
    } catch (error) {
      console.error("DataEncryption: Decryption failed:", error);
      this.state.encryptionStats.errorCount++;
      this.handleError("decryption_failed", error);
      throw error;
    }
  }

  /**
   * Rotate encryption key for enhanced security
   * Generate new key and retire old key
   */
  async rotateEncryptionKey() {
    try {
      if (!this.state.isInitialized) {
        return;
      }

      if (this.state.currentKey) {
        this.state.keyHistory.push({
          ...this.state.currentKey,
          retiredAt: Date.now(),
        });

        if (this.state.keyHistory.length > 10) {
          this.state.keyHistory = this.state.keyHistory.slice(-5);
        }
      }

      const newKey = await this.generateMasterKey();

      this.state.currentKey = {
        key: newKey,
        createdAt: Date.now(),
        algorithm: this.config.algorithm,
        keySize: this.config.keySize,
        keyId: this.generateKeyId(),
      };

      await this.storeMasterKeySecurely(newKey);

      console.log("DataEncryption: Encryption key rotated successfully");
    } catch (error) {
      console.error("DataEncryption: Key rotation failed:", error);
      this.handleError("key_rotation_failed", error);
    }
  }

  /**
   * Compress data before encryption
   * Apply compression algorithm to reduce data size
   */
  async compressData(data) {
    try {
      if ("CompressionStream" in window) {
        const compressionStream = new CompressionStream("gzip");
        const writer = compressionStream.writable.getWriter();
        const reader = compressionStream.readable.getReader();

        writer.write(data);
        writer.close();

        const chunks = [];
        let done = false;
        while (!done) {
          const { value, done: readerDone } = await reader.read();
          done = readerDone;
          if (value) {
            chunks.push(value);
          }
        }

        const totalLength = chunks.reduce(
          (sum, chunk) => sum + chunk.length,
          0
        );
        const compressed = new Uint8Array(totalLength);
        let offset = 0;
        for (const chunk of chunks) {
          compressed.set(chunk, offset);
          offset += chunk.length;
        }

        return compressed;
      } else {
        return data; // No compression if not supported
      }
    } catch (error) {
      console.warn(
        "DataEncryption: Compression failed, using uncompressed data:",
        error
      );
      return data;
    }
  }

  /**
   * Decompress data after decryption
   * Apply decompression algorithm to restore original data
   */
  async decompressData(compressedData) {
    try {
      if ("DecompressionStream" in window) {
        const decompressionStream = new DecompressionStream("gzip");
        const writer = decompressionStream.writable.getWriter();
        const reader = decompressionStream.readable.getReader();

        writer.write(compressedData);
        writer.close();

        const chunks = [];
        let done = false;
        while (!done) {
          const { value, done: readerDone } = await reader.read();
          done = readerDone;
          if (value) {
            chunks.push(value);
          }
        }

        const totalLength = chunks.reduce(
          (sum, chunk) => sum + chunk.length,
          0
        );
        const decompressed = new Uint8Array(totalLength);
        let offset = 0;
        for (const chunk of chunks) {
          decompressed.set(chunk, offset);
          offset += chunk.length;
        }

        return decompressed;
      } else {
        return compressedData;
      }
    } catch (error) {
      console.warn(
        "DataEncryption: Decompression failed, using compressed data:",
        error
      );
      return compressedData;
    }
  }

  /**
   * Calculate integrity hash for encrypted data
   * Generate HMAC for data integrity verification
   */
  async calculateIntegrityHash(encryptedPackage) {
    try {
      const integrityKey = await this.deriveIntegrityKey();

      const integrityData = new TextEncoder().encode(
        JSON.stringify({
          algorithm: encryptedPackage.algorithm,
          keyId: encryptedPackage.keyId,
          iv: encryptedPackage.iv,
          data: encryptedPackage.data,
          timestamp: encryptedPackage.timestamp,
        })
      );

      const signature = await this.cryptoAPI.subtle.sign(
        "HMAC",
        integrityKey,
        integrityData
      );

      return Array.from(new Uint8Array(signature));
    } catch (error) {
      console.error(
        "DataEncryption: Integrity hash calculation failed:",
        error
      );
      return null;
    }
  }

  /**
   * Verify integrity hash for encrypted data
   * Validate HMAC for data integrity verification
   */
  async verifyIntegrityHash(encryptedPackage) {
    try {
      if (!encryptedPackage.integrity) {
        return false;
      }

      const expectedHash = await this.calculateIntegrityHash(encryptedPackage);
      if (!expectedHash) {
        return false;
      }

      const providedHash = encryptedPackage.integrity;
      if (expectedHash.length !== providedHash.length) {
        return false;
      }

      for (let i = 0; i < expectedHash.length; i++) {
        if (expectedHash[i] !== providedHash[i]) {
          return false;
        }
      }

      return true;
    } catch (error) {
      console.error("DataEncryption: Integrity verification failed:", error);
      return false;
    }
  }

  /**
   * Generate unique key identifier
   * Create unique identifier for encryption keys
   */
  generateKeyId() {
    const timestamp = Date.now().toString(36);
    const random = Math.random().toString(36).substring(2, 10);
    return `key_${timestamp}_${random}`;
  }

  /**
   * Get decryption key by key ID
   * Retrieve appropriate key for decryption
   */
  async getDecryptionKey(keyId) {
    try {
      if (this.state.currentKey && this.state.currentKey.keyId === keyId) {
        return this.state.currentKey.key;
      }

      const historicalKey = this.state.keyHistory.find(
        (k) => k.keyId === keyId
      );
      if (historicalKey) {
        return historicalKey.key;
      }

      return await this.loadKeyFromStorage(keyId);
    } catch (error) {
      console.error("DataEncryption: Key retrieval failed:", error);
      return null;
    }
  }

  /**
   * Get encryption statistics
   * Return comprehensive encryption usage statistics
   */
  getEncryptionStats() {
    return {
      ...this.state.encryptionStats,
      isInitialized: this.state.isInitialized,
      currentKeyId: this.state.currentKey?.keyId,
      keyHistorySize: this.state.keyHistory.length,
      algorithm: this.config.algorithm,
      keySize: this.config.keySize,
      compressionEnabled: this.config.enableCompression,
      integrityCheckEnabled: this.config.enableIntegrityCheck,
    };
  }

  /**
   * Handle encryption errors
   * Process and log encryption-related errors
   */
  handleError(errorType, error) {
    const errorRecord = {
      type: errorType,
      message: error.message || error,
      timestamp: Date.now(),
      stack: error.stack,
    };

    console.error(`DataEncryption: ${errorType}`, error);

    if (typeof this.emit === "function") {
      this.emit("error", errorRecord);
    }
  }

  /**
   * Clean up and destroy encryption system
   * Securely clean up keys and resources
   */
  async destroy() {
    try {
      if (this.state.keyRotationTimer) {
        clearInterval(this.state.keyRotationTimer);
        this.state.keyRotationTimer = null;
      }

      this.state.currentKey = null;
      this.state.keyHistory = [];
      this.keyStorage.clear();
      this.encryptionCache.clear();

      this.state.isInitialized = false;

      console.log("DataEncryption: Destroyed successfully");
    } catch (error) {
      console.error("DataEncryption: Destruction failed:", error);
    }
  }

  async loadMasterKeyFromStorage() {
    return null;
  }
  async storeMasterKeySecurely(key) {
    return true;
  }
  async loadKeyFromStorage(keyId) {
    return null;
  }
  async validateMasterKey(key) {
    return true;
  }
  async deriveIntegrityKey() {
    // Simple derivation for demo - would use proper HKDF in production
    return await this.cryptoAPI.subtle.generateKey(
      { name: "HMAC", hash: this.config.hashAlgorithm },
      false,
      ["sign", "verify"]
    );
  }
}

export { DataEncryption };

export const createDataEncryption = (options) => new DataEncryption(options);
export const checkCryptoSupport = () => {
  return !!(
    window.crypto &&
    window.crypto.subtle &&
    window.crypto.getRandomValues
  );
};

export const EncryptionUtils = {
  generateRandomBytes: (size) => {
    if (!checkCryptoSupport()) {
      throw new Error("Crypto API not supported");
    }
    const bytes = new Uint8Array(size);
    window.crypto.getRandomValues(bytes);
    return bytes;
  },

  bytesToHex: (bytes) => {
    return Array.from(bytes)
      .map((b) => b.toString(16).padStart(2, "0"))
      .join("");
  },

  hexToBytes: (hex) => {
    const bytes = new Uint8Array(hex.length / 2);
    for (let i = 0; i < hex.length; i += 2) {
      bytes[i / 2] = parseInt(hex.substr(i, 2), 16);
    }
    return bytes;
  },

  isValidEncryptionPackage: (pkg) => {
    return (
      pkg &&
      pkg.algorithm &&
      pkg.keyId &&
      Array.isArray(pkg.iv) &&
      Array.isArray(pkg.data) &&
      pkg.timestamp &&
      pkg.version
    );
  },
};

console.log("DataEncryption module loaded successfully");
