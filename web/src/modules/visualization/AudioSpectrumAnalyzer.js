/**
 * AudioSpectrumAnalyzer.js - Interactive Real-time Spectrum Analyzer
 *
 * Advanced spectrum analyzer component with real-time FFT visualization, configurable
 * frequency ranges, peak detection, harmonic analysis, and frequency response overlays.
 *
 * Features:
 * - Real-time FFT visualization with customizable parameters
 * - Configurable frequency ranges and logarithmic/linear scales
 * - Peak detection and automatic annotation
 * - Harmonic analysis with overtone visualization
 * - Frequency response overlay capabilities
 * - Interactive frequency band selection and zooming
 * - Multiple visualization modes (line, bars, filled curve)
 * - Exportable analysis data and screenshots
 *
 * Dependencies: Web Audio API, EventManager, PerformanceMonitor
 */

import { EventManager } from "../core/EventManager.js";
import { PerformanceMonitor } from "../core/PerformanceMonitor.js";

export class AudioSpectrumAnalyzer {
  constructor(container, options = {}) {
    this.container = container;
    this.options = {
      // Analysis configuration
      fftSize: options.fftSize || 2048,
      smoothingTimeConstant: options.smoothingTimeConstant || 0.8,
      minDecibels: options.minDecibels || -100,
      maxDecibels: options.maxDecibels || -30,

      // Display configuration
      width: options.width || 800,
      height: options.height || 400,
      backgroundColor: options.backgroundColor || "#000000",
      gridColor: options.gridColor || "#333333",
      primaryColor: options.primaryColor || "#00ff00",
      peakColor: options.peakColor || "#ff0000",

      // Frequency range
      minFrequency: options.minFrequency || 20,
      maxFrequency: options.maxFrequency || 20000,
      frequencyScale: options.frequencyScale || "logarithmic", // 'linear' or 'logarithmic'

      // Visualization modes
      visualMode: options.visualMode || "line", // 'line', 'bars', 'filled'
      showGrid: options.showGrid !== false,
      showPeaks: options.showPeaks !== false,
      showHarmonics: options.showHarmonics || false,
      showLabels: options.showLabels !== false,

      // Peak detection
      peakThreshold: options.peakThreshold || -40, // dB
      peakHoldTime: options.peakHoldTime || 1000, // ms
      harmonicTolerance: options.harmonicTolerance || 0.02,

      // Interaction
      enableZoom: options.enableZoom !== false,
      enableSelection: options.enableSelection !== false,
      enableFrequencyTracking: options.enableFrequencyTracking || false,

      // Performance
      updateRate: options.updateRate || 60, // fps
      enableSmoothing: options.enableSmoothing !== false,

      ...options,
    };

    // Initialize state
    this.isInitialized = false;
    this.isPlaying = false;
    this.audioContext = null;
    this.analyserNode = null;

    // Analysis data
    this.frequencyData = null;
    this.smoothedData = null;
    this.peaks = [];
    this.harmonics = [];
    this.selectedBand = null;

    // Canvas and rendering context
    this.canvas = null;
    this.ctx = null;
    this.animationId = null;

    // Interaction state
    this.mousePosition = { x: 0, y: 0 };
    this.isDragging = false;
    this.zoomLevel = 1.0;
    this.panOffset = { x: 0, y: 0 };

    // Performance tracking
    this.frameCount = 0;
    this.lastFrameTime = 0;
    this.fpsCounter = 0;
    this.lastFpsUpdate = 0;

    // Event system
    this.eventManager = EventManager.getInstance();
    this.performanceMonitor = PerformanceMonitor.getInstance();

    // Initialize component
    this.init();
  }

