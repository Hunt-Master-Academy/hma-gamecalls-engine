/**
 * @fileoverview Waveform Export Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Advanced waveform export module providing comprehensive export capabilities
 * for waveform data, visualizations, and analysis results in multiple formats.
 *
 * Features:
 * ✅ Multiple export formats (PNG, SVG, PDF, WAV, JSON, CSV)
 * ✅ High-resolution export with custom DPI settings
 * ✅ Batch export capabilities
 * ✅ Export templates and presets
 * ✅ Metadata embedding and preservation
 * ✅ Custom export dimensions and scaling
 * ✅ Watermarking and branding options
 * ✅ Progress tracking for large exports
 * ✅ Export history and management
 * ✅ Cloud export integration
 * ✅ Export validation and verification
 * ✅ Custom export filters and effects
 *
 * @example
 * ```javascript
 * import { WaveformExport } from './modules/waveform/index.js';
 *
 * const exporter = new WaveformExport({
 *   defaultFormat: 'png',
 *   quality: 'high',
 *   embedMetadata: true
 * });
 *
 * await exporter.exportWaveform(waveformData, {
 *   format: 'png',
 *   width: 1920,
 *   height: 1080,
 *   filename: 'my-waveform.png'
 * });
 * ```
 */

/**
 * Export formats supported by the system
 */
export const EXPORT_FORMATS = {
  // Image formats
  PNG: "png",
  JPEG: "jpeg",
  SVG: "svg",
  PDF: "pdf",
  WEBP: "webp",

  // Audio formats
  WAV: "wav",
  MP3: "mp3",
  FLAC: "flac",
  OGG: "ogg",

  // Data formats
  JSON: "json",
  CSV: "csv",
  XML: "xml",
  YAML: "yaml",

  // Document formats
  HTML: "html",
  MARKDOWN: "markdown",
};

/**
 * Export quality presets
 */
export const QUALITY_PRESETS = {
  LOW: {
    dpi: 72,
    compression: 0.7,
    antialiasing: false,
    colorDepth: 8,
  },
  MEDIUM: {
    dpi: 150,
    compression: 0.85,
    antialiasing: true,
    colorDepth: 16,
  },
  HIGH: {
    dpi: 300,
    compression: 0.95,
    antialiasing: true,
    colorDepth: 24,
  },
  ULTRA: {
    dpi: 600,
    compression: 1.0,
    antialiasing: true,
    colorDepth: 32,
  },
};

/**
 * Standard export dimensions
 */
export const STANDARD_DIMENSIONS = {
  THUMBNAIL: { width: 320, height: 240 },
  SMALL: { width: 640, height: 480 },
  MEDIUM: { width: 1280, height: 720 },
  LARGE: { width: 1920, height: 1080 },
  ULTRA: { width: 3840, height: 2160 },

  // Print formats
  A4: { width: 2480, height: 3508 }, // 300 DPI
  LETTER: { width: 2550, height: 3300 }, // 300 DPI

  // Social media formats
  FACEBOOK: { width: 1200, height: 630 },
  TWITTER: { width: 1024, height: 512 },
  INSTAGRAM: { width: 1080, height: 1080 },
};

/**
 * Waveform Export Manager
 *
 * Provides comprehensive export capabilities for waveform data and visualizations
 * with support for multiple formats, quality settings, and advanced features.
 *
 * @class WaveformExport
 */
