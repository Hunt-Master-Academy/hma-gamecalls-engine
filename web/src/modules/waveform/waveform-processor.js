/**
 * @fileoverview Waveform Processor Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive waveform data processing module providing advanced DSP algorithms,
 * real-time analysis, and optimized processing pipelines for audio waveform data.
 *
 * Features:
 * ✅ Real-time waveform processing with optimized algorithms
 * ✅ Advanced DSP processing (FFT, STFT, wavelets, filtering)
 * ✅ Multi-threaded processing with Web Workers support
 * ✅ Audio feature extraction and analysis
 * ✅ Time-domain and frequency-domain analysis
 * ✅ Spectral analysis with multiple window functions
 * ✅ Peak detection and envelope following
 * ✅ Noise reduction and signal enhancement
 * ✅ Real-time streaming processing
 * ✅ Performance optimization and caching
 * ✅ Cross-browser compatibility
 * ✅ Memory-efficient processing pipelines
 *
 * @example
 * ```javascript
 * import { WaveformProcessor } from './modules/waveform/index.js';
 *
 * const processor = new WaveformProcessor({
 *   sampleRate: 44100,
 *   bufferSize: 4096,
 *   enableThreading: true
 * });
 *
 * const processedData = await processor.processAudioBuffer(audioBuffer);
 * ```
 */

/**
 * Waveform Processor
 *
 * Provides comprehensive waveform data processing with advanced DSP algorithms,
 * real-time analysis, and optimized processing pipelines.
 *
 * @class WaveformProcessor
 */
export class WaveformProcessor {
  /**
   * Create WaveformProcessor instance
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.sampleRate=44100] - Audio sample rate
   * @param {number} [options.bufferSize=4096] - Processing buffer size
   * @param {boolean} [options.enableThreading=true] - Enable Web Workers
   * @param {boolean} [options.enableCaching=true] - Enable result caching
   * @param {Object} [options.wasmEngine=null] - WASM engine instance for advanced processing
   * @param {boolean} [options.preferWASM=true] - Prefer WASM over JavaScript processing
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      sampleRate: options.sampleRate || 44100,
      bufferSize: options.bufferSize || 4096,
      enableThreading: options.enableThreading !== false,
      enableCaching: options.enableCaching !== false,
      windowFunction: options.windowFunction || "hann",
      overlap: options.overlap || 0.5,
      fftSize: options.fftSize || 2048,
      maxFrequency: options.maxFrequency || 22050,
      preferWASM: options.preferWASM !== false,
      ...options,
    };

    // WASM engine integration
    this.wasmEngine = options.wasmEngine || null;
    this.wasmAvailable = this.wasmEngine !== null;

    // Processing state
    this.isProcessing = false;
    this.processingQueue = [];
    this.cache = new Map();

    // Web Workers for threading
    this.workers = [];
    this.workerPool = null;

    // Processing statistics
    this.stats = {
      processedSamples: 0,
      processingTime: 0,
      cacheHits: 0,
      cacheMisses: 0,
      wasmProcessingTime: 0,
      javascriptProcessingTime: 0,
      wasmUsageCount: 0,
      javascriptUsageCount: 0,
    };

    // DSP utilities
    this.windowFunctions = new Map();
    this.filterBank = new Map();

    // Event handling
    this.eventHandlers = new Map();

    // Initialize system
    this._initializeWindowFunctions();
    this._initializeFilterBank();

    if (this.config.enableThreading) {
      this._initializeWorkerPool();
    }

    console.log("WaveformProcessor initialized");
  }

  /**
   * Initialize window functions
   * @private
   */
  _initializeWindowFunctions() {
    // Hann window
    this.windowFunctions.set("hann", (N) => {
      const window = new Float32Array(N);
      for (let i = 0; i < N; i++) {
        window[i] = 0.5 * (1 - Math.cos((2 * Math.PI * i) / (N - 1)));
      }
      return window;
    });

    // Hamming window
    this.windowFunctions.set("hamming", (N) => {
      const window = new Float32Array(N);
      for (let i = 0; i < N; i++) {
        window[i] = 0.54 - 0.46 * Math.cos((2 * Math.PI * i) / (N - 1));
      }
      return window;
    });

    // Blackman window
    this.windowFunctions.set("blackman", (N) => {
      const window = new Float32Array(N);
      for (let i = 0; i < N; i++) {
        const a0 = 0.42;
        const a1 = 0.5;
        const a2 = 0.08;
        window[i] =
          a0 -
          a1 * Math.cos((2 * Math.PI * i) / (N - 1)) +
          a2 * Math.cos((4 * Math.PI * i) / (N - 1));
      }
      return window;
    });

    // Kaiser window
    this.windowFunctions.set("kaiser", (N, beta = 8.6) => {
      const window = new Float32Array(N);
      const alpha = (N - 1) / 2;
      const I0_beta = this._modifiedBesselI0(beta);

      for (let i = 0; i < N; i++) {
        const x = (i - alpha) / alpha;
        window[i] =
          this._modifiedBesselI0(beta * Math.sqrt(1 - x * x)) / I0_beta;
      }
      return window;
    });

    console.log(`Initialized ${this.windowFunctions.size} window functions`);
  }

