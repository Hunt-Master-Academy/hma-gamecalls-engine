/**
 * @file session-storage.js
 * @brief Session Storage Management System
 *
 * This module provides comprehensive storage management for session data,
 * user preferences, and persistent state with IndexedDB, localStorage,
 * and sessionStorage support including fallback mechanisms.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

/**
 * @class SessionStorage
 * @brief Unified storage interface for session persistence
 *
 * Provides high-level storage operations with automatic fallback,
 * data validation, quota management, and cross-browser compatibility.
 */
export class SessionStorage {
  constructor(options = {}) {
    this.options = {
      dbName: "HuntmasterSessions",
      dbVersion: 1,
      maxStorageSize: 50 * 1024 * 1024, // 50MB
      compressionEnabled: true,
      encryptionEnabled: false,
      ...options,
    };

    // Storage interfaces
    this.indexedDB = null;
    this.localStorage = null;
    this.sessionStorage = null;

    // Storage availability flags
    this.isIndexedDBAvailable = false;
    this.isLocalStorageAvailable = false;
    this.isSessionStorageAvailable = false;

    // Storage statistics
    this.storageStats = {
      totalSize: 0,
      itemCount: 0,
      lastCleanup: Date.now(),
      quotaExceeded: false,
    };

    // Error tracking
    this.lastError = null;
  }

  /**
   * Initialize all storage systems with feature detection and fallbacks
   */
  async initialize() {
    try {
      console.log("Initializing SessionStorage...");

      // Initialize IndexedDB for large data storage
      await this.initializeIndexedDB();

      // Initialize localStorage for settings and preferences
      this.initializeLocalStorage();

      // Initialize sessionStorage for temporary data
      this.initializeSessionStorage();

      // Perform initial storage assessment
      await this.assessStorageCapacity();

      console.log("SessionStorage initialization complete");
      return true;
    } catch (error) {
      console.error("SessionStorage initialization failed:", error);
      this.lastError = error;
      return false;
    }
  }

  /**
   * Initialize IndexedDB with database schema and upgrade handling
   */
  async initializeIndexedDB() {
    return new Promise((resolve, reject) => {
      if (!window.indexedDB) {
        console.warn("IndexedDB not supported");
        this.isIndexedDBAvailable = false;
        resolve(false);
        return;
      }

      const request = indexedDB.open(
        this.options.dbName,
        this.options.dbVersion
      );

      request.onerror = () => {
        console.error("IndexedDB initialization failed:", request.error);
        this.isIndexedDBAvailable = false;
        resolve(false);
      };

      request.onsuccess = (event) => {
        this.indexedDB = event.target.result;
        this.isIndexedDBAvailable = true;
        console.log("IndexedDB initialized successfully");
        resolve(true);
      };

      request.onupgradeneeded = (event) => {
        const db = event.target.result;

        // Create sessions object store
        if (!db.objectStoreNames.contains("sessions")) {
          const sessionStore = db.createObjectStore("sessions", {
            keyPath: "id",
          });
          sessionStore.createIndex("timestamp", "timestamp");
          sessionStore.createIndex("status", "status");
        }

        // Create preferences object store
        if (!db.objectStoreNames.contains("preferences")) {
          db.createObjectStore("preferences", { keyPath: "key" });
        }

        // Create recordings object store
        if (!db.objectStoreNames.contains("recordings")) {
          const recordingStore = db.createObjectStore("recordings", {
            keyPath: "id",
          });
          recordingStore.createIndex("sessionId", "sessionId");
          recordingStore.createIndex("timestamp", "timestamp");
        }

        console.log("IndexedDB schema created/upgraded");
      };
    });
  }

  /**
   * Initialize localStorage with feature detection and quota handling
   */
  initializeLocalStorage() {
    try {
      if (!window.localStorage) {
        console.warn("localStorage not supported");
        this.isLocalStorageAvailable = false;
        return false;
      }

      // Test localStorage availability
      const testKey = "__huntmaster_test__";
      localStorage.setItem(testKey, "test");
      localStorage.removeItem(testKey);

      this.localStorage = window.localStorage;
      this.isLocalStorageAvailable = true;
      console.log("localStorage initialized successfully");
      return true;
    } catch (error) {
      console.error("localStorage initialization failed:", error);
      this.isLocalStorageAvailable = false;
      return false;
    }
  }

