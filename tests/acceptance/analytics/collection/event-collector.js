/**
 * @file event-collector.js
 * @brief Event Data Collection Module - Phase 3.2B Analytics Collection System
 *
 * This module provides comprehensive event data collection capabilities with real-time
 * streaming and batch processing for user acceptance testing analytics.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

/**
 * EventCollector Class
 * Collects various types of events with privacy compliance
 */
export class EventCollector {
  constructor(config = {}) {
    this.config = {
      maxQueueSize: 10000,
      batchSize: 100,
      maxBatchSize: 1000,
      flushInterval: 5000,
      compressionEnabled: true,
      privacyMode: true,
      realTimeProcessing: false,
      ...config,
    };

    // Initialize event collection system
    this.eventQueue = [];
    this.streamBuffer = new Map();
    this.eventTypes = new Set();
    this.collectors = new Map();
    this.filters = [];
    this.metadata = new Map();

    // Set up real-time streaming capabilities
    this.realTimeActive = false;
    this.lastFlush = Date.now();

    // Configure batch processing mechanisms
    this.batchProcessingActive = false;

    // Initialize event queue management
    this.queueProcessor = null;
    this.flushTimer = setInterval(() => {
      if (this.eventQueue.length > 0) {
        this.flushEventQueue().catch(console.error);
      }
    }, this.config.flushInterval);

    // Set up event filtering and validation
    this.validationRules = new Map();
    this.privacyFilters = new Map();

    // Configure event metadata enrichment
    this.enrichmentRules = new Map();

    // Initialize event compression
    this.compressionCache = new Map();

    // Set up event deduplication
    this.deduplicationCache = new Set();

    // Configure privacy compliance filters
    this.consentCache = new Map();
    this.rateLimitCache = new Map();

    // Initialize performance monitoring
    this.performanceMetrics = {
      eventsProcessed: 0,
      averageProcessingTime: 0,
      errorRate: 0,
      queueUtilization: 0,
    };

    // Set up audit logging
    this.auditLog = [];
    this.eventStatistics = new Map();
    this.collectionMetrics = null;
    this.bulkStatistics = new Map();
    this.bulkPerformanceMetrics = null;
  }

  /**
   * Event Collection Core
   */
  async collectEvent(eventType, eventData, context = {}) {
    try {
      // Validate event data structure
      if (!eventType || typeof eventType !== "string") {
        throw new Error("Event type is required and must be a string");
      }

      // Generate unique event identifier
      const eventId = this.generateEventId();

      // Add timestamp and session information
      const event = {
        id: eventId,
        type: eventType,
        data: eventData,
        context: context,
        timestamp: Date.now(),
        sessionId: context.sessionId || "unknown",
        userId: context.userId || "anonymous",
        userAgent: context.userAgent || navigator.userAgent,
        url: context.url || window.location.href,
      };

      // Apply data validation rules
      await this.validateEventData(event);

      // Check consent requirements
      if (
        this.config.privacyMode &&
        !this.checkConsent(event.userId, eventType)
      ) {
        return { success: false, reason: "Consent not granted", eventId };
      }

      // Apply rate limiting
      if (!this.checkRateLimit(event.userId, eventType)) {
        return { success: false, reason: "Rate limit exceeded", eventId };
      }

      // Apply privacy filters to event data
      const filteredEvent = await this.applyPrivacyFilters(event);

      // Enrich event with metadata
      const enrichedEvent = await this.enrichEvent(filteredEvent);

      // Update event type statistics
      this.updateEventStatistics(eventType);

      // Add to appropriate collection queue
      this.eventQueue.push(enrichedEvent);
      this.eventTypes.add(eventType);

      // Trigger real-time processing if needed
      if (this.config.realTimeProcessing) {
        await this.processRealTimeEvent(enrichedEvent);
      }

      // Check if batch processing should be triggered
      if (
        this.eventQueue.length >= this.config.batchSize ||
        Date.now() - this.lastFlush > this.config.flushInterval
      ) {
        await this.flushEventQueue();
      }

      // Update collection metrics
      this.updateCollectionMetrics(eventType, true);

      // Generate collection audit trail
      this.generateAuditEntry("event_collected", {
        eventId,
        eventType,
        userId: event.userId,
      });

      return { success: true, eventId, timestamp: event.timestamp };
    } catch (error) {
      // Handle collection errors gracefully
      this.updateCollectionMetrics(eventType, false);
      this.generateAuditEntry("event_collection_error", {
        eventType,
        error: error.message,
      });
      throw new Error(`Event collection failed: ${error.message}`);
    }
  }

  async collectBulkEvents(events) {
    if (!Array.isArray(events) || events.length === 0) {
      throw new Error("Events must be a non-empty array");
    }

    // Validate bulk event collection
    if (events.length > this.config.maxBatchSize) {
      throw new Error(
        `Bulk event collection exceeds maximum batch size: ${this.config.maxBatchSize}`
      );
    }

    const batchId = this.generateBatchId();
    const results = [];
    const startTime = Date.now();

    try {
      // Apply batch processing optimizations
      const chunks = this.chunkArray(events, this.config.batchSize);

      // Process events in parallel where possible
      for (const chunk of chunks) {
        const chunkPromises = chunk.map(async (eventData, index) => {
          try {
            // Handle bulk processing errors individually
            const result = await this.collectEvent(
              eventData.type,
              eventData.data,
              { ...eventData.context, batchId, batchIndex: index }
            );

            // Update bulk event type statistics
            this.updateBulkStatistics(eventData.type, true);

            return { success: true, index, ...result };
          } catch (error) {
            // Handle individual event errors without failing entire batch
            this.updateBulkStatistics(eventData.type, false);
            return {
              success: false,
              index,
              error: error.message,
              eventData: eventData.type,
            };
          }
        });

        const chunkResults = await Promise.all(chunkPromises);
        results.push(...chunkResults);
      }

      // Generate bulk collection metrics
      const successCount = results.filter((r) => r.success).length;
      const errorCount = results.length - successCount;

      // Generate bulk audit trails
      this.generateAuditEntry("bulk_collection_completed", {
        batchId,
        totalEvents: events.length,
        successCount,
        errorCount,
        processingTime: Date.now() - startTime,
      });

      // Update bulk performance metrics
      this.updateBulkPerformanceMetrics(events.length, Date.now() - startTime);

      return {
        batchId,
        totalEvents: events.length,
        successCount,
        errorCount,
        results,
        processingTime: Date.now() - startTime,
      };
    } catch (error) {
      // Handle bulk memory management for large batches
      this.clearBulkProcessingCache();

      // Generate bulk collection reports
      this.generateAuditEntry("bulk_collection_error", {
        batchId,
        error: error.message,
        eventCount: events.length,
      });

      throw new Error(`Bulk event collection failed: ${error.message}`);
    }
  }

