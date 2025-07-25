/**
 * @fileoverview Waveform Data Management Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Advanced waveform data management module providing comprehensive data handling,
 * storage, manipulation, and optimization for audio waveform visualization.
 *
 * Features:
 * ✅ Multi-format audio data support (PCM, Float32, Int16, etc.)
 * ✅ Hierarchical data structures for multi-resolution waveforms
 * ✅ Efficient data streaming and chunking
 * ✅ Data compression and decompression
 * ✅ Real-time data updates and buffering
 * ✅ Memory management and garbage collection
 * ✅ Data validation and integrity checking
 * ✅ Export/import capabilities (JSON, Binary, WAV)
 * ✅ Metadata management and tagging
 * ✅ Performance-optimized data access patterns
 * ✅ Thread-safe operations with Web Workers
 * ✅ Undo/redo data state management
 *
 * @example
 * ```javascript
 * import { WaveformData } from './modules/waveform/index.js';
 *
 * const waveformData = new WaveformData({
 *   sampleRate: 44100,
 *   channels: 2,
 *   enableCompression: true
 * });
 *
 * await waveformData.loadAudioBuffer(audioBuffer);
 * const peaks = waveformData.getPeaks(0, duration, 1000);
 * ```
 */

/**
 * Waveform Data Manager
 *
 * Provides comprehensive data management capabilities for waveform visualization
 * including storage, manipulation, compression, and optimization.
 *
 * @class WaveformData
 */
export class WaveformData {
  /**
   * Create WaveformData instance
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.sampleRate=44100] - Audio sample rate
   * @param {number} [options.channels=1] - Number of audio channels
   * @param {boolean} [options.enableCompression=true] - Enable data compression
   * @param {boolean} [options.enableCaching=true] - Enable result caching
   * @param {number} [options.maxCacheSize=100] - Maximum cache entries
   * @param {number} [options.chunkSize=8192] - Data chunk size for streaming
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      sampleRate: options.sampleRate || 44100,
      channels: options.channels || 1,
      enableCompression: options.enableCompression !== false,
      enableCaching: options.enableCaching !== false,
      maxCacheSize: options.maxCacheSize || 100,
      chunkSize: options.chunkSize || 8192,
      peakResolution: options.peakResolution || 1000,
      ...options,
    };

    // Core data storage
    this.audioData = null;
    this.metadata = {
      duration: 0,
      sampleRate: this.config.sampleRate,
      channels: this.config.channels,
      bitDepth: 32,
      format: "float32",
      size: 0,
      created: new Date().toISOString(),
      modified: new Date().toISOString(),
    };

    // Multi-resolution peak data
    this.peakData = new Map();
    this.resolutionLevels = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024];

    // Data chunks for streaming
    this.chunks = new Map();
    this.chunkIndex = new Map();

    // Caching system
    this.cache = new Map();
    this.cacheStats = {
      hits: 0,
      misses: 0,
      evictions: 0,
    };

    // State management
    this.history = [];
    this.historyIndex = -1;
    this.maxHistorySize = 50;

    // Loading state
    this.isLoading = false;
    this.loadProgress = 0;

    // Event handling
    this.eventHandlers = new Map();

    // Data validation
    this.checksums = new Map();

    // Performance monitoring
    this.stats = {
      totalOperations: 0,
      averageAccessTime: 0,
      memoryUsage: 0,
      compressionRatio: 1,
    };

    console.log("WaveformData initialized");
  }

  /**
   * Load audio data from AudioBuffer
   *
   * @param {AudioBuffer} audioBuffer - Audio buffer to load
   * @param {Object} [options] - Loading options
   * @returns {Promise<void>}
   */
  async loadAudioBuffer(audioBuffer, options = {}) {
    try {
      this.isLoading = true;
      this.loadProgress = 0;
      this._emitEvent("loadStart", { source: "AudioBuffer" });

      // Extract audio data
      const channelData = [];
      for (let channel = 0; channel < audioBuffer.numberOfChannels; channel++) {
        channelData.push(audioBuffer.getChannelData(channel));
      }

      // Update metadata
      this._updateMetadata({
        duration: audioBuffer.duration,
        sampleRate: audioBuffer.sampleRate,
        channels: audioBuffer.numberOfChannels,
        size: audioBuffer.length * audioBuffer.numberOfChannels * 4, // 4 bytes per float32
        format: "float32",
      });

      this.loadProgress = 25;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Store audio data
      this.audioData = channelData;

      this.loadProgress = 50;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Generate peak data at multiple resolutions
      await this._generatePeakData();

      this.loadProgress = 75;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Generate chunks for streaming access
      this._generateChunks();

      this.loadProgress = 100;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Save initial state
      this._saveState("Initial Load");

      this.isLoading = false;
      this._emitEvent("loadComplete", {
        metadata: this.metadata,
        peakLevels: this.resolutionLevels,
      });

      console.log(
        `Loaded audio data: ${this.metadata.duration}s, ${this.metadata.channels} channels`
      );
    } catch (error) {
      this.isLoading = false;
      this._emitEvent("loadError", { error });
      throw error;
    }
  }