  /**
   * Initialize sessionStorage for temporary data
   */
  initializeSessionStorage() {
    try {
      if (!window.sessionStorage) {
        console.warn("sessionStorage not supported");
        this.isSessionStorageAvailable = false;
        return false;
      }

      // Test sessionStorage availability
      const testKey = "__huntmaster_session_test__";
      sessionStorage.setItem(testKey, "test");
      sessionStorage.removeItem(testKey);

      this.sessionStorage = window.sessionStorage;
      this.isSessionStorageAvailable = true;
      console.log("sessionStorage initialized successfully");
      return true;
    } catch (error) {
      console.error("sessionStorage initialization failed:", error);
      this.isSessionStorageAvailable = false;
      return false;
    }
  }

  /**
   * Save session data with automatic storage selection and compression
   */
  async saveSession(sessionData) {
    try {
      const dataSize = this.calculateDataSize(sessionData);

      // Add metadata
      const enrichedData = {
        ...sessionData,
        timestamp: Date.now(),
        dataSize,
        version: this.options.dbVersion,
      };

      // Compress data if enabled and size warrants it
      let processedData = enrichedData;
      if (this.options.compressionEnabled && dataSize > 1024) {
        processedData = await this.compressData(enrichedData);
      }

      // Choose storage method based on data size and availability
      if (this.isIndexedDBAvailable && dataSize > 5000) {
        return await this.saveToIndexedDB("sessions", processedData);
      } else if (this.isLocalStorageAvailable) {
        return this.saveToLocalStorage(
          `session_${sessionData.id}`,
          processedData
        );
      } else if (this.isSessionStorageAvailable) {
        return this.saveToSessionStorage(
          `session_${sessionData.id}`,
          processedData
        );
      }

      throw new Error("No storage methods available");
    } catch (error) {
      console.error("Failed to save session:", error);
      this.lastError = error;
      throw error;
    }
  }

  /**
   * Load session data with automatic decompression and validation
   */
  async loadSession(sessionId) {
    try {
      let sessionData = null;

      // Try IndexedDB first for large data
      if (this.isIndexedDBAvailable) {
        sessionData = await this.loadFromIndexedDB("sessions", sessionId);
      }

      // Fallback to localStorage
      if (!sessionData && this.isLocalStorageAvailable) {
        sessionData = this.loadFromLocalStorage(`session_${sessionId}`);
      }

      // Fallback to sessionStorage
      if (!sessionData && this.isSessionStorageAvailable) {
        sessionData = this.loadFromSessionStorage(`session_${sessionId}`);
      }

      if (!sessionData) {
        return null;
      }

      // Decompress if necessary
      if (sessionData.compressed) {
        sessionData = await this.decompressData(sessionData);
      }

      // Validate data integrity
      if (!this.validateSessionData(sessionData)) {
        throw new Error("Session data validation failed");
      }

      return sessionData;
    } catch (error) {
      console.error("Failed to load session:", error);
      this.lastError = error;
      throw error;
    }
  }

  /**
   * Save to IndexedDB with transaction management
   */
  async saveToIndexedDB(storeName, data) {
    return new Promise((resolve, reject) => {
      const transaction = this.indexedDB.transaction([storeName], "readwrite");
      const store = transaction.objectStore(storeName);
      const request = store.put(data);

      request.onsuccess = () => {
        this.updateStorageStats("indexedDB", data);
        resolve(true);
      };

      request.onerror = () => {
        reject(new Error(`IndexedDB save failed: ${request.error}`));
      };

      transaction.onerror = () => {
        reject(new Error(`Transaction failed: ${transaction.error}`));
      };
    });
  }

  /**
   * Load from IndexedDB with error handling
   */
  async loadFromIndexedDB(storeName, key) {
    return new Promise((resolve, reject) => {
      const transaction = this.indexedDB.transaction([storeName], "readonly");
      const store = transaction.objectStore(storeName);
      const request = store.get(key);

      request.onsuccess = () => {
        resolve(request.result);
      };

      request.onerror = () => {
        reject(new Error(`IndexedDB load failed: ${request.error}`));
      };
    });
  }

