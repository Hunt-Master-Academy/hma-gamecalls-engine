/**
 * @file waveform-renderer.js
 * @brief High-performance waveform visualization and rendering system
 *
 * This module provides comprehensive waveform rendering capabilities including:
 * - Canvas-based high-performance rendering
 * - Multiple visualization styles (line, bar, filled, etc.)
 * - Zoom and pan functionality
 * - Time-based and sample-based rendering
 * - Color theming and customization
 * - Performance optimization and caching
 */

/**
 * Waveform rendering styles
 */
const RENDER_STYLES = {
  LINE: "line", // Connected line waveform
  BARS: "bars", // Vertical bars
  FILLED: "filled", // Filled area under curve
  MIRROR: "mirror", // Mirrored top and bottom
  HISTOGRAM: "histogram", // Histogram-style bars
  DOTS: "dots", // Individual sample points
};

/**
 * Color schemes for waveform rendering
 */
const COLOR_SCHEMES = {
  BLUE: {
    waveform: "#4A90E2",
    background: "#F5F5F5",
    grid: "#E0E0E0",
    cursor: "#FF6B6B",
    selection: "rgba(74, 144, 226, 0.3)",
  },
  GREEN: {
    waveform: "#7ED321",
    background: "#F8F8F8",
    grid: "#E5E5E5",
    cursor: "#F5A623",
    selection: "rgba(126, 211, 33, 0.3)",
  },
  DARK: {
    waveform: "#00D4AA",
    background: "#1A1A1A",
    grid: "#333333",
    cursor: "#FF6B6B",
    selection: "rgba(0, 212, 170, 0.3)",
  },
  CUSTOM: {}, // User-defined colors
};

/**
 * High-performance waveform renderer with Canvas API
 */
export class WaveformRenderer {
  constructor(canvas, options = {}) {
    if (!canvas || !(canvas instanceof HTMLCanvasElement)) {
      throw new Error("Valid HTML Canvas element is required");
    }

    this.canvas = canvas;
    this.ctx = canvas.getContext("2d");

    // Configuration options
    this.options = {
      // Rendering options
      renderStyle: options.renderStyle || RENDER_STYLES.FILLED,
      colorScheme: options.colorScheme || COLOR_SCHEMES.BLUE,
      lineWidth: options.lineWidth || 2,
      pixelRatio: options.pixelRatio || window.devicePixelRatio || 1,

      // Performance options
      enableCaching: options.enableCaching !== false,
      cacheThreshold: options.cacheThreshold || 1000, // Cache if more than 1000 samples
      maxCacheSize: options.maxCacheSize || 50 * 1024 * 1024, // 50MB cache limit
      renderBatchSize: options.renderBatchSize || 10000, // Samples per render batch

      // Zoom and pan options
      enableZoom: options.enableZoom !== false,
      enablePan: options.enablePan !== false,
      minZoom: options.minZoom || 0.1,
      maxZoom: options.maxZoom || 100,
      zoomSensitivity: options.zoomSensitivity || 0.1,

      // Grid and markers
      showGrid: options.showGrid !== false,
      gridSpacing: options.gridSpacing || 50, // pixels
      showTimeLabels: options.showTimeLabels !== false,
      showAmplitudeLabels: options.showAmplitudeLabels !== false,

      // Interaction options
      enableSelection: options.enableSelection !== false,
      enableCursor: options.enableCursor !== false,
      cursorFollowMouse: options.cursorFollowMouse || false,

      // Animation options
      enableAnimations: options.enableAnimations !== false,
      animationDuration: options.animationDuration || 300,
      animationEasing: options.animationEasing || "easeOutCubic",

      // Event handling
      eventManager: options.eventManager || null,

      // Debug options
      debugMode: options.debugMode || false,
      showPerformanceStats: options.showPerformanceStats || false,

      ...options,
    };

    // Waveform data and state
    this.audioData = null;
    this.sampleRate = 44100;
    this.duration = 0;
    this.channels = 1;

    // Rendering state
    this.viewport = {
      startTime: 0,
      endTime: 0,
      startSample: 0,
      endSample: 0,
      zoom: 1,
      panOffset: 0,
    };

    // Interaction state
    this.selection = { start: -1, end: -1, active: false };
    this.cursor = { position: -1, visible: false };
    this.isInteracting = false;
    this.lastMousePosition = { x: 0, y: 0 };

    // Performance tracking
    this.metrics = {
      renderCount: 0,
      totalRenderTime: 0,
      averageRenderTime: 0,
      lastRenderTime: 0,
      cacheHits: 0,
      cacheMisses: 0,
      memoryUsage: 0,
    };

    // Caching system
    this.renderCache = new Map();
    this.cacheMemoryUsage = 0;

    // Animation system
    this.animationFrame = null;
    this.animations = new Set();

    // Initialize the renderer
    this._initialize();
  }