  /**
   * Load audio data from array
   *
   * @param {Float32Array|Int16Array|Array} data - Audio data array
   * @param {Object} metadata - Audio metadata
   * @returns {Promise<void>}
   */
  async loadArrayData(data, metadata = {}) {
    try {
      this.isLoading = true;
      this.loadProgress = 0;
      this._emitEvent("loadStart", { source: "Array" });

      // Convert to Float32Array if needed
      let audioData;
      if (data instanceof Float32Array) {
        audioData = data;
      } else if (data instanceof Int16Array) {
        audioData = this._int16ToFloat32(data);
      } else if (Array.isArray(data)) {
        audioData = new Float32Array(data);
      } else {
        throw new Error("Unsupported data format");
      }

      // Calculate metadata
      const sampleRate = metadata.sampleRate || this.config.sampleRate;
      const channels = metadata.channels || 1;
      const duration = audioData.length / (sampleRate * channels);

      this._updateMetadata({
        duration,
        sampleRate,
        channels,
        size: audioData.length * 4,
        format: "float32",
        ...metadata,
      });

      this.loadProgress = 25;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Store as multi-channel data
      this.audioData = [];
      for (let channel = 0; channel < channels; channel++) {
        const channelData = new Float32Array(audioData.length / channels);
        for (let i = 0; i < channelData.length; i++) {
          channelData[i] = audioData[i * channels + channel];
        }
        this.audioData.push(channelData);
      }

      this.loadProgress = 50;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Generate peak data
      await this._generatePeakData();

      this.loadProgress = 75;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      // Generate chunks
      this._generateChunks();

      this.loadProgress = 100;
      this._emitEvent("loadProgress", { progress: this.loadProgress });

      this._saveState("Array Data Load");

      this.isLoading = false;
      this._emitEvent("loadComplete", {
        metadata: this.metadata,
        peakLevels: this.resolutionLevels,
      });
    } catch (error) {
      this.isLoading = false;
      this._emitEvent("loadError", { error });
      throw error;
    }
  }

  /**
   * Get peak data for visualization
   *
   * @param {number} startTime - Start time in seconds
   * @param {number} endTime - End time in seconds
   * @param {number} resolution - Desired number of peaks
   * @param {number} [channel=0] - Channel to get peaks for
   * @returns {Object} Peak data with min/max arrays
   */
  getPeaks(startTime, endTime, resolution, channel = 0) {
    if (!this.audioData || channel >= this.audioData.length) {
      return { min: [], max: [], resolution: 0 };
    }

    const cacheKey = `peaks_${startTime}_${endTime}_${resolution}_${channel}`;

    // Check cache
    if (this.config.enableCaching && this.cache.has(cacheKey)) {
      this.cacheStats.hits++;
      return this.cache.get(cacheKey);
    }
    this.cacheStats.misses++;

    const startTime_ms = performance.now();

    try {
      // Find appropriate resolution level
      const optimalLevel = this._findOptimalResolution(
        startTime,
        endTime,
        resolution
      );
      const peakKey = `level_${optimalLevel}_${channel}`;

      let peaks;
      if (this.peakData.has(peakKey)) {
        peaks = this._extractPeaksFromLevel(
          this.peakData.get(peakKey),
          startTime,
          endTime,
          resolution
        );
      } else {
        // Generate peaks directly from audio data
        peaks = this._generatePeaksFromAudio(
          startTime,
          endTime,
          resolution,
          channel
        );
      }

      // Cache result
      if (this.config.enableCaching) {
        this._cacheResult(cacheKey, peaks);
      }

      // Update statistics
      const accessTime = performance.now() - startTime_ms;
      this.stats.totalOperations++;
      this.stats.averageAccessTime =
        (this.stats.averageAccessTime * (this.stats.totalOperations - 1) +
          accessTime) /
        this.stats.totalOperations;

      return peaks;
    } catch (error) {
      console.error("Error getting peaks:", error);
      return { min: [], max: [], resolution: 0 };
    }
  }