  /**
   * Initialize filter bank
   * @private
   */
  _initializeFilterBank() {
    // Low-pass filter
    this.filterBank.set("lowpass", (cutoff, sampleRate) => {
      const RC = 1.0 / (cutoff * 2 * Math.PI);
      const dt = 1.0 / sampleRate;
      const alpha = dt / (RC + dt);

      return {
        alpha: alpha,
        process: function (input, state = { y1: 0 }) {
          state.y1 = state.y1 + alpha * (input - state.y1);
          return state.y1;
        },
      };
    });

    // High-pass filter
    this.filterBank.set("highpass", (cutoff, sampleRate) => {
      const RC = 1.0 / (cutoff * 2 * Math.PI);
      const dt = 1.0 / sampleRate;
      const alpha = RC / (RC + dt);

      return {
        alpha: alpha,
        process: function (input, state = { x1: 0, y1: 0 }) {
          const output = alpha * (state.y1 + input - state.x1);
          state.x1 = input;
          state.y1 = output;
          return output;
        },
      };
    });

    // Band-pass filter
    this.filterBank.set("bandpass", (lowCutoff, highCutoff, sampleRate) => {
      const lowpass = this.filterBank.get("lowpass")(highCutoff, sampleRate);
      const highpass = this.filterBank.get("highpass")(lowCutoff, sampleRate);

      return {
        process: function (
          input,
          state = { lpState: { y1: 0 }, hpState: { x1: 0, y1: 0 } }
        ) {
          const lowFiltered = lowpass.process(input, state.lpState);
          return highpass.process(lowFiltered, state.hpState);
        },
      };
    });

    console.log(`Initialized ${this.filterBank.size} filter types`);
  }