  /**
   * Load waveform data for rendering
   */
  loadWaveformData(audioData, sampleRate = 44100, channels = 1) {
    if (!audioData || !Array.isArray(audioData)) {
      throw new Error("Valid audio data array is required");
    }

    this.audioData = audioData;
    this.sampleRate = sampleRate;
    this.channels = channels;
    this.duration = audioData.length / sampleRate;

    // Reset viewport to show full waveform
    this.viewport = {
      startTime: 0,
      endTime: this.duration,
      startSample: 0,
      endSample: audioData.length,
      zoom: 1,
      panOffset: 0,
    };

    // Clear cache when new data is loaded
    this._clearCache();

    // Trigger initial render
    this.render();

    // Emit data loaded event
    this._emitEvent("waveformDataLoaded", {
      sampleCount: audioData.length,
      duration: this.duration,
      sampleRate: sampleRate,
      channels: channels,
    });

    return this;
  }

  /**
   * Render the waveform to the canvas
   */
  render(force = false) {
    if (!this.audioData) {
      this._renderEmpty();
      return this;
    }

    const startTime = performance.now();

    try {
      // Check cache if enabled
      const cacheKey = this._generateCacheKey();
      if (
        this.options.enableCaching &&
        !force &&
        this.renderCache.has(cacheKey)
      ) {
        this._renderFromCache(cacheKey);
        this.metrics.cacheHits++;
      } else {
        this._renderWaveform();
        this.metrics.cacheMisses++;

        // Cache the result if beneficial
        if (this.options.enableCaching && this._shouldCache()) {
          this._cacheRender(cacheKey);
        }
      }

      // Render overlays (grid, cursor, selection)
      this._renderOverlays();

      // Update performance metrics
      const renderTime = performance.now() - startTime;
      this._updatePerformanceMetrics(renderTime);

      // Show performance stats if enabled
      if (this.options.showPerformanceStats) {
        this._renderPerformanceStats();
      }

      // Emit render complete event
      this._emitEvent("waveformRendered", {
        renderTime: renderTime,
        viewport: { ...this.viewport },
        cacheHit: this.renderCache.has(cacheKey),
      });
    } catch (error) {
      console.error("Waveform render failed:", error);
      this._renderError(error);
    }

    return this;
  }

  /**
   * Set zoom level and update viewport
   */
  setZoom(zoomLevel, centerTime = null) {
    const newZoom = Math.max(
      this.options.minZoom,
      Math.min(this.options.maxZoom, zoomLevel)
    );

    if (newZoom === this.viewport.zoom) {
      return this; // No change needed
    }

    const oldZoom = this.viewport.zoom;
    this.viewport.zoom = newZoom;

    // Calculate new viewport based on center point
    if (centerTime !== null) {
      this._recalculateViewportFromCenter(centerTime);
    } else {
      this._recalculateViewport();
    }

    // Animate zoom if enabled
    if (this.options.enableAnimations) {
      this._animateZoom(oldZoom, newZoom);
    } else {
      this.render();
    }

    // Emit zoom change event
    this._emitEvent("zoomChanged", {
      from: oldZoom,
      to: newZoom,
      viewport: { ...this.viewport },
    });

    return this;
  }

  /**
   * Pan the viewport by a time offset
   */
  pan(timeOffset) {
    const newPanOffset = this.viewport.panOffset + timeOffset;

    // Constrain pan to valid range
    const maxPan = Math.max(
      0,
      this.duration - (this.viewport.endTime - this.viewport.startTime)
    );
    const constrainedOffset = Math.max(0, Math.min(maxPan, newPanOffset));

    if (constrainedOffset === this.viewport.panOffset) {
      return this; // No change needed
    }

    const oldOffset = this.viewport.panOffset;
    this.viewport.panOffset = constrainedOffset;

    // Update viewport times
    this._recalculateViewport();

    // Animate pan if enabled
    if (this.options.enableAnimations) {
      this._animatePan(oldOffset, constrainedOffset);
    } else {
      this.render();
    }

    // Emit pan change event
    this._emitEvent("panChanged", {
      from: oldOffset,
      to: constrainedOffset,
      viewport: { ...this.viewport },
    });

    return this;
  }