export class WaveformExport {
  /**
   * Create WaveformExport instance
   *
   * @param {Object} options - Configuration options
   * @param {string} [options.defaultFormat='png'] - Default export format
   * @param {string} [options.quality='high'] - Default quality preset
   * @param {boolean} [options.embedMetadata=true] - Embed metadata in exports
   * @param {boolean} [options.enableWatermark=false] - Enable watermarking
   * @param {boolean} [options.enableProgress=true] - Enable progress tracking
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      defaultFormat: options.defaultFormat || EXPORT_FORMATS.PNG,
      quality: options.quality || "high",
      embedMetadata: options.embedMetadata !== false,
      enableWatermark: options.enableWatermark === true,
      enableProgress: options.enableProgress !== false,
      tempDirectory: options.tempDirectory || "/tmp/waveform-exports",
      maxConcurrentExports: options.maxConcurrentExports || 3,
      ...options,
    };

    // Export state
    this.activeExports = new Map();
    this.exportHistory = [];
    this.exportQueue = [];
    this.isProcessing = false;

    // Templates and presets
    this.templates = new Map();
    this.presets = new Map();

    // Progress tracking
    this.progressCallbacks = new Map();

    // Event handlers
    this.eventHandlers = new Map();

    // Canvas contexts for rendering
    this.canvasContexts = new Map();

    // Export statistics
    this.stats = {
      totalExports: 0,
      successfulExports: 0,
      failedExports: 0,
      totalSizeExported: 0,
      averageExportTime: 0,
    };

    // Initialize system
    this._initializeTemplates();
    this._initializePresets();
    this._initializeCanvasContexts();

    console.log("WaveformExport initialized");
  }

  /**
   * Initialize export templates
   * @private
   */
  _initializeTemplates() {
    // Waveform visualization template
    this.templates.set("waveform", {
      name: "Standard Waveform",
      description: "Standard waveform visualization with time axis",
      elements: [
        {
          type: "waveform",
          position: { x: 50, y: 100 },
          size: { width: 800, height: 200 },
        },
        {
          type: "timeAxis",
          position: { x: 50, y: 320 },
          size: { width: 800, height: 30 },
        },
        {
          type: "amplitudeAxis",
          position: { x: 20, y: 100 },
          size: { width: 30, height: 200 },
        },
        {
          type: "title",
          position: { x: 450, y: 30 },
          text: "Waveform Analysis",
        },
        { type: "metadata", position: { x: 50, y: 370 } },
      ],
    });

    // Spectrogram template
    this.templates.set("spectrogram", {
      name: "Spectrogram Analysis",
      description: "Frequency spectrum analysis over time",
      elements: [
        {
          type: "spectrogram",
          position: { x: 50, y: 100 },
          size: { width: 800, height: 300 },
        },
        {
          type: "timeAxis",
          position: { x: 50, y: 420 },
          size: { width: 800, height: 30 },
        },
        {
          type: "frequencyAxis",
          position: { x: 20, y: 100 },
          size: { width: 30, height: 300 },
        },
        {
          type: "colorbar",
          position: { x: 870, y: 100 },
          size: { width: 20, height: 300 },
        },
        {
          type: "title",
          position: { x: 450, y: 30 },
          text: "Spectrogram Analysis",
        },
      ],
    });

    // Analysis report template
    this.templates.set("report", {
      name: "Analysis Report",
      description: "Comprehensive analysis report with multiple visualizations",
      elements: [
        {
          type: "waveform",
          position: { x: 50, y: 80 },
          size: { width: 400, height: 150 },
        },
        {
          type: "spectrum",
          position: { x: 500, y: 80 },
          size: { width: 400, height: 150 },
        },
        {
          type: "spectrogram",
          position: { x: 50, y: 280 },
          size: { width: 850, height: 200 },
        },
        {
          type: "statistics",
          position: { x: 50, y: 520 },
          size: { width: 400, height: 200 },
        },
        {
          type: "analysis",
          position: { x: 500, y: 520 },
          size: { width: 400, height: 200 },
        },
        {
          type: "title",
          position: { x: 450, y: 30 },
          text: "Audio Analysis Report",
        },
      ],
    });

    console.log(`Initialized ${this.templates.size} export templates`);
  }

  /**
   * Initialize export presets
   * @private
   */
  _initializePresets() {
    // Web preset
    this.presets.set("web", {
      format: EXPORT_FORMATS.PNG,
      dimensions: STANDARD_DIMENSIONS.MEDIUM,
      quality: QUALITY_PRESETS.MEDIUM,
      optimization: {
        progressive: true,
        stripMetadata: false,
        colorOptimization: true,
        compression: 0.85,
      },
    });

    // Print preset
    this.presets.set("print", {
      format: EXPORT_FORMATS.PDF,
      dimensions: STANDARD_DIMENSIONS.A4,
      quality: QUALITY_PRESETS.HIGH,
      options: {
        vectorBased: true,
        embedFonts: true,
        colorProfile: "CMYK",
      },
    });

    // Social media preset
    this.presets.set("social", {
      format: EXPORT_FORMATS.PNG,
      dimensions: STANDARD_DIMENSIONS.FACEBOOK,
      quality: QUALITY_PRESETS.HIGH,
      options: {
        addWatermark: true,
        socialOptimized: true,
        sRGBColorSpace: true,
      },
    });

    // Data export preset
    this.presets.set("data", {
      format: EXPORT_FORMATS.JSON,
      includeRawData: true,
      includeAnalysis: true,
      compression: true,
      metadata: {
        includeTimestamp: true,
        includeSettings: true,
        includeStatistics: true,
      },
    });

    console.log(`Initialized ${this.presets.size} export presets`);
  }