  /**
   * Initialize the spectrum analyzer
   * TODO: Create canvas and rendering context
   * TODO: Set up audio analysis pipeline
   * TODO: Configure frequency mapping
   * TODO: Initialize peak detection system
   * TODO: Set up interaction handlers
   */
  async init() {
    try {
      this.performanceMonitor.startOperation("AudioSpectrumAnalyzer.init");

      // TODO: Create canvas and context
      await this.createCanvas();

      // TODO: Initialize audio analysis
      await this.initAudioAnalysis();

      // TODO: Set up frequency mapping
      this.initFrequencyMapping();

      // TODO: Initialize peak detection
      this.initPeakDetection();

      // TODO: Set up interaction handlers
      this.setupInteractionHandlers();

      // TODO: Start render loop
      this.startRenderLoop();

      this.isInitialized = true;
      this.eventManager.emit("spectrumAnalyzer:initialized", {
        component: "AudioSpectrumAnalyzer",
        options: this.options,
      });

      this.performanceMonitor.endOperation("AudioSpectrumAnalyzer.init");
    } catch (error) {
      console.error("AudioSpectrumAnalyzer initialization failed:", error);
      this.eventManager.emit("spectrumAnalyzer:error", {
        error: error.message,
        component: "AudioSpectrumAnalyzer",
      });
      throw error;
    }
  }

  /**
   * Create canvas element and configure rendering context
   * TODO: Create responsive canvas with proper DPI handling
   * TODO: Configure 2D rendering context for optimal performance
   * TODO: Set up proper pixel ratio handling
   * TODO: Add canvas to container with accessibility attributes
   */
  async createCanvas() {
    try {
      // TODO: Create canvas element
      this.canvas = document.createElement("canvas");
      this.canvas.className = "spectrum-analyzer-canvas";

      // TODO: Set up responsive sizing with DPI awareness
      const pixelRatio = window.devicePixelRatio || 1;
      this.canvas.width = this.options.width * pixelRatio;
      this.canvas.height = this.options.height * pixelRatio;
      this.canvas.style.width = `${this.options.width}px`;
      this.canvas.style.height = `${this.options.height}px`;

      // TODO: Get 2D context with performance optimizations
      this.ctx = this.canvas.getContext("2d", {
        alpha: false,
        desynchronized: true,
      });
      this.ctx.scale(pixelRatio, pixelRatio);

      // TODO: Add accessibility attributes
      this.canvas.setAttribute("role", "img");
      this.canvas.setAttribute(
        "aria-label",
        "Real-time audio spectrum analyzer"
      );
      this.canvas.setAttribute("tabindex", "0");

      // TODO: Add to container
      this.container.appendChild(this.canvas);

      // TODO: Handle resize events
      window.addEventListener("resize", this.handleResize.bind(this));
    } catch (error) {
      console.error("Canvas creation failed:", error);
      throw error;
    }
  }

  /**
   * Initialize audio analysis pipeline
   * TODO: Create and configure analyser node
   * TODO: Set up frequency data buffers
   * TODO: Configure FFT parameters
   * TODO: Initialize smoothing filters
   */
  async initAudioAnalysis() {
    try {
      // TODO: Initialize frequency data arrays
      const bufferLength = this.options.fftSize / 2;
      this.frequencyData = new Float32Array(bufferLength);
      this.smoothedData = new Float32Array(bufferLength);
      this.previousData = new Float32Array(bufferLength);

      // TODO: Initialize smoothing parameters
      this.smoothingFactor = 0.9;
      this.attackTime = 0.1;
      this.releaseTime = 0.3;

      this.eventManager.emit("spectrumAnalyzer:audioAnalysisReady", {
        bufferLength,
        fftSize: this.options.fftSize,
      });
    } catch (error) {
      console.error("Audio analysis initialization failed:", error);
      throw error;
    }
  }