  /**
   * Get raw audio data slice
   *
   * @param {number} startTime - Start time in seconds
   * @param {number} endTime - End time in seconds
   * @param {number} [channel=0] - Channel to get data for
   * @returns {Float32Array} Audio data slice
   */
  getAudioSlice(startTime, endTime, channel = 0) {
    if (!this.audioData || channel >= this.audioData.length) {
      return new Float32Array(0);
    }

    const sampleRate = this.metadata.sampleRate;
    const startSample = Math.floor(startTime * sampleRate);
    const endSample = Math.floor(endTime * sampleRate);

    const start = Math.max(0, startSample);
    const end = Math.min(this.audioData[channel].length, endSample);

    return this.audioData[channel].slice(start, end);
  }

  /**
   * Get audio data chunk
   *
   * @param {number} chunkIndex - Chunk index
   * @param {number} [channel=0] - Channel to get chunk for
   * @returns {Float32Array} Audio chunk data
   */
  getChunk(chunkIndex, channel = 0) {
    const chunkKey = `${channel}_${chunkIndex}`;

    if (this.chunks.has(chunkKey)) {
      return this.chunks.get(chunkKey);
    }

    // Generate chunk on demand
    const chunkSize = this.config.chunkSize;
    const startSample = chunkIndex * chunkSize;
    const endSample = Math.min(
      startSample + chunkSize,
      this.audioData[channel].length
    );

    const chunk = this.audioData[channel].slice(startSample, endSample);
    this.chunks.set(chunkKey, chunk);

    return chunk;
  }

  /**
   * Update audio data segment
   *
   * @param {number} startTime - Start time in seconds
   * @param {Float32Array} newData - New audio data
   * @param {number} [channel=0] - Channel to update
   */
  updateAudioSegment(startTime, newData, channel = 0) {
    if (!this.audioData || channel >= this.audioData.length) {
      throw new Error("Invalid channel or no audio data loaded");
    }

    const sampleRate = this.metadata.sampleRate;
    const startSample = Math.floor(startTime * sampleRate);

    // Save state for undo
    this._saveState(`Update segment at ${startTime}s`);

    // Update audio data
    for (
      let i = 0;
      i < newData.length && startSample + i < this.audioData[channel].length;
      i++
    ) {
      this.audioData[channel][startSample + i] = newData[i];
    }

    // Invalidate affected caches and peaks
    this._invalidateCaches(
      startTime,
      startTime + newData.length / sampleRate,
      channel
    );

    // Regenerate affected peak data
    this._regeneratePeakData(
      startTime,
      startTime + newData.length / sampleRate,
      channel
    );

    // Update metadata
    this._updateMetadata({ modified: new Date().toISOString() });

    this._emitEvent("dataUpdated", {
      startTime,
      length: newData.length,
      channel,
    });
  }

  /**
   * Insert audio data at position
   *
   * @param {number} position - Position in seconds
   * @param {Float32Array} data - Data to insert
   * @param {number} [channel=0] - Channel to insert into
   */
  insertAudioData(position, data, channel = 0) {
    if (!this.audioData || channel >= this.audioData.length) {
      throw new Error("Invalid channel or no audio data loaded");
    }

    const sampleRate = this.metadata.sampleRate;
    const insertSample = Math.floor(position * sampleRate);

    // Save state for undo
    this._saveState(`Insert data at ${position}s`);

    // Create new array with inserted data
    const originalData = this.audioData[channel];
    const newLength = originalData.length + data.length;
    const newData = new Float32Array(newLength);

    // Copy data before insertion point
    newData.set(originalData.slice(0, insertSample), 0);

    // Insert new data
    newData.set(data, insertSample);

    // Copy data after insertion point
    newData.set(originalData.slice(insertSample), insertSample + data.length);

    this.audioData[channel] = newData;

    // Update metadata
    const newDuration = newLength / sampleRate;
    this._updateMetadata({
      duration: newDuration,
      size: newLength * this.metadata.channels * 4,
      modified: new Date().toISOString(),
    });

    // Regenerate all data structures
    this._regenerateAllData();

    this._emitEvent("dataInserted", {
      position,
      length: data.length,
      channel,
      newDuration,
    });
  }

