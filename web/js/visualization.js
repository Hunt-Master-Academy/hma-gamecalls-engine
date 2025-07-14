/**
 * Visualization Components for Huntmaster Engine Testing
 * Real-time waveform, frequency spectrum, and similarity visualization
 */

class Visualization {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.isRunning = false;

    // Waveform data
    this.waveformData = new Float32Array(1024);
    this.waveformHistory = [];
    this.maxHistoryLength = 100;

    // Styling
    this.primaryColor = "#2563eb";
    this.secondaryColor = "#059669";
    this.backgroundColor = "#f8fafc";
    this.gridColor = "#e2e8f0";

    this.setupCanvas();
  }

  setupCanvas() {
    // Set up high DPI rendering
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();

    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;

    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = rect.width + "px";
    this.canvas.style.height = rect.height + "px";

    this.width = rect.width;
    this.height = rect.height;
  }

  start() {
    this.isRunning = true;
    this.animationFrame();
  }

  stop() {
    this.isRunning = false;
  }

  updateWaveform(audioData) {
    // Downsample if necessary
    if (audioData.length > this.waveformData.length) {
      const step = audioData.length / this.waveformData.length;
      for (let i = 0; i < this.waveformData.length; i++) {
        this.waveformData[i] = audioData[Math.floor(i * step)];
      }
    } else {
      this.waveformData.set(audioData);
    }

    // Add to history for scrolling effect
    this.waveformHistory.push(new Float32Array(this.waveformData));
    if (this.waveformHistory.length > this.maxHistoryLength) {
      this.waveformHistory.shift();
    }
  }

  animationFrame() {
    if (!this.isRunning) return;

    this.draw();
    requestAnimationFrame(() => this.animationFrame());
  }

  draw() {
    this.clearCanvas();
    this.drawGrid();
    this.drawWaveform();
    this.drawLabels();
  }

  clearCanvas() {
    this.ctx.fillStyle = this.backgroundColor;
    this.ctx.fillRect(0, 0, this.width, this.height);
  }

  drawGrid() {
    this.ctx.strokeStyle = this.gridColor;
    this.ctx.lineWidth = 1;
    this.ctx.setLineDash([2, 2]);

    // Horizontal lines
    for (let i = 0; i <= 4; i++) {
      const y = (i / 4) * this.height;
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(this.width, y);
      this.ctx.stroke();
    }

    // Vertical lines
    for (let i = 0; i <= 8; i++) {
      const x = (i / 8) * this.width;
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, this.height);
      this.ctx.stroke();
    }

    this.ctx.setLineDash([]);
  }

  drawWaveform() {
    if (this.waveformData.length === 0) return;

    this.ctx.strokeStyle = this.primaryColor;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();

    const centerY = this.height / 2;
    const amplitude = this.height * 0.4;

    for (let i = 0; i < this.waveformData.length; i++) {
      const x = (i / this.waveformData.length) * this.width;
      const y = centerY + this.waveformData[i] * amplitude;

      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    }

    this.ctx.stroke();

    // Draw envelope
    this.drawEnvelope();
  }

  drawEnvelope() {
    if (this.waveformData.length === 0) return;

    this.ctx.strokeStyle = this.secondaryColor;
    this.ctx.lineWidth = 1;
    this.ctx.globalAlpha = 0.7;

    const windowSize = 64;
    const centerY = this.height / 2;
    const amplitude = this.height * 0.4;

    this.ctx.beginPath();

    for (let i = 0; i < this.waveformData.length; i += windowSize) {
      const end = Math.min(i + windowSize, this.waveformData.length);
      let max = 0;

      for (let j = i; j < end; j++) {
        max = Math.max(max, Math.abs(this.waveformData[j]));
      }

      const x = (i / this.waveformData.length) * this.width;
      const y = centerY - max * amplitude;

      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    }

    this.ctx.stroke();

    // Mirror for bottom envelope
    this.ctx.beginPath();

    for (let i = 0; i < this.waveformData.length; i += windowSize) {
      const end = Math.min(i + windowSize, this.waveformData.length);
      let max = 0;

      for (let j = i; j < end; j++) {
        max = Math.max(max, Math.abs(this.waveformData[j]));
      }

      const x = (i / this.waveformData.length) * this.width;
      const y = centerY + max * amplitude;

      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    }

    this.ctx.stroke();
    this.ctx.globalAlpha = 1;
  }

  drawLabels() {
    this.ctx.fillStyle = "#64748b";
    this.ctx.font = "12px sans-serif";
    this.ctx.textAlign = "left";

    // Amplitude labels
    this.ctx.fillText("1.0", 5, 15);
    this.ctx.fillText("0.0", 5, this.height / 2 + 5);
    this.ctx.fillText("-1.0", 5, this.height - 5);

    // Time label
    this.ctx.textAlign = "right";
    this.ctx.fillText("Real-time", this.width - 5, 15);
  }
}