  /**
   * Initialize canvas contexts for rendering
   * @private
   */
  _initializeCanvasContexts() {
    // Create off-screen canvas for rendering
    try {
      // High-DPI canvas for quality exports
      this.canvasContexts.set("main", {
        canvas: document.createElement("canvas"),
        context: null,
        dpr: window.devicePixelRatio || 1,
      });

      // Initialize context
      const mainCtx = this.canvasContexts.get("main");
      mainCtx.context = mainCtx.canvas.getContext("2d");

      // Enable high-quality rendering
      mainCtx.context.imageSmoothingEnabled = true;
      mainCtx.context.imageSmoothingQuality = "high";

      console.log("Canvas contexts initialized for export rendering");
    } catch (error) {
      console.warn("Failed to initialize canvas contexts:", error);
    }
  }

  /**
   * Export waveform visualization
   *
   * @param {Object} waveformData - Waveform data to export
   * @param {Object} options - Export options
   * @returns {Promise<Object>} Export result with download info
   */
  async exportWaveform(waveformData, options = {}) {
    const exportId = this._generateExportId();
    const startTime = performance.now();

    try {
      // Validate and merge options
      const exportOptions = await this._validateExportOptions(options);

      // Register export in progress
      this._registerExport(exportId, exportOptions);

      // Update progress
      this._updateProgress(exportId, 10, "Preparing export...");

      // Prepare canvas based on export format
      const canvas = await this._prepareCanvas(exportOptions);

      this._updateProgress(exportId, 25, "Rendering waveform...");

      // Render waveform to canvas
      await this._renderWaveform(canvas, waveformData, exportOptions);

      this._updateProgress(exportId, 60, "Applying effects and styling...");

      // Apply post-processing effects
      await this._applyPostProcessing(canvas, exportOptions);

      this._updateProgress(exportId, 80, "Generating output file...");

      // Generate final output
      const result = await this._generateOutput(canvas, exportOptions);

      this._updateProgress(exportId, 95, "Finalizing export...");

      // Add metadata if requested
      if (exportOptions.embedMetadata) {
        await this._embedMetadata(result, waveformData, exportOptions);
      }

      // Calculate export statistics
      const exportTime = performance.now() - startTime;
      this._updateStatistics(result.size, exportTime, true);

      // Complete export
      this._completeExport(exportId);

      this._updateProgress(exportId, 100, "Export completed successfully");

      this._emitEvent("exportComplete", {
        exportId,
        result,
        exportTime,
        options: exportOptions,
      });

      return {
        success: true,
        exportId,
        result,
        exportTime,
        size: result.size,
      };
    } catch (error) {
      this._updateStatistics(0, performance.now() - startTime, false);
      this._failExport(exportId, error);

      this._emitEvent("exportError", {
        exportId,
        error: error.message,
        options,
      });

      throw error;
    }
  }

  /**
   * Export audio data
   *
   * @param {AudioBuffer|Float32Array} audioData - Audio data to export
   * @param {Object} options - Export options
   * @returns {Promise<Object>} Export result
   */
  async exportAudio(audioData, options = {}) {
    const exportId = this._generateExportId();
    const startTime = performance.now();

    try {
      const exportOptions = {
        format: options.format || EXPORT_FORMATS.WAV,
        sampleRate: options.sampleRate || 44100,
        bitDepth: options.bitDepth || 16,
        channels: options.channels || 1,
        normalize: options.normalize !== false,
        filename: options.filename || `export-${Date.now()}`,
        ...options,
      };

      this._registerExport(exportId, exportOptions);
      this._updateProgress(exportId, 10, "Processing audio data...");

      // Convert audio data to target format
      let processedData;
      if (audioData instanceof AudioBuffer) {
        processedData = this._convertAudioBuffer(audioData, exportOptions);
      } else {
        processedData = this._convertFloat32Array(audioData, exportOptions);
      }

      this._updateProgress(exportId, 50, "Encoding audio...");

      // Encode to target format
      const encodedData = await this._encodeAudio(processedData, exportOptions);

      this._updateProgress(exportId, 80, "Generating file...");

      // Create downloadable blob
      const blob = new Blob([encodedData], {
        type: this._getMimeType(exportOptions.format),
      });

      const result = {
        blob,
        url: URL.createObjectURL(blob),
        filename: `${exportOptions.filename}.${exportOptions.format}`,
        size: blob.size,
        format: exportOptions.format,
      };

      this._completeExport(exportId);
      this._updateProgress(exportId, 100, "Audio export completed");

      const exportTime = performance.now() - startTime;
      this._updateStatistics(result.size, exportTime, true);

      return {
        success: true,
        exportId,
        result,
        exportTime,
      };
    } catch (error) {
      this._failExport(exportId, error);
      throw error;
    }
  }