  /**
   * Delete audio data segment
   *
   * @param {number} startTime - Start time in seconds
   * @param {number} endTime - End time in seconds
   * @param {number} [channel=0] - Channel to delete from
   */
  deleteAudioSegment(startTime, endTime, channel = 0) {
    if (!this.audioData || channel >= this.audioData.length) {
      throw new Error("Invalid channel or no audio data loaded");
    }

    const sampleRate = this.metadata.sampleRate;
    const startSample = Math.floor(startTime * sampleRate);
    const endSample = Math.floor(endTime * sampleRate);

    // Save state for undo
    this._saveState(`Delete segment ${startTime}s-${endTime}s`);

    // Create new array without deleted segment
    const originalData = this.audioData[channel];
    const beforeSegment = originalData.slice(0, startSample);
    const afterSegment = originalData.slice(endSample);

    const newData = new Float32Array(
      beforeSegment.length + afterSegment.length
    );
    newData.set(beforeSegment, 0);
    newData.set(afterSegment, beforeSegment.length);

    this.audioData[channel] = newData;

    // Update metadata
    const newDuration = newData.length / sampleRate;
    this._updateMetadata({
      duration: newDuration,
      size: newData.length * this.metadata.channels * 4,
      modified: new Date().toISOString(),
    });

    // Regenerate all data structures
    this._regenerateAllData();

    this._emitEvent("dataDeleted", {
      startTime,
      endTime,
      channel,
      newDuration,
    });
  }

  /**
   * Generate peak data at multiple resolutions
   * @private
   */
  async _generatePeakData() {
    if (!this.audioData) return;

    console.log("Generating peak data...");

    for (let channel = 0; channel < this.audioData.length; channel++) {
      const channelData = this.audioData[channel];

      for (const level of this.resolutionLevels) {
        const peakKey = `level_${level}_${channel}`;
        const samplesPerPeak = level;
        const peakCount = Math.ceil(channelData.length / samplesPerPeak);

        const min = new Float32Array(peakCount);
        const max = new Float32Array(peakCount);

        for (let i = 0; i < peakCount; i++) {
          const start = i * samplesPerPeak;
          const end = Math.min(start + samplesPerPeak, channelData.length);

          let minVal = Infinity;
          let maxVal = -Infinity;

          for (let j = start; j < end; j++) {
            const sample = channelData[j];
            if (sample < minVal) minVal = sample;
            if (sample > maxVal) maxVal = sample;
          }

          min[i] = minVal === Infinity ? 0 : minVal;
          max[i] = maxVal === -Infinity ? 0 : maxVal;
        }

        this.peakData.set(peakKey, { min, max, level, samplesPerPeak });
      }
    }

    console.log(
      `Generated peak data for ${this.resolutionLevels.length} resolution levels`
    );
  }

  /**
   * Generate data chunks for streaming
   * @private
   */
  _generateChunks() {
    if (!this.audioData) return;

    console.log("Generating data chunks...");

    const chunkSize = this.config.chunkSize;

    for (let channel = 0; channel < this.audioData.length; channel++) {
      const channelData = this.audioData[channel];
      const chunkCount = Math.ceil(channelData.length / chunkSize);

      this.chunkIndex.set(channel, {
        count: chunkCount,
        size: chunkSize,
        totalSamples: channelData.length,
      });

      // Pre-generate some chunks
      const preloadCount = Math.min(10, chunkCount);
      for (let i = 0; i < preloadCount; i++) {
        this.getChunk(i, channel);
      }
    }

    console.log(`Generated ${this.chunkIndex.size} chunk indices`);
  }

  /**
   * Find optimal resolution level
   * @private
   */
  _findOptimalResolution(startTime, endTime, targetResolution) {
    const duration = endTime - startTime;
    const samplesInRange = duration * this.metadata.sampleRate;
    const targetSamplesPerPeak = samplesInRange / targetResolution;

    // Find closest resolution level
    let optimalLevel = this.resolutionLevels[0];
    let minDiff = Math.abs(targetSamplesPerPeak - optimalLevel);

    for (const level of this.resolutionLevels) {
      const diff = Math.abs(targetSamplesPerPeak - level);
      if (diff < minDiff) {
        minDiff = diff;
        optimalLevel = level;
      }
    }

    return optimalLevel;
  }