  /**
   * Initialize frequency mapping for different scales
   * TODO: Create logarithmic frequency mapping
   * TODO: Set up linear frequency mapping
   * TODO: Calculate frequency bin to pixel mapping
   * TODO: Generate frequency labels for display
   */
  initFrequencyMapping() {
    const bufferLength = this.options.fftSize / 2;
    this.nyquistFrequency = (this.audioContext?.sampleRate || 44100) / 2;

    // TODO: Create frequency mapping arrays
    this.frequencyBins = new Array(bufferLength);
    this.pixelToFrequency = new Array(this.options.width);
    this.frequencyToPixel = new Map();

    if (this.options.frequencyScale === "logarithmic") {
      // TODO: Logarithmic frequency mapping
      const logMin = Math.log(this.options.minFrequency);
      const logMax = Math.log(this.options.maxFrequency);
      const logRange = logMax - logMin;

      for (let i = 0; i < bufferLength; i++) {
        const frequency = (i / bufferLength) * this.nyquistFrequency;
        this.frequencyBins[i] = frequency;
      }

      for (let pixel = 0; pixel < this.options.width; pixel++) {
        const logFreq = logMin + (pixel / this.options.width) * logRange;
        this.pixelToFrequency[pixel] = Math.exp(logFreq);
      }
    } else {
      // TODO: Linear frequency mapping
      const freqRange = this.options.maxFrequency - this.options.minFrequency;

      for (let i = 0; i < bufferLength; i++) {
        const frequency = (i / bufferLength) * this.nyquistFrequency;
        this.frequencyBins[i] = frequency;
      }

      for (let pixel = 0; pixel < this.options.width; pixel++) {
        this.pixelToFrequency[pixel] =
          this.options.minFrequency + (pixel / this.options.width) * freqRange;
      }
    }

    // TODO: Create reverse mapping
    this.pixelToFrequency.forEach((freq, pixel) => {
      this.frequencyToPixel.set(Math.round(freq), pixel);
    });

    // TODO: Generate frequency labels
    this.generateFrequencyLabels();
  }

  /**
   * Generate frequency labels for display
   * TODO: Create appropriate frequency intervals
   * TODO: Format frequency values (Hz, kHz)
   * TODO: Position labels correctly on canvas
   */
  generateFrequencyLabels() {
    this.frequencyLabels = [];

    // TODO: Determine appropriate label intervals
    const labelFrequencies =
      this.options.frequencyScale === "logarithmic"
        ? [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]
        : [0, 2000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000, 20000];

    for (const freq of labelFrequencies) {
      if (
        freq >= this.options.minFrequency &&
        freq <= this.options.maxFrequency
      ) {
        const pixel =
          this.frequencyToPixel.get(freq) ||
          this.frequencyToPixel.get(this.findNearestFrequency(freq));

        if (pixel !== undefined) {
          this.frequencyLabels.push({
            frequency: freq,
            pixel: pixel,
            label: this.formatFrequency(freq),
          });
        }
      }
    }
  }

  /**
   * Initialize peak detection system
   * TODO: Set up peak detection algorithm
   * TODO: Configure peak hold functionality
   * TODO: Initialize harmonic analysis
   * TODO: Set up peak tracking history
   */
  initPeakDetection() {
    this.peakDetector = {
      threshold: this.options.peakThreshold,
      holdTime: this.options.peakHoldTime,
      minDistance: 5, // minimum distance between peaks in bins
      peaks: [],
      peakHistory: [],
    };

    // TODO: Initialize harmonic analysis
    this.harmonicAnalyzer = {
      fundamentalFrequency: null,
      harmonics: [],
      tolerance: this.options.harmonicTolerance,
    };
  }

  /**
   * Set up interaction handlers for zooming and frequency selection
   * TODO: Configure mouse interaction for zoom and pan
   * TODO: Set up touch handling for mobile devices
   * TODO: Implement frequency band selection
   * TODO: Add keyboard shortcuts for analysis controls
   */
  setupInteractionHandlers() {
    // TODO: Mouse event handlers
    this.canvas.addEventListener("mousedown", this.handleMouseDown.bind(this));
    this.canvas.addEventListener("mousemove", this.handleMouseMove.bind(this));
    this.canvas.addEventListener("mouseup", this.handleMouseUp.bind(this));
    this.canvas.addEventListener("wheel", this.handleWheel.bind(this));
    this.canvas.addEventListener("click", this.handleClick.bind(this));

    // TODO: Touch event handlers
    this.canvas.addEventListener(
      "touchstart",
      this.handleTouchStart.bind(this)
    );
    this.canvas.addEventListener("touchmove", this.handleTouchMove.bind(this));
    this.canvas.addEventListener("touchend", this.handleTouchEnd.bind(this));

    // TODO: Keyboard event handlers
    this.canvas.addEventListener("keydown", this.handleKeyDown.bind(this));

    // TODO: Context menu for analysis options
    this.canvas.addEventListener(
      "contextmenu",
      this.handleContextMenu.bind(this)
    );
  }

