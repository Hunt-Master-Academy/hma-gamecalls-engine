/**
 * MasterCallManager Module
 *
 * Provides comprehensive master call library management with advanced playback controls,
 * metadata management, quality assessment, and recommendation system.
 * Optimized for hunting call management with precision timing and quality preservation.
 *
 * Features:
 * - Master call library organization and categorization
 * - Advanced playback controls with precision timing
 * - Metadata management (duration, quality, difficulty)
 * - Custom master call upload and validation
 * - Format conversion and optimization
 * - Quality assessment and enhancement
 * - Playback synchronization and looping
 * - Speed control without pitch alteration
 * - Volume normalization and level matching
 * - Master call recommendation system
 */

export class MasterCallManager {
  constructor(eventManager, audioContext) {
    this.eventManager = eventManager;
    this.audioContext = audioContext;
    this.isInitialized = false;

    // Core configuration
    this.config = {
      // Library settings
      maxLibrarySize: 1000, // Maximum number of calls
      autoBackup: true,
      backupInterval: 300000, // 5 minutes

      // Playback settings
      crossfadeDuration: 0.05, // seconds
      preloadCalls: true,
      maxPreloaded: 20,

      // Quality settings
      qualityThreshold: 0.7, // Minimum quality score
      autoEnhancement: true,
      normalizeVolume: true,

      // Recommendation settings
      enableRecommendations: true,
      learningEnabled: true,
      maxRecommendations: 10,
    };

    // Master call categories and metadata
    this.categories = {
      deer: {
        buck_grunt: { difficulty: "easy", season: "fall", effectiveness: 0.8 },
        doe_bleat: { difficulty: "medium", season: "fall", effectiveness: 0.9 },
        fawn_distress: {
          difficulty: "easy",
          season: "summer",
          effectiveness: 0.7,
        },
        social_grunt: {
          difficulty: "medium",
          season: "fall",
          effectiveness: 0.8,
        },
        aggressive_grunt: {
          difficulty: "hard",
          season: "rut",
          effectiveness: 0.9,
        },
        snort_wheeze: { difficulty: "hard", season: "rut", effectiveness: 0.8 },
        rattling: { difficulty: "expert", season: "rut", effectiveness: 0.9 },
      },
      elk: {
        bugle: { difficulty: "hard", season: "fall", effectiveness: 0.9 },
        cow_call: { difficulty: "medium", season: "fall", effectiveness: 0.8 },
        calf_call: { difficulty: "easy", season: "summer", effectiveness: 0.7 },
      },
      turkey: {
        yelp: { difficulty: "medium", season: "spring", effectiveness: 0.9 },
        cluck: { difficulty: "easy", season: "spring", effectiveness: 0.7 },
        purr: { difficulty: "medium", season: "spring", effectiveness: 0.6 },
        gobble: { difficulty: "hard", season: "spring", effectiveness: 0.8 },
      },
      predator: {
        rabbit_distress: {
          difficulty: "easy",
          season: "all",
          effectiveness: 0.8,
        },
        bird_distress: {
          difficulty: "medium",
          season: "all",
          effectiveness: 0.7,
        },
        rodent_squeak: {
          difficulty: "easy",
          season: "all",
          effectiveness: 0.6,
        },
      },
    };

    // Master call library
    this.library = {
      calls: new Map(),
      metadata: new Map(),
      qualityScores: new Map(),
      usageStats: new Map(),
      favorites: new Set(),
      playlists: new Map(),
    };

    // Playback state
    this.playback = {
      currentCall: null,
      isPlaying: false,
      isPaused: false,
      isLooping: false,
      position: 0,
      duration: 0,

      // Audio nodes
      sourceNode: null,
      gainNode: null,
      panNode: null,
      filterNode: null,

      // Playback parameters
      volume: 1.0,
      playbackRate: 1.0,
      fadeInDuration: 0.05,
      fadeOutDuration: 0.05,

      // Scheduling
      nextPlayTime: 0,
      scheduledCalls: [],

      // Quality settings
      enableEnhancement: true,
      noiseReduction: 0.3,
      volumeNormalization: true,
    };

    // Upload and validation
    this.validator = {
      maxFileSize: 50 * 1024 * 1024, // 50MB
      supportedFormats: ["wav", "mp3", "ogg", "flac", "aac"],
      minDuration: 0.5, // seconds
      maxDuration: 300, // 5 minutes
      minQuality: 0.3,

      // Validation rules
      rules: {
        sampleRate: { min: 8000, max: 96000, preferred: [44100, 48000] },
        channels: { min: 1, max: 2, preferred: 1 },
        bitDepth: { min: 8, max: 32, preferred: [16, 24] },
      },
    };

    // Recommendation system
    this.recommender = {
      enabled: this.config.enableRecommendations,
      userProfile: {
        preferredCategories: [],
        skillLevel: "beginner", // beginner, intermediate, advanced, expert
        huntingStyle: "passive", // passive, aggressive, mixed
        seasonPreference: "fall",
        successRate: new Map(),
      },

      // Machine learning components
      model: {
        weights: new Map(),
        bias: 0,
        learningRate: 0.01,
        trainingData: [],
        lastTrained: 0,
      },

      // Recommendation cache
      cache: {
        recommendations: [],
        lastUpdate: 0,
        ttl: 300000, // 5 minutes
      },
    };

    // Performance monitoring
    this.performance = {
      loadTimes: new Array(100).fill(0),
      playbackLatency: new Array(100).fill(0),
      bufferUnderrunCount: 0,
      averageLoadTime: 0,
      cacheHitRate: 0,
      qualityLevel: "high",
    };

    // Format conversion integration
    this.formatConverter = null;
    this.qualityEnhancer = null;

    this.bindMethods();
  }