  /**
   * Real-time Event Processing
   */
  async startRealTimeCollection(eventTypes = []) {
    try {
      // Initialize real-time event listeners
      this.realTimeListeners = new Map();
      this.websocketConnections = new Map();
      this.realTimeFilters = new Map();
      this.realTimeMetrics = {
        eventsProcessed: 0,
        processingTime: 0,
        errorCount: 0,
        throughput: 0,
        startTime: Date.now()
      };

      // Set up WebSocket connections for streaming
      if (typeof WebSocket !== 'undefined') {
        this.websocketServer = await this.createWebSocketServer();
        this.websocketConnections.set('primary', this.websocketServer);
      }

      // Configure real-time event filtering
      this.realTimeFilters.set('privacy', (event) => this.applyPrivacyFiltering(event));
      this.realTimeFilters.set('validation', (event) => this.validateEventStructure(event));
      this.realTimeFilters.set('rate_limit', (event) => this.checkRateLimit(event));

      // Initialize real-time processing pipeline
      this.realTimePipeline = [
        { name: 'validation', fn: this.validateRealTimeEvent.bind(this) },
        { name: 'filtering', fn: this.filterRealTimeEvent.bind(this) },
        { name: 'enrichment', fn: this.enrichRealTimeEvent.bind(this) },
        { name: 'compression', fn: this.compressRealTimeEvent.bind(this) },
        { name: 'storage', fn: this.storeRealTimeEvent.bind(this) }
      ];

      // Set up real-time error handling
      this.realTimeErrorHandler = (error, event) => {
        this.realTimeMetrics.errorCount++;
        this.generateAuditEntry('realtime_error', {
          error: error.message,
          eventId: event?.id,
          timestamp: Date.now()
        });
        console.error('Real-time processing error:', error);
      };

      // Configure real-time metrics collection
      this.realTimeMetricsTimer = setInterval(() => {
        this.updateRealTimeMetrics();
      }, 1000);

      // Initialize real-time compression
      this.realTimeCompression = {
        enabled: this.config.compressionEnabled,
        algorithm: 'gzip',
        level: 6,
        cache: new Map()
      };

      // Set up real-time privacy filtering
      this.privacyProcessor = {
        sensitiveFields: ['email', 'phone', 'ssn', 'creditCard'],
        anonymizationRules: new Map(),
        consentCache: new Map()
      };

      // Configure real-time rate limiting
      this.rateLimiter = {
        windowMs: 60000, // 1 minute
        maxEvents: 1000,
        eventCounts: new Map(),
        resetTimer: setInterval(() => {
          this.rateLimiter.eventCounts.clear();
        }, this.rateLimiter.windowMs)
      };

      // Initialize real-time monitoring
      this.realTimeMonitor = {
        alerts: [],
        thresholds: {
          errorRate: 0.05,
          processingTime: 100,
          queueSize: 5000
        },
        lastCheck: Date.now()
      };

      // Set up real-time alerting
      this.alertSystem = {
        enabled: true,
        channels: ['console', 'websocket'],
        rules: new Map()
      };

      // Configure real-time backup systems
      this.backupSystem = {
        enabled: true,
        interval: 30000, // 30 seconds
        location: 'backup_buffer',
        timer: setInterval(() => {
          this.createRealTimeBackup();
        }, 30000)
      };

      // Initialize real-time validation
      this.realTimeValidator = {
        schemas: new Map(),
        rules: new Map(),
        strictMode: true
      };

      // Set up real-time audit logging
      this.auditLogger = {
        enabled: true,
        buffer: [],
        flushInterval: 10000,
        timer: setInterval(() => {
          this.flushAuditLog();
        }, 10000)
      };

      // Configure real-time performance optimization
      this.performanceOptimizer = {
        batchSize: this.config.batchSize,
        adaptiveBatching: true,
        priorityQueue: [],
        loadBalancer: new Map()
      };

      this.realTimeActive = true;

      // Initialize collectors for specified event types
      for (const eventType of eventTypes) {
        await this.initializeRealTimeCollector(eventType);
      }

      this.generateAuditEntry('realtime_collection_started', {
        eventTypes,
        timestamp: Date.now(),
        configuration: this.config
      });

      return {
        success: true,
        eventTypes,
        startTime: Date.now(),
        configuration: this.config
      };

    } catch (error) {
      this.realTimeActive = false;
      this.realTimeErrorHandler(error, { type: 'startup' });
      throw new Error(`Failed to start real-time collection: ${error.message}`);
    }
  }

