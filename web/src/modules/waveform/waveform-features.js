/**
 * @fileoverview Waveform Features Module - Feature Extraction and Analysis
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Provides comprehensive audio feature extraction and analysis capabilities
 * including time-domain, frequency-domain, and advanced perceptual features.
 *
 * Key Features:
 * - Time-domain feature extraction (RMS, zero-crossing rate, envelope)
 * - Frequency-domain features (spectral centroid, rolloff, flux, flatness)
 * - Perceptual features (MFCCs, chroma, spectral contrast)
 * - Advanced analysis (onset detection, tempo estimation, pitch tracking)
 * - Machine learning feature vectors for classification
 * - Real-time feature computation with efficient algorithms
 *
 * Dependencies:
 * - FFT libraries for frequency analysis
 * - Digital signal processing utilities
 * - Statistical analysis functions
 * - WASM acceleration for performance-critical operations
 *
 * @example
 * ```javascript
 * import { WaveformFeatures } from './modules/waveform/waveform-features.js';
 *
 * const extractor = new WaveformFeatures({
 *   sampleRate: 44100,
 *   frameSize: 2048,
 *   hopSize: 512
 * });
 *
 * const features = extractor.extractFeatures(audioBuffer);
 * console.log(features.spectralCentroid, features.mfcc);
 * ```
 */

/**
 * Comprehensive Audio Feature Extraction Engine
 *
 * Extracts various audio features for analysis, visualization, and machine
 * learning applications with optimized algorithms and real-time processing.
 *
 * @class WaveformFeatures
 */
export class WaveformFeatures {
  /**
   * Create a WaveformFeatures instance
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.sampleRate=44100] - Audio sample rate
   * @param {number} [options.frameSize=2048] - Analysis frame size
   * @param {number} [options.hopSize=512] - Hop size between frames
   * @param {string} [options.windowFunction='hann'] - Window function type
   * @param {Object} [options.mfccConfig] - MFCC configuration
   * @param {Object} [options.chromaConfig] - Chroma configuration
   * @param {boolean} [options.enableCache=true] - Enable feature caching
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      sampleRate: options.sampleRate || 44100,
      frameSize: options.frameSize || 2048,
      hopSize: options.hopSize || 512,
      windowFunction: options.windowFunction || "hann",
      enableCache: options.enableCache !== false,
      mfccConfig: {
        numCoefficients: 13,
        numFilters: 26,
        minFreq: 0,
        maxFreq: null, // Will be set to Nyquist
        lifterParam: 22,
        ...options.mfccConfig,
      },
      chromaConfig: {
        numBins: 12,
        tuningFreq: 440,
        ...options.chromaConfig,
      },
      ...options,
    };

    // Set default max frequency for MFCC
    if (!this.config.mfccConfig.maxFreq) {
      this.config.mfccConfig.maxFreq = this.config.sampleRate / 2;
    }

    // Feature extraction state
    this.state = {
      isExtracting: false,
      currentFrame: 0,
      totalFrames: 0,
      lastExtractTime: 0,
      cacheEnabled: this.config.enableCache,
    };

    // Feature cache
    this.cache = new Map();

    // Analysis buffers
    this.buffers = {
      window: null,
      fftInput: new Float32Array(this.config.frameSize),
      fftOutput: {
        real: new Float32Array(this.config.frameSize),
        imag: new Float32Array(this.config.frameSize),
      },
      magnitude: new Float32Array(this.config.frameSize / 2),
      powerSpectrum: new Float32Array(this.config.frameSize / 2),
      melFilters: null,
      chromaFilters: null,
    };

    // Feature extractors
    this.extractors = {
      timeDomain: null,
      frequencyDomain: null,
      perceptual: null,
      advanced: null,
    };

    // Statistical utilities
    this.stats = {
      mean: (arr) => arr.reduce((sum, val) => sum + val, 0) / arr.length,
      variance: (arr) => {
        const mean = this.stats.mean(arr);
        return (
          arr.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) /
          arr.length
        );
      },
      std: (arr) => Math.sqrt(this.stats.variance(arr)),
      skewness: (arr) => {
        const mean = this.stats.mean(arr);
        const std = this.stats.std(arr);
        const n = arr.length;
        return (
          arr.reduce((sum, val) => sum + Math.pow((val - mean) / std, 3), 0) / n
        );
      },
      kurtosis: (arr) => {
        const mean = this.stats.mean(arr);
        const std = this.stats.std(arr);
        const n = arr.length;
        return (
          arr.reduce((sum, val) => sum + Math.pow((val - mean) / std, 4), 0) /
            n -
          3
        );
      },
    };

    // Event handlers
    this.eventHandlers = new Map();

    this._initialize();
  }

  /**
   * Initialize feature extraction system
   * @private
   */
  async _initialize() {
    try {
      // Generate window function
      this._generateWindowFunction();

      // Initialize mel filter bank
      await this._initializeMelFilters();

      // Initialize chroma filters
      await this._initializeChromaFilters();

      // Initialize feature extractors
      await this._initializeExtractors();

      console.log("WaveformFeatures initialized successfully");
    } catch (error) {
      console.error("WaveformFeatures initialization failed:", error);
    }
  }