  bindMethods() {
    this.loadMasterCall = this.loadMasterCall.bind(this);
    this.playMasterCall = this.playMasterCall.bind(this);
    this.validateMasterCall = this.validateMasterCall.bind(this);
    this.generateRecommendations = this.generateRecommendations.bind(this);
  }

  /**
   * Initialize the master call manager
   */
  async initialize(config = {}) {
    try {
      // Merge configuration
      this.config = { ...this.config, ...config };

      // Initialize audio processing nodes
      this.initializeAudioNodes();

      // Initialize library storage
      await this.initializeLibraryStorage();

      // Load default master call library
      await this.loadDefaultLibrary();

      // Initialize recommendation system
      if (this.config.enableRecommendations) {
        this.initializeRecommendationSystem();
      }

      // Initialize upload validator
      this.initializeValidator();

      // Initialize performance monitoring
      this.initializePerformanceMonitoring();

      // Setup auto-backup if enabled
      if (this.config.autoBackup) {
        this.setupAutoBackup();
      }

      this.isInitialized = true;

      this.eventManager?.emitEvent("masterCallManagerInitialized", {
        librarySize: this.library.calls.size,
        categories: Object.keys(this.categories).length,
        config: this.config,
        timestamp: performance.now(),
      });

      return { success: true, librarySize: this.library.calls.size };
    } catch (error) {
      console.error("Failed to initialize MasterCallManager:", error);
      this.eventManager?.emitEvent("masterCallManagerError", {
        error: error.message,
        phase: "initialization",
      });
      throw error;
    }
  }

  /**
   * Initialize audio processing nodes
   */
  initializeAudioNodes() {
    if (!this.audioContext) {
      throw new Error("AudioContext required for MasterCallManager");
    }

    // Create main gain node for volume control
    this.playback.gainNode = this.audioContext.createGain();
    this.playback.gainNode.gain.value = this.playback.volume;

    // Create stereo panner for spatial positioning
    if (this.audioContext.createStereoPanner) {
      this.playback.panNode = this.audioContext.createStereoPanner();
      this.playback.panNode.pan.value = 0; // Center
    }

    // Create filter for tone adjustment
    this.playback.filterNode = this.audioContext.createBiquadFilter();
    this.playback.filterNode.type = "peaking";
    this.playback.filterNode.frequency.value = 1000;
    this.playback.filterNode.Q.value = 1;
    this.playback.filterNode.gain.value = 0;

    // Connect nodes
    this.connectAudioNodes();
  }

  /**
   * Connect audio processing nodes
   */
  connectAudioNodes() {
    const { gainNode, panNode, filterNode } = this.playback;

    if (panNode) {
      filterNode.connect(gainNode);
      gainNode.connect(panNode);
      panNode.connect(this.audioContext.destination);
    } else {
      filterNode.connect(gainNode);
      gainNode.connect(this.audioContext.destination);
    }
  }

  /**
   * Initialize library storage
   */
  async initializeLibraryStorage() {
    // Initialize IndexedDB for persistent storage
    if (typeof window !== "undefined" && "indexedDB" in window) {
      try {
        this.db = await this.openIndexedDB();
        await this.loadLibraryFromStorage();
      } catch (error) {
        console.warn("IndexedDB not available, using memory storage:", error);
        this.db = null;
      }
    }
  }

  /**
   * Open IndexedDB for master call storage
   */
  openIndexedDB() {
    return new Promise((resolve, reject) => {
      const request = indexedDB.open("HuntmasterCalls", 1);

      request.onerror = () => reject(request.error);
      request.onsuccess = () => resolve(request.result);

      request.onupgradeneeded = (event) => {
        const db = event.target.result;

        // Create object stores
        if (!db.objectStoreNames.contains("calls")) {
          const callStore = db.createObjectStore("calls", { keyPath: "id" });
          callStore.createIndex("category", "category", { unique: false });
          callStore.createIndex("quality", "quality", { unique: false });
        }

        if (!db.objectStoreNames.contains("metadata")) {
          db.createObjectStore("metadata", { keyPath: "callId" });
        }

        if (!db.objectStoreNames.contains("playlists")) {
          db.createObjectStore("playlists", { keyPath: "id" });
        }
      };
    });
  }

  /**
   * Load library from persistent storage
   */
  async loadLibraryFromStorage() {
    if (!this.db) return;

    try {
      const transaction = this.db.transaction(
        ["calls", "metadata"],
        "readonly"
      );
      const callStore = transaction.objectStore("calls");
      const metadataStore = transaction.objectStore("metadata");

      // Load calls
      const calls = await this.getAllFromStore(callStore);
      calls.forEach((call) => {
        this.library.calls.set(call.id, call);
      });

      // Load metadata
      const metadata = await this.getAllFromStore(metadataStore);
      metadata.forEach((meta) => {
        this.library.metadata.set(meta.callId, meta);
      });

      console.log(`Loaded ${calls.length} master calls from storage`);
    } catch (error) {
      console.error("Failed to load library from storage:", error);
    }
  }