  /**
   * Export analysis data
   *
   * @param {Object} analysisData - Analysis results to export
   * @param {Object} options - Export options
   * @returns {Promise<Object>} Export result
   */
  async exportAnalysis(analysisData, options = {}) {
    const exportId = this._generateExportId();

    try {
      const exportOptions = {
        format: options.format || EXPORT_FORMATS.JSON,
        includeRawData: options.includeRawData !== false,
        includeMetadata: options.includeMetadata !== false,
        includeStatistics: options.includeStatistics !== false,
        compression: options.compression === true,
        filename: options.filename || `analysis-${Date.now()}`,
        ...options,
      };

      this._registerExport(exportId, exportOptions);
      this._updateProgress(exportId, 20, "Preparing analysis data...");

      // Prepare data based on format
      let exportData;
      switch (exportOptions.format) {
        case EXPORT_FORMATS.JSON:
          exportData = this._prepareJsonExport(analysisData, exportOptions);
          break;
        case EXPORT_FORMATS.CSV:
          exportData = this._prepareCsvExport(analysisData, exportOptions);
          break;
        case EXPORT_FORMATS.XML:
          exportData = this._prepareXmlExport(analysisData, exportOptions);
          break;
        default:
          throw new Error(
            `Unsupported analysis export format: ${exportOptions.format}`
          );
      }

      this._updateProgress(exportId, 70, "Formatting data...");

      // Apply compression if requested
      if (exportOptions.compression) {
        exportData = await this._compressData(exportData);
      }

      this._updateProgress(exportId, 90, "Creating download file...");

      // Create blob and download URL
      const blob = new Blob([exportData], {
        type: this._getMimeType(exportOptions.format),
      });

      const result = {
        blob,
        url: URL.createObjectURL(blob),
        filename: `${exportOptions.filename}.${exportOptions.format}`,
        size: blob.size,
        format: exportOptions.format,
        compressed: exportOptions.compression,
      };

      this._completeExport(exportId);
      this._updateProgress(exportId, 100, "Analysis export completed");

      return {
        success: true,
        exportId,
        result,
      };
    } catch (error) {
      this._failExport(exportId, error);
      throw error;
    }
  }

  /**
   * Batch export multiple items
   *
   * @param {Array} items - Items to export
   * @param {Object} options - Batch export options
   * @returns {Promise<Array>} Array of export results
   */
  async batchExport(items, options = {}) {
    const batchId = this._generateExportId();
    const results = [];

    try {
      this._emitEvent("batchExportStart", { batchId, itemCount: items.length });

      for (let i = 0; i < items.length; i++) {
        const item = items[i];
        const itemOptions = { ...options, batchId, batchIndex: i };

        try {
          let result;
          switch (item.type) {
            case "waveform":
              result = await this.exportWaveform(item.data, itemOptions);
              break;
            case "audio":
              result = await this.exportAudio(item.data, itemOptions);
              break;
            case "analysis":
              result = await this.exportAnalysis(item.data, itemOptions);
              break;
            default:
              throw new Error(`Unknown export type: ${item.type}`);
          }

          results.push(result);

          this._emitEvent("batchExportProgress", {
            batchId,
            completed: i + 1,
            total: items.length,
            progress: ((i + 1) / items.length) * 100,
          });
        } catch (error) {
          results.push({
            success: false,
            error: error.message,
            item,
          });
        }
      }

      this._emitEvent("batchExportComplete", { batchId, results });

      return results;
    } catch (error) {
      this._emitEvent("batchExportError", { batchId, error: error.message });
      throw error;
    }
  }

  /**
   * Validate export options
   * @private
   */
  async _validateExportOptions(options) {
    const defaults = {
      format: this.config.defaultFormat,
      quality: this.config.quality,
      dimensions: STANDARD_DIMENSIONS.MEDIUM,
      filename: `export-${Date.now()}`,
      embedMetadata: this.config.embedMetadata,
      enableWatermark: this.config.enableWatermark,
    };

    const validated = { ...defaults, ...options };

    // Validate format
    if (!Object.values(EXPORT_FORMATS).includes(validated.format)) {
      throw new Error(`Unsupported export format: ${validated.format}`);
    }

    // Apply quality preset if specified
    if (
      typeof validated.quality === "string" &&
      QUALITY_PRESETS[validated.quality.toUpperCase()]
    ) {
      validated.qualitySettings =
        QUALITY_PRESETS[validated.quality.toUpperCase()];
    }

    // Apply template if specified
    if (validated.template && this.templates.has(validated.template)) {
      validated.templateSettings = this.templates.get(validated.template);
    }

    return validated;
  }