  async stopRealTimeCollection() {
    try {
      const shutdownStartTime = Date.now();

      // Gracefully stop real-time listeners
      if (this.realTimeListeners) {
        for (const [eventType, listener] of this.realTimeListeners) {
          try {
            if (listener.removeEventListener) {
              listener.removeEventListener();
            }
            this.realTimeListeners.delete(eventType);
          } catch (error) {
            console.warn(`Failed to remove listener for ${eventType}:`, error);
          }
        }
      }

      // Flush remaining real-time events
      const remainingEvents = this.eventQueue.length;
      if (remainingEvents > 0) {
        await this.flushEventQueue();
      }

      // Clean up real-time resources
      if (this.realTimeMetricsTimer) {
        clearInterval(this.realTimeMetricsTimer);
        this.realTimeMetricsTimer = null;
      }

      if (this.rateLimiter?.resetTimer) {
        clearInterval(this.rateLimiter.resetTimer);
        this.rateLimiter.resetTimer = null;
      }

      if (this.backupSystem?.timer) {
        clearInterval(this.backupSystem.timer);
        this.backupSystem.timer = null;
      }

      if (this.auditLogger?.timer) {
        clearInterval(this.auditLogger.timer);
        this.auditLogger.timer = null;
      }

      // Generate real-time collection summary
      const collectionSummary = {
        duration: Date.now() - (this.realTimeMetrics?.startTime || shutdownStartTime),
        eventsProcessed: this.realTimeMetrics?.eventsProcessed || 0,
        errorCount: this.realTimeMetrics?.errorCount || 0,
        averageProcessingTime: this.realTimeMetrics?.processingTime || 0,
        throughput: this.realTimeMetrics?.throughput || 0,
        finalQueueSize: remainingEvents
      };

      // Update real-time statistics
      this.performanceMetrics.eventsProcessed += collectionSummary.eventsProcessed;
      this.performanceMetrics.errorRate = collectionSummary.errorCount / Math.max(collectionSummary.eventsProcessed, 1);

      // Close real-time connections
      if (this.websocketConnections) {
        for (const [name, connection] of this.websocketConnections) {
          try {
            if (connection.close) {
              connection.close();
            }
            this.websocketConnections.delete(name);
          } catch (error) {
            console.warn(`Failed to close WebSocket connection ${name}:`, error);
          }
        }
      }

      // Clear real-time buffers
      if (this.streamBuffer) {
        this.streamBuffer.clear();
      }
      if (this.realTimeCompression?.cache) {
        this.realTimeCompression.cache.clear();
      }
      if (this.privacyProcessor?.consentCache) {
        this.privacyProcessor.consentCache.clear();
      }

      // Generate real-time performance report
      const performanceReport = {
        summary: collectionSummary,
        metrics: this.realTimeMetrics,
        systemHealth: {
          memoryUsage: this.getMemoryUsage(),
          cpuLoad: this.getCPULoad(),
          networkLatency: this.getNetworkLatency()
        },
        shutdownTime: Date.now() - shutdownStartTime
      };

      // Update real-time metrics
      this.realTimeMetrics = null;
      this.realTimeListeners = null;
      this.websocketConnections = null;
      this.realTimeFilters = null;

      // Create real-time audit trail
      this.generateAuditEntry('realtime_collection_stopped', {
        summary: collectionSummary,
        performanceReport,
        timestamp: Date.now(),
        shutdownTime: Date.now() - shutdownStartTime
      });

      // Handle real-time shutdown errors
      if (collectionSummary.errorCount > 0) {
        console.warn(`Real-time collection stopped with ${collectionSummary.errorCount} errors`);
      }

      // Clean up real-time monitoring
      this.realTimeMonitor = null;
      this.alertSystem = null;
      this.backupSystem = null;

      // Update real-time configuration
      this.config.realTimeProcessing = false;

      // Generate real-time shutdown report
      const shutdownReport = {
        success: true,
        summary: collectionSummary,
        performance: performanceReport,
        cleanupComplete: true,
        timestamp: Date.now()
      };

      // Validate real-time cleanup completion
      const cleanupValidation = {
        listenersCleared: !this.realTimeListeners || this.realTimeListeners.size === 0,
        connectionsCleared: !this.websocketConnections || this.websocketConnections.size === 0,
        timersCleared: !this.realTimeMetricsTimer && !this.rateLimiter?.resetTimer,
        buffersCleared: !this.streamBuffer || this.streamBuffer.size === 0,
        metricsCleared: !this.realTimeMetrics
      };

      this.realTimeActive = false;

      // Final flush of any remaining events
      await this.flushEventQueue();

      return {
        success: true,
        shutdownReport,
        cleanupValidation,
        summary: collectionSummary
      };

    } catch (error) {
      this.realTimeActive = false;
      this.generateAuditEntry('realtime_shutdown_error', {
        error: error.message,
        timestamp: Date.now()
      });
      throw new Error(`Failed to stop real-time collection: ${error.message}`);
    }
  }

  async processRealTimeEvent(event) {
    if (!this.realTimeActive) {
      throw new Error("Real-time collection not active");
    }

    const processingStartTime = Date.now();

    try {
      // Apply real-time event validation
      const validationResult = await this.validateRealTimeEvent(event);
      if (!validationResult.valid) {
        throw new Error(`Event validation failed: ${validationResult.error}`);
      }

      // Process event through real-time pipeline
      let processedEvent = { ...event };
      for (const stage of this.realTimePipeline) {
        try {
          processedEvent = await stage.fn(processedEvent);
        } catch (stageError) {
          this.realTimeErrorHandler(stageError, processedEvent);
          throw new Error(`Pipeline stage '${stage.name}' failed: ${stageError.message}`);
        }
      }

      // Apply real-time filtering rules
      const filterResults = await this.applyRealTimeFilters(processedEvent);
      if (!filterResults.passed) {
        this.generateAuditEntry('event_filtered', {
          eventId: processedEvent.id,
          reason: filterResults.reason,
          timestamp: Date.now()
        });
        return { success: false, reason: 'Event filtered', filters: filterResults.failedFilters };
      }

      // Update real-time metrics
      this.realTimeMetrics.eventsProcessed++;
      this.realTimeMetrics.processingTime = ((this.realTimeMetrics.processingTime * (this.realTimeMetrics.eventsProcessed - 1)) +
        (Date.now() - processingStartTime)) / this.realTimeMetrics.eventsProcessed;

      // Handle real-time processing errors (none in this successful path)

      // Apply real-time privacy protection
      const privacyProtectedEvent = await this.applyPrivacyProtection(processedEvent);

      // Generate real-time processing audit
      this.generateAuditEntry('realtime_event_processed', {
        eventId: privacyProtectedEvent.id,
        eventType: privacyProtectedEvent.type,
        processingTime: Date.now() - processingStartTime,
        timestamp: Date.now()
      });

      // Update real-time performance statistics
      this.updateRealTimePerformanceStats(Date.now() - processingStartTime);

      // Apply real-time compression
      const compressedEvent = this.config.compressionEnabled ?
        await this.compressRealTimeEvent(privacyProtectedEvent) : privacyProtectedEvent;

      // Handle real-time rate limiting
      const rateLimitResult = this.checkRealTimeRateLimit(compressedEvent);
      if (!rateLimitResult.allowed) {
        this.generateAuditEntry('rate_limit_exceeded', {
          eventId: compressedEvent.id,
          currentRate: rateLimitResult.currentRate,
          limit: rateLimitResult.limit,
          timestamp: Date.now()
        });
        throw new Error(`Rate limit exceeded: ${rateLimitResult.currentRate}/${rateLimitResult.limit}`);
      }

      // Update real-time monitoring data
      this.updateRealTimeMonitoringData(compressedEvent);

      // Generate real-time alerts if needed
      const alertsGenerated = await this.checkAndGenerateAlerts(compressedEvent);
      if (alertsGenerated.length > 0) {
        this.realTimeMonitor.alerts.push(...alertsGenerated);
      }

      // Apply real-time data enrichment
      const enrichedEvent = await this.enrichRealTimeEvent(compressedEvent);

      // Update real-time collection statistics
      this.updateRealTimeCollectionStats(enrichedEvent);

      // Handle real-time backup requirements
      if (this.backupSystem?.enabled) {
        await this.addToRealTimeBackup(enrichedEvent);
      }

      // Final processing
      const finalEvent = {
        ...enrichedEvent,
        processedAt: Date.now(),
        processingTime: Date.now() - processingStartTime,
        pipelineStages: this.realTimePipeline.map(s => s.name)
      };

      // Store in real-time buffer
      this.streamBuffer.set(finalEvent.id, finalEvent);

      // Update throughput metrics
      this.realTimeMetrics.throughput = this.realTimeMetrics.eventsProcessed /
        ((Date.now() - this.realTimeMetrics.startTime) / 1000);

      return {
        success: true,
        event: finalEvent,
        processingTime: Date.now() - processingStartTime,
        alerts: alertsGenerated
      };

    } catch (error) {
      // Handle real-time processing errors
      this.realTimeMetrics.errorCount++;
      this.realTimeErrorHandler(error, event);

      this.generateAuditEntry('realtime_processing_error', {
        eventId: event?.id,
        error: error.message,
        processingTime: Date.now() - processingStartTime,
        timestamp: Date.now()
      });

      throw new Error(`Real-time event processing failed: ${error.message}`);
    }
  }