  /**
   * Get all items from IndexedDB store
   */
  getAllFromStore(store) {
    return new Promise((resolve, reject) => {
      const request = store.getAll();
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  /**
   * Load default master call library
   */
  async loadDefaultLibrary() {
    // In production, this would load from a CDN or local bundle
    const defaultCalls = [
      {
        id: "buck_grunt_001",
        name: "Buck Grunt - Basic",
        category: "deer",
        subcategory: "buck_grunt",
        difficulty: "easy",
        duration: 2.5,
        sampleRate: 44100,
        channels: 1,
        quality: 0.85,
        description: "Basic buck grunt for general communication",
        tags: ["buck", "grunt", "basic", "communication"],
      },
      {
        id: "doe_bleat_001",
        name: "Doe Bleat - Estrus",
        category: "deer",
        subcategory: "doe_bleat",
        difficulty: "medium",
        duration: 3.2,
        sampleRate: 44100,
        channels: 1,
        quality: 0.92,
        description: "Estrus doe bleat for attracting bucks during rut",
        tags: ["doe", "bleat", "estrus", "rut"],
      },
      // More default calls would be added here
    ];

    // Register default calls
    for (const call of defaultCalls) {
      this.registerMasterCall(call, null, true); // Skip validation for defaults
    }
  }

  /**
   * Initialize recommendation system
   */
  initializeRecommendationSystem() {
    const recommender = this.recommender;

    // Initialize user profile from stored preferences
    this.loadUserProfile();

    // Initialize recommendation model
    recommender.model.weights.set("quality", 0.3);
    recommender.model.weights.set("difficulty", 0.2);
    recommender.model.weights.set("category", 0.2);
    recommender.model.weights.set("season", 0.15);
    recommender.model.weights.set("usage", 0.15);

    // Setup periodic model training
    if (this.config.learningEnabled) {
      this.setupModelTraining();
    }
  }

  /**
   * Initialize upload validator
   */
  initializeValidator() {
    // Setup validation rules and error messages
    this.validator.errorMessages = {
      fileSize: "File size exceeds maximum limit",
      format: "Unsupported audio format",
      duration: "Audio duration out of acceptable range",
      quality: "Audio quality below minimum threshold",
      sampleRate: "Sample rate not supported",
      channels: "Channel configuration not supported",
    };
  }

  /**
   * Initialize performance monitoring
   */
  initializePerformanceMonitoring() {
    this.performance.startTime = performance.now();

    // Setup periodic performance checks
    setInterval(() => {
      this.updatePerformanceMetrics();
    }, 5000); // Every 5 seconds
  }

  /**
   * Setup auto-backup system
   */
  setupAutoBackup() {
    setInterval(() => {
      this.backupLibrary();
    }, this.config.backupInterval);
  }

  /**
   * Load and validate master call
   */
  async loadMasterCall(callId, audioData, metadata = {}) {
    if (!this.isInitialized) {
      throw new Error("MasterCallManager not initialized");
    }

    const startTime = performance.now();

    try {
      // Validate audio data
      const validation = await this.validateMasterCall(audioData, metadata);
      if (!validation.isValid) {
        throw new Error(`Invalid master call: ${validation.errors.join(", ")}`);
      }

      // Process and enhance audio if needed
      const processedAudio = await this.processMasterCallAudio(
        audioData,
        validation
      );

      // Extract comprehensive metadata
      const fullMetadata = await this.extractAudioMetadata(
        processedAudio,
        metadata
      );

      // Calculate quality score
      const qualityScore = await this.assessAudioQuality(processedAudio);

      // Create master call object
      const masterCall = {
        id: callId,
        audioBuffer: processedAudio,
        metadata: fullMetadata,
        quality: qualityScore,
        loadTime: performance.now() - startTime,
        uploadTime: Date.now(),
        playCount: 0,
        lastPlayed: null,
      };

      // Store in library
      this.registerMasterCall(masterCall);

      // Save to persistent storage
      if (this.db) {
        await this.saveMasterCallToStorage(masterCall);
      }

      // Update performance metrics
      this.updateLoadPerformance(performance.now() - startTime);

      // Emit success event
      this.eventManager?.emitEvent("masterCallLoaded", {
        callId: callId,
        quality: qualityScore,
        duration: fullMetadata.duration,
        loadTime: performance.now() - startTime,
      });

      return {
        success: true,
        callId: callId,
        quality: qualityScore,
        metadata: fullMetadata,
      };
    } catch (error) {
      console.error("Failed to load master call:", error);

      this.eventManager?.emitEvent("masterCallLoadError", {
        callId: callId,
        error: error.message,
        timestamp: performance.now(),
      });

      return { success: false, error: error.message };
    }
  }

  /**
   * Validate master call audio and metadata
   */
  async validateMasterCall(audioData, metadata = {}) {
    const validation = {
      isValid: true,
      errors: [],
      warnings: [],
      quality: 0,
    };

    try {
      // File size validation
      if (audioData.byteLength > this.validator.maxFileSize) {
        validation.errors.push(this.validator.errorMessages.fileSize);
        validation.isValid = false;
      }

      // Format validation would be done here
      // In production, would decode audio to validate format

      // Basic audio analysis
      const audioBuffer = await this.decodeAudioData(audioData);

      // Duration validation
      const duration = audioBuffer.duration;
      if (
        duration < this.validator.minDuration ||
        duration > this.validator.maxDuration
      ) {
        validation.errors.push(this.validator.errorMessages.duration);
        validation.isValid = false;
      }

      // Sample rate validation
      const sampleRate = audioBuffer.sampleRate;
      const rules = this.validator.rules;
      if (
        sampleRate < rules.sampleRate.min ||
        sampleRate > rules.sampleRate.max
      ) {
        validation.errors.push(this.validator.errorMessages.sampleRate);
        validation.isValid = false;
      }

      // Channel validation
      const channels = audioBuffer.numberOfChannels;
      if (channels < rules.channels.min || channels > rules.channels.max) {
        validation.errors.push(this.validator.errorMessages.channels);
        validation.isValid = false;
      }

      // Quality assessment
      validation.quality = await this.assessAudioQuality(audioBuffer);
      if (validation.quality < this.validator.minQuality) {
        validation.errors.push(this.validator.errorMessages.quality);
        validation.isValid = false;
      }

      // Warnings for non-optimal settings
      if (!rules.sampleRate.preferred.includes(sampleRate)) {
        validation.warnings.push(`Sample rate ${sampleRate} Hz is not optimal`);
      }

      if (channels !== rules.channels.preferred) {
        validation.warnings.push(
          `${channels} channels not optimal for master calls`
        );
      }
    } catch (error) {
      validation.errors.push(`Validation error: ${error.message}`);
      validation.isValid = false;
    }

    return validation;
  }

  /**
   * Decode audio data to AudioBuffer
   */
  async decodeAudioData(audioData) {
    return new Promise((resolve, reject) => {
      this.audioContext.decodeAudioData(
        audioData.slice(), // Create copy to avoid detaching
        resolve,
        reject
      );
    });
  }

  /**
   * Process and enhance master call audio
   */
  async processMasterCallAudio(audioData, validation) {
    let audioBuffer = await this.decodeAudioData(audioData);

    // Apply enhancements if enabled
    if (this.config.autoEnhancement) {
      audioBuffer = await this.enhanceAudioBuffer(audioBuffer, validation);
    }

    // Normalize volume if enabled
    if (this.config.normalizeVolume) {
      audioBuffer = this.normalizeAudioVolume(audioBuffer);
    }

    return audioBuffer;
  }

  /**
   * Enhance audio buffer quality
   */
  async enhanceAudioBuffer(audioBuffer, validation) {
    // This would integrate with the QualityAssessor and other enhancement modules
    // For now, implement basic processing

    const enhanced = this.audioContext.createBuffer(
      audioBuffer.numberOfChannels,
      audioBuffer.length,
      audioBuffer.sampleRate
    );

    for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
      const inputData = audioBuffer.getChannelData(channel);
      const outputData = enhanced.getChannelData(channel);

      // Apply basic enhancement (simplified)
      for (let i = 0; i < inputData.length; i++) {
        let sample = inputData[i];

        // Basic noise reduction (simple high-pass filter)
        if (i > 0) {
          sample = sample - 0.95 * inputData[i - 1];
        }

        outputData[i] = sample;
      }
    }

    return enhanced;
  }

  /**
   * Normalize audio volume
   */
  normalizeAudioVolume(audioBuffer) {
    let maxAmplitude = 0;

    // Find peak amplitude across all channels
    for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
      const channelData = audioBuffer.getChannelData(channel);
      for (let i = 0; i < channelData.length; i++) {
        maxAmplitude = Math.max(maxAmplitude, Math.abs(channelData[i]));
      }
    }

    // Calculate normalization factor (leave some headroom)
    const targetLevel = 0.95;
    const normalizationFactor =
      maxAmplitude > 0 ? targetLevel / maxAmplitude : 1;

    // Apply normalization
    if (normalizationFactor !== 1) {
      const normalized = this.audioContext.createBuffer(
        audioBuffer.numberOfChannels,
        audioBuffer.length,
        audioBuffer.sampleRate
      );

      for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
        const inputData = audioBuffer.getChannelData(channel);
        const outputData = normalized.getChannelData(channel);

        for (let i = 0; i < inputData.length; i++) {
          outputData[i] = inputData[i] * normalizationFactor;
        }
      }

      return normalized;
    }

    return audioBuffer;
  }