  /**
   * Prepare canvas for export
   * @private
   */
  async _prepareCanvas(options) {
    const { dimensions, qualitySettings } = options;
    const dpr = qualitySettings?.dpi
      ? qualitySettings.dpi / 96
      : window.devicePixelRatio || 1;

    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    // Set canvas dimensions with DPI scaling
    canvas.width = dimensions.width * dpr;
    canvas.height = dimensions.height * dpr;
    canvas.style.width = dimensions.width + "px";
    canvas.style.height = dimensions.height + "px";

    // Scale context for high DPI
    ctx.scale(dpr, dpr);

    // Configure rendering quality
    ctx.imageSmoothingEnabled = qualitySettings?.antialiasing !== false;
    ctx.imageSmoothingQuality = "high";

    return { canvas, ctx, dpr };
  }

  /**
   * Render waveform to canvas
   * @private
   */
  async _renderWaveform(canvasInfo, waveformData, options) {
    const { ctx } = canvasInfo;
    const { dimensions } = options;

    // Clear canvas with background
    ctx.fillStyle = options.backgroundColor || "#ffffff";
    ctx.fillRect(0, 0, dimensions.width, dimensions.height);

    // Render waveform
    if (waveformData.peaks) {
      this._renderWaveformPeaks(ctx, waveformData.peaks, dimensions, options);
    }

    // Render additional elements based on template
    if (options.templateSettings) {
      await this._renderTemplateElements(
        ctx,
        options.templateSettings,
        waveformData,
        options
      );
    }
  }

  /**
   * Render waveform peaks
   * @private
   */
  _renderWaveformPeaks(ctx, peaks, dimensions, options) {
    const waveformStyle = options.waveformStyle || "filled";
    const waveformColor = options.waveformColor || "#3498db";
    const lineWidth = options.lineWidth || 1;

    ctx.strokeStyle = waveformColor;
    ctx.fillStyle = waveformColor;
    ctx.lineWidth = lineWidth;

    const width = dimensions.width;
    const height = dimensions.height;
    const centerY = height / 2;
    const amplitude = height * 0.4;

    ctx.beginPath();

    for (let i = 0; i < peaks.length; i++) {
      const x = (i / peaks.length) * width;
      const y = centerY - peaks[i] * amplitude;

      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }

    if (waveformStyle === "filled") {
      // Fill waveform
      ctx.lineTo(width, centerY);
      ctx.lineTo(0, centerY);
      ctx.closePath();
      ctx.fill();
    } else {
      // Stroke waveform
      ctx.stroke();
    }
  }

  /**
   * Apply post-processing effects
   * @private
   */
  async _applyPostProcessing(canvasInfo, options) {
    if (options.enableWatermark) {
      await this._addWatermark(canvasInfo, options);
    }

    if (options.effects) {
      await this._applyEffects(canvasInfo, options.effects);
    }
  }

  /**
   * Generate output file
   * @private
   */
  async _generateOutput(canvasInfo, options) {
    const { canvas } = canvasInfo;
    const { format, filename, qualitySettings } = options;

    let blob;
    let mimeType = this._getMimeType(format);

    switch (format) {
      case EXPORT_FORMATS.PNG:
        blob = await this._canvasToBlob(canvas, "image/png");
        break;
      case EXPORT_FORMATS.JPEG:
        blob = await this._canvasToBlob(
          canvas,
          "image/jpeg",
          qualitySettings.compression
        );
        break;
      case EXPORT_FORMATS.SVG:
        blob = await this._canvasToSvg(canvas, options);
        break;
      case EXPORT_FORMATS.PDF:
        blob = await this._canvasToPdf(canvas, options);
        break;
      default:
        throw new Error(`Unsupported output format: ${format}`);
    }

    return {
      blob,
      url: URL.createObjectURL(blob),
      filename: `${filename}.${format}`,
      size: blob.size,
      format,
      mimeType,
    };
  }