  /**
   * Set selection range in time
   */
  setSelection(startTime, endTime) {
    if (startTime < 0 || endTime > this.duration || startTime >= endTime) {
      this.clearSelection();
      return this;
    }

    this.selection = {
      start: startTime,
      end: endTime,
      active: true,
    };

    this.render();

    // Emit selection change event
    this._emitEvent("selectionChanged", {
      start: startTime,
      end: endTime,
      duration: endTime - startTime,
    });

    return this;
  }

  /**
   * Clear current selection
   */
  clearSelection() {
    if (!this.selection.active) {
      return this; // No selection to clear
    }

    this.selection = { start: -1, end: -1, active: false };
    this.render();

    // Emit selection cleared event
    this._emitEvent("selectionCleared");

    return this;
  }

  /**
   * Set cursor position in time
   */
  setCursor(time, visible = true) {
    if (time < 0 || time > this.duration) {
      this.hideCursor();
      return this;
    }

    this.cursor = {
      position: time,
      visible: visible,
    };

    this.render();

    // Emit cursor change event
    this._emitEvent("cursorChanged", {
      position: time,
      visible: visible,
    });

    return this;
  }

  /**
   * Hide cursor
   */
  hideCursor() {
    if (!this.cursor.visible) {
      return this; // Already hidden
    }

    this.cursor.visible = false;
    this.render();

    // Emit cursor hidden event
    this._emitEvent("cursorHidden");

    return this;
  }

  /**
   * Get performance metrics
   */
  getPerformanceMetrics() {
    return {
      ...this.metrics,
      cacheSize: this.renderCache.size,
      cacheMemoryUsage: this.cacheMemoryUsage,
      cacheHitRate:
        this.metrics.cacheHits /
          (this.metrics.cacheHits + this.metrics.cacheMisses) || 0,
    };
  }

  /**
   * Update renderer options
   */
  updateOptions(newOptions) {
    const oldOptions = { ...this.options };
    this.options = { ...this.options, ...newOptions };

    // Handle specific option changes
    if (
      newOptions.colorScheme &&
      newOptions.colorScheme !== oldOptions.colorScheme
    ) {
      this._clearCache(); // Color change requires cache clear
    }

    if (
      newOptions.renderStyle &&
      newOptions.renderStyle !== oldOptions.renderStyle
    ) {
      this._clearCache(); // Style change requires cache clear
    }

    // Re-render with new options
    this.render(true);

    return this;
  }

  /**
   * Resize canvas and update rendering
   */
  resize(width, height) {
    // Update canvas size with device pixel ratio
    const pixelRatio = this.options.pixelRatio;

    this.canvas.width = width * pixelRatio;
    this.canvas.height = height * pixelRatio;
    this.canvas.style.width = `${width}px`;
    this.canvas.style.height = `${height}px`;

    // Scale context for high DPI displays
    this.ctx.scale(pixelRatio, pixelRatio);

    // Clear cache as dimensions changed
    this._clearCache();

    // Re-render
    this.render(true);

    // Emit resize event
    this._emitEvent("rendererResized", { width, height });

    return this;
  }

  /**
   * Clean up resources and event listeners
   */
  destroy() {
    try {
      // Cancel any ongoing animations
      if (this.animationFrame) {
        cancelAnimationFrame(this.animationFrame);
      }

      // Clear all caches
      this._clearCache();

      // Remove event listeners
      this._removeEventListeners();

      // Clear canvas
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

      // Emit destruction event
      this._emitEvent("rendererDestroyed");
    } catch (error) {
      console.error("Waveform renderer cleanup failed:", error);
    }
  }

  // Private methods

  /**
   * Initialize the renderer
   */
  _initialize() {
    // Set up canvas with proper DPI scaling
    this._setupCanvas();

    // Set up event listeners
    this._setupEventListeners();

    // Initial render
    this._renderEmpty();
  }

  /**
   * Set up canvas with proper pixel ratio scaling
   */
  _setupCanvas() {
    const rect = this.canvas.getBoundingClientRect();
    this.resize(rect.width, rect.height);
  }