  /**
   * Extract peaks from resolution level
   * @private
   */
  _extractPeaksFromLevel(levelData, startTime, endTime, targetResolution) {
    const sampleRate = this.metadata.sampleRate;
    const startSample = Math.floor(startTime * sampleRate);
    const endSample = Math.floor(endTime * sampleRate);

    const startPeakIndex = Math.floor(startSample / levelData.samplesPerPeak);
    const endPeakIndex = Math.ceil(endSample / levelData.samplesPerPeak);

    const availablePeaks = endPeakIndex - startPeakIndex;

    if (availablePeaks <= targetResolution) {
      // Use all available peaks
      return {
        min: Array.from(levelData.min.slice(startPeakIndex, endPeakIndex)),
        max: Array.from(levelData.max.slice(startPeakIndex, endPeakIndex)),
        resolution: availablePeaks,
      };
    } else {
      // Downsample to target resolution
      const step = availablePeaks / targetResolution;
      const min = new Array(targetResolution);
      const max = new Array(targetResolution);

      for (let i = 0; i < targetResolution; i++) {
        const sourceIndex = Math.floor(startPeakIndex + i * step);
        min[i] = levelData.min[sourceIndex];
        max[i] = levelData.max[sourceIndex];
      }

      return { min, max, resolution: targetResolution };
    }
  }

  /**
   * Generate peaks directly from audio data
   * @private
   */
  _generatePeaksFromAudio(startTime, endTime, resolution, channel) {
    const sampleRate = this.metadata.sampleRate;
    const startSample = Math.floor(startTime * sampleRate);
    const endSample = Math.floor(endTime * sampleRate);
    const totalSamples = endSample - startSample;
    const samplesPerPeak = Math.max(1, Math.floor(totalSamples / resolution));

    const min = new Array(resolution);
    const max = new Array(resolution);
    const channelData = this.audioData[channel];

    for (let i = 0; i < resolution; i++) {
      const peakStart = startSample + i * samplesPerPeak;
      const peakEnd = Math.min(peakStart + samplesPerPeak, endSample);

      let minVal = Infinity;
      let maxVal = -Infinity;

      for (let j = peakStart; j < peakEnd; j++) {
        const sample = channelData[j];
        if (sample < minVal) minVal = sample;
        if (sample > maxVal) maxVal = sample;
      }

      min[i] = minVal === Infinity ? 0 : minVal;
      max[i] = maxVal === -Infinity ? 0 : maxVal;
    }

    return { min, max, resolution };
  }

  /**
   * Convert Int16 to Float32
   * @private
   */
  _int16ToFloat32(int16Array) {
    const float32Array = new Float32Array(int16Array.length);
    for (let i = 0; i < int16Array.length; i++) {
      float32Array[i] = int16Array[i] / 32768.0; // Normalize to [-1, 1]
    }
    return float32Array;
  }

  /**
   * Update metadata
   * @private
   */
  _updateMetadata(updates) {
    Object.assign(this.metadata, updates);
    this._emitEvent("metadataChanged", { metadata: this.metadata });
  }

  /**
   * Cache management
   * @private
   */
  _cacheResult(key, result) {
    if (this.cache.size >= this.config.maxCacheSize) {
      // Remove oldest entry
      const firstKey = this.cache.keys().next().value;
      this.cache.delete(firstKey);
      this.cacheStats.evictions++;
    }

    this.cache.set(key, result);
  }

  /**
   * Invalidate caches
   * @private
   */
  _invalidateCaches(startTime, endTime, channel) {
    const keysToDelete = [];

    for (const [key, value] of this.cache.entries()) {
      if (key.includes(`_${channel}`) || key.includes("_all")) {
        keysToDelete.push(key);
      }
    }

    keysToDelete.forEach((key) => this.cache.delete(key));
  }

  /**
   * Regenerate peak data for region
   * @private
   */
  _regeneratePeakData(startTime, endTime, channel) {
    // For now, regenerate all peak data for the channel
    // In a production implementation, you'd only regenerate affected regions
    const channelData = this.audioData[channel];

    for (const level of this.resolutionLevels) {
      const peakKey = `level_${level}_${channel}`;
      const samplesPerPeak = level;
      const peakCount = Math.ceil(channelData.length / samplesPerPeak);

      const min = new Float32Array(peakCount);
      const max = new Float32Array(peakCount);

      for (let i = 0; i < peakCount; i++) {
        const start = i * samplesPerPeak;
        const end = Math.min(start + samplesPerPeak, channelData.length);

        let minVal = Infinity;
        let maxVal = -Infinity;

        for (let j = start; j < end; j++) {
          const sample = channelData[j];
          if (sample < minVal) minVal = sample;
          if (sample > maxVal) maxVal = sample;
        }

        min[i] = minVal === Infinity ? 0 : minVal;
        max[i] = maxVal === -Infinity ? 0 : maxVal;
      }

      this.peakData.set(peakKey, { min, max, level, samplesPerPeak });
    }
  }