  /**
   * Generate window function
   * @private
   */
  _generateWindowFunction() {
    const size = this.config.frameSize;
    this.buffers.window = new Float32Array(size);

    switch (this.config.windowFunction) {
      case "hann":
        for (let i = 0; i < size; i++) {
          this.buffers.window[i] =
            0.5 * (1 - Math.cos((2 * Math.PI * i) / (size - 1)));
        }
        break;
      case "hamming":
        for (let i = 0; i < size; i++) {
          this.buffers.window[i] =
            0.54 - 0.46 * Math.cos((2 * Math.PI * i) / (size - 1));
        }
        break;
      case "blackman":
        for (let i = 0; i < size; i++) {
          this.buffers.window[i] =
            0.42 -
            0.5 * Math.cos((2 * Math.PI * i) / (size - 1)) +
            0.08 * Math.cos((4 * Math.PI * i) / (size - 1));
        }
        break;
      default:
        this.buffers.window.fill(1.0);
    }
  }

  /**
   * Initialize mel filter bank
   * @private
   */
  async _initializeMelFilters() {
    const config = this.config.mfccConfig;
    const numFilters = config.numFilters;
    const fftSize = this.config.frameSize;
    const sampleRate = this.config.sampleRate;

    // Convert Hz to mel scale
    const hzToMel = (hz) => 2595 * Math.log10(1 + hz / 700);
    const melToHz = (mel) => 700 * (Math.pow(10, mel / 2595) - 1);

    // Create mel-spaced frequency points
    const minMel = hzToMel(config.minFreq);
    const maxMel = hzToMel(config.maxFreq);
    const melPoints = new Array(numFilters + 2);

    for (let i = 0; i < melPoints.length; i++) {
      melPoints[i] = melToHz(
        minMel + ((maxMel - minMel) * i) / (numFilters + 1)
      );
    }

    // Convert to FFT bin indices
    const binPoints = melPoints.map((freq) =>
      Math.floor(((fftSize + 1) * freq) / sampleRate)
    );

    // Create triangular filters
    this.buffers.melFilters = new Array(numFilters);
    for (let i = 0; i < numFilters; i++) {
      const filter = new Float32Array(fftSize / 2);
      const left = binPoints[i];
      const center = binPoints[i + 1];
      const right = binPoints[i + 2];

      // Left slope
      for (let j = left; j < center; j++) {
        if (j >= 0 && j < filter.length) {
          filter[j] = (j - left) / (center - left);
        }
      }

      // Right slope
      for (let j = center; j < right; j++) {
        if (j >= 0 && j < filter.length) {
          filter[j] = (right - j) / (right - center);
        }
      }

      this.buffers.melFilters[i] = filter;
    }
  }

  /**
   * Initialize chroma filters
   * @private
   */
  async _initializeChromaFilters() {
    const config = this.config.chromaConfig;
    const numBins = config.numBins;
    const fftSize = this.config.frameSize;
    const sampleRate = this.config.sampleRate;
    const tuningFreq = config.tuningFreq;

    this.buffers.chromaFilters = new Array(numBins);

    for (let i = 0; i < numBins; i++) {
      const filter = new Float32Array(fftSize / 2);

      for (let j = 0; j < filter.length; j++) {
        const freq = (j * sampleRate) / fftSize;
        if (freq > 0) {
          // Calculate chroma bin for this frequency
          const chroma = (12 * Math.log2(freq / tuningFreq)) % 12;
          const distance = Math.min(
            Math.abs(chroma - i),
            12 - Math.abs(chroma - i)
          );

          // Gaussian weighting
          filter[j] = Math.exp(-0.5 * Math.pow(distance / 0.5, 2));
        }
      }

      this.buffers.chromaFilters[i] = filter;
    }
  }