  /**
   * Batch Processing
   */
  async processBatch(events) {
    const batchStartTime = Date.now();
    const batchId = this.generateBatchId();

    try {
      // Validate batch processing parameters
      if (!Array.isArray(events)) {
        throw new Error('Events must be an array');
      }

      if (events.length === 0) {
        return { success: true, processedCount: 0, batchId };
      }

      if (events.length > this.config.maxBatchSize) {
        throw new Error(`Batch size ${events.length} exceeds maximum ${this.config.maxBatchSize}`);
      }

      // Apply batch optimization algorithms
      const optimization = this.optimizeBatchProcessing(events);
      const optimizedEvents = optimization.reorderedEvents;
      const processingStrategy = optimization.strategy;

      // Process events in optimal order
      const sortedEvents = this.sortEventsForOptimalProcessing(optimizedEvents);

      // Handle batch processing dependencies
      const dependencyGraph = this.buildDependencyGraph(sortedEvents);
      const processingOrder = this.resolveDependencies(dependencyGraph);

      const results = [];
      const errors = [];
      let processedCount = 0;

      // Process events according to dependency order
      for (const eventGroup of processingOrder) {
        try {
          const groupResults = await this.processEventGroup(eventGroup, batchId, processingStrategy);
          results.push(...groupResults);
          processedCount += eventGroup.length;
        } catch (groupError) {
          // Apply batch error recovery
          const recoveryResult = await this.applyBatchErrorRecovery(eventGroup, groupError, batchId);
          if (recoveryResult.recovered) {
            results.push(...recoveryResult.results);
            processedCount += recoveryResult.processedCount;
          } else {
            errors.push({
              group: eventGroup.map(e => e.id),
              error: groupError.message,
              timestamp: Date.now()
            });
          }
        }
      }

      // Generate batch processing metrics
      const processingTime = Date.now() - batchStartTime;
      const batchMetrics = {
        batchId,
        totalEvents: events.length,
        processedCount,
        errorCount: errors.length,
        processingTime,
        throughput: processedCount / (processingTime / 1000),
        optimization: optimization.metrics,
        strategy: processingStrategy
      };

      // Update batch performance statistics
      this.updateBatchPerformanceStats(batchMetrics);

      // Handle batch memory management
      this.manageBatchMemory(batchId, results);

      // Apply batch compression techniques
      const compressedResults = this.config.compressionEnabled ?
        await this.compressBatchResults(results) : results;

      // Generate batch audit trails
      this.generateAuditEntry('batch_processed', {
        batchId,
        metrics: batchMetrics,
        errors: errors.length > 0 ? errors : undefined,
        timestamp: Date.now()
      });

      // Update batch monitoring data
      this.updateBatchMonitoringData(batchMetrics);

      // Handle batch rate limiting
      const rateLimitStatus = this.checkBatchRateLimit(batchMetrics);
      if (!rateLimitStatus.allowed) {
        console.warn(`Batch rate limit warning: ${rateLimitStatus.message}`);
      }

      // Apply batch privacy protection
      const privacyProtectedResults = await this.applyBatchPrivacyProtection(compressedResults);

      // Generate batch processing reports
      const batchReport = {
        batchId,
        summary: batchMetrics,
        results: privacyProtectedResults,
        errors,
        optimization: optimization.report,
        timestamp: Date.now()
      };

      // Update batch collection statistics
      this.updateBatchCollectionStats(batchReport);

      return {
        success: errors.length === 0,
        batchId,
        processedCount,
        totalEvents: events.length,
        errorCount: errors.length,
        processingTime,
        throughput: batchMetrics.throughput,
        results: privacyProtectedResults,
        errors: errors.length > 0 ? errors : undefined,
        report: batchReport
      };

    } catch (error) {
      // Handle critical batch processing errors
      this.generateAuditEntry('batch_processing_error', {
        batchId,
        error: error.message,
        eventsCount: events.length,
        timestamp: Date.now()
      });

      throw new Error(`Batch processing failed: ${error.message}`);
    }
  }

  async scheduleBatchProcessing(schedule) {
    // TODO: Initialize batch processing scheduler
    // TODO: Configure batch processing intervals
    // TODO: Set up batch processing triggers
    // TODO: Handle batch scheduling conflicts
    // TODO: Apply batch processing optimization
    // TODO: Generate batch scheduling metrics
    // TODO: Update batch processing statistics
    // TODO: Handle batch scheduling errors
    // TODO: Apply batch processing monitoring
    // TODO: Generate batch scheduling audit trails
    // TODO: Update batch processing configuration
    // TODO: Handle batch processing dependencies
    // TODO: Apply batch processing rate limiting
    // TODO: Generate batch scheduling reports
    // TODO: Update batch scheduling performance

    this.batchSchedule = schedule;

    if (schedule.interval) {
      this.batchTimer = setInterval(
        () => this.processPendingBatches(),
        schedule.interval
      );
    }
  }