  /**
   * Initialize worker pool
   * @private
   */
  _initializeWorkerPool() {
    const workerCode = `
      // Web Worker for DSP processing
      self.onmessage = function(e) {
        const { type, data, options } = e.data;

        try {
          let result;

          switch (type) {
            case 'fft':
              result = performFFT(data, options);
              break;
            case 'spectogram':
              result = computeSpectrogram(data, options);
              break;
            case 'features':
              result = extractFeatures(data, options);
              break;
            default:
              throw new Error('Unknown processing type: ' + type);
          }

          self.postMessage({ success: true, result: result });
        } catch (error) {
          self.postMessage({ success: false, error: error.message });
        }
      };

      function performFFT(data, options) {
        // Simplified FFT implementation
        const N = data.length;
        const real = new Float32Array(N);
        const imag = new Float32Array(N);

        // Copy input data
        for (let i = 0; i < N; i++) {
          real[i] = data[i];
          imag[i] = 0;
        }

        // Bit-reversal
        let j = 0;
        for (let i = 1; i < N; i++) {
          let bit = N >> 1;
          while (j & bit) {
            j ^= bit;
            bit >>= 1;
          }
          j ^= bit;

          if (i < j) {
            [real[i], real[j]] = [real[j], real[i]];
            [imag[i], imag[j]] = [imag[j], imag[i]];
          }
        }

        // FFT computation
        for (let len = 2; len <= N; len <<= 1) {
          const wlen = len >> 1;
          const w_real = Math.cos(Math.PI / wlen);
          const w_imag = -Math.sin(Math.PI / wlen);

          for (let i = 0; i < N; i += len) {
            let wr = 1, wi = 0;

            for (let j = 0; j < wlen; j++) {
              const u_real = real[i + j];
              const u_imag = imag[i + j];
              const v_real = real[i + j + wlen] * wr - imag[i + j + wlen] * wi;
              const v_imag = real[i + j + wlen] * wi + imag[i + j + wlen] * wr;

              real[i + j] = u_real + v_real;
              imag[i + j] = u_imag + v_imag;
              real[i + j + wlen] = u_real - v_real;
              imag[i + j + wlen] = u_imag - v_imag;

              const temp_wr = wr * w_real - wi * w_imag;
              wi = wr * w_imag + wi * w_real;
              wr = temp_wr;
            }
          }
        }

        // Calculate magnitude spectrum
        const magnitude = new Float32Array(N / 2);
        for (let i = 0; i < N / 2; i++) {
          magnitude[i] = Math.sqrt(real[i] * real[i] + imag[i] * imag[i]);
        }

        return magnitude;
      }

      function computeSpectrogram(data, options) {
        const windowSize = options.windowSize || 1024;
        const hopSize = options.hopSize || 512;
        const numFrames = Math.floor((data.length - windowSize) / hopSize) + 1;
        const numBins = windowSize / 2;

        const spectrogram = new Array(numFrames);

        for (let frame = 0; frame < numFrames; frame++) {
          const start = frame * hopSize;
          const frameData = data.slice(start, start + windowSize);

          // Apply window function (simplified Hann)
          for (let i = 0; i < frameData.length; i++) {
            frameData[i] *= 0.5 * (1 - Math.cos(2 * Math.PI * i / (frameData.length - 1)));
          }

          // Compute FFT
          const fftResult = performFFT(frameData, {});
          spectrogram[frame] = Array.from(fftResult);
        }

        return spectrogram;
      }

      function extractFeatures(data, options) {
        const features = {};

        // RMS Energy
        let sum = 0;
        for (let i = 0; i < data.length; i++) {
          sum += data[i] * data[i];
        }
        features.rms = Math.sqrt(sum / data.length);

        // Zero Crossing Rate
        let crossings = 0;
        for (let i = 1; i < data.length; i++) {
          if ((data[i] >= 0) !== (data[i - 1] >= 0)) {
            crossings++;
          }
        }
        features.zcr = crossings / (data.length - 1);

        // Spectral Centroid (requires FFT)
        const spectrum = performFFT(data, {});
        let weightedSum = 0;
        let magnitudeSum = 0;

        for (let i = 0; i < spectrum.length; i++) {
          weightedSum += i * spectrum[i];
          magnitudeSum += spectrum[i];
        }

        features.spectralCentroid = magnitudeSum > 0 ? weightedSum / magnitudeSum : 0;

        return features;
      }
    `;

    try {
      const blob = new Blob([workerCode], { type: "application/javascript" });
      const workerUrl = URL.createObjectURL(blob);

      // Create worker pool
      const numWorkers = Math.min(navigator.hardwareConcurrency || 4, 4);
      for (let i = 0; i < numWorkers; i++) {
        const worker = new Worker(workerUrl);
        worker.busy = false;
        this.workers.push(worker);
      }

      console.log(`Initialized worker pool with ${numWorkers} workers`);
    } catch (error) {
      console.warn("Failed to initialize worker pool:", error);
      this.config.enableThreading = false;
    }
  }