  /**
   * Save to localStorage with quota handling
   */
  saveToLocalStorage(key, data) {
    try {
      const serialized = JSON.stringify(data);
      localStorage.setItem(key, serialized);
      this.updateStorageStats("localStorage", data);
      return true;
    } catch (error) {
      if (error.name === "QuotaExceededError") {
        this.handleQuotaExceeded("localStorage");
        // Retry after cleanup
        try {
          localStorage.setItem(key, JSON.stringify(data));
          return true;
        } catch (retryError) {
          throw new Error("localStorage quota exceeded even after cleanup");
        }
      }
      throw error;
    }
  }

  /**
   * Load from localStorage with error handling
   */
  loadFromLocalStorage(key) {
    try {
      const serialized = localStorage.getItem(key);
      return serialized ? JSON.parse(serialized) : null;
    } catch (error) {
      console.error("localStorage load failed:", error);
      return null;
    }
  }

  /**
   * Save to sessionStorage
   */
  saveToSessionStorage(key, data) {
    try {
      const serialized = JSON.stringify(data);
      sessionStorage.setItem(key, serialized);
      this.updateStorageStats("sessionStorage", data);
      return true;
    } catch (error) {
      if (error.name === "QuotaExceededError") {
        this.handleQuotaExceeded("sessionStorage");
        throw new Error("sessionStorage quota exceeded");
      }
      throw error;
    }
  }

  /**
   * Load from sessionStorage
   */
  loadFromSessionStorage(key) {
    try {
      const serialized = sessionStorage.getItem(key);
      return serialized ? JSON.parse(serialized) : null;
    } catch (error) {
      console.error("sessionStorage load failed:", error);
      return null;
    }
  }

  /**
   * Compress data using built-in compression algorithms
   */
  async compressData(data) {
    try {
      const serialized = JSON.stringify(data);

      // Use CompressionStream if available (modern browsers)
      if (window.CompressionStream) {
        const stream = new CompressionStream("gzip");
        const writer = stream.writable.getWriter();
        const reader = stream.readable.getReader();

        writer.write(new TextEncoder().encode(serialized));
        writer.close();

        const compressed = await reader.read();
        return {
          ...data,
          compressed: true,
          compressionType: "gzip",
          originalSize: serialized.length,
          compressedData: Array.from(compressed.value),
        };
      }

      // Fallback: simple string compression
      return {
        ...data,
        compressed: true,
        compressionType: "simple",
        originalSize: serialized.length,
        compressedData: this.simpleCompress(serialized),
      };
    } catch (error) {
      console.warn("Compression failed, storing uncompressed:", error);
      return data;
    }
  }

  /**
   * Decompress data
   */
  async decompressData(compressedData) {
    try {
      if (
        compressedData.compressionType === "gzip" &&
        window.DecompressionStream
      ) {
        const stream = new DecompressionStream("gzip");
        const writer = stream.writable.getWriter();
        const reader = stream.readable.getReader();

        writer.write(new Uint8Array(compressedData.compressedData));
        writer.close();

        const decompressed = await reader.read();
        const serialized = new TextDecoder().decode(decompressed.value);
        return JSON.parse(serialized);
      }

      if (compressedData.compressionType === "simple") {
        const serialized = this.simpleDecompress(compressedData.compressedData);
        return JSON.parse(serialized);
      }

      return compressedData;
    } catch (error) {
      console.error("Decompression failed:", error);
      throw error;
    }
  }

  /**
   * Simple string compression (fallback)
   */
  simpleCompress(str) {
    // Basic RLE compression
    return str.replace(/(.)\1+/g, (match, char) => `${char}${match.length}`);
  }

  /**
   * Simple string decompression (fallback)
   */
  simpleDecompress(str) {
    // Basic RLE decompression
    return str.replace(/(.)\d+/g, (match, char) => {
      const count = parseInt(match.slice(1));
      return char.repeat(count);
    });
  }

  /**
   * Calculate data size in bytes
   */
  calculateDataSize(data) {
    return new Blob([JSON.stringify(data)]).size;
  }

  /**
   * Validate session data integrity
   */
  validateSessionData(data) {
    return (
      data &&
      typeof data.id === "string" &&
      typeof data.timestamp === "number" &&
      data.timestamp > 0
    );
  }

  /**
   * Update storage statistics
   */
  updateStorageStats(storageType, data) {
    this.storageStats.totalSize += this.calculateDataSize(data);
    this.storageStats.itemCount += 1;
  }

  /**
   * Handle quota exceeded errors
   */
  handleQuotaExceeded(storageType) {
    console.warn(`${storageType} quota exceeded, attempting cleanup`);
    this.storageStats.quotaExceeded = true;

    // Implement cleanup strategy
    this.performStorageCleanup(storageType);
  }