class FrequencyAnalyzer {
  constructor(canvasId, fftSize = 2048) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.fftSize = fftSize;
    this.frequencyData = new Uint8Array(fftSize / 2);
    this.isRunning = false;

    this.setupCanvas();
  }

  setupCanvas() {
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();

    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;

    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = rect.width + "px";
    this.canvas.style.height = rect.height + "px";

    this.width = rect.width;
    this.height = rect.height;
  }

  updateFrequencyData(frequencyData) {
    this.frequencyData = frequencyData;
  }

  start() {
    this.isRunning = true;
    this.animationFrame();
  }

  stop() {
    this.isRunning = false;
  }

  animationFrame() {
    if (!this.isRunning) return;

    this.draw();
    requestAnimationFrame(() => this.animationFrame());
  }

  draw() {
    this.ctx.fillStyle = "#f8fafc";
    this.ctx.fillRect(0, 0, this.width, this.height);

    // Draw frequency bars
    const barWidth = this.width / this.frequencyData.length;

    for (let i = 0; i < this.frequencyData.length; i++) {
      const barHeight = (this.frequencyData[i] / 255) * this.height;

      // Color gradient based on frequency
      const hue = (i / this.frequencyData.length) * 240; // Blue to red
      this.ctx.fillStyle = `hsl(${hue}, 70%, 50%)`;

      this.ctx.fillRect(
        i * barWidth,
        this.height - barHeight,
        barWidth - 1,
        barHeight
      );
    }

    // Draw frequency labels
    this.ctx.fillStyle = "#64748b";
    this.ctx.font = "10px sans-serif";
    this.ctx.textAlign = "center";

    const sampleRate = 44100;
    const nyquist = sampleRate / 2;

    for (let i = 0; i <= 4; i++) {
      const freq = (i / 4) * nyquist;
      const x = (i / 4) * this.width;
      const label =
        freq >= 1000 ? `${(freq / 1000).toFixed(1)}k` : `${freq.toFixed(0)}`;

      this.ctx.fillText(label, x, this.height - 5);
    }
  }
}

class SimilarityGraph {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.scores = [];
    this.maxPoints = 200;
    this.isRunning = false;

    this.setupCanvas();
  }

  setupCanvas() {
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();

    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;

    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = rect.width + "px";
    this.canvas.style.height = rect.height + "px";

    this.width = rect.width;
    this.height = rect.height;
  }

  addScore(score, timestamp = Date.now()) {
    this.scores.push({ score, timestamp });

    if (this.scores.length > this.maxPoints) {
      this.scores.shift();
    }
  }

  start() {
    this.isRunning = true;
    this.animationFrame();
  }

  stop() {
    this.isRunning = false;
  }

  animationFrame() {
    if (!this.isRunning) return;

    this.draw();
    requestAnimationFrame(() => this.animationFrame());
  }

  draw() {
    this.ctx.fillStyle = "#f8fafc";
    this.ctx.fillRect(0, 0, this.width, this.height);

    if (this.scores.length < 2) return;

    // Draw grid
    this.drawGrid();

    // Draw similarity line
    this.ctx.strokeStyle = "#2563eb";
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();

    for (let i = 0; i < this.scores.length; i++) {
      const x = (i / (this.maxPoints - 1)) * this.width;
      const y = this.height - this.scores[i].score * this.height;

      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    }

    this.ctx.stroke();

    // Draw threshold lines
    this.drawThresholds();

    // Draw current score
    this.drawCurrentScore();
  }

  drawGrid() {
    this.ctx.strokeStyle = "#e2e8f0";
    this.ctx.lineWidth = 1;
    this.ctx.setLineDash([2, 2]);

    // Horizontal lines (similarity levels)
    for (let i = 0; i <= 4; i++) {
      const y = (i / 4) * this.height;
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(this.width, y);
      this.ctx.stroke();
    }

    this.ctx.setLineDash([]);
  }

  drawThresholds() {
    // High similarity threshold (70%)
    this.ctx.strokeStyle = "#059669";
    this.ctx.lineWidth = 1;
    this.ctx.setLineDash([5, 5]);

    const highThreshold = this.height * 0.3; // 70% from bottom
    this.ctx.beginPath();
    this.ctx.moveTo(0, highThreshold);
    this.ctx.lineTo(this.width, highThreshold);
    this.ctx.stroke();

    // Medium similarity threshold (30%)
    this.ctx.strokeStyle = "#d97706";
    const medThreshold = this.height * 0.7; // 30% from bottom
    this.ctx.beginPath();
    this.ctx.moveTo(0, medThreshold);
    this.ctx.lineTo(this.width, medThreshold);
    this.ctx.stroke();

    this.ctx.setLineDash([]);
  }

  drawCurrentScore() {
    if (this.scores.length === 0) return;

    const lastScore = this.scores[this.scores.length - 1];
    const y = this.height - lastScore.score * this.height;

    // Draw current score indicator
    this.ctx.fillStyle = "#dc2626";
    this.ctx.beginPath();
    this.ctx.arc(this.width - 10, y, 4, 0, 2 * Math.PI);
    this.ctx.fill();

    // Draw score text
    this.ctx.fillStyle = "#1e293b";
    this.ctx.font = "12px sans-serif";
    this.ctx.textAlign = "right";
    this.ctx.fillText(
      `${(lastScore.score * 100).toFixed(1)}%`,
      this.width - 20,
      y - 10
    );
  }
}