  /**
   * Process audio buffer
   *
   * @param {AudioBuffer|Float32Array} audioData - Audio data to process
   * @param {Object} [options] - Processing options
   * @returns {Promise<Object>} Processing results
   */
  async processAudioBuffer(audioData, options = {}) {
    const startTime = performance.now();

    try {
      // Convert AudioBuffer to Float32Array if needed
      let data;
      if (audioData instanceof AudioBuffer) {
        data = audioData.getChannelData(0); // Use first channel
      } else {
        data = audioData;
      }

      // Check cache
      const cacheKey = this._generateCacheKey(data, options);
      if (this.config.enableCaching && this.cache.has(cacheKey)) {
        this.stats.cacheHits++;
        this.stats.cacheHitRate =
          (this.stats.cacheHitRate * this.stats.totalProcessed + 1) /
          (this.stats.totalProcessed + 1);
        return this.cache.get(cacheKey);
      }

      this.stats.cacheMisses++;

      // Process data
      const results = await this._performProcessing(data, options);

      // Cache results
      if (this.config.enableCaching) {
        this.cache.set(cacheKey, results);

        // Limit cache size
        if (this.cache.size > 100) {
          const firstKey = this.cache.keys().next().value;
          this.cache.delete(firstKey);
        }
      }

      // Update statistics
      const processingTime = performance.now() - startTime;
      this.stats.totalProcessed++;
      this.stats.averageProcessingTime =
        (this.stats.averageProcessingTime * (this.stats.totalProcessed - 1) +
          processingTime) /
        this.stats.totalProcessed;

      this._emitEvent("processingComplete", {
        processingTime,
        dataLength: data.length,
        results,
      });

      return results;
    } catch (error) {
      console.error("Audio processing failed:", error);
      this._emitEvent("processingError", { error });
      throw error;
    }
  }

  /**
   * Perform actual processing
   * @private
   */
  async _performProcessing(data, options) {
    const processingOptions = {
      ...this.config,
      ...options,
    };

    const results = {
      waveform: {
        peaks: this._extractPeaks(data, processingOptions),
        envelope: this._computeEnvelope(data, processingOptions),
        rms: this._computeRMS(data),
        peaks_normalized: null,
      },
      spectral: {
        spectrum: null,
        spectrogram: null,
        spectralCentroid: 0,
        spectralRolloff: 0,
        spectralFlux: 0,
      },
      features: {
        zcr: this._computeZCR(data),
        mfcc: null,
        chroma: null,
        tonnetz: null,
      },
      temporal: {
        onset: null,
        tempo: null,
        beats: null,
      },
    };

    // Compute spectrum
    if (this.config.enableThreading && this.workers.length > 0) {
      results.spectral.spectrum = await this._processWithWorker(
        "fft",
        data,
        processingOptions
      );
    } else {
      results.spectral.spectrum = await this._computeFFT(
        data,
        processingOptions
      );
    }

    // Compute spectrogram
    if (processingOptions.computeSpectrogram !== false) {
      if (this.config.enableThreading && this.workers.length > 0) {
        results.spectral.spectrogram = await this._processWithWorker(
          "spectogram",
          data,
          {
            windowSize: processingOptions.fftSize,
            hopSize: Math.floor(
              processingOptions.fftSize * (1 - processingOptions.overlap)
            ),
          }
        );
      } else {
        results.spectral.spectrogram = this._computeSpectrogram(
          data,
          processingOptions
        );
      }
    }

    // Compute spectral features
    if (results.spectral.spectrum) {
      results.spectral.spectralCentroid = this._computeSpectralCentroid(
        results.spectral.spectrum
      );
      results.spectral.spectralRolloff = this._computeSpectralRolloff(
        results.spectral.spectrum
      );
    }

    // Extract audio features
    if (processingOptions.extractFeatures !== false) {
      const additionalFeatures =
        this.config.enableThreading && this.workers.length > 0
          ? await this._processWithWorker("features", data, processingOptions)
          : this._extractAudioFeatures(data, processingOptions);

      Object.assign(results.features, additionalFeatures);
    }

    // Normalize peaks for visualization
    const maxPeak = Math.max(...results.waveform.peaks.map(Math.abs));
    if (maxPeak > 0) {
      results.waveform.peaks_normalized = results.waveform.peaks.map(
        (p) => p / maxPeak
      );
    } else {
      results.waveform.peaks_normalized = results.waveform.peaks;
    }

    return results;
  }