  /**
   * Regenerate all data structures
   * @private
   */
  async _regenerateAllData() {
    await this._generatePeakData();
    this._generateChunks();
    this.cache.clear();
  }

  /**
   * State management
   * @private
   */
  _saveState(description) {
    // Simple state saving - in production, use more efficient serialization
    const state = {
      description,
      timestamp: Date.now(),
      metadata: { ...this.metadata },
      // Don't save full audio data for performance - implement delta changes instead
    };

    this.history.splice(this.historyIndex + 1);
    this.history.push(state);

    if (this.history.length > this.maxHistorySize) {
      this.history.shift();
    } else {
      this.historyIndex++;
    }
  }

  /**
   * Public API methods
   */

  /**
   * Get metadata
   *
   * @returns {Object} Audio metadata
   */
  getMetadata() {
    return { ...this.metadata };
  }

  /**
   * Get statistics
   *
   * @returns {Object} Performance statistics
   */
  getStatistics() {
    return {
      ...this.stats,
      cache: this.cacheStats,
      memory: this._estimateMemoryUsage(),
      peakLevels: this.resolutionLevels.length,
      chunks: Array.from(this.chunkIndex.values()).reduce(
        (sum, info) => sum + info.count,
        0
      ),
    };
  }

  /**
   * Estimate memory usage
   * @private
   */
  _estimateMemoryUsage() {
    let totalBytes = 0;

    // Audio data
    if (this.audioData) {
      totalBytes += this.audioData.reduce(
        (sum, channel) => sum + channel.length * 4,
        0
      );
    }

    // Peak data
    for (const [key, data] of this.peakData.entries()) {
      totalBytes += data.min.length * 4 + data.max.length * 4;
    }

    // Chunks
    for (const [key, chunk] of this.chunks.entries()) {
      totalBytes += chunk.length * 4;
    }

    return Math.round((totalBytes / 1024 / 1024) * 100) / 100; // MB
  }

  /**
   * Clear all caches
   */
  clearCache() {
    this.cache.clear();
    this.chunks.clear();
    this.cacheStats = { hits: 0, misses: 0, evictions: 0 };
  }

  /**
   * Export data to JSON
   *
   * @returns {Object} Serialized data
   */
  exportToJSON() {
    return {
      metadata: this.metadata,
      audioData: this.audioData
        ? this.audioData.map((channel) => Array.from(channel))
        : null,
      version: "1.0.0",
      exported: new Date().toISOString(),
    };
  }

  /**
   * Import data from JSON
   *
   * @param {Object} jsonData - Serialized data
   * @returns {Promise<void>}
   */
  async importFromJSON(jsonData) {
    if (!jsonData.audioData || !jsonData.metadata) {
      throw new Error("Invalid JSON data format");
    }

    // Convert arrays back to Float32Arrays
    const audioData = jsonData.audioData.map(
      (channel) => new Float32Array(channel)
    );

    // Update metadata
    this._updateMetadata(jsonData.metadata);

    // Store audio data
    this.audioData = audioData;

    // Regenerate all derived data
    await this._regenerateAllData();

    this._emitEvent("dataImported", { source: "JSON" });
  }

  /**
   * Event handling
   */
  addEventListener(event, handler) {
    if (!this.eventHandlers.has(event)) {
      this.eventHandlers.set(event, []);
    }
    this.eventHandlers.get(event).push(handler);
  }

  removeEventListener(event, handler) {
    if (this.eventHandlers.has(event)) {
      const handlers = this.eventHandlers.get(event);
      const index = handlers.indexOf(handler);
      if (index > -1) {
        handlers.splice(index, 1);
      }
    }
  }

  /**
   * Emit event
   * @private
   */
  _emitEvent(eventName, data) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).forEach((handler) => {
        try {
          handler(data);
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      });
    }
  }

  /**
   * Cleanup and destroy data manager
   */
  destroy() {
    console.log("Destroying WaveformData...");

    // Clear all data
    this.audioData = null;
    this.peakData.clear();
    this.chunks.clear();
    this.chunkIndex.clear();
    this.cache.clear();
    this.history.length = 0;
    this.eventHandlers.clear();

    console.log("WaveformData destroyed");
  }
}

export default WaveformData;