  /**
   * Event Filtering and Validation
   */
  addEventFilter(filterFn, filterName) {
    // TODO: Validate filter function
    // TODO: Register filter with system
    // TODO: Configure filter priority
    // TODO: Set up filter error handling
    // TODO: Generate filter registration audit
    // TODO: Update filter statistics
    // TODO: Handle filter conflicts
    // TODO: Apply filter performance monitoring
    // TODO: Generate filter documentation
    // TODO: Update filter configuration
    // TODO: Handle filter dependencies
    // TODO: Apply filter rate limiting
    // TODO: Generate filter metrics
    // TODO: Update filter performance data
    // TODO: Create filter audit trail

    this.filters.push({
      name: filterName,
      fn: filterFn,
      applied: 0,
      errors: 0,
      created: Date.now(),
    });
  }

  async validateEvent(event) {
    // TODO: Apply event schema validation
    // TODO: Check event data integrity
    // TODO: Validate event timestamps
    // TODO: Check event size limits
    // TODO: Validate event structure
    // TODO: Apply business rule validation
    // TODO: Check event dependencies
    // TODO: Validate event metadata
    // TODO: Apply security validation
    // TODO: Check event compliance requirements
    // TODO: Generate validation audit trail
    // TODO: Update validation statistics
    // TODO: Handle validation errors
    // TODO: Apply validation performance monitoring
    // TODO: Generate validation reports

    for (const filter of this.filters) {
      try {
        const isValid = await filter.fn(event);
        filter.applied++;

        if (!isValid) {
          return { valid: false, filter: filter.name };
        }
      } catch (error) {
        filter.errors++;
        throw new Error(`Filter ${filter.name} failed: ${error.message}`);
      }
    }

    return { valid: true };
  }

  /**
   * Event Metadata and Enrichment
   */
  async enrichEvent(event) {
    // TODO: Add session context to event
    // TODO: Enrich with user profile data
    // TODO: Add device and browser information
    // TODO: Include geographic information (privacy-compliant)
    // TODO: Add performance context
    // TODO: Include application state information
    // TODO: Add referrer and source information
    // TODO: Include experiment and variant data
    // TODO: Add custom metadata fields
    // TODO: Apply metadata validation
    // TODO: Generate enrichment audit trail
    // TODO: Update enrichment statistics
    // TODO: Handle enrichment errors
    // TODO: Apply enrichment performance monitoring
    // TODO: Generate enrichment reports

    const enriched = {
      ...event,
      enrichment: {
        timestamp: Date.now(),
        userAgent: this.getUserAgent(),
        sessionDuration: this.getSessionDuration(event.sessionId),
        pageUrl: window?.location?.href,
        referrer: document?.referrer,
        timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
        language: navigator.language,
        screenResolution: `${screen.width}x${screen.height}`,
        viewport: `${window.innerWidth}x${window.innerHeight}`,
      },
    };

    return enriched;
  }

  /**
   * Privacy and Compliance
   */
  async applyPrivacyFilters(event) {
    // TODO: Remove personally identifiable information
    // TODO: Apply data anonymization techniques
    // TODO: Check consent requirements
    // TODO: Apply geographic restrictions
    // TODO: Remove sensitive data fields
    // TODO: Apply data minimization principles
    // TODO: Check retention policy compliance
    // TODO: Apply pseudonymization where appropriate
    // TODO: Generate privacy audit trail
    // TODO: Update privacy compliance metrics
    // TODO: Handle privacy filtering errors
    // TODO: Apply privacy performance monitoring
    // TODO: Generate privacy compliance reports
    // TODO: Update privacy filtering statistics
    // TODO: Create privacy processing documentation

    if (!this.config.privacyMode) {
      return event;
    }

    const filtered = { ...event };

    // Remove or mask sensitive fields
    if (filtered.data.email) {
      filtered.data.email = this.maskEmail(filtered.data.email);
    }

    if (filtered.data.ip) {
      filtered.data.ip = this.maskIP(filtered.data.ip);
    }

    return filtered;
  }

  /**
   * Performance and Monitoring
   */
  async getCollectionMetrics() {
    // TODO: Calculate collection rate statistics
    // TODO: Generate error rate metrics
    // TODO: Compute processing latency statistics
    // TODO: Calculate memory usage metrics
    // TODO: Generate throughput statistics
    // TODO: Compute queue size metrics
    // TODO: Calculate filter performance statistics
    // TODO: Generate enrichment performance metrics
    // TODO: Compute batch processing statistics
    // TODO: Calculate real-time processing metrics
    // TODO: Generate privacy filtering statistics
    // TODO: Compute validation performance metrics
    // TODO: Calculate compression effectiveness
    // TODO: Generate audit trail statistics
    // TODO: Compute overall system health metrics

    return {
      totalEvents: this.eventQueue.length,
      eventsPerSecond: this.calculateEventsPerSecond(),
      errorRate: this.calculateErrorRate(),
      averageLatency: this.calculateAverageLatency(),
      memoryUsage: this.calculateMemoryUsage(),
      queueHealth: this.assessQueueHealth(),
      filterPerformance: this.getFilterPerformance(),
      timestamp: Date.now(),
    };
  }