  /**
   * Set up mouse and keyboard event listeners
   */
  _setupEventListeners() {
    // Mouse events
    this.canvas.addEventListener("mousedown", this._handleMouseDown.bind(this));
    this.canvas.addEventListener("mousemove", this._handleMouseMove.bind(this));
    this.canvas.addEventListener("mouseup", this._handleMouseUp.bind(this));
    this.canvas.addEventListener("wheel", this._handleWheel.bind(this), {
      passive: false,
    });

    // Touch events for mobile
    this.canvas.addEventListener(
      "touchstart",
      this._handleTouchStart.bind(this),
      { passive: false }
    );
    this.canvas.addEventListener(
      "touchmove",
      this._handleTouchMove.bind(this),
      { passive: false }
    );
    this.canvas.addEventListener("touchend", this._handleTouchEnd.bind(this));

    // Context menu
    this.canvas.addEventListener("contextmenu", (e) => e.preventDefault());
  }

  /**
   * Remove event listeners
   */
  _removeEventListeners() {
    this.canvas.removeEventListener("mousedown", this._handleMouseDown);
    this.canvas.removeEventListener("mousemove", this._handleMouseMove);
    this.canvas.removeEventListener("mouseup", this._handleMouseUp);
    this.canvas.removeEventListener("wheel", this._handleWheel);
    this.canvas.removeEventListener("touchstart", this._handleTouchStart);
    this.canvas.removeEventListener("touchmove", this._handleTouchMove);
    this.canvas.removeEventListener("touchend", this._handleTouchEnd);
  }

  /**
   * Render empty state
   */
  _renderEmpty() {
    const { width, height } = this.canvas.getBoundingClientRect();

    // Clear canvas
    this.ctx.clearRect(0, 0, width, height);

    // Draw empty state
    this.ctx.fillStyle = this.options.colorScheme.background;
    this.ctx.fillRect(0, 0, width, height);

    // Draw placeholder text
    this.ctx.fillStyle = this.options.colorScheme.grid;
    this.ctx.font = "16px Arial";
    this.ctx.textAlign = "center";
    this.ctx.textBaseline = "middle";
    this.ctx.fillText("No waveform data loaded", width / 2, height / 2);
  }

  /**
   * Main waveform rendering logic
   */
  _renderWaveform() {
    const { width, height } = this.canvas.getBoundingClientRect();

    // Clear canvas
    this.ctx.clearRect(0, 0, width, height);

    // Draw background
    this.ctx.fillStyle = this.options.colorScheme.background;
    this.ctx.fillRect(0, 0, width, height);

    // Calculate samples to render
    const startSample = Math.floor(this.viewport.startSample);
    const endSample = Math.ceil(this.viewport.endSample);
    const samplesPerPixel = (endSample - startSample) / width;

    // Choose rendering method based on zoom level
    if (samplesPerPixel > 1) {
      this._renderDownsampled(startSample, endSample, width, height);
    } else {
      this._renderFullDetail(startSample, endSample, width, height);
    }
  }

  /**
   * Render downsampled waveform for zoomed out view
   */
  _renderDownsampled(startSample, endSample, width, height) {
    const samplesPerPixel = (endSample - startSample) / width;
    const centerY = height / 2;
    const amplitudeScale = height / 2;

    this.ctx.strokeStyle = this.options.colorScheme.waveform;
    this.ctx.lineWidth = this.options.lineWidth;
    this.ctx.beginPath();

    for (let x = 0; x < width; x++) {
      const sampleStart = startSample + x * samplesPerPixel;
      const sampleEnd = Math.min(
        startSample + (x + 1) * samplesPerPixel,
        this.audioData.length
      );

      // Find min/max in this pixel range
      let min = 0,
        max = 0;
      for (let i = Math.floor(sampleStart); i < Math.ceil(sampleEnd); i++) {
        if (i < this.audioData.length) {
          const sample = this.audioData[i];
          min = Math.min(min, sample);
          max = Math.max(max, sample);
        }
      }

      // Draw based on render style
      switch (this.options.renderStyle) {
        case RENDER_STYLES.LINE:
          const avgSample = (min + max) / 2;
          const y = centerY - avgSample * amplitudeScale;
          if (x === 0) {
            this.ctx.moveTo(x, y);
          } else {
            this.ctx.lineTo(x, y);
          }
          break;

        case RENDER_STYLES.BARS:
          this.ctx.moveTo(x, centerY - max * amplitudeScale);
          this.ctx.lineTo(x, centerY - min * amplitudeScale);
          break;

        case RENDER_STYLES.FILLED:
          if (x === 0) {
            this.ctx.moveTo(x, centerY);
          }
          this.ctx.lineTo(x, centerY - max * amplitudeScale);
          break;

        case RENDER_STYLES.MIRROR:
          this.ctx.moveTo(x, centerY - max * amplitudeScale);
          this.ctx.lineTo(x, centerY - min * amplitudeScale);
          this.ctx.moveTo(x, centerY + min * amplitudeScale);
          this.ctx.lineTo(x, centerY + max * amplitudeScale);
          break;
      }
    }

    // Apply the stroke
    if (this.options.renderStyle === RENDER_STYLES.FILLED) {
      this.ctx.lineTo(width, centerY);
      this.ctx.closePath();
      this.ctx.fillStyle = this.options.colorScheme.waveform;
      this.ctx.fill();
    } else {
      this.ctx.stroke();
    }
  }