class LevelMeter {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.level = 0;
    this.peak = 0;
    this.peakHoldTime = 1000; // ms
    this.lastPeakTime = 0;

    this.setupCanvas();
  }

  setupCanvas() {
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();

    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;

    this.ctx.scale(dpr, dpr);
    this.canvas.style.width = rect.width + "px";
    this.canvas.style.height = rect.height + "px";

    this.width = rect.width;
    this.height = rect.height;
  }

  updateLevel(rmsLevel, peakLevel) {
    this.level = rmsLevel;

    if (peakLevel > this.peak) {
      this.peak = peakLevel;
      this.lastPeakTime = Date.now();
    } else if (Date.now() - this.lastPeakTime > this.peakHoldTime) {
      this.peak *= 0.95; // Gradual decay
    }

    this.draw();
  }

  draw() {
    this.ctx.fillStyle = "#1e293b";
    this.ctx.fillRect(0, 0, this.width, this.height);

    // Draw level bars
    const levelHeight = this.level * this.height;
    const peakHeight = this.peak * this.height;

    // Level gradient
    const gradient = this.ctx.createLinearGradient(0, this.height, 0, 0);
    gradient.addColorStop(0, "#059669"); // Green at bottom
    gradient.addColorStop(0.7, "#d97706"); // Yellow
    gradient.addColorStop(1, "#dc2626"); // Red at top

    this.ctx.fillStyle = gradient;
    this.ctx.fillRect(0, this.height - levelHeight, this.width, levelHeight);

    // Peak indicator
    this.ctx.fillStyle = "#ffffff";
    this.ctx.fillRect(0, this.height - peakHeight - 2, this.width, 2);

    // Scale marks
    this.ctx.strokeStyle = "#64748b";
    this.ctx.lineWidth = 1;

    for (let i = 0; i <= 10; i++) {
      const y = (i / 10) * this.height;
      const db = -60 + (i / 10) * 60; // -60 to 0 dB

      this.ctx.beginPath();
      this.ctx.moveTo(this.width - 10, y);
      this.ctx.lineTo(this.width, y);
      this.ctx.stroke();

      if (i % 2 === 0) {
        this.ctx.fillStyle = "#64748b";
        this.ctx.font = "8px sans-serif";
        this.ctx.textAlign = "right";
        this.ctx.fillText(`${db}`, this.width - 12, y + 3);
      }
    }
  }
}

// Export classes
if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    Visualization,
    FrequencyAnalyzer,
    SimilarityGraph,
    LevelMeter,
  };
} else {
  window.Visualization = Visualization;
  window.FrequencyAnalyzer = FrequencyAnalyzer;
  window.SimilarityGraph = SimilarityGraph;
  window.LevelMeter = LevelMeter;
}