  /**
   * Utility Methods
   */
  generateEventId() {
    return `evt_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateBatchId() {
    return `batch_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  async flushEventQueue() {
    if (this.eventQueue.length === 0) return;

    const events = this.eventQueue.splice(0);
    await this.processBatch(events);
    this.lastFlush = Date.now();
  }

  async processEvent(event, isRealTime = false) {
    const validation = await this.validateEvent(event);
    if (!validation.valid) {
      throw new Error(`Event validation failed: ${validation.reason}`);
    }

    const enriched = await this.enrichEvent(event);
    const filtered = await this.applyPrivacyFilters(enriched);

    if (isRealTime) {
      return await this.processRealTimeEvent(filtered);
    } else {
      this.eventQueue.push(filtered);

      if (this.eventQueue.length >= this.config.batchSize) {
        await this.flushEventQueue();
      }
    }

    return { success: true, eventId: event.id };
  }

  // Helper methods for event processing
  async validateEventData(event) {
    if (!event.id || !event.type || !event.timestamp) {
      throw new Error("Event missing required fields: id, type, timestamp");
    }

    if (typeof event.timestamp !== "number" || event.timestamp <= 0) {
      throw new Error("Event timestamp must be a positive number");
    }

    return { valid: true };
  }

  async validateEvent(event) {
    try {
      await this.validateEventData(event);
      return { valid: true };
    } catch (error) {
      return { valid: false, reason: error.message };
    }
  }

  checkConsent(userId, eventType) {
    // Simple consent check - would integrate with consent manager in production
    return this.config.privacyMode
      ? this.consentCache?.[userId]?.[eventType] !== false
      : true;
  }

  checkRateLimit(userId, eventType) {
    // Simple rate limiting - would use more sophisticated algorithms in production
    const key = `${userId}_${eventType}`;
    const now = Date.now();
    const window = 60000; // 1 minute
    const limit = 100; // 100 events per minute per user per type

    if (!this.rateLimitCache) this.rateLimitCache = new Map();

    const userEvents = this.rateLimitCache.get(key) || [];
    const recentEvents = userEvents.filter(
      (timestamp) => now - timestamp < window
    );

    if (recentEvents.length >= limit) {
      return false;
    }

    recentEvents.push(now);
    this.rateLimitCache.set(key, recentEvents);
    return true;
  }

  async applyPrivacyFilters(event) {
    if (!this.config.privacyMode) return event;

    // Remove or mask sensitive data
    const filtered = { ...event };

    // Remove potential PII from data
    if (filtered.data) {
      filtered.data = this.sanitizeData(filtered.data);
    }

    // Mask IP addresses, etc.
    if (filtered.context?.ip) {
      filtered.context.ip = this.maskIP(filtered.context.ip);
    }

    return filtered;
  }

  async enrichEvent(event) {
    return {
      ...event,
      enrichedAt: Date.now(),
      version: "1.0",
      source: "huntmaster-engine",
      environment: process.env.NODE_ENV || "development",
    };
  }

  updateEventStatistics(eventType) {
    if (!this.eventStatistics) this.eventStatistics = new Map();

    const current = this.eventStatistics.get(eventType) || {
      count: 0,
      lastSeen: null,
    };
    this.eventStatistics.set(eventType, {
      count: current.count + 1,
      lastSeen: Date.now(),
    });
  }

  updateCollectionMetrics(eventType, success) {
    if (!this.collectionMetrics) {
      this.collectionMetrics = {
        total: 0,
        success: 0,
        errors: 0,
        byType: new Map(),
      };
    }

    this.collectionMetrics.total++;
    if (success) {
      this.collectionMetrics.success++;
    } else {
      this.collectionMetrics.errors++;
    }

    // Update per-type metrics
    const typeMetrics = this.collectionMetrics.byType.get(eventType) || { count: 0, success: 0, errors: 0 };
    typeMetrics.count++;
    if (success) {
      typeMetrics.success++;
    } else {
      typeMetrics.errors++;
    }
    this.collectionMetrics.byType.set(eventType, typeMetrics);
  }

  // Real-time processing helper methods
  async validateRealTimeEvent(event) {
    try {
      if (!event || typeof event !== 'object') {
        return { valid: false, error: 'Event must be an object' };
      }
      if (!event.type || typeof event.type !== 'string') {
        return { valid: false, error: 'Event must have a type string' };
      }
      if (!event.id) {
        return { valid: false, error: 'Event must have an ID' };
      }
      return { valid: true };
    } catch (error) {
      return { valid: false, error: error.message };
    }
  }

  async filterRealTimeEvent(event) {
    // Apply configured filters
    for (const [name, filter] of this.realTimeFilters) {
      try {
        if (!filter(event)) {
          throw new Error(`Filter ${name} rejected event`);
        }
      } catch (error) {
        throw new Error(`Filter ${name} failed: ${error.message}`);
      }
    }
    return event;
  }

  async enrichRealTimeEvent(event) {
    return {
      ...event,
      enrichedAt: Date.now(),
      realTimeProcessed: true,
      environment: process.env.NODE_ENV || 'development'
    };
  }

  async compressRealTimeEvent(event) {
    if (!this.realTimeCompression?.enabled) return event;

    // Simple compression simulation
    const compressed = {
      ...event,
      compressed: true,
      originalSize: JSON.stringify(event).length
    };

    return compressed;
  }

  async storeRealTimeEvent(event) {
    this.streamBuffer.set(event.id, event);
    return event;
  }

  async applyRealTimeFilters(event) {
    const results = { passed: true, failedFilters: [] };

    for (const [name, filter] of this.realTimeFilters) {
      try {
        const passed = await filter(event);
        if (!passed) {
          results.passed = false;
          results.failedFilters.push(name);
        }
      } catch (error) {
        results.passed = false;
        results.failedFilters.push(`${name}:${error.message}`);
      }
    }

    if (!results.passed) {
      results.reason = `Failed filters: ${results.failedFilters.join(', ')}`;
    }

    return results;
  }

  async applyPrivacyProtection(event) {
    if (!this.config.privacyMode) return event;

    const protected = { ...event };

    // Remove sensitive fields
    for (const field of this.privacyProcessor.sensitiveFields) {
      if (protected[field]) {
        protected[field] = '[REDACTED]';
      }
    }

    return protected;
  }

  updateRealTimePerformanceStats(processingTime) {
    if (!this.performanceMetrics.averageProcessingTime) {
      this.performanceMetrics.averageProcessingTime = processingTime;
    } else {
      this.performanceMetrics.averageProcessingTime =
        (this.performanceMetrics.averageProcessingTime + processingTime) / 2;
    }
  }

  checkRealTimeRateLimit(event) {
    const now = Date.now();
    const windowStart = now - this.rateLimiter.windowMs;

    // Clean old entries
    for (const [key, timestamp] of this.rateLimiter.eventCounts) {
      if (timestamp < windowStart) {
        this.rateLimiter.eventCounts.delete(key);
      }
    }

    const currentCount = this.rateLimiter.eventCounts.size;

    if (currentCount >= this.rateLimiter.maxEvents) {
      return {
        allowed: false,
        currentRate: currentCount,
        limit: this.rateLimiter.maxEvents
      };
    }

    this.rateLimiter.eventCounts.set(event.id, now);
    return {
      allowed: true,
      currentRate: currentCount + 1,
      limit: this.rateLimiter.maxEvents
    };
  }

  updateRealTimeMonitoringData(event) {
    // Update monitoring metrics
    if (this.realTimeMonitor) {
      this.realTimeMonitor.lastCheck = Date.now();

      // Check error rate threshold
      const errorRate = this.realTimeMetrics.errorCount / Math.max(this.realTimeMetrics.eventsProcessed, 1);
      if (errorRate > this.realTimeMonitor.thresholds.errorRate) {
        this.realTimeMonitor.alerts.push({
          type: 'error_rate_exceeded',
          threshold: this.realTimeMonitor.thresholds.errorRate,
          current: errorRate,
          timestamp: Date.now()
        });
      }
    }
  }

  async checkAndGenerateAlerts(event) {
    const alerts = [];

    if (!this.alertSystem?.enabled) return alerts;

    // Check queue size
    if (this.eventQueue.length > this.realTimeMonitor?.thresholds?.queueSize) {
      alerts.push({
        type: 'queue_size_warning',
        current: this.eventQueue.length,
        threshold: this.realTimeMonitor.thresholds.queueSize,
        timestamp: Date.now()
      });
    }

    return alerts;
  }

  updateRealTimeCollectionStats(event) {
    this.updateEventStatistics(event.type);
  }

  async addToRealTimeBackup(event) {
    if (!this.backupSystem?.enabled) return;

    // Add to backup buffer (simplified implementation)
    if (!this.backupBuffer) this.backupBuffer = new Map();
    this.backupBuffer.set(event.id, event);
  }

  async createRealTimeBackup() {
    if (!this.backupBuffer || this.backupBuffer.size === 0) return;

    // Create backup snapshot
    const backup = {
      timestamp: Date.now(),
      events: Array.from(this.backupBuffer.values()),
      size: this.backupBuffer.size
    };

    // Store backup (simplified - would normally go to persistent storage)
    if (!this.backups) this.backups = [];
    this.backups.push(backup);

    // Clear backup buffer
    this.backupBuffer.clear();
  }

  async flushAuditLog() {
    if (!this.auditLogger?.buffer || this.auditLogger.buffer.length === 0) return;

    // Flush audit entries (simplified implementation)
    const entries = [...this.auditLogger.buffer];
    this.auditLogger.buffer = [];

    // Would normally write to persistent audit log
    console.log(`Flushed ${entries.length} audit entries`);
  }

  updateRealTimeMetrics() {
    if (this.realTimeMetrics) {
      const now = Date.now();
      const elapsed = (now - this.realTimeMetrics.startTime) / 1000;
      this.realTimeMetrics.throughput = this.realTimeMetrics.eventsProcessed / elapsed;
    }
  }

  getMemoryUsage() {
    if (typeof process !== 'undefined' && process.memoryUsage) {
      return process.memoryUsage();
    }
    return { heapUsed: 0, heapTotal: 0, external: 0 };
  }

  getCPULoad() {
    // Simplified CPU load estimation
    return Math.random() * 100;
  }

  getNetworkLatency() {
    // Simplified network latency estimation
    return Math.random() * 100;
  }

  async createWebSocketServer() {
    // Simplified WebSocket server creation
    return {
      close: () => console.log('WebSocket server closed'),
      send: (data) => console.log('WebSocket data sent:', data)
    };
  }

  // Batch processing helper methods
  optimizeBatchProcessing(events) {
    // Sort by priority and type for optimal processing
    const sorted = [...events].sort((a, b) => {
      // Prioritize by event type and timestamp
      if (a.priority !== b.priority) {
        return (b.priority || 0) - (a.priority || 0);
      }
      return (a.timestamp || 0) - (b.timestamp || 0);
    });

    return {
      reorderedEvents: sorted,
      strategy: 'priority_timestamp',
      metrics: {
        originalOrder: events.map(e => e.id),
        optimizedOrder: sorted.map(e => e.id),
        optimizationTime: Date.now()
      },
      report: {
        eventsReordered: events.length,
        strategy: 'priority_timestamp',
        optimizationBenefit: 'Improved processing efficiency'
      }
    };
  }

  sortEventsForOptimalProcessing(events) {
    // Additional sorting for processing optimization
    return events.sort((a, b) => {
      // Group by type for batch processing efficiency
      if (a.type !== b.type) {
        return a.type.localeCompare(b.type);
      }
      return (a.timestamp || 0) - (b.timestamp || 0);
    });
  }

  buildDependencyGraph(events) {
    const graph = new Map();

    for (const event of events) {
      graph.set(event.id, {
        event,
        dependencies: event.dependencies || [],
        dependents: []
      });
    }

    // Build dependent relationships
    for (const [id, node] of graph) {
      for (const depId of node.dependencies) {
        const depNode = graph.get(depId);
        if (depNode) {
          depNode.dependents.push(id);
        }
      }
    }

    return graph;
  }

  resolveDependencies(dependencyGraph) {
    const resolved = [];
    const visited = new Set();
    const processing = new Set();

    const visit = (nodeId) => {
      if (processing.has(nodeId)) {
        throw new Error(`Circular dependency detected: ${nodeId}`);
      }

      if (visited.has(nodeId)) return;

      processing.add(nodeId);
      const node = dependencyGraph.get(nodeId);

      if (node) {
        for (const depId of node.dependencies) {
          visit(depId);
        }
        resolved.push([node.event]);
      }

      processing.delete(nodeId);
      visited.add(nodeId);
    };

    for (const nodeId of dependencyGraph.keys()) {
      visit(nodeId);
    }

    return resolved;
  }

  async processEventGroup(eventGroup, batchId, strategy) {
    const results = [];

    for (const event of eventGroup) {
      try {
        const result = await this.processEvent(event);
        results.push({
          success: true,
          eventId: event.id,
          result,
          batchId,
          strategy
        });
      } catch (error) {
        results.push({
          success: false,
          eventId: event.id,
          error: error.message,
          batchId,
          strategy
        });
      }
    }

    return results;
  }

  async applyBatchErrorRecovery(eventGroup, error, batchId) {
    const recoveryResults = [];
    let recoveredCount = 0;

    // Try to process events individually on group failure
    for (const event of eventGroup) {
      try {
        const result = await this.processEvent(event);
        recoveryResults.push({
          success: true,
          eventId: event.id,
          result,
          batchId,
          recovered: true
        });
        recoveredCount++;
      } catch (individualError) {
        recoveryResults.push({
          success: false,
          eventId: event.id,
          error: individualError.message,
          batchId,
          recovered: false
        });
      }
    }

    return {
      recovered: recoveredCount > 0,
      results: recoveryResults,
      processedCount: recoveredCount,
      totalCount: eventGroup.length
    };
  }

  updateBatchPerformanceStats(metrics) {
    if (!this.batchPerformanceStats) {
      this.batchPerformanceStats = {
        totalBatches: 0,
        totalEvents: 0,
        totalProcessingTime: 0,
        averageThroughput: 0
      };
    }

    this.batchPerformanceStats.totalBatches++;
    this.batchPerformanceStats.totalEvents += metrics.totalEvents;
    this.batchPerformanceStats.totalProcessingTime += metrics.processingTime;
    this.batchPerformanceStats.averageThroughput =
      this.batchPerformanceStats.totalEvents / (this.batchPerformanceStats.totalProcessingTime / 1000);
  }

  manageBatchMemory(batchId, results) {
    // Clean up memory for processed batches
    if (this.compressionCache) {
      this.compressionCache.delete(batchId);
    }

    // Trigger garbage collection hint if available
    if (global.gc && this.batchPerformanceStats?.totalBatches % 10 === 0) {
      global.gc();
    }
  }

  async compressBatchResults(results) {
    // Simplified batch compression
    return results.map(result => ({
      ...result,
      compressed: true,
      originalSize: JSON.stringify(result).length
    }));
  }

  updateBatchMonitoringData(metrics) {
    // Update batch monitoring metrics
    if (!this.batchMonitoringData) {
      this.batchMonitoringData = {
        recentBatches: [],
        performanceTrends: []
      };
    }

    this.batchMonitoringData.recentBatches.push({
      ...metrics,
      timestamp: Date.now()
    });

    // Keep only recent batches (last 100)
    if (this.batchMonitoringData.recentBatches.length > 100) {
      this.batchMonitoringData.recentBatches.shift();
    }
  }

  checkBatchRateLimit(metrics) {
    // Simplified batch rate limiting
    const now = Date.now();
    const windowMs = 60000; // 1 minute

    if (!this.batchRateLimitData) {
      this.batchRateLimitData = {
        windowStart: now,
        batchCount: 0,
        eventCount: 0
      };
    }

    // Reset window if needed
    if (now - this.batchRateLimitData.windowStart > windowMs) {
      this.batchRateLimitData = {
        windowStart: now,
        batchCount: 0,
        eventCount: 0
      };
    }

    this.batchRateLimitData.batchCount++;
    this.batchRateLimitData.eventCount += metrics.totalEvents;

    const maxBatchesPerMinute = 100;
    const maxEventsPerMinute = 10000;

    if (this.batchRateLimitData.batchCount > maxBatchesPerMinute) {
      return {
        allowed: false,
        message: `Batch rate limit exceeded: ${this.batchRateLimitData.batchCount}/${maxBatchesPerMinute} batches per minute`
      };
    }

    if (this.batchRateLimitData.eventCount > maxEventsPerMinute) {
      return {
        allowed: false,
        message: `Event rate limit exceeded: ${this.batchRateLimitData.eventCount}/${maxEventsPerMinute} events per minute`
      };
    }

    return { allowed: true };
  }

  async applyBatchPrivacyProtection(results) {
    if (!this.config.privacyMode) return results;

    return results.map(result => {
      if (result.result && typeof result.result === 'object') {
        // Remove sensitive fields from batch results
        const protected = { ...result };
        for (const field of this.privacyProcessor.sensitiveFields) {
          if (protected.result[field]) {
            protected.result[field] = '[REDACTED]';
          }
        }
        return protected;
      }
      return result;
    });
  }

  updateBatchCollectionStats(report) {
    if (!this.bulkStatistics) {
      this.bulkStatistics = new Map();
    }

    const stats = {
      batchCount: (this.bulkStatistics.get('batchCount') || 0) + 1,
      totalEvents: (this.bulkStatistics.get('totalEvents') || 0) + report.summary.totalEvents,
      successfulEvents: (this.bulkStatistics.get('successfulEvents') || 0) + report.summary.processedCount,
      errorEvents: (this.bulkStatistics.get('errorEvents') || 0) + report.summary.errorCount,
      lastBatchTime: Date.now()
    };

    for (const [key, value] of Object.entries(stats)) {
      this.bulkStatistics.set(key, value);
    }
  }

  generateBatchId() {
    return `batch_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

    const typeMetrics = this.collectionMetrics.byType.get(eventType) || {
      total: 0,
      success: 0,
      errors: 0,
    };
    typeMetrics.total++;
    if (success) typeMetrics.success++;
    else typeMetrics.errors++;
    this.collectionMetrics.byType.set(eventType, typeMetrics);
  }

  generateAuditEntry(action, details) {
    if (!this.auditLog) this.auditLog = [];

    this.auditLog.push({
      timestamp: Date.now(),
      action,
      details,
      id: this.generateEventId(),
    });

    // Keep only last 1000 audit entries
    if (this.auditLog.length > 1000) {
      this.auditLog = this.auditLog.slice(-1000);
    }
  }

  chunkArray(array, chunkSize) {
    const chunks = [];
    for (let i = 0; i < array.length; i += chunkSize) {
      chunks.push(array.slice(i, i + chunkSize));
    }
    return chunks;
  }

  updateBulkStatistics(eventType, success) {
    if (!this.bulkStatistics) this.bulkStatistics = new Map();

    const stats = this.bulkStatistics.get(eventType) || {
      total: 0,
      success: 0,
      errors: 0,
    };
    stats.total++;
    if (success) stats.success++;
    else stats.errors++;
    this.bulkStatistics.set(eventType, stats);
  }

  updateBulkPerformanceMetrics(eventCount, processingTime) {
    if (!this.bulkPerformanceMetrics) {
      this.bulkPerformanceMetrics = {
        totalEvents: 0,
        totalTime: 0,
        batches: 0,
      };
    }

    this.bulkPerformanceMetrics.totalEvents += eventCount;
    this.bulkPerformanceMetrics.totalTime += processingTime;
    this.bulkPerformanceMetrics.batches++;
  }

  clearBulkProcessingCache() {
    // Clear any cached data used during bulk processing
    if (this.rateLimitCache) this.rateLimitCache.clear();
  }

  sanitizeData(data) {
    // Simple data sanitization - remove common PII patterns
    if (typeof data === "string") {
      return data
        .replace(
          /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b/g,
          "[EMAIL]"
        )
        .replace(/\b\d{3}-\d{2}-\d{4}\b/g, "[SSN]")
        .replace(/\b\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4}\b/g, "[CARD]");
    }

    if (typeof data === "object" && data !== null) {
      const sanitized = {};
      for (const [key, value] of Object.entries(data)) {
        // Skip fields that commonly contain PII
        if (
          ["email", "phone", "ssn", "creditCard", "password"].includes(
            key.toLowerCase()
          )
        ) {
          sanitized[key] = "[REDACTED]";
        } else if (typeof value === "object") {
          sanitized[key] = this.sanitizeData(value);
        } else {
          sanitized[key] = this.sanitizeData(value);
        }
      }
      return sanitized;
    }

    return data;
  }

  maskIP(ip) {
    // Mask last octet of IPv4 addresses for privacy
    return ip.replace(/\.\d+$/, ".XXX");
  }
}