  /**
   * Render full detail waveform for zoomed in view
   */
  _renderFullDetail(startSample, endSample, width, height) {
    const centerY = height / 2;
    const amplitudeScale = height / 2;
    const sampleWidth = width / (endSample - startSample);

    this.ctx.strokeStyle = this.options.colorScheme.waveform;
    this.ctx.lineWidth = this.options.lineWidth;
    this.ctx.beginPath();

    for (let i = startSample; i < endSample && i < this.audioData.length; i++) {
      const x = (i - startSample) * sampleWidth;
      const sample = this.audioData[i];
      const y = centerY - sample * amplitudeScale;

      if (this.options.renderStyle === RENDER_STYLES.DOTS) {
        this.ctx.fillRect(x - 0.5, y - 0.5, 1, 1);
      } else {
        if (i === startSample) {
          this.ctx.moveTo(x, y);
        } else {
          this.ctx.lineTo(x, y);
        }
      }
    }

    if (this.options.renderStyle !== RENDER_STYLES.DOTS) {
      this.ctx.stroke();
    }
  }

  /**
   * Render overlays (grid, cursor, selection)
   */
  _renderOverlays() {
    if (this.options.showGrid) {
      this._renderGrid();
    }

    if (this.selection.active) {
      this._renderSelection();
    }

    if (this.cursor.visible) {
      this._renderCursor();
    }
  }