  /**
   * Start the main render loop
   * TODO: Implement frame rate limiting
   * TODO: Set up performance monitoring
   * TODO: Handle visibility changes for optimization
   * TODO: Implement adaptive quality based on performance
   */
  startRenderLoop() {
    let lastTime = 0;
    const targetFrameTime = 1000 / this.options.updateRate;

    const renderFrame = (currentTime) => {
      // TODO: Frame rate limiting
      if (currentTime - lastTime >= targetFrameTime) {
        this.performanceMonitor.startFrame();

        try {
          this.updateAnalysisData();
          this.detectPeaks();
          this.analyzeHarmonics();
          this.render(currentTime);
          this.updatePerformanceStats(currentTime);
        } catch (error) {
          console.error("Render error:", error);
          this.eventManager.emit("spectrumAnalyzer:renderError", { error });
        }

        this.performanceMonitor.endFrame();
        lastTime = currentTime;
      }

      if (this.isPlaying) {
        this.animationId = requestAnimationFrame(renderFrame);
      }
    };

    this.isPlaying = true;
    this.animationId = requestAnimationFrame(renderFrame);
  }

  /**
   * Update analysis data with latest frequency information
   * TODO: Get frequency data from analyser node
   * TODO: Apply smoothing filters
   * TODO: Convert to display coordinates
   * TODO: Update data buffers
   */
  updateAnalysisData() {
    if (!this.analyserNode || !this.frequencyData) return;

    try {
      // TODO: Get current frequency data
      this.analyserNode.getFloatFrequencyData(this.frequencyData);

      // TODO: Apply smoothing if enabled
      if (this.options.enableSmoothing) {
        this.applySmoothing();
      } else {
        this.smoothedData.set(this.frequencyData);
      }

      // TODO: Store previous frame for comparison
      this.previousData.set(this.smoothedData);
    } catch (error) {
      console.error("Analysis data update failed:", error);
    }
  }

  /**
   * Apply smoothing to frequency data
   * TODO: Implement attack/release smoothing
   * TODO: Apply different smoothing for rising vs falling values
   * TODO: Handle noise floor management
   */
  applySmoothing() {
    for (let i = 0; i < this.frequencyData.length; i++) {
      const current = this.frequencyData[i];
      const previous = this.smoothedData[i];

      // TODO: Use different time constants for attack and release
      const timeConstant =
        current > previous ? this.attackTime : this.releaseTime;
      this.smoothedData[i] = previous + (current - previous) * timeConstant;
    }
  }

  /**
   * Detect peaks in the frequency spectrum
   * TODO: Implement peak detection algorithm
   * TODO: Apply peak hold functionality
   * TODO: Filter out noise and spurious peaks
   * TODO: Track peak changes over time
   */
  detectPeaks() {
    if (!this.options.showPeaks) return;

    try {
      const newPeaks = [];
      const data = this.smoothedData;
      const threshold = this.options.peakThreshold;
      const minDistance = this.peakDetector.minDistance;

      // TODO: Find local maxima above threshold
      for (let i = minDistance; i < data.length - minDistance; i++) {
        if (data[i] > threshold) {
          let isPeak = true;

          // Check if this is a local maximum
          for (let j = i - minDistance; j <= i + minDistance; j++) {
            if (j !== i && data[j] >= data[i]) {
              isPeak = false;
              break;
            }
          }

          if (isPeak) {
            const frequency = this.frequencyBins[i];
            const amplitude = data[i];

            newPeaks.push({
              bin: i,
              frequency: frequency,
              amplitude: amplitude,
              pixel: this.frequencyToPixel.get(Math.round(frequency)),
              timestamp: Date.now(),
            });
          }
        }
      }

      // TODO: Update peak tracking with hold time
      this.updatePeakTracking(newPeaks);
    } catch (error) {
      console.error("Peak detection failed:", error);
    }
  }