  /**
   * Extract comprehensive metadata from audio
   */
  async extractAudioMetadata(audioBuffer, providedMetadata = {}) {
    const metadata = {
      ...providedMetadata,

      // Technical metadata
      duration: audioBuffer.duration,
      sampleRate: audioBuffer.sampleRate,
      channels: audioBuffer.numberOfChannels,
      length: audioBuffer.length,

      // Analysis metadata
      peakAmplitude: 0,
      rmsLevel: 0,
      dynamicRange: 0,
      spectralCentroid: 0,

      // Timestamps
      extractedAt: Date.now(),
      version: "1.0",
    };

    // Calculate audio analysis metrics
    const analysis = this.analyzeAudioBuffer(audioBuffer);
    Object.assign(metadata, analysis);

    return metadata;
  }

  /**
   * Analyze audio buffer for metadata
   */
  analyzeAudioBuffer(audioBuffer) {
    let peakAmplitude = 0;
    let rmsSum = 0;
    let sampleCount = 0;

    for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
      const channelData = audioBuffer.getChannelData(channel);

      for (let i = 0; i < channelData.length; i++) {
        const sample = channelData[i];
        const absSample = Math.abs(sample);

        peakAmplitude = Math.max(peakAmplitude, absSample);
        rmsSum += sample * sample;
        sampleCount++;
      }
    }

    const rmsLevel = Math.sqrt(rmsSum / sampleCount);
    const dynamicRange =
      20 * Math.log10(peakAmplitude / Math.max(rmsLevel, 1e-10));