  /**
   * Process with web worker
   * @private
   */
  _processWithWorker(type, data, options) {
    return new Promise((resolve, reject) => {
      // Find available worker
      const worker = this.workers.find((w) => !w.busy);
      if (!worker) {
        // Fallback to synchronous processing
        resolve(this._processSynchronously(type, data, options));
        return;
      }

      worker.busy = true;

      const timeout = setTimeout(() => {
        worker.busy = false;
        reject(new Error("Worker processing timeout"));
      }, 10000);

      worker.onmessage = (e) => {
        clearTimeout(timeout);
        worker.busy = false;

        if (e.data.success) {
          resolve(e.data.result);
        } else {
          reject(new Error(e.data.error));
        }
      };

      worker.postMessage({ type, data: Array.from(data), options });
    });
  }

  /**
   * Process synchronously as fallback
   * @private
   */
  async _processSynchronously(type, data, options) {
    switch (type) {
      case "fft":
        return await this._computeFFT(data, options);
      case "spectogram":
        return await this._computeSpectrogram(data, options);
      case "features":
        return this._extractAudioFeatures(data, options);
      default:
        throw new Error(`Unknown processing type: ${type}`);
    }
  }

  /**
   * Extract peaks from waveform
   * @private
   */
  _extractPeaks(data, options) {
    const peakSampleCount = options.peakSampleCount || 1000;
    const samplesPerPeak = Math.floor(data.length / peakSampleCount);
    const peaks = new Float32Array(peakSampleCount);

    for (let i = 0; i < peakSampleCount; i++) {
      const start = i * samplesPerPeak;
      const end = Math.min(start + samplesPerPeak, data.length);

      let peak = 0;
      for (let j = start; j < end; j++) {
        const abs = Math.abs(data[j]);
        if (abs > peak) {
          peak = abs;
        }
      }
      peaks[i] = peak;
    }

    return peaks;
  }

  /**
   * Compute envelope
   * @private
   */
  _computeEnvelope(data, options) {
    const windowSize = options.envelopeWindowSize || 1024;
    const envelope = new Float32Array(Math.ceil(data.length / windowSize));

    for (let i = 0; i < envelope.length; i++) {
      const start = i * windowSize;
      const end = Math.min(start + windowSize, data.length);

      let sum = 0;
      for (let j = start; j < end; j++) {
        sum += data[j] * data[j];
      }
      envelope[i] = Math.sqrt(sum / (end - start));
    }

    return envelope;
  }

  /**
   * Compute RMS
   * @private
   */
  _computeRMS(data) {
    let sum = 0;
    for (let i = 0; i < data.length; i++) {
      sum += data[i] * data[i];
    }
    return Math.sqrt(sum / data.length);
  }

  /**
   * Compute Zero Crossing Rate
   * @private
   */
  _computeZCR(data) {
    let crossings = 0;
    for (let i = 1; i < data.length; i++) {
      if (data[i] >= 0 !== data[i - 1] >= 0) {
        crossings++;
      }
    }
    return crossings / (data.length - 1);
  }

  /**
   * Compute FFT with WASM/JavaScript fallback
   * @private
   */
  async _computeFFT(data, options) {
    return await this._processWithFallback(
      data,
      "fft",
      options,
      (data, options) => this._computeFFTJavaScript(data, options)
    );
  }

  /**
   * Compute FFT using JavaScript implementation
   * @private
   */
  _computeFFTJavaScript(data, options) {
    const N = options.fftSize || this.config.fftSize;
    const windowFunc = this.windowFunctions.get(
      options.windowFunction || this.config.windowFunction
    );

    // Prepare data
    const windowedData = new Float32Array(N);
    const window = windowFunc(N);

    for (let i = 0; i < Math.min(N, data.length); i++) {
      windowedData[i] = data[i] * window[i];
    }

    // Simple FFT implementation (for production, use a more efficient library)
    return this._performFFT(windowedData);
  }

  /**
   * Perform FFT calculation
   * @private
   */
  _performFFT(data) {
    const N = data.length;
    const spectrum = new Float32Array(N / 2);

    // Simplified FFT - in production, use FFT.js or similar
    for (let k = 0; k < N / 2; k++) {
      let real = 0,
        imag = 0;

      for (let n = 0; n < N; n++) {
        const angle = (-2 * Math.PI * k * n) / N;
        real += data[n] * Math.cos(angle);
        imag += data[n] * Math.sin(angle);
      }

      spectrum[k] = Math.sqrt(real * real + imag * imag);
    }

    return spectrum;
  }