  /**
   * Update peak tracking with hold time
   * TODO: Merge new peaks with existing ones
   * TODO: Apply peak hold timing
   * TODO: Remove expired peaks
   */
  updatePeakTracking(newPeaks) {
    const currentTime = Date.now();
    const holdTime = this.options.peakHoldTime;

    // TODO: Add new peaks
    this.peaks = this.peaks.filter(
      (peak) => currentTime - peak.timestamp < holdTime
    );

    // TODO: Merge with new peaks (avoid duplicates)
    for (const newPeak of newPeaks) {
      const existingPeak = this.peaks.find(
        (peak) => Math.abs(peak.frequency - newPeak.frequency) < 10
      );

      if (existingPeak) {
        if (newPeak.amplitude > existingPeak.amplitude) {
          existingPeak.amplitude = newPeak.amplitude;
          existingPeak.timestamp = currentTime;
        }
      } else {
        this.peaks.push(newPeak);
      }
    }
  }

  /**
   * Analyze harmonics in the frequency spectrum
   * TODO: Identify fundamental frequency
   * TODO: Detect harmonic series
   * TODO: Calculate harmonic ratios
   * TODO: Track harmonic changes over time
   */
  analyzeHarmonics() {
    if (!this.options.showHarmonics || this.peaks.length === 0) return;

    try {
      // TODO: Find fundamental frequency (lowest significant peak)
      const sortedPeaks = [...this.peaks].sort(
        (a, b) => a.frequency - b.frequency
      );
      const fundamental = sortedPeaks.find(
        (peak) =>
          peak.frequency >= 80 &&
          peak.amplitude > this.options.peakThreshold + 10
      );

      if (!fundamental) {
        this.harmonics = [];
        return;
      }

      this.harmonicAnalyzer.fundamentalFrequency = fundamental.frequency;
      const harmonics = [];

      // TODO: Look for harmonic multiples
      for (let harmonic = 2; harmonic <= 10; harmonic++) {
        const expectedFreq = fundamental.frequency * harmonic;
        const tolerance = expectedFreq * this.options.harmonicTolerance;

        const harmonicPeak = this.peaks.find(
          (peak) => Math.abs(peak.frequency - expectedFreq) < tolerance
        );

        if (harmonicPeak) {
          harmonics.push({
            harmonic: harmonic,
            frequency: harmonicPeak.frequency,
            amplitude: harmonicPeak.amplitude,
            ratio: harmonicPeak.amplitude - fundamental.amplitude,
            pixel: harmonicPeak.pixel,
          });
        }
      }

      this.harmonics = harmonics;
    } catch (error) {
      console.error("Harmonic analysis failed:", error);
    }
  }

  /**
   * Main render function
   * TODO: Clear canvas and set up rendering context
   * TODO: Draw background and grid
   * TODO: Render frequency spectrum
   * TODO: Draw peaks and harmonics
   * TODO: Render labels and annotations
   */
  render(currentTime) {
    if (!this.ctx) return;

    try {
      // TODO: Clear canvas
      this.ctx.fillStyle = this.options.backgroundColor;
      this.ctx.fillRect(0, 0, this.options.width, this.options.height);

      // TODO: Draw grid if enabled
      if (this.options.showGrid) {
        this.drawGrid();
      }

      // TODO: Render spectrum based on visualization mode
      switch (this.options.visualMode) {
        case "line":
          this.drawLineSpectrum();
          break;
        case "bars":
          this.drawBarSpectrum();
          break;
        case "filled":
          this.drawFilledSpectrum();
          break;
      }

      // TODO: Draw peaks if enabled
      if (this.options.showPeaks) {
        this.drawPeaks();
      }

      // TODO: Draw harmonics if enabled
      if (this.options.showHarmonics) {
        this.drawHarmonics();
      }

      // TODO: Draw frequency labels
      if (this.options.showLabels) {
        this.drawLabels();
      }

      // TODO: Draw selection overlay
      if (this.selectedBand) {
        this.drawSelection();
      }

      // TODO: Draw cursor tracking
      if (this.options.enableFrequencyTracking) {
        this.drawCursor();
      }

      this.frameCount++;
    } catch (error) {
      console.error("Render failed:", error);
      throw error;
    }
  }