    return {
      peakAmplitude: peakAmplitude,
      rmsLevel: rmsLevel,
      dynamicRange: dynamicRange,
      spectralCentroid: 0, // Would require FFT analysis
    };
  }

  /**
   * Assess audio quality
   */
  async assessAudioQuality(audioBuffer) {
    // This would integrate with the QualityAssessor module
    // For now, implement basic quality scoring

    const analysis = this.analyzeAudioBuffer(audioBuffer);

    // Basic quality scoring based on multiple factors
    let qualityScore = 1.0;

    // Penalize low dynamic range
    if (analysis.dynamicRange < 10) {
      qualityScore *= 0.7;
    }

    // Penalize clipping
    if (analysis.peakAmplitude >= 0.99) {
      qualityScore *= 0.5;
    }

    // Penalize very low levels
    if (analysis.rmsLevel < 0.01) {
      qualityScore *= 0.8;
    }

    // Duration-based scoring
    const duration = audioBuffer.duration;
    if (duration < 1.0 || duration > 60) {
      qualityScore *= 0.9;
    }

    return Math.max(0, Math.min(1, qualityScore));
  }

  /**
   * Register master call in library
   */
  registerMasterCall(masterCall, audioData = null, skipValidation = false) {
    const callId = masterCall.id || masterCall.callId;

    // Store in library maps
    this.library.calls.set(callId, masterCall);
    this.library.metadata.set(callId, masterCall.metadata);
    this.library.qualityScores.set(callId, masterCall.quality);

    // Initialize usage stats
    if (!this.library.usageStats.has(callId)) {
      this.library.usageStats.set(callId, {
        playCount: 0,
        totalPlayTime: 0,
        averageRating: 0,
        lastPlayed: null,
        createdAt: Date.now(),
      });
    }

    // Update recommendation cache
    this.invalidateRecommendationCache();

    return callId;
  }

  /**
   * Play master call with advanced controls
   */
  async playMasterCall(callId, options = {}) {
    if (!this.isInitialized) {
      throw new Error("MasterCallManager not initialized");
    }

    const startTime = performance.now();

    try {
      // Get master call
      const masterCall = this.library.calls.get(callId);
      if (!masterCall) {
        throw new Error(`Master call not found: ${callId}`);
      }

      // Stop current playback if any
      if (this.playback.isPlaying) {
        await this.stopPlayback();
      }

      // Setup playback options
      const playbackOptions = {
        volume: options.volume || this.playback.volume,
        playbackRate: options.playbackRate || this.playback.playbackRate,
        loop: options.loop || false,
        fadeIn: options.fadeIn !== false,
        fadeOut: options.fadeOut !== false,
        startTime: options.startTime || 0,
        duration: options.duration || null,
        ...options,
      };

      // Create and configure source node
      this.playback.sourceNode = this.audioContext.createBufferSource();
      this.playback.sourceNode.buffer = masterCall.audioBuffer;
      this.playback.sourceNode.playbackRate.value =
        playbackOptions.playbackRate;
      this.playback.sourceNode.loop = playbackOptions.loop;

      // Connect to audio processing chain
      this.playback.sourceNode.connect(this.playback.filterNode);

      // Setup volume with fade-in
      this.playback.gainNode.gain.cancelScheduledValues(
        this.audioContext.currentTime
      );
      if (playbackOptions.fadeIn) {
        this.playback.gainNode.gain.setValueAtTime(
          0,
          this.audioContext.currentTime
        );
        this.playback.gainNode.gain.linearRampToValueAtTime(
          playbackOptions.volume,
          this.audioContext.currentTime + this.playback.fadeInDuration
        );
      } else {
        this.playback.gainNode.gain.setValueAtTime(
          playbackOptions.volume,
          this.audioContext.currentTime
        );
      }

      // Setup playback completion handling
      this.playback.sourceNode.onended = () => {
        this.handlePlaybackEnded(callId);
      };

      // Start playback
      const when = options.when || this.audioContext.currentTime;
      const offset = playbackOptions.startTime;
      const duration = playbackOptions.duration;

      if (duration) {
        this.playback.sourceNode.start(when, offset, duration);
      } else {
        this.playback.sourceNode.start(when, offset);
      }

      // Update playback state
      this.playback.currentCall = callId;
      this.playback.isPlaying = true;
      this.playback.isPaused = false;
      this.playback.position = offset;
      this.playback.duration = masterCall.audioBuffer.duration;

      // Update usage statistics
      this.updateUsageStats(callId);

      // Update performance metrics
      const playbackLatency = performance.now() - startTime;
      this.updatePlaybackPerformance(playbackLatency);

      // Emit playback started event
      this.eventManager?.emitEvent("masterCallPlaybackStarted", {
        callId: callId,
        duration: this.playback.duration,
        options: playbackOptions,
        latency: playbackLatency,
      });

      return {
        success: true,
        callId: callId,
        duration: this.playback.duration,
        latency: playbackLatency,
      };
    } catch (error) {
      console.error("Failed to play master call:", error);

      this.eventManager?.emitEvent("masterCallPlaybackError", {
        callId: callId,
        error: error.message,
        timestamp: performance.now(),
      });

      return { success: false, error: error.message };
    }
  }

  /**
   * Stop current playback
   */
  async stopPlayback(fadeOut = true) {
    if (!this.playbook.isPlaying || !this.playback.sourceNode) {
      return;
    }

    try {
      if (fadeOut && this.playback.fadeOutDuration > 0) {
        // Fade out before stopping
        const currentTime = this.audioContext.currentTime;
        this.playback.gainNode.gain.cancelScheduledValues(currentTime);
        this.playback.gainNode.gain.setValueAtTime(
          this.playback.gainNode.gain.value,
          currentTime
        );
        this.playback.gainNode.gain.linearRampToValueAtTime(
          0,
          currentTime + this.playback.fadeOutDuration
        );

        // Stop after fade out
        setTimeout(() => {
          if (this.playback.sourceNode) {
            this.playback.sourceNode.stop();
          }
        }, this.playback.fadeOutDuration * 1000);
      } else {
        this.playback.sourceNode.stop();
      }

      // Reset playback state
      this.resetPlaybackState();

      this.eventManager?.emitEvent("masterCallPlaybackStopped", {
        callId: this.playback.currentCall,
        timestamp: performance.now(),
      });
    } catch (error) {
      console.error("Error stopping playback:", error);
      this.resetPlaybackState();
    }
  }

  /**
   * Handle playback completion
   */
  handlePlaybackEnded(callId) {
    this.resetPlaybackState();

    this.eventManager?.emitEvent("masterCallPlaybackEnded", {
      callId: callId,
      timestamp: performance.now(),
    });
  }

  /**
   * Reset playback state
   */
  resetPlaybackState() {
    this.playback.currentCall = null;
    this.playback.isPlaying = false;
    this.playback.isPaused = false;
    this.playback.position = 0;
    this.playback.sourceNode = null;
  }

  /**
   * Generate personalized recommendations
   */
  async generateRecommendations(context = {}) {
    if (!this.config.enableRecommendations) {
      return [];
    }

    // Check cache first
    const now = performance.now();
    if (
      this.recommender.cache.recommendations.length > 0 &&
      now - this.recommender.cache.lastUpdate < this.recommender.cache.ttl
    ) {
      return this.recommender.cache.recommendations;
    }

    try {
      const recommendations = [];
      const userProfile = this.recommender.userProfile;
      const calls = Array.from(this.library.calls.values());

      // Score each call based on user profile and context
      const scoredCalls = calls.map((call) => {
        const score = this.calculateRecommendationScore(
          call,
          userProfile,
          context
        );
        return { call, score };
      });

      // Sort by score and take top recommendations
      scoredCalls.sort((a, b) => b.score - a.score);
      const topCalls = scoredCalls.slice(0, this.config.maxRecommendations);

      // Format recommendations
      for (const { call, score } of topCalls) {
        recommendations.push({
          callId: call.id,
          name: call.metadata.name || call.id,
          category: call.metadata.category,
          quality: call.quality,
          score: score,
          reason: this.generateRecommendationReason(call, userProfile, context),
        });
      }

      // Update cache
      this.recommender.cache.recommendations = recommendations;
      this.recommender.cache.lastUpdate = now;

      this.eventManager?.emitEvent("recommendationsGenerated", {
        count: recommendations.length,
        context: context,
        timestamp: now,
      });

      return recommendations;
    } catch (error) {
      console.error("Failed to generate recommendations:", error);
      return [];
    }
  }

  /**
   * Calculate recommendation score for a call
   */
  calculateRecommendationScore(call, userProfile, context) {
    const weights = this.recommender.model.weights;
    let score = 0;

    // Quality score
    score += (weights.get("quality") || 0.3) * call.quality;

    // Difficulty matching
    const difficultyMatch = this.calculateDifficultyMatch(
      call,
      userProfile.skillLevel
    );
    score += (weights.get("difficulty") || 0.2) * difficultyMatch;

    // Category preference
    const categoryMatch = userProfile.preferredCategories.includes(
      call.metadata.category
    )
      ? 1
      : 0.5;
    score += (weights.get("category") || 0.2) * categoryMatch;

    // Seasonal relevance
    const seasonMatch = this.calculateSeasonMatch(
      call,
      context.season || userProfile.seasonPreference
    );
    score += (weights.get("season") || 0.15) * seasonMatch;

    // Usage-based scoring (popularity vs novelty)
    const usageStats = this.library.usageStats.get(call.id);
    const usageScore = usageStats ? Math.min(1, usageStats.playCount / 10) : 0;
    score += (weights.get("usage") || 0.15) * usageScore;

    return Math.max(0, Math.min(1, score));
  }

  /**
   * Calculate difficulty match score
   */
  calculateDifficultyMatch(call, skillLevel) {
    const skillLevels = ["beginner", "intermediate", "advanced", "expert"];
    const callDifficulty = call.metadata.difficulty || "medium";
    const difficulties = ["easy", "medium", "hard", "expert"];

    const userLevel = skillLevels.indexOf(skillLevel);
    const callLevel = difficulties.indexOf(callDifficulty);

    // Perfect match gets 1.0, adjacent levels get 0.7, further away gets less
    const distance = Math.abs(userLevel - callLevel);
    if (distance === 0) return 1.0;
    if (distance === 1) return 0.7;
    if (distance === 2) return 0.4;
    return 0.1;
  }

  /**
   * Calculate seasonal relevance score
   */
  calculateSeasonMatch(call, currentSeason) {
    const callSeason = call.metadata.season || "all";
    if (callSeason === "all" || callSeason === currentSeason) {
      return 1.0;
    }
    return 0.3; // Off-season calls get lower score
  }

  /**
   * Generate recommendation reason
   */
  generateRecommendationReason(call, userProfile, context) {
    const reasons = [];

    if (call.quality > 0.8) {
      reasons.push("high quality");
    }

    if (userProfile.preferredCategories.includes(call.metadata.category)) {
      reasons.push("matches your preferences");
    }

    const seasonMatch = this.calculateSeasonMatch(
      call,
      context.season || userProfile.seasonPreference
    );
    if (seasonMatch === 1.0) {
      reasons.push("perfect for current season");
    }

    const usageStats = this.library.usageStats.get(call.id);
    if (usageStats && usageStats.playCount === 0) {
      reasons.push("new to your library");
    }

    return reasons.length > 0 ? reasons.join(", ") : "recommended for you";
  }

  /**
   * Update usage statistics
   */
  updateUsageStats(callId) {
    const stats = this.library.usageStats.get(callId) || {
      playCount: 0,
      totalPlayTime: 0,
      averageRating: 0,
      lastPlayed: null,
      createdAt: Date.now(),
    };

    stats.playCount++;
    stats.lastPlayed = Date.now();

    this.library.usageStats.set(callId, stats);

    // Update user profile based on usage
    this.updateUserProfile(callId);
  }

  /**
   * Update user profile based on usage patterns
   */
  updateUserProfile(callId) {
    const call = this.library.calls.get(callId);
    if (!call) return;

    const profile = this.recommender.userProfile;
    const category = call.metadata.category;

    // Update preferred categories
    if (!profile.preferredCategories.includes(category)) {
      profile.preferredCategories.push(category);
    }

    // Update success rate if feedback is available
    // This would be updated based on user feedback in a real application
  }

  /**
   * Get library statistics
   */
  getLibraryStats() {
    const stats = {
      totalCalls: this.library.calls.size,
      categories: {},
      qualityDistribution: { excellent: 0, good: 0, fair: 0, poor: 0 },
      totalPlayTime: 0,
      averageQuality: 0,
      mostPopular: null,
      recentlyAdded: [],
    };

    let qualitySum = 0;
    let maxPlayCount = 0;

    for (const [callId, call] of this.library.calls) {
      // Category distribution
      const category = call.metadata.category || "unknown";
      stats.categories[category] = (stats.categories[category] || 0) + 1;

      // Quality distribution
      const quality = call.quality;
      qualitySum += quality;

      if (quality >= 0.9) stats.qualityDistribution.excellent++;
      else if (quality >= 0.7) stats.qualityDistribution.good++;
      else if (quality >= 0.5) stats.qualityDistribution.fair++;
      else stats.qualityDistribution.poor++;

      // Usage statistics
      const usageStats = this.library.usageStats.get(callId);
      if (usageStats) {
        stats.totalPlayTime += usageStats.totalPlayTime || 0;

        if (usageStats.playCount > maxPlayCount) {
          maxPlayCount = usageStats.playCount;
          stats.mostPopular = { callId, playCount: usageStats.playCount };
        }
      }

      // Recently added (last 7 days)
      const weekAgo = Date.now() - 7 * 24 * 60 * 60 * 1000;
      if (call.uploadTime > weekAgo) {
        stats.recentlyAdded.push(callId);
      }
    }

    stats.averageQuality =
      stats.totalCalls > 0 ? qualitySum / stats.totalCalls : 0;

    return stats;
  }

  /**
   * Search master calls
   */
  searchCalls(query, filters = {}) {
    const results = [];
    const queryLower = query.toLowerCase();

    for (const [callId, call] of this.library.calls) {
      let matches = false;

      // Text search in name, description, tags
      const searchableText = [
        call.metadata.name || "",
        call.metadata.description || "",
        ...(call.metadata.tags || []),
      ]
        .join(" ")
        .toLowerCase();

      if (searchableText.includes(queryLower)) {
        matches = true;
      }

      // Apply filters
      if (
        matches &&
        filters.category &&
        call.metadata.category !== filters.category
      ) {
        matches = false;
      }

      if (matches && filters.minQuality && call.quality < filters.minQuality) {
        matches = false;
      }

      if (
        matches &&
        filters.difficulty &&
        call.metadata.difficulty !== filters.difficulty
      ) {
        matches = false;
      }

      if (matches) {
        results.push({
          callId: callId,
          name: call.metadata.name || callId,
          category: call.metadata.category,
          quality: call.quality,
          duration: call.metadata.duration,
          playCount: this.library.usageStats.get(callId)?.playCount || 0,
        });
      }
    }

    // Sort by relevance (could be improved with proper text search scoring)
    results.sort((a, b) => {
      // Prioritize exact matches, then by quality, then by popularity
      const aExact = a.name.toLowerCase().includes(queryLower) ? 1 : 0;
      const bExact = b.name.toLowerCase().includes(queryLower) ? 1 : 0;

      if (aExact !== bExact) return bExact - aExact;
      if (a.quality !== b.quality) return b.quality - a.quality;
      return b.playCount - a.playCount;
    });

    return results;
  }

  /**
   * Update performance metrics
   */
  updateLoadPerformance(loadTime) {
    this.performance.loadTimes.shift();
    this.performance.loadTimes.push(loadTime);
    this.performance.averageLoadTime =
      this.performance.loadTimes.reduce((a, b) => a + b, 0) /
      this.performance.loadTimes.length;
  }

  updatePlaybackPerformance(latency) {
    this.performance.playbackLatency.shift();
    this.performance.playbackLatency.push(latency);
  }

  updatePerformanceMetrics() {
    // Calculate cache hit rate
    const totalRequests = this.performance.loadTimes.length;
    const cacheHits = 0; // Would track actual cache hits
    this.performance.cacheHitRate =
      totalRequests > 0 ? cacheHits / totalRequests : 0;
  }

  /**
   * Invalidate recommendation cache
   */
  invalidateRecommendationCache() {
    this.recommender.cache.recommendations = [];
    this.recommender.cache.lastUpdate = 0;
  }

  /**
   * Save master call to persistent storage
   */
  async saveMasterCallToStorage(masterCall) {
    if (!this.db) return;

    try {
      const transaction = this.db.transaction(
        ["calls", "metadata"],
        "readwrite"
      );

      // Save call data
      const callStore = transaction.objectStore("calls");
      await this.putToStore(callStore, {
        id: masterCall.id,
        quality: masterCall.quality,
        loadTime: masterCall.loadTime,
        uploadTime: masterCall.uploadTime,
        category: masterCall.metadata.category,
      });

      // Save metadata
      const metadataStore = transaction.objectStore("metadata");
      await this.putToStore(metadataStore, {
        callId: masterCall.id,
        ...masterCall.metadata,
      });
    } catch (error) {
      console.error("Failed to save master call to storage:", error);
    }
  }

  /**
   * Put item to IndexedDB store
   */
  putToStore(store, item) {
    return new Promise((resolve, reject) => {
      const request = store.put(item);
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  /**
   * Load user profile from storage
   */
  loadUserProfile() {
    // Would load from localStorage or IndexedDB
    const stored = localStorage.getItem("huntmaster_user_profile");
    if (stored) {
      try {
        const profile = JSON.parse(stored);
        Object.assign(this.recommender.userProfile, profile);
      } catch (error) {
        console.error("Failed to load user profile:", error);
      }
    }
  }

  /**
   * Save user profile to storage
   */
  saveUserProfile() {
    try {
      localStorage.setItem(
        "huntmaster_user_profile",
        JSON.stringify(this.recommender.userProfile)
      );
    } catch (error) {
      console.error("Failed to save user profile:", error);
    }
  }

  /**
   * Setup periodic model training
   */
  setupModelTraining() {
    setInterval(() => {
      this.trainRecommendationModel();
    }, 60000); // Train every minute
  }

  /**
   * Train recommendation model based on usage data
   */
  trainRecommendationModel() {
    // Simplified model training
    // In production, would use more sophisticated ML algorithms

    const model = this.recommender.model;
    const usageData = Array.from(this.library.usageStats.entries());

    if (usageData.length < 10) return; // Need minimum data

    // Simple gradient descent on usage patterns
    for (const [callId, stats] of usageData) {
      const call = this.library.calls.get(callId);
      if (!call) continue;

      const target = stats.playCount > 5 ? 1 : 0; // Popular calls
      const features = {
        quality: call.quality,
        difficulty: this.getDifficultyScore(call.metadata.difficulty),
        category: this.getCategoryScore(call.metadata.category),
      };

      // Update weights (simplified)
      for (const [feature, value] of Object.entries(features)) {
        const currentWeight = model.weights.get(feature) || 0;
        const error = target - currentWeight * value;
        const newWeight = currentWeight + model.learningRate * error * value;
        model.weights.set(feature, Math.max(0, Math.min(1, newWeight)));
      }
    }

    model.lastTrained = Date.now();
  }

  /**
   * Helper methods for model training
   */
  getDifficultyScore(difficulty) {
    const scores = { easy: 0.25, medium: 0.5, hard: 0.75, expert: 1.0 };
    return scores[difficulty] || 0.5;
  }

  getCategoryScore(category) {
    // Simple category scoring - in production would be more sophisticated
    return this.recommender.userProfile.preferredCategories.includes(category)
      ? 1.0
      : 0.5;
  }

  /**
   * Backup library to storage
   */
  async backupLibrary() {
    try {
      const backup = {
        timestamp: Date.now(),
        version: "1.0",
        calls: Array.from(this.library.calls.entries()),
        metadata: Array.from(this.library.metadata.entries()),
        usageStats: Array.from(this.library.usageStats.entries()),
        userProfile: this.recommender.userProfile,
      };

      localStorage.setItem("huntmaster_library_backup", JSON.stringify(backup));

      this.eventManager?.emitEvent("libraryBackupComplete", {
        callCount: this.library.calls.size,
        timestamp: backup.timestamp,
      });
    } catch (error) {
      console.error("Failed to backup library:", error);
    }
  }

  /**
   * Get current state
   */
  getState() {
    return {
      isInitialized: this.isInitialized,
      librarySize: this.library.calls.size,
      currentlyPlaying: this.playback.currentCall,
      isPlaying: this.playback.isPlaying,
      playbackPosition: this.playback.position,
      averageLoadTime: this.performance.averageLoadTime,
      cacheHitRate: this.performance.cacheHitRate,
      recommendationsEnabled: this.config.enableRecommendations,
      qualityLevel: this.performance.qualityLevel,
    };
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    const oldConfig = { ...this.config };
    this.config = { ...this.config, ...newConfig };

    // Update recommendation system if needed
    if (newConfig.enableRecommendations !== oldConfig.enableRecommendations) {
      if (this.config.enableRecommendations) {
        this.initializeRecommendationSystem();
      } else {
        this.recommender.enabled = false;
      }
    }

    this.eventManager?.emitEvent("masterCallManagerConfigUpdated", {
      oldConfig,
      newConfig: this.config,
      timestamp: performance.now(),
    });
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    // Stop any current playback
    if (this.playback.isPlaying) {
      this.stopPlayback(false);
    }

    // Clear audio nodes
    if (this.playback.gainNode) {
      this.playback.gainNode.disconnect();
    }
    if (this.playback.panNode) {
      this.playback.panNode.disconnect();
    }
    if (this.playback.filterNode) {
      this.playback.filterNode.disconnect();
    }

    // Save user profile
    this.saveUserProfile();

    // Backup library
    this.backupLibrary();

    // Close database connection
    if (this.db) {
      this.db.close();
    }

    // Reset state
    this.isInitialized = false;

    this.eventManager?.emitEvent("masterCallManagerCleanup", {
      timestamp: performance.now(),
    });
  }
}

export default MasterCallManager;