  /**
   * Render time grid
   */
  _renderGrid() {
    const { width, height } = this.canvas.getBoundingClientRect();
    const spacing = this.options.gridSpacing;

    this.ctx.strokeStyle = this.options.colorScheme.grid;
    this.ctx.lineWidth = 1;
    this.ctx.setLineDash([2, 2]);

    // Vertical lines (time)
    for (let x = 0; x < width; x += spacing) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, height);
      this.ctx.stroke();
    }

    // Horizontal lines (amplitude)
    for (let y = 0; y < height; y += spacing) {
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(width, y);
      this.ctx.stroke();
    }

    this.ctx.setLineDash([]);
  }

  /**
   * Render selection area
   */
  _renderSelection() {
    const { width, height } = this.canvas.getBoundingClientRect();
    const startX = this._timeToPixel(this.selection.start);
    const endX = this._timeToPixel(this.selection.end);

    this.ctx.fillStyle = this.options.colorScheme.selection;
    this.ctx.fillRect(startX, 0, endX - startX, height);
  }

  /**
   * Render cursor line
   */
  _renderCursor() {
    const { height } = this.canvas.getBoundingClientRect();
    const x = this._timeToPixel(this.cursor.position);

    this.ctx.strokeStyle = this.options.colorScheme.cursor;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(x, 0);
    this.ctx.lineTo(x, height);
    this.ctx.stroke();
  }

  /**
   * Convert time to pixel position
   */
  _timeToPixel(time) {
    const { width } = this.canvas.getBoundingClientRect();
    const timeRange = this.viewport.endTime - this.viewport.startTime;
    return ((time - this.viewport.startTime) / timeRange) * width;
  }

  /**
   * Convert pixel position to time
   */
  _pixelToTime(pixel) {
    const { width } = this.canvas.getBoundingClientRect();
    const timeRange = this.viewport.endTime - this.viewport.startTime;
    return this.viewport.startTime + (pixel / width) * timeRange;
  }

  /**
   * Recalculate viewport bounds
   */
  _recalculateViewport() {
    const viewportDuration = this.duration / this.viewport.zoom;
    this.viewport.startTime = this.viewport.panOffset;
    this.viewport.endTime = Math.min(
      this.viewport.startTime + viewportDuration,
      this.duration
    );

    this.viewport.startSample = this.viewport.startTime * this.sampleRate;
    this.viewport.endSample = this.viewport.endTime * this.sampleRate;
  }

  /**
   * Recalculate viewport from center point
   */
  _recalculateViewportFromCenter(centerTime) {
    const viewportDuration = this.duration / this.viewport.zoom;
    const halfDuration = viewportDuration / 2;

    this.viewport.startTime = Math.max(0, centerTime - halfDuration);
    this.viewport.endTime = Math.min(this.duration, centerTime + halfDuration);

    // Adjust if we hit boundaries
    if (this.viewport.endTime === this.duration) {
      this.viewport.startTime = Math.max(0, this.duration - viewportDuration);
    }
    if (this.viewport.startTime === 0) {
      this.viewport.endTime = Math.min(this.duration, viewportDuration);
    }

    this.viewport.panOffset = this.viewport.startTime;
    this.viewport.startSample = this.viewport.startTime * this.sampleRate;
    this.viewport.endSample = this.viewport.endTime * this.sampleRate;
  }

  /**
   * Generate cache key for current render state
   */
  _generateCacheKey() {
    return `${this.viewport.startSample}-${this.viewport.endSample}-${
      this.options.renderStyle
    }-${JSON.stringify(this.options.colorScheme)}`;
  }

  /**
   * Check if current render should be cached
   */
  _shouldCache() {
    const sampleCount = this.viewport.endSample - this.viewport.startSample;
    return (
      sampleCount > this.options.cacheThreshold &&
      this.cacheMemoryUsage < this.options.maxCacheSize
    );
  }

  /**
   * Cache current render
   */
  _cacheRender(cacheKey) {
    const imageData = this.ctx.getImageData(
      0,
      0,
      this.canvas.width,
      this.canvas.height
    );
    const cacheEntry = {
      imageData: imageData,
      timestamp: Date.now(),
      memorySize: imageData.data.length,
    };

    this.renderCache.set(cacheKey, cacheEntry);
    this.cacheMemoryUsage += cacheEntry.memorySize;

    // Clean up old cache entries if needed
    this._cleanupCache();
  }

  /**
   * Render from cache
   */
  _renderFromCache(cacheKey) {
    const cacheEntry = this.renderCache.get(cacheKey);
    if (cacheEntry) {
      this.ctx.putImageData(cacheEntry.imageData, 0, 0);
    }
  }

  /**
   * Clear render cache
   */
  _clearCache() {
    this.renderCache.clear();
    this.cacheMemoryUsage = 0;
  }

  /**
   * Clean up old cache entries
   */
  _cleanupCache() {
    if (this.cacheMemoryUsage <= this.options.maxCacheSize) {
      return;
    }

    // Sort by timestamp and remove oldest entries
    const entries = Array.from(this.renderCache.entries()).sort(
      ([, a], [, b]) => a.timestamp - b.timestamp
    );

    while (
      this.cacheMemoryUsage > this.options.maxCacheSize * 0.8 &&
      entries.length > 0
    ) {
      const [key, entry] = entries.shift();
      this.renderCache.delete(key);
      this.cacheMemoryUsage -= entry.memorySize;
    }
  }

  /**
   * Update performance metrics
   */
  _updatePerformanceMetrics(renderTime) {
    this.metrics.renderCount++;
    this.metrics.totalRenderTime += renderTime;
    this.metrics.averageRenderTime =
      this.metrics.totalRenderTime / this.metrics.renderCount;
    this.metrics.lastRenderTime = renderTime;
    this.metrics.memoryUsage = this.cacheMemoryUsage;
  }

  /**
   * Render performance statistics
   */
  _renderPerformanceStats() {
    const { width } = this.canvas.getBoundingClientRect();

    this.ctx.fillStyle = "rgba(0, 0, 0, 0.8)";
    this.ctx.fillRect(width - 200, 10, 190, 100);

    this.ctx.fillStyle = "white";
    this.ctx.font = "12px monospace";
    this.ctx.textAlign = "left";

    const stats = [
      `Renders: ${this.metrics.renderCount}`,
      `Last: ${this.metrics.lastRenderTime.toFixed(2)}ms`,
      `Avg: ${this.metrics.averageRenderTime.toFixed(2)}ms`,
      `Cache: ${this.renderCache.size} entries`,
      `Hit rate: ${(
        (this.metrics.cacheHits /
          (this.metrics.cacheHits + this.metrics.cacheMisses)) *
          100 || 0
      ).toFixed(1)}%`,
      `Memory: ${(this.cacheMemoryUsage / 1024 / 1024).toFixed(1)}MB`,
    ];

    stats.forEach((stat, i) => {
      this.ctx.fillText(stat, width - 195, 30 + i * 15);
    });
  }

  /**
   * Render error state
   */
  _renderError(error) {
    const { width, height } = this.canvas.getBoundingClientRect();

    this.ctx.clearRect(0, 0, width, height);
    this.ctx.fillStyle = "#ffebee";
    this.ctx.fillRect(0, 0, width, height);

    this.ctx.fillStyle = "#c62828";
    this.ctx.font = "16px Arial";
    this.ctx.textAlign = "center";
    this.ctx.textBaseline = "middle";
    this.ctx.fillText(`Render Error: ${error.message}`, width / 2, height / 2);
  }

  /**
   * Handle mouse down events
   */
  _handleMouseDown(event) {
    this.isInteracting = true;
    this.lastMousePosition = { x: event.offsetX, y: event.offsetY };

    if (this.options.enableSelection) {
      const time = this._pixelToTime(event.offsetX);
      this.setSelection(time, time);
    }
  }

  /**
   * Handle mouse move events
   */
  _handleMouseMove(event) {
    this.lastMousePosition = { x: event.offsetX, y: event.offsetY };

    if (this.options.cursorFollowMouse) {
      const time = this._pixelToTime(event.offsetX);
      this.setCursor(time);
    }

    if (
      this.isInteracting &&
      this.options.enableSelection &&
      this.selection.active
    ) {
      const time = this._pixelToTime(event.offsetX);
      this.setSelection(this.selection.start, time);
    }
  }

  /**
   * Handle mouse up events
   */
  _handleMouseUp(event) {
    this.isInteracting = false;
  }

  /**
   * Handle wheel events for zooming
   */
  _handleWheel(event) {
    if (!this.options.enableZoom) return;

    event.preventDefault();

    const zoomDelta =
      event.deltaY > 0
        ? -this.options.zoomSensitivity
        : this.options.zoomSensitivity;
    const newZoom = this.viewport.zoom * (1 + zoomDelta);
    const centerTime = this._pixelToTime(event.offsetX);

    this.setZoom(newZoom, centerTime);
  }

  /**
   * Handle touch start events
   */
  _handleTouchStart(event) {
    if (event.touches.length === 1) {
      const touch = event.touches[0];
      const rect = this.canvas.getBoundingClientRect();
      this._handleMouseDown({
        offsetX: touch.clientX - rect.left,
        offsetY: touch.clientY - rect.top,
      });
    }
  }

  /**
   * Handle touch move events
   */
  _handleTouchMove(event) {
    event.preventDefault();

    if (event.touches.length === 1) {
      const touch = event.touches[0];
      const rect = this.canvas.getBoundingClientRect();
      this._handleMouseMove({
        offsetX: touch.clientX - rect.left,
        offsetY: touch.clientY - rect.top,
      });
    }
  }

  /**
   * Handle touch end events
   */
  _handleTouchEnd(event) {
    this._handleMouseUp(event);
  }

  /**
   * Emit events through event manager
   */
  _emitEvent(eventName, data = {}) {
    if (
      this.options.eventManager &&
      typeof this.options.eventManager.emit === "function"
    ) {
      this.options.eventManager.emit(eventName, {
        source: "WaveformRenderer",
        timestamp: Date.now(),
        ...data,
      });
    }

    // Also dispatch as DOM event
    const event = new CustomEvent(`waveform:${eventName}`, { detail: data });
    this.canvas.dispatchEvent(event);
  }
}

// Export constants and utilities
export { RENDER_STYLES, COLOR_SCHEMES };

// Export default
export default WaveformRenderer;