  /**
   * Initialize feature extractors
   * @private
   */
  async _initializeExtractors() {
    // Time-domain feature extractor
    this.extractors.timeDomain = {
      rms: (buffer) => {
        let sum = 0;
        for (let i = 0; i < buffer.length; i++) {
          sum += buffer[i] * buffer[i];
        }
        return Math.sqrt(sum / buffer.length);
      },

      zeroCrossingRate: (buffer) => {
        let crossings = 0;
        for (let i = 1; i < buffer.length; i++) {
          if (buffer[i] >= 0 !== buffer[i - 1] >= 0) {
            crossings++;
          }
        }
        return crossings / (buffer.length - 1);
      },

      energy: (buffer) => {
        let energy = 0;
        for (let i = 0; i < buffer.length; i++) {
          energy += buffer[i] * buffer[i];
        }
        return energy;
      },

      envelope: (buffer) => {
        const envelope = new Float32Array(buffer.length);
        const alpha = 0.01; // Smoothing factor

        envelope[0] = Math.abs(buffer[0]);
        for (let i = 1; i < buffer.length; i++) {
          envelope[i] =
            alpha * Math.abs(buffer[i]) + (1 - alpha) * envelope[i - 1];
        }

        return envelope;
      },

      peakAmplitude: (buffer) => {
        let peak = 0;
        for (let i = 0; i < buffer.length; i++) {
          peak = Math.max(peak, Math.abs(buffer[i]));
        }
        return peak;
      },

      crestFactor: (buffer) => {
        const peak = this.extractors.timeDomain.peakAmplitude(buffer);
        const rms = this.extractors.timeDomain.rms(buffer);
        return rms > 0 ? peak / rms : 0;
      },
    };

    // Frequency-domain feature extractor
    this.extractors.frequencyDomain = {
      spectralCentroid: (spectrum) => {
        let numerator = 0;
        let denominator = 0;

        for (let i = 0; i < spectrum.length; i++) {
          const freq = (i * this.config.sampleRate) / (2 * spectrum.length);
          numerator += freq * spectrum[i];
          denominator += spectrum[i];
        }

        return denominator > 0 ? numerator / denominator : 0;
      },

      spectralRolloff: (spectrum, threshold = 0.85) => {
        const totalEnergy = spectrum.reduce((sum, val) => sum + val, 0);
        const targetEnergy = totalEnergy * threshold;

        let cumulativeEnergy = 0;
        for (let i = 0; i < spectrum.length; i++) {
          cumulativeEnergy += spectrum[i];
          if (cumulativeEnergy >= targetEnergy) {
            return (i * this.config.sampleRate) / (2 * spectrum.length);
          }
        }

        return this.config.sampleRate / 2;
      },

      spectralFlux: (spectrum, previousSpectrum) => {
        if (!previousSpectrum) return 0;

        let flux = 0;
        for (let i = 0; i < spectrum.length; i++) {
          const diff = spectrum[i] - previousSpectrum[i];
          flux += diff * diff;
        }

        return Math.sqrt(flux);
      },

      spectralFlatness: (spectrum) => {
        let geometricMean = 1;
        let arithmeticMean = 0;
        let count = 0;

        for (let i = 1; i < spectrum.length; i++) {
          if (spectrum[i] > 0) {
            geometricMean *= Math.pow(spectrum[i], 1 / (spectrum.length - 1));
            arithmeticMean += spectrum[i];
            count++;
          }
        }

        arithmeticMean /= count;
        return count > 0 ? geometricMean / arithmeticMean : 0;
      },

      spectralContrast: (spectrum) => {
        const numBands = 6;
        const bandWidth = Math.floor(spectrum.length / numBands);
        const contrast = new Float32Array(numBands);

        for (let i = 0; i < numBands; i++) {
          const start = i * bandWidth;
          const end = Math.min((i + 1) * bandWidth, spectrum.length);
          const band = spectrum.slice(start, end);

          band.sort((a, b) => b - a); // Sort descending
          const peakMean =
            band
              .slice(0, Math.floor(band.length * 0.2))
              .reduce((sum, val) => sum + val, 0) /
            Math.floor(band.length * 0.2);
          const valleyMean =
            band
              .slice(-Math.floor(band.length * 0.2))
              .reduce((sum, val) => sum + val, 0) /
            Math.floor(band.length * 0.2);

          contrast[i] = valleyMean > 0 ? Math.log(peakMean / valleyMean) : 0;
        }

        return contrast;
      },
    };

    // Perceptual feature extractor
    this.extractors.perceptual = {
      mfcc: (spectrum) => {
        const config = this.config.mfccConfig;
        const numCoefficients = config.numCoefficients;
        const numFilters = config.numFilters;

        // Apply mel filters
        const melSpectrum = new Float32Array(numFilters);
        for (let i = 0; i < numFilters; i++) {
          let sum = 0;
          for (let j = 0; j < spectrum.length; j++) {
            sum += spectrum[j] * this.buffers.melFilters[i][j];
          }
          melSpectrum[i] = Math.max(sum, 1e-10); // Avoid log(0)
        }

        // Take logarithm
        const logMelSpectrum = melSpectrum.map((val) => Math.log(val));

        // Apply DCT
        const mfcc = new Float32Array(numCoefficients);
        for (let i = 0; i < numCoefficients; i++) {
          let sum = 0;
          for (let j = 0; j < numFilters; j++) {
            sum +=
              logMelSpectrum[j] *
              Math.cos((Math.PI * i * (j + 0.5)) / numFilters);
          }
          mfcc[i] = sum;
        }

        // Apply liftering
        if (config.lifterParam > 0) {
          for (let i = 1; i < numCoefficients; i++) {
            mfcc[i] *=
              1 +
              (config.lifterParam / 2) *
                Math.sin((Math.PI * i) / config.lifterParam);
          }
        }

        return mfcc;
      },

      chroma: (spectrum) => {
        const numBins = this.config.chromaConfig.numBins;
        const chroma = new Float32Array(numBins);

        for (let i = 0; i < numBins; i++) {
          let sum = 0;
          for (let j = 0; j < spectrum.length; j++) {
            sum += spectrum[j] * this.buffers.chromaFilters[i][j];
          }
          chroma[i] = sum;
        }

        // Normalize
        const sum = chroma.reduce((sum, val) => sum + val, 0);
        if (sum > 0) {
          for (let i = 0; i < numBins; i++) {
            chroma[i] /= sum;
          }
        }

        return chroma;
      },

      tonnetz: (chroma) => {
        // Tonnetz (tonal centroid) features
        const tonnetz = new Float32Array(6);

        // Major thirds
        tonnetz[0] = chroma[0] + chroma[4] + chroma[8]; // C, E, G#
        tonnetz[1] = chroma[1] + chroma[5] + chroma[9]; // C#, F, A
        tonnetz[2] = chroma[2] + chroma[6] + chroma[10]; // D, F#, Bb
        tonnetz[3] = chroma[3] + chroma[7] + chroma[11]; // Eb, G, B

        // Minor thirds
        tonnetz[4] = chroma[0] + chroma[3] + chroma[6] + chroma[9]; // C, Eb, F#, A
        tonnetz[5] = chroma[1] + chroma[4] + chroma[7] + chroma[10]; // C#, E, G, Bb

        return tonnetz;
      },
    };

    // Advanced feature extractor
    this.extractors.advanced = {
      onsetStrength: (spectrum, previousSpectrum) => {
        if (!previousSpectrum) return 0;

        let strength = 0;
        for (let i = 0; i < spectrum.length; i++) {
          const diff = spectrum[i] - previousSpectrum[i];
          if (diff > 0) {
            strength += diff;
          }
        }

        return strength;
      },

      pitchSalience: (spectrum) => {
        // Simplified pitch salience using harmonic product spectrum
        const hps = new Float32Array(spectrum.length);

        for (let i = 0; i < spectrum.length; i++) {
          hps[i] = spectrum[i];

          // Multiply with downsampled versions
          for (let harmonic = 2; harmonic <= 5; harmonic++) {
            const index = Math.floor(i / harmonic);
            if (index < spectrum.length) {
              hps[i] *= spectrum[index];
            }
          }
        }

        // Find maximum
        let maxIndex = 0;
        let maxValue = 0;
        for (let i = 1; i < hps.length; i++) {
          if (hps[i] > maxValue) {
            maxValue = hps[i];
            maxIndex = i;
          }
        }

        return {
          frequency:
            (maxIndex * this.config.sampleRate) / (2 * spectrum.length),
          salience: maxValue,
        };
      },

      spectralComplexity: (spectrum) => {
        // Measure of spectral complexity based on peak distribution
        const peaks = [];

        // Find peaks
        for (let i = 1; i < spectrum.length - 1; i++) {
          if (spectrum[i] > spectrum[i - 1] && spectrum[i] > spectrum[i + 1]) {
            peaks.push({ index: i, magnitude: spectrum[i] });
          }
        }

        // Sort by magnitude
        peaks.sort((a, b) => b.magnitude - a.magnitude);

        // Calculate complexity as ratio of significant peaks
        const significantPeaks = peaks.filter(
          (peak) => peak.magnitude > 0.1 * peaks[0]?.magnitude || 0
        );
        return significantPeaks.length / Math.max(peaks.length, 1);
      },
    };
  }

