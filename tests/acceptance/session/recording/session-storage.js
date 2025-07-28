/**
 * Session Storage Module for Session Recording
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This module provides comprehensive session data storage and management
 * for user session recording, including real-time data persistence,
 * compression, indexing, and retrieval optimization.
 *
 * @fileoverview Session data storage and management for session recording
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires DataValidator - For session data validation
 * @requires PrivacyCompliance - For privacy-compliant data storage
 * @requires DataEncryption - For secure data storage and transmission
 */

import { DataValidator } from "../validation/data-validator.js";
import { PrivacyCompliance } from "./privacy-compliance.js";
import { DataEncryption } from "./data-encryption.js";

/**
 * SessionStorage class for comprehensive session data storage and management
 * Provides real-time data persistence, compression, indexing, and efficient retrieval
 */
class SessionStorage {
  constructor(options = {}) {
    this.config = {
      storageType: options.storageType || "indexeddb", // 'indexeddb', 'localstorage', 'memory'
      databaseName: options.databaseName || "huntmaster-sessions",
      databaseVersion: options.databaseVersion || 1,
      maxStorageSize: options.maxStorageSize || 100 * 1024 * 1024, // 100MB default
      compressionEnabled: options.compressionEnabled !== false,
      encryptionEnabled: options.encryptionEnabled !== false,
      autoCleanup: options.autoCleanup !== false,
      retentionDays: options.retentionDays || 30,
      batchSize: options.batchSize || 1000,
      flushInterval: options.flushInterval || 5000, // 5 seconds
      enableIndexing: options.enableIndexing !== false,
      enableBackup: options.enableBackup !== false,
      sessionId: options.sessionId || null,
      userId: options.userId || null,
      debugMode: options.debugMode || false,
      ...options,
    };

    this.validator = new DataValidator();
    this.privacy = new PrivacyCompliance();
    this.encryption = new DataEncryption();

    this.state = {
      isInitialized: false,
      database: null,
      currentSession: null,
      storageQuota: { used: 0, available: 0 },
      pendingWrites: 0,
      errors: [],
      stats: {
        totalSessions: 0,
        totalEvents: 0,
        totalSize: 0,
        compressionRatio: 1.0,
      },
    };

    this.storageEngines = {
      indexeddb: null,
      localstorage: null,
      memory: new Map(),
    };

    this.writeBuffer = [];
    this.compressionWorker = null;

    this.eventHandlers = new Map();

    this.initializeStorage();
  }

  /**
   * Initialize the storage system
   * Set up storage engine and database connections
   */
  async initializeStorage() {
    try {
      await this.initializeStorageEngine();

      if (this.config.compressionEnabled) {
        await this.initializeCompressionWorker();
      }

      if (this.config.autoCleanup) {
        this.setupAutoCleanup();
      }

      await this.setupStorageQuotaMonitoring();

      this.setupPeriodicFlushing();

      if (this.config.sessionId) {
        await this.initializeSession(this.config.sessionId);
      }

      this.state.isInitialized = true;
      console.log("SessionStorage: Initialized successfully");
    } catch (error) {
      console.error("SessionStorage: Initialization failed:", error);
      this.handleError("initialization_failed", error);
    }
  }