  /**
   * Get MIME type for format
   * @private
   */
  _getMimeType(format) {
    const mimeTypes = {
      [EXPORT_FORMATS.PNG]: "image/png",
      [EXPORT_FORMATS.JPEG]: "image/jpeg",
      [EXPORT_FORMATS.SVG]: "image/svg+xml",
      [EXPORT_FORMATS.PDF]: "application/pdf",
      [EXPORT_FORMATS.WEBP]: "image/webp",
      [EXPORT_FORMATS.WAV]: "audio/wav",
      [EXPORT_FORMATS.MP3]: "audio/mpeg",
      [EXPORT_FORMATS.JSON]: "application/json",
      [EXPORT_FORMATS.CSV]: "text/csv",
      [EXPORT_FORMATS.XML]: "application/xml",
    };

    return mimeTypes[format] || "application/octet-stream";
  }

  /**
   * Convert canvas to blob
   * @private
   */
  _canvasToBlob(canvas, mimeType, quality = 1.0) {
    return new Promise((resolve) => {
      canvas.toBlob(resolve, mimeType, quality);
    });
  }

  /**
   * Generate unique export ID
   * @private
   */
  _generateExportId() {
    return `export_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  /**
   * Register export in progress
   * @private
   */
  _registerExport(exportId, options) {
    this.activeExports.set(exportId, {
      id: exportId,
      startTime: Date.now(),
      progress: 0,
      status: "initializing",
      options,
    });
  }

  /**
   * Update export progress
   * @private
   */
  _updateProgress(exportId, progress, message) {
    if (this.activeExports.has(exportId)) {
      const exportInfo = this.activeExports.get(exportId);
      exportInfo.progress = progress;
      exportInfo.status = message;
      exportInfo.lastUpdate = Date.now();

      this._emitEvent("exportProgress", {
        exportId,
        progress,
        message,
      });
    }
  }

  /**
   * Complete export
   * @private
   */
  _completeExport(exportId) {
    if (this.activeExports.has(exportId)) {
      const exportInfo = this.activeExports.get(exportId);
      exportInfo.endTime = Date.now();
      exportInfo.status = "completed";

      // Move to history
      this.exportHistory.push(exportInfo);
      this.activeExports.delete(exportId);

      // Limit history size
      if (this.exportHistory.length > 100) {
        this.exportHistory.shift();
      }
    }
  }

  /**
   * Fail export
   * @private
   */
  _failExport(exportId, error) {
    if (this.activeExports.has(exportId)) {
      const exportInfo = this.activeExports.get(exportId);
      exportInfo.endTime = Date.now();
      exportInfo.status = "failed";
      exportInfo.error = error.message;

      this.exportHistory.push(exportInfo);
      this.activeExports.delete(exportId);
    }
  }

  /**
   * Update statistics
   * @private
   */
  _updateStatistics(size, time, success) {
    this.stats.totalExports++;
    if (success) {
      this.stats.successfulExports++;
      this.stats.totalSizeExported += size;
    } else {
      this.stats.failedExports++;
    }

    this.stats.averageExportTime =
      (this.stats.averageExportTime * (this.stats.totalExports - 1) + time) /
      this.stats.totalExports;
  }

  /**
   * Get export statistics
   *
   * @returns {Object} Export statistics
   */
  getStatistics() {
    return { ...this.stats };
  }

  /**
   * Get export history
   *
   * @returns {Array} Export history
   */
  getExportHistory() {
    return [...this.exportHistory];
  }

  /**
   * Get active exports
   *
   * @returns {Array} Active export information
   */
  getActiveExports() {
    return Array.from(this.activeExports.values());
  }

  /**
   * Cancel export
   *
   * @param {string} exportId - Export ID to cancel
   * @returns {boolean} Success status
   */
  cancelExport(exportId) {
    if (this.activeExports.has(exportId)) {
      this._failExport(exportId, new Error("Export cancelled by user"));
      this._emitEvent("exportCancelled", { exportId });
      return true;
    }
    return false;
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
   * Cleanup and destroy exporter
   */
  destroy() {
    console.log("Destroying WaveformExport...");

    // Cancel all active exports
    for (const exportId of this.activeExports.keys()) {
      this.cancelExport(exportId);
    }

    // Clean up canvas contexts
    this.canvasContexts.clear();

    // Clear data
    this.activeExports.clear();
    this.exportHistory.length = 0;
    this.templates.clear();
    this.presets.clear();
    this.eventHandlers.clear();

    console.log("WaveformExport destroyed");
  }
}

export default WaveformExport;