  // TODO: Implement all drawing methods (drawGrid, drawLineSpectrum, etc.)
  // TODO: Implement interaction handlers (handleMouseDown, handleWheel, etc.)
  // TODO: Implement utility methods (formatFrequency, findNearestFrequency, etc.)
  // TODO: Implement export functionality
  // TODO: Implement cleanup and destruction methods

  /**
   * Connect to audio source for real-time analysis
   * TODO: Connect analyser node to audio graph
   * TODO: Configure analysis parameters
   * TODO: Update frequency mapping based on sample rate
   */
  connectAudioSource(audioNode) {
    try {
      if (!audioNode) {
        throw new Error("Invalid audio node");
      }

      // TODO: Create analyser if needed
      if (!this.analyserNode && audioNode.context) {
        this.audioContext = audioNode.context;
        this.analyserNode = this.audioContext.createAnalyser();
        this.analyserNode.fftSize = this.options.fftSize;
        this.analyserNode.smoothingTimeConstant =
          this.options.smoothingTimeConstant;
        this.analyserNode.minDecibels = this.options.minDecibels;
        this.analyserNode.maxDecibels = this.options.maxDecibels;
      }

      audioNode.connect(this.analyserNode);

      // TODO: Update frequency mapping with actual sample rate
      this.initFrequencyMapping();

      this.eventManager.emit("spectrumAnalyzer:audioConnected", {
        sourceType: audioNode.constructor.name,
        sampleRate: this.audioContext.sampleRate,
      });
    } catch (error) {
      console.error("Audio source connection failed:", error);
      throw error;
    }
  }

  /**
   * Export spectrum analysis data
   * TODO: Export current spectrum as image
   * TODO: Export peak data as JSON
   * TODO: Export frequency response data
   */
  async exportAnalysis(format = "png", options = {}) {
    try {
      this.performanceMonitor.startOperation("AudioSpectrumAnalyzer.export");

      switch (format.toLowerCase()) {
        case "png":
          return this.canvas.toDataURL("image/png");
        case "peaks":
          return JSON.stringify(this.peaks, null, 2);
        case "harmonics":
          return JSON.stringify(this.harmonics, null, 2);
        case "spectrum":
          return JSON.stringify(Array.from(this.smoothedData), null, 2);
        default:
          throw new Error(`Unsupported export format: ${format}`);
      }
    } catch (error) {
      console.error("Export failed:", error);
      throw error;
    } finally {
      this.performanceMonitor.endOperation("AudioSpectrumAnalyzer.export");
    }
  }

  /**
   * Clean up resources and remove event listeners
   * TODO: Stop animation loop
   * TODO: Clean up audio connections
   * TODO: Remove event listeners
   * TODO: Clear canvas and remove from DOM
   */
  destroy() {
    try {
      this.isPlaying = false;

      if (this.animationId) {
        cancelAnimationFrame(this.animationId);
      }

      // TODO: Remove event listeners
      window.removeEventListener("resize", this.handleResize);

      // TODO: Clean up DOM
      if (this.canvas && this.canvas.parentNode) {
        this.canvas.parentNode.removeChild(this.canvas);
      }

      this.eventManager.emit("spectrumAnalyzer:destroyed");
    } catch (error) {
      console.error("Cleanup failed:", error);
    }
  }

  // Getter methods for external access
  get isReady() {
    return this.isInitialized;
  }
  get currentPeaks() {
    return this.peaks;
  }
  get currentHarmonics() {
    return this.harmonics;
  }
  get frequencyRange() {
    return { min: this.options.minFrequency, max: this.options.maxFrequency };
  }
}

export default AudioSpectrumAnalyzer;