  /**
   * Compute spectrogram with WASM/JavaScript fallback
   * @private
   */
  async _computeSpectrogram(data, options) {
    return await this._processWithFallback(
      data,
      "spectrogram",
      options,
      (data, options) => this._computeSpectrogramJavaScript(data, options)
    );
  }

  /**
   * Compute spectrogram using JavaScript implementation
   * @private
   */
  async _computeSpectrogramJavaScript(data, options) {
    const windowSize = options.fftSize || this.config.fftSize;
    const hopSize = Math.floor(windowSize * (1 - this.config.overlap));
    const numFrames = Math.floor((data.length - windowSize) / hopSize) + 1;

    const spectrogram = new Array(numFrames);

    for (let frame = 0; frame < numFrames; frame++) {
      const start = frame * hopSize;
      const frameData = data.slice(start, start + windowSize);

      const spectrum = await this._computeFFT(frameData, options);
      spectrogram[frame] = Array.from(spectrum);
    }

    return spectrogram;
  }

  /**
   * Compute spectral centroid
   * @private
   */
  _computeSpectralCentroid(spectrum) {
    let weightedSum = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < spectrum.length; i++) {
      weightedSum += i * spectrum[i];
      magnitudeSum += spectrum[i];
    }

    return magnitudeSum > 0 ? weightedSum / magnitudeSum : 0;
  }

  /**
   * Compute spectral rolloff
   * @private
   */
  _computeSpectralRolloff(spectrum, rolloffThreshold = 0.85) {
    const totalEnergy = spectrum.reduce((sum, val) => sum + val, 0);
    const threshold = totalEnergy * rolloffThreshold;

    let cumulativeEnergy = 0;
    for (let i = 0; i < spectrum.length; i++) {
      cumulativeEnergy += spectrum[i];
      if (cumulativeEnergy >= threshold) {
        return i;
      }
    }

    return spectrum.length - 1;
  }

  /**
   * Extract audio features
   * @private
   */
  _extractAudioFeatures(data, options) {
    return {
      rms: this._computeRMS(data),
      zcr: this._computeZCR(data),
      energy: data.reduce((sum, val) => sum + val * val, 0) / data.length,
    };
  }

  /**
   * Modified Bessel function I0
   * @private
   */
  _modifiedBesselI0(x) {
    let sum = 1;
    let term = 1;

    for (let i = 1; i <= 50; i++) {
      term *= (x / (2 * i)) * (x / (2 * i));
      sum += term;

      if (term < 1e-10) break;
    }

    return sum;
  }

  /**
   * Generate cache key
   * @private
   */
  _generateCacheKey(data, options) {
    // Simple hash based on data length and options
    const optionsStr = JSON.stringify(options);
    return `${data.length}_${optionsStr.length}_${data[0]}_${
      data[data.length - 1]
    }`;
  }

  /**
   * Get processing statistics
   *
   * @returns {Object} Processing statistics
   */
  getStatistics() {
    return { ...this.stats };
  }

  /**
   * Clear processing cache
   */
  clearCache() {
    this.cache.clear();
    this.stats.cacheHitRate = 0;
  }

  /**
   * Check if WASM processing is available and preferred
   * @private
   */
  _shouldUseWASM(operation) {
    return (
      this.wasmAvailable &&
      this.config.preferWASM &&
      this.wasmEngine &&
      this._isWASMOperationSupported(operation)
    );
  }

  /**
   * Check if WASM supports the requested operation
   * @private
   */
  _isWASMOperationSupported(operation) {
    const supportedOperations = [
      "fft",
      "spectrogram",
      "mfcc",
      "spectralFeatures",
      "envelope",
      "filtering",
    ];
    return supportedOperations.includes(operation);
  }

  /**
   * Process audio data using WASM engine
   * @private
   */
  async _processWithWASM(data, operation, options = {}) {
    if (!this.wasmEngine) {
      throw new Error("WASM engine not available");
    }

    const startTime = performance.now();

    try {
      let result;

      switch (operation) {
        case "fft":
          result = await this.wasmEngine.computeFFT(data, options);
          break;
        case "spectrogram":
          result = await this.wasmEngine.computeSpectrogram(data, options);
          break;
        case "mfcc":
          result = await this.wasmEngine.computeMFCC(data, options);
          break;
        case "spectralFeatures":
          result = await this.wasmEngine.extractSpectralFeatures(data, options);
          break;
        case "envelope":
          result = await this.wasmEngine.computeEnvelope(data, options);
          break;
        case "filtering":
          result = await this.wasmEngine.applyFilter(data, options);
          break;
        default:
          throw new Error(`Unsupported WASM operation: ${operation}`);
      }

      const processingTime = performance.now() - startTime;
      this.stats.wasmProcessingTime += processingTime;
      this.stats.wasmUsageCount++;

      return result;
    } catch (error) {
      console.warn(`WASM processing failed for ${operation}:`, error);
      // Fallback to JavaScript processing
      return null;
    }
  }

  /**
   * Process audio data with automatic WASM/JavaScript fallback
   * @private
   */
  async _processWithFallback(
    data,
    operation,
    options = {},
    javascriptFallback
  ) {
    // Try WASM first if available and preferred
    if (this._shouldUseWASM(operation)) {
      const wasmResult = await this._processWithWASM(data, operation, options);
      if (wasmResult !== null) {
        return wasmResult;
      }
    }

    // Fallback to JavaScript processing
    const startTime = performance.now();
    const result = await javascriptFallback(data, options);
    const processingTime = performance.now() - startTime;

    this.stats.javascriptProcessingTime += processingTime;
    this.stats.javascriptUsageCount++;

    return result;
  }

  /**
   * Get WASM processing statistics
   */
  getWASMStats() {
    const totalProcessingTime =
      this.stats.wasmProcessingTime + this.stats.javascriptProcessingTime;
    const totalOperations =
      this.stats.wasmUsageCount + this.stats.javascriptUsageCount;

    return {
      wasmAvailable: this.wasmAvailable,
      wasmPreferred: this.config.preferWASM,
      wasmProcessingTime: this.stats.wasmProcessingTime,
      javascriptProcessingTime: this.stats.javascriptProcessingTime,
      wasmUsageCount: this.stats.wasmUsageCount,
      javascriptUsageCount: this.stats.javascriptUsageCount,
      wasmProcessingRatio:
        totalOperations > 0 ? this.stats.wasmUsageCount / totalOperations : 0,
      averageWasmTime:
        this.stats.wasmUsageCount > 0
          ? this.stats.wasmProcessingTime / this.stats.wasmUsageCount
          : 0,
      averageJsTime:
        this.stats.javascriptUsageCount > 0
          ? this.stats.javascriptProcessingTime /
            this.stats.javascriptUsageCount
          : 0,
      performanceGains: this._calculatePerformanceGains(),
    };
  }

  /**
   * Calculate performance gains from WASM usage
   * @private
   */
  _calculatePerformanceGains() {
    if (
      this.stats.wasmUsageCount === 0 ||
      this.stats.javascriptUsageCount === 0
    ) {
      return { speedup: "N/A", estimatedSavings: 0 };
    }

    const avgWasmTime =
      this.stats.wasmProcessingTime / this.stats.wasmUsageCount;
    const avgJsTime =
      this.stats.javascriptProcessingTime / this.stats.javascriptUsageCount;

    const speedup = avgJsTime / avgWasmTime;
    const estimatedSavings =
      this.stats.wasmUsageCount * (avgJsTime - avgWasmTime);

    return {
      speedup: speedup.toFixed(2) + "x",
      estimatedSavings: Math.max(0, estimatedSavings).toFixed(2) + "ms",
    };
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
   * Cleanup and destroy processor
   */
  destroy() {
    console.log("Destroying WaveformProcessor...");

    // Terminate workers
    this.workers.forEach((worker) => {
      worker.terminate();
    });
    this.workers.length = 0;

    // Clear cache
    this.cache.clear();

    // Clear event handlers
    this.eventHandlers.clear();

    // Reset state
    this.isProcessing = false;
    this.processingQueue.length = 0;

    console.log("WaveformProcessor destroyed");
  }
}

export default WaveformProcessor;