  /**
   * Perform storage cleanup to free space
   */
  performStorageCleanup(storageType) {
    try {
      if (storageType === "localStorage" && this.isLocalStorageAvailable) {
        this.cleanupLocalStorage();
      } else if (
        storageType === "sessionStorage" &&
        this.isSessionStorageAvailable
      ) {
        this.cleanupSessionStorage();
      }

      this.storageStats.lastCleanup = Date.now();
      this.storageStats.quotaExceeded = false;
    } catch (error) {
      console.error("Storage cleanup failed:", error);
    }
  }

  /**
   * Clean up old localStorage entries
   */
  cleanupLocalStorage() {
    const keys = Object.keys(localStorage);
    const huntmasterKeys = keys.filter(
      (key) => key.startsWith("session_") || key.startsWith("huntmaster_")
    );

    // Sort by timestamp and remove oldest entries
    const entries = huntmasterKeys
      .map((key) => {
        try {
          const data = JSON.parse(localStorage.getItem(key));
          return { key, timestamp: data.timestamp || 0 };
        } catch {
          return { key, timestamp: 0 };
        }
      })
      .sort((a, b) => a.timestamp - b.timestamp);

    // Remove oldest 25% of entries
    const toRemove = Math.ceil(entries.length * 0.25);
    for (let i = 0; i < toRemove; i++) {
      localStorage.removeItem(entries[i].key);
    }

    console.log(`Cleaned up ${toRemove} localStorage entries`);
  }

  /**
   * Clean up sessionStorage entries
   */
  cleanupSessionStorage() {
    const keys = Object.keys(sessionStorage);
    const huntmasterKeys = keys.filter(
      (key) => key.startsWith("session_") || key.startsWith("huntmaster_")
    );

    // Remove all but most recent entries
    huntmasterKeys.slice(0, -5).forEach((key) => {
      sessionStorage.removeItem(key);
    });

    console.log("Cleaned up sessionStorage entries");
  }

  /**
   * Assess total storage capacity and usage
   */
  async assessStorageCapacity() {
    try {
      if (navigator.storage && navigator.storage.estimate) {
        const estimate = await navigator.storage.estimate();
        console.log("Storage quota:", estimate.quota);
        console.log("Storage usage:", estimate.usage);

        this.storageStats.totalQuota = estimate.quota;
        this.storageStats.totalUsage = estimate.usage;
      }
    } catch (error) {
      console.warn("Could not assess storage capacity:", error);
    }
  }

  /**
   * Get storage statistics
   */
  getStorageStats() {
    return {
      ...this.storageStats,
      indexedDBAvailable: this.isIndexedDBAvailable,
      localStorageAvailable: this.isLocalStorageAvailable,
      sessionStorageAvailable: this.isSessionStorageAvailable,
    };
  }

  /**
   * Clear all session storage
   */
  async clearAllStorage() {
    try {
      // Clear IndexedDB
      if (this.isIndexedDBAvailable) {
        const transaction = this.indexedDB.transaction(
          ["sessions"],
          "readwrite"
        );
        const store = transaction.objectStore("sessions");
        await new Promise((resolve, reject) => {
          const request = store.clear();
          request.onsuccess = () => resolve();
          request.onerror = () => reject(request.error);
        });
      }

      // Clear localStorage entries
      if (this.isLocalStorageAvailable) {
        const keys = Object.keys(localStorage).filter(
          (key) => key.startsWith("session_") || key.startsWith("huntmaster_")
        );
        keys.forEach((key) => localStorage.removeItem(key));
      }

      // Clear sessionStorage entries
      if (this.isSessionStorageAvailable) {
        const keys = Object.keys(sessionStorage).filter(
          (key) => key.startsWith("session_") || key.startsWith("huntmaster_")
        );
        keys.forEach((key) => sessionStorage.removeItem(key));
      }

      // Reset statistics
      this.storageStats = {
        totalSize: 0,
        itemCount: 0,
        lastCleanup: Date.now(),
        quotaExceeded: false,
      };

      console.log("All session storage cleared");
      return true;
    } catch (error) {
      console.error("Failed to clear storage:", error);
      this.lastError = error;
      return false;
    }
  }

  /**
   * Get last error information
   */
  getLastError() {
    return this.lastError;
  }
}

export default SessionStorage;