  /**
   * Extract comprehensive features from audio buffer
   *
   * @param {Float32Array} audioBuffer - Input audio buffer
   * @param {Object} [options={}] - Extraction options
   * @returns {Object} Extracted features
   */
  extractFeatures(audioBuffer, options = {}) {
    const startTime = performance.now();

    try {
      this.state.isExtracting = true;

      // Check cache if enabled
      const cacheKey = this._generateCacheKey(audioBuffer, options);
      if (this.state.cacheEnabled && this.cache.has(cacheKey)) {
        return this.cache.get(cacheKey);
      }

      // Frame-based analysis
      const frameSize = this.config.frameSize;
      const hopSize = this.config.hopSize;
      const numFrames =
        Math.floor((audioBuffer.length - frameSize) / hopSize) + 1;

      this.state.totalFrames = numFrames;

      // Initialize feature arrays
      const features = {
        // Time-domain features
        rms: new Float32Array(numFrames),
        zeroCrossingRate: new Float32Array(numFrames),
        energy: new Float32Array(numFrames),
        peakAmplitude: new Float32Array(numFrames),
        crestFactor: new Float32Array(numFrames),

        // Frequency-domain features
        spectralCentroid: new Float32Array(numFrames),
        spectralRolloff: new Float32Array(numFrames),
        spectralFlux: new Float32Array(numFrames),
        spectralFlatness: new Float32Array(numFrames),
        spectralContrast: [],

        // Perceptual features
        mfcc: [],
        chroma: [],
        tonnetz: [],

        // Advanced features
        onsetStrength: new Float32Array(numFrames),
        pitchSalience: [],
        spectralComplexity: new Float32Array(numFrames),

        // Metadata
        frameCount: numFrames,
        frameSize: frameSize,
        hopSize: hopSize,
        sampleRate: this.config.sampleRate,
        duration: audioBuffer.length / this.config.sampleRate,
      };

      let previousSpectrum = null;

      // Process each frame
      for (let frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        this.state.currentFrame = frameIndex;
        const frameStart = frameIndex * hopSize;
        const frameEnd = Math.min(frameStart + frameSize, audioBuffer.length);

        // Extract frame
        const frame = new Float32Array(frameSize);
        for (
          let i = 0;
          i < frameSize && frameStart + i < audioBuffer.length;
          i++
        ) {
          frame[i] = audioBuffer[frameStart + i] * this.buffers.window[i];
        }

        // Time-domain features
        features.rms[frameIndex] = this.extractors.timeDomain.rms(frame);
        features.zeroCrossingRate[frameIndex] =
          this.extractors.timeDomain.zeroCrossingRate(frame);
        features.energy[frameIndex] = this.extractors.timeDomain.energy(frame);
        features.peakAmplitude[frameIndex] =
          this.extractors.timeDomain.peakAmplitude(frame);
        features.crestFactor[frameIndex] =
          this.extractors.timeDomain.crestFactor(frame);

        // FFT analysis
        const spectrum = this._computeSpectrum(frame);

        // Frequency-domain features
        features.spectralCentroid[frameIndex] =
          this.extractors.frequencyDomain.spectralCentroid(spectrum);
        features.spectralRolloff[frameIndex] =
          this.extractors.frequencyDomain.spectralRolloff(spectrum);
        features.spectralFlux[frameIndex] =
          this.extractors.frequencyDomain.spectralFlux(
            spectrum,
            previousSpectrum
          );
        features.spectralFlatness[frameIndex] =
          this.extractors.frequencyDomain.spectralFlatness(spectrum);
        features.spectralContrast[frameIndex] =
          this.extractors.frequencyDomain.spectralContrast(spectrum);

        // Perceptual features
        const mfcc = this.extractors.perceptual.mfcc(spectrum);
        const chroma = this.extractors.perceptual.chroma(spectrum);
        const tonnetz = this.extractors.perceptual.tonnetz(chroma);

        features.mfcc[frameIndex] = mfcc;
        features.chroma[frameIndex] = chroma;
        features.tonnetz[frameIndex] = tonnetz;

        // Advanced features
        features.onsetStrength[frameIndex] =
          this.extractors.advanced.onsetStrength(spectrum, previousSpectrum);
        features.pitchSalience[frameIndex] =
          this.extractors.advanced.pitchSalience(spectrum);
        features.spectralComplexity[frameIndex] =
          this.extractors.advanced.spectralComplexity(spectrum);

        previousSpectrum = spectrum.slice(); // Copy for next frame

        // Emit progress event
        if (frameIndex % 100 === 0) {
          this._emitEvent("extractionProgress", {
            currentFrame: frameIndex,
            totalFrames: numFrames,
            progress: frameIndex / numFrames,
          });
        }
      }

      // Compute summary statistics
      const summaryFeatures = this._computeSummaryFeatures(features);

      // Combine features
      const allFeatures = {
        ...features,
        summary: summaryFeatures,
        extractionTime: performance.now() - startTime,
      };

      // Cache results if enabled
      if (this.state.cacheEnabled) {
        this.cache.set(cacheKey, allFeatures);
      }

      this._emitEvent("extractionComplete", { features: allFeatures });

      return allFeatures;
    } catch (error) {
      console.error("Feature extraction failed:", error);
      this._emitEvent("extractionError", { error });
      return null;
    } finally {
      this.state.isExtracting = false;
    }
  }