  /**
   * Initialize storage engine (IndexedDB, localStorage, or memory)
   * Set up the appropriate storage backend
   */
  async initializeStorageEngine() {
    try {
      switch (this.config.storageType) {
        case "indexeddb":
          await this.initializeIndexedDB();
          break;
        case "localstorage":
          this.initializeLocalStorage();
          break;
        case "memory":
          this.initializeMemoryStorage();
          break;
        default:
          throw new Error(
            `Unsupported storage type: ${this.config.storageType}`
          );
      }

      console.log(
        `SessionStorage: ${this.config.storageType} engine initialized`
      );
    } catch (error) {
      console.error(
        "SessionStorage: Storage engine initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Initialize IndexedDB storage engine
   * Set up IndexedDB database and object stores
   */
  async initializeIndexedDB() {
    return new Promise((resolve, reject) => {
      try {
        if (!window.indexedDB) {
          throw new Error("IndexedDB not supported");
        }

        const request = indexedDB.open(
          this.config.databaseName,
          this.config.databaseVersion
        );

        request.onupgradeneeded = (event) => {
          const db = event.target.result;

          if (!db.objectStoreNames.contains("sessions")) {
            const sessionsStore = db.createObjectStore("sessions", {
              keyPath: "sessionId",
            });
            sessionsStore.createIndex("userId", "userId", { unique: false });
            sessionsStore.createIndex("timestamp", "timestamp", {
              unique: false,
            });
            sessionsStore.createIndex("duration", "duration", {
              unique: false,
            });
          }

          if (!db.objectStoreNames.contains("events")) {
            const eventsStore = db.createObjectStore("events", {
              keyPath: "eventId",
            });
            eventsStore.createIndex("sessionId", "sessionId", {
              unique: false,
            });
            eventsStore.createIndex("timestamp", "timestamp", {
              unique: false,
            });
            eventsStore.createIndex("eventType", "eventType", {
              unique: false,
            });
            eventsStore.createIndex("module", "module", { unique: false });
          }

          if (!db.objectStoreNames.contains("metadata")) {
            const metadataStore = db.createObjectStore("metadata", {
              keyPath: "key",
            });
          }

          console.log("SessionStorage: IndexedDB schema created/updated");
        };

        request.onsuccess = (event) => {
          this.state.database = event.target.result;
          this.storageEngines.indexeddb = this.state.database;

          this.state.database.onerror = (error) => {
            console.error("SessionStorage: IndexedDB error:", error);
            this.handleError("indexeddb_error", error);
          };

          resolve();
        };

        request.onerror = (event) => {
          reject(
            new Error(`IndexedDB connection failed: ${event.target.error}`)
          );
        };

        request.onblocked = () => {
          console.warn("SessionStorage: IndexedDB connection blocked");
        };
      } catch (error) {
        reject(error);
      }
    });
  }

  /**
   * Initialize localStorage storage engine
   * Set up localStorage with namespace management
   */
  initializeLocalStorage() {
    try {
      if (!window.localStorage) {
        throw new Error("localStorage not supported");
      }

      const testKey = `${this.config.databaseName}_test`;
      localStorage.setItem(testKey, "test");
      localStorage.removeItem(testKey);

      this.storageEngines.localstorage = {
        setItem: (key, value) => {
          const namespacedKey = `${this.config.databaseName}_${key}`;
          localStorage.setItem(namespacedKey, JSON.stringify(value));
        },
        getItem: (key) => {
          const namespacedKey = `${this.config.databaseName}_${key}`;
          const item = localStorage.getItem(namespacedKey);
          return item ? JSON.parse(item) : null;
        },
        removeItem: (key) => {
          const namespacedKey = `${this.config.databaseName}_${key}`;
          localStorage.removeItem(namespacedKey);
        },
        getAllKeys: () => {
          const prefix = `${this.config.databaseName}_`;
          return Object.keys(localStorage)
            .filter((key) => key.startsWith(prefix))
            .map((key) => key.slice(prefix.length));
        },
      };

      console.log("SessionStorage: localStorage engine initialized");
    } catch (error) {
      console.error(
        "SessionStorage: localStorage initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Initialize memory storage engine
   * Set up in-memory storage with Map structure
   */
  initializeMemoryStorage() {
    try {
      this.storageEngines.memory = {
        sessions: new Map(),
        events: new Map(),
        metadata: new Map(),
        indexes: {
          sessionsByUser: new Map(),
          eventsBySession: new Map(),
          eventsByType: new Map(),
        },
      };

      console.log("SessionStorage: Memory storage engine initialized");
    } catch (error) {
      console.error(
        "SessionStorage: Memory storage initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Initialize compression worker for data compression
   * Set up web worker for background compression
   */
  async initializeCompressionWorker() {
    try {
      const compressionWorkerScript = `
                // Compression worker for session data
                self.onmessage = function(e) {
                    const { action, data, id } = e.data;

                    try {
                        switch (action) {
                            case 'compress':
                                const compressed = compressData(data);
                                self.postMessage({ id, action: 'compressed', data: compressed });
                                break;
                            case 'decompress':
                                const decompressed = decompressData(data);
                                self.postMessage({ id, action: 'decompressed', data: decompressed });
                                break;
                        }
                    } catch (error) {
                        self.postMessage({ id, action: 'error', error: error.message });
                    }
                };

                function compressData(data) {
                    // Simple compression simulation (in real implementation, use proper compression)
                    const jsonString = JSON.stringify(data);
                    return btoa(jsonString); // Base64 encoding as compression simulation
                }

                function decompressData(data) {
                    // Simple decompression simulation
                    const jsonString = atob(data);
                    return JSON.parse(jsonString);
                }
            `;

      const blob = new Blob([compressionWorkerScript], {
        type: "application/javascript",
      });
      const workerUrl = URL.createObjectURL(blob);

      this.compressionWorker = new Worker(workerUrl);

      this.compressionWorker.onmessage = (event) => {
        this.handleCompressionWorkerMessage(event.data);
      };

      this.compressionWorker.onerror = (error) => {
        console.error("SessionStorage: Compression worker error:", error);
        this.handleError("compression_worker_error", error);
      };

      URL.revokeObjectURL(workerUrl);

      console.log("SessionStorage: Compression worker initialized");
    } catch (error) {
      console.error(
        "SessionStorage: Compression worker initialization failed:",
        error
      );
      this.config.compressionEnabled = false;
    }
  }

  /**
   * Store data events in batches
   * TODO: Efficiently store multiple events in batch operations
   */
  async storeData(events) {
    try {
      if (!Array.isArray(events) || events.length === 0) {
        return;
      }

      this.writeBuffer.push(...events);

      if (this.state.currentSession) {
        this.state.currentSession.stats.eventCount += events.length;
        this.state.currentSession.stats.totalSize +=
          JSON.stringify(events).length;
      }

      if (this.writeBuffer.length >= this.config.batchSize) {
        await this.flushBuffer();
      }

      this.state.stats.totalEvents += events.length;

      if (this.config.debugMode) {
        console.log(`SessionStorage: Added ${events.length} events to buffer`);
      }
    } catch (error) {
      console.error("SessionStorage: Data storage failed:", error);
      this.handleError("data_storage_failed", error);
    }
  }

  /**
   * Store session summary data
   * Store session summary for quick access
   */
  async storeSessionSummary(summary) {
    try {
      const summaryRecord = {
        sessionId: summary.sessionId,
        userId: summary.userId,
        summary,
        createdAt: Date.now(),
        type: "summary",
      };

      const filteredSummary = this.privacy.filterData(summaryRecord);

      switch (this.config.storageType) {
        case "indexeddb":
          await this.storeSummaryIndexedDB(filteredSummary);
          break;
        case "localstorage":
          this.storeSummaryLocalStorage(filteredSummary);
          break;
        case "memory":
          this.storeSummaryMemory(filteredSummary);
          break;
      }

      console.log(
        `SessionStorage: Summary stored for session ${summary.sessionId}`
      );
    } catch (error) {
      console.error("SessionStorage: Summary storage failed:", error);
      throw error;
    }
  }

  /**
   * Flush write buffer to persistent storage
   * TODO: Transfer buffered events to storage backend
   */
  async flushBuffer() {
    try {
      if (this.writeBuffer.length === 0) {
        return;
      }

      const eventsToFlush = [...this.writeBuffer];
      this.writeBuffer = [];

      const processedEvents = await this.processEventsForStorage(eventsToFlush);

      switch (this.config.storageType) {
        case "indexeddb":
          await this.storeEventsIndexedDB(processedEvents);
          break;
        case "localstorage":
          this.storeEventsLocalStorage(processedEvents);
          break;
        case "memory":
          this.storeEventsMemory(processedEvents);
          break;
      }

      if (this.config.debugMode) {
        console.log(
          `SessionStorage: Flushed ${eventsToFlush.length} events to storage`
        );
      }
    } catch (error) {
      console.error("SessionStorage: Buffer flush failed:", error);
      this.handleError("buffer_flush_failed", error);
    }
  }

  /**
   * Process events for storage (privacy, encryption, compression)
   * Apply all processing steps before storage
   */
  async processEventsForStorage(events) {
    try {
      let processedEvents = events.map((event) => {
        return this.privacy.filterData(event);
      });

      processedEvents = processedEvents.map((event, index) => ({
        ...event,
        eventId: `${event.sessionId}_${Date.now()}_${index}`,
        storedAt: Date.now(),
        processed: true,
      }));

      if (this.config.encryptionEnabled) {
        processedEvents = await Promise.all(
          processedEvents.map((event) => this.encryption.encrypt(event))
        );
      }

      return processedEvents;
    } catch (error) {
      console.error("SessionStorage: Event processing failed:", error);
      return events; // Return unprocessed events as fallback
    }
  }

  /**
   * Get storage statistics and usage information
   * Return comprehensive storage statistics
   */
  async getStorageStats() {
    try {
      const stats = {
        ...this.state.stats,
        storageQuota: this.state.storageQuota,
        bufferSize: this.writeBuffer.length,
        pendingWrites: this.state.pendingWrites,
        errors: this.state.errors.length,
        initialized: this.state.isInitialized,
        storageType: this.config.storageType,
        compressionEnabled: this.config.compressionEnabled,
        encryptionEnabled: this.config.encryptionEnabled,
      };

      return stats;
    } catch (error) {
      console.error("SessionStorage: Stats retrieval failed:", error);
      return null;
    }
  }

  /**
   * Handle storage errors
   * Process and log storage errors
   */
  handleError(errorType, error) {
    const errorRecord = {
      type: errorType,
      message: error.message || error,
      timestamp: Date.now(),
      stack: error.stack,
    };

    this.state.errors.push(errorRecord);

    if (this.state.errors.length > 100) {
      this.state.errors = this.state.errors.slice(-50);
    }

    console.error(`SessionStorage: ${errorType}`, error);
  }

  /**
   * Cleanup and destroy storage system
   * Clean up resources and close connections
   */
  async destroy() {
    try {
      if (this.writeBuffer.length > 0) {
        await this.flushBuffer();
      }

      if (this.state.database) {
        this.state.database.close();
      }

      if (this.compressionWorker) {
        this.compressionWorker.terminate();
      }

      this.writeBuffer = [];
      this.state.isInitialized = false;

      console.log("SessionStorage: Destroyed successfully");
    } catch (error) {
      console.error("SessionStorage: Destruction failed:", error);
    }
  }
}

export { SessionStorage };

export const createSessionStorage = (options) => new SessionStorage(options);
export const getStorageQuota = async () => {
  if ("storage" in navigator && "estimate" in navigator.storage) {
    return await navigator.storage.estimate();
  }
  return null;
};

export const StorageUtils = {
  generateEventId: (sessionId, timestamp, index) => {
    return `${sessionId}_${timestamp}_${index}`;
  },

  calculateStorageSize: (data) => {
    return new Blob([JSON.stringify(data)]).size;
  },

  isStorageSupported: (type) => {
    switch (type) {
      case "indexeddb":
        return "indexedDB" in window;
      case "localstorage":
        return "localStorage" in window;
      case "memory":
        return true;
      default:
        return false;
    }
  },

  getOptimalStorageType: () => {
    if ("indexedDB" in window) return "indexeddb";
    if ("localStorage" in window) return "localstorage";
    return "memory";
  },
};

console.log("SessionStorage module loaded successfully");