  /**
   * Compute spectrum from time-domain frame
   * @private
   */
  _computeSpectrum(frame) {
    // Copy to FFT buffer
    this.buffers.fftInput.set(frame);

    // Perform FFT (simplified - would use optimized FFT library)
    this._performFFT(this.buffers.fftInput, this.buffers.fftOutput);

    // Compute magnitude spectrum
    const spectrum = this.buffers.magnitude;
    for (let i = 0; i < spectrum.length; i++) {
      const real = this.buffers.fftOutput.real[i];
      const imag = this.buffers.fftOutput.imag[i];
      spectrum[i] = Math.sqrt(real * real + imag * imag);
    }

    return spectrum.slice(); // Return copy
  }

  /**
   * Simplified FFT implementation (placeholder)
   * @private
   */
  _performFFT(input, output) {
    // This would be replaced with optimized FFT library
    const N = input.length;

    // Copy input to real part
    output.real.set(input);
    output.imag.fill(0);

    // Simplified FFT computation (placeholder)
    // In production, use libraries like KissFFT, FFTW, or Web Assembly implementations
    for (let i = 0; i < N / 2; i++) {
      const angle = (-2 * Math.PI * i) / N;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);

      output.real[i] = input[i] * cos;
      output.imag[i] = input[i] * sin;
    }
  }

  /**
   * Compute summary statistics for extracted features
   * @private
   */
  _computeSummaryFeatures(features) {
    const summary = {};

    // Statistical summaries for frame-based features
    const frameFeatures = [
      "rms",
      "zeroCrossingRate",
      "energy",
      "peakAmplitude",
      "crestFactor",
      "spectralCentroid",
      "spectralRolloff",
      "spectralFlux",
      "spectralFlatness",
      "onsetStrength",
      "spectralComplexity",
    ];

    frameFeatures.forEach((featureName) => {
      const values = features[featureName];
      if (values && values.length > 0) {
        summary[featureName] = {
          mean: this.stats.mean(values),
          std: this.stats.std(values),
          min: Math.min(...values),
          max: Math.max(...values),
          median: this._median(values),
          skewness: this.stats.skewness(values),
          kurtosis: this.stats.kurtosis(values),
        };
      }
    });

    // Aggregated MFCC statistics
    if (features.mfcc && features.mfcc.length > 0) {
      const numCoeffs = features.mfcc[0].length;
      summary.mfcc = [];

      for (let i = 0; i < numCoeffs; i++) {
        const coeffValues = features.mfcc.map((frame) => frame[i]);
        summary.mfcc[i] = {
          mean: this.stats.mean(coeffValues),
          std: this.stats.std(coeffValues),
        };
      }
    }

    // Aggregated chroma statistics
    if (features.chroma && features.chroma.length > 0) {
      const numBins = features.chroma[0].length;
      summary.chroma = [];

      for (let i = 0; i < numBins; i++) {
        const binValues = features.chroma.map((frame) => frame[i]);
        summary.chroma[i] = {
          mean: this.stats.mean(binValues),
          std: this.stats.std(binValues),
        };
      }
    }

    return summary;
  }

  /**
   * Calculate median value
   * @private
   */
  _median(arr) {
    const sorted = arr.slice().sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);
    return sorted.length % 2 === 0
      ? (sorted[mid - 1] + sorted[mid]) / 2
      : sorted[mid];
  }

  /**
   * Generate cache key for feature extraction
   * @private
   */
  _generateCacheKey(audioBuffer, options) {
    // Simple hash based on buffer content and options
    let hash = 0;
    const step = Math.max(1, Math.floor(audioBuffer.length / 1000));

    for (let i = 0; i < audioBuffer.length; i += step) {
      hash = ((hash << 5) - hash + audioBuffer[i] * 1000) & 0xffffffff;
    }

    return `features_${hash}_${JSON.stringify(options)}`;
  }

  /**
   * Extract features for machine learning
   *
   * @param {Float32Array} audioBuffer - Input audio buffer
   * @returns {Float32Array} Feature vector for ML
   */
  extractMLFeatures(audioBuffer) {
    const features = this.extractFeatures(audioBuffer);
    if (!features) return null;

    // Create feature vector for machine learning
    const mlFeatures = [];

    // Add summary statistics
    const summaryFeatures = [
      "rms",
      "zeroCrossingRate",
      "spectralCentroid",
      "spectralRolloff",
      "spectralFlatness",
      "spectralComplexity",
    ];

    summaryFeatures.forEach((featureName) => {
      const summary = features.summary[featureName];
      if (summary) {
        mlFeatures.push(summary.mean, summary.std, summary.min, summary.max);
      }
    });

    // Add MFCC coefficients (first 13)
    if (features.summary.mfcc) {
      features.summary.mfcc.slice(0, 13).forEach((coeff) => {
        mlFeatures.push(coeff.mean, coeff.std);
      });
    }

    // Add chroma features
    if (features.summary.chroma) {
      features.summary.chroma.forEach((bin) => {
        mlFeatures.push(bin.mean);
      });
    }

    return new Float32Array(mlFeatures);
  }

  /**
   * Get feature extraction progress
   *
   * @returns {Object} Progress information
   */
  getProgress() {
    return {
      isExtracting: this.state.isExtracting,
      currentFrame: this.state.currentFrame,
      totalFrames: this.state.totalFrames,
      progress:
        this.state.totalFrames > 0
          ? this.state.currentFrame / this.state.totalFrames
          : 0,
    };
  }

  /**
   * Clear feature cache
   */
  clearCache() {
    this.cache.clear();
  }

  /**
   * Enable or disable caching
   *
   * @param {boolean} enabled - Whether to enable caching
   */
  setCacheEnabled(enabled) {
    this.state.cacheEnabled = enabled;
    if (!enabled) {
      this.clearCache();
    }
  }

  /**
   * Setup event handling
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
   * Cleanup resources
   */
  destroy() {
    this.state.isExtracting = false;
    this.eventHandlers.clear();
    this.clearCache();
  }
}

export default WaveformFeatures;
