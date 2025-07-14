/**
 * Main Application Controller for Huntmaster Engine User Testing
 * Handles WASM engine initialization, UI state management, and testing workflows
 */

class HuntmasterApp {
  constructor() {
    this.engine = null;
    this.isInitialized = false;
    this.masterCalls = [];
    this.testResults = [];
    this.currentTestSession = null;
    this.audioContext = null;
    this.mediaStream = null;

    // Performance tracking
    this.stats = {
      totalTests: 0,
      avgSimilarity: 0,
      avgProcessingTime: 0,
      startTime: Date.now(),
    };

    // UI state
    this.currentStep = 1;
    this.testMode = null; // 'live' or 'file'

    this.initializeUI();
    this.loadMasterCalls();
  }

  /**
   * Initialize UI event handlers and state
   */
  initializeUI() {
    // Setup step buttons
    document
      .getElementById("initEngine")
      .addEventListener("click", () => this.initializeEngine());
    document
      .getElementById("loadMasterCall")
      .addEventListener("click", () => this.loadSelectedMasterCall());
    document
      .getElementById("startLiveTest")
      .addEventListener("click", () => this.startLiveTest());
    document
      .getElementById("startFileTest")
      .addEventListener("click", () => this.startFileTest());

    // Audio controls
    document
      .getElementById("startRecording")
      .addEventListener("click", () => this.startRecording());
    document
      .getElementById("stopRecording")
      .addEventListener("click", () => this.stopRecording());
    document
      .getElementById("clearResults")
      .addEventListener("click", () => this.clearResults());

    // File controls
    document
      .getElementById("audioFileInput")
      .addEventListener("change", (e) => this.handleFileUpload(e));

    // Results controls
    document
      .getElementById("exportResults")
      .addEventListener("click", () => this.exportResults());
    document
      .getElementById("clearLog")
      .addEventListener("click", () => this.clearLog());

    // Initialize performance monitoring
    this.performanceMonitor = new PerformanceMonitor();
    this.performanceMonitor.start();

    // Update UI every second
    setInterval(() => this.updateUI(), 1000);

    this.updateStatus("Initializing...", "loading");
  }

  /**
   * Load available master calls from the server
   */
  async loadMasterCalls() {
    try {
      const response = await fetch("/api/master-calls");
      const data = await response.json();

      this.masterCalls = data.calls || [];
      this.populateMasterCallSelect();

      if (this.masterCalls.length === 0) {
        this.showMessage(
          "No master calls found. Please ensure audio files are in data/master_calls/",
          "warning"
        );
      }
    } catch (error) {
      console.error("Failed to load master calls:", error);
      this.showMessage("Failed to load master calls from server", "error");
    }
  }

  /**
   * Populate master call selection dropdown
   */
  populateMasterCallSelect() {
    const select = document.getElementById("masterCallSelect");
    select.innerHTML = '<option value="">Select a master call...</option>';

    this.masterCalls.forEach((call) => {
      const option = document.createElement("option");
      option.value = call.name;
      option.textContent = `${call.name} (${(call.size / 1024).toFixed(1)} KB)`;
      select.appendChild(option);
    });

    if (this.masterCalls.length > 0) {
      select.disabled = false;
    }
  }

  /**
   * Initialize the Huntmaster WASM engine
   */
  async initializeEngine() {
    this.showLoading("Initializing Huntmaster Engine...");

    try {
      // Check if WASM files are available
      const wasmResponse = await fetch("/web/dist/huntmaster-engine.wasm");
      if (!wasmResponse.ok) {
        throw new Error("WASM files not found. Please build the engine first.");
      }

      // Load the WASM module
      const Module = await import("/web/dist/huntmaster-engine.js");
      await Module.ready;

      // Create engine instance
      this.engine = new Module.HuntmasterEngine();

      // Initialize with web audio settings
      const sampleRate = 44100;
      const frameSize = 1024;
      const mfccCoeffs = 13;

      const success = this.engine.initialize(sampleRate, frameSize, mfccCoeffs);

      if (!success) {
        throw new Error(
          "Failed to initialize engine with specified parameters"
        );
      }

      this.isInitialized = true;
      this.hideLoading();

      this.updateStatus("Engine initialized successfully", "ready");
      this.showMessage(
        "Huntmaster Engine initialized successfully!",
        "success"
      );

      this.setStepCompleted(1);
      this.setStepActive(2);

      // Enable master call loading
      document.getElementById("loadMasterCall").disabled = false;
    } catch (error) {
      this.hideLoading();
      console.error("Engine initialization failed:", error);
      this.updateStatus("Engine initialization failed", "error");
      this.showMessage(
        `Engine initialization failed: ${error.message}`,
        "error"
      );
    }
  }

  /**
   * Load the selected master call into the engine
   */
  async loadSelectedMasterCall() {
    const select = document.getElementById("masterCallSelect");
    const selectedCall = select.value;

    if (!selectedCall) {
      this.showMessage("Please select a master call first", "warning");
      return;
    }

    if (!this.isInitialized) {
      this.showMessage("Please initialize the engine first", "warning");
      return;
    }

    this.showLoading("Loading master call...");

    try {
      // Find the selected call data
      const callData = this.masterCalls.find(
        (call) => call.name === selectedCall
      );
      if (!callData) {
        throw new Error("Selected call not found");
      }

      // Load the audio file
      const audioResponse = await fetch(callData.path);
      if (!audioResponse.ok) {
        throw new Error(`Failed to load audio file: ${audioResponse.status}`);
      }

      const arrayBuffer = await audioResponse.arrayBuffer();

      // Create audio context to decode the audio
      if (!this.audioContext) {
        this.audioContext = new (window.AudioContext ||
          window.webkitAudioContext)();
      }

      const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
      const audioData = audioBuffer.getChannelData(0); // Use first channel

      // Load into engine
      const success = this.engine.loadMasterCall(selectedCall, audioData);

      if (!success) {
        throw new Error("Engine failed to load the master call");
      }

      this.hideLoading();
      this.showMessage(
        `Master call "${selectedCall}" loaded successfully!`,
        "success"
      );

      this.setStepCompleted(2);
      this.setStepActive(3);

      // Enable testing modes
      document.getElementById("startLiveTest").disabled = false;
      document.getElementById("startFileTest").disabled = false;
    } catch (error) {
      this.hideLoading();
      console.error("Failed to load master call:", error);
      this.showMessage(`Failed to load master call: ${error.message}`, "error");
    }
  }

  /**
   * Start live microphone testing
   */
  async startLiveTest() {
    if (!this.isInitialized) {
      this.showMessage("Please initialize the engine first", "warning");
      return;
    }

    this.testMode = "live";
    this.showTestingSection();
    this.showTestPanel("liveTestPanel");

    // Initialize audio context if needed
    if (!this.audioContext) {
      this.audioContext = new (window.AudioContext ||
        window.webkitAudioContext)();
    }

    // Resume audio context (required by browser policies)
    if (this.audioContext.state === "suspended") {
      await this.audioContext.resume();
    }

    this.showMessage(
      'Live testing mode activated. Click "Start Recording" to begin.',
      "info"
    );
  }

  /**
   * Start file-based testing
   */
  startFileTest() {
    if (!this.isInitialized) {
      this.showMessage("Please initialize the engine first", "warning");
      return;
    }

    this.testMode = "file";
    this.showTestingSection();
    this.showTestPanel("fileTestPanel");

    this.loadPresetFiles();
    this.showMessage(
      "File testing mode activated. Upload files or select from presets.",
      "info"
    );
  }

  /**
   * Start audio recording from microphone
   */
  async startRecording() {
    try {
      // Request microphone access
      this.mediaStream = await navigator.mediaDevices.getUserMedia({
        audio: {
          sampleRate: 44100,
          channelCount: 1,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
        },
      });

      // Create audio processing pipeline
      const source = this.audioContext.createMediaStreamSource(
        this.mediaStream
      );
      const processor = this.audioContext.createScriptProcessor(1024, 1, 1);

      processor.onaudioprocess = (event) => this.processAudioFrame(event);

      source.connect(processor);
      processor.connect(this.audioContext.destination);

      this.audioProcessor = processor;
      this.audioSource = source;

      // Update UI
      document.getElementById("startRecording").disabled = true;
      document.getElementById("stopRecording").disabled = false;

      // Start visualization
      this.visualization = new Visualization("waveformCanvas");
      this.visualization.start();

      this.showMessage(
        "Recording started! Speak or play audio to test the engine.",
        "success"
      );
    } catch (error) {
      console.error("Failed to start recording:", error);
      this.showMessage(
        `Failed to access microphone: ${error.message}`,
        "error"
      );
    }
  }

  /**
   * Stop audio recording
   */
  stopRecording() {
    if (this.mediaStream) {
      this.mediaStream.getTracks().forEach((track) => track.stop());
      this.mediaStream = null;
    }

    if (this.audioProcessor) {
      this.audioProcessor.disconnect();
      this.audioProcessor = null;
    }

    if (this.audioSource) {
      this.audioSource.disconnect();
      this.audioSource = null;
    }

    if (this.visualization) {
      this.visualization.stop();
      this.visualization = null;
    }

    // Update UI
    document.getElementById("startRecording").disabled = false;
    document.getElementById("stopRecording").disabled = true;

    this.showMessage("Recording stopped.", "info");
  }

  /**
   * Process audio frame from microphone
   */
  processAudioFrame(event) {
    if (!this.engine || !this.isInitialized) return;

    const inputBuffer = event.inputBuffer;
    const audioData = inputBuffer.getChannelData(0);

    // Update audio level meter
    this.updateAudioLevels(audioData);

    // Update waveform visualization
    if (this.visualization) {
      this.visualization.updateWaveform(audioData);
    }

    // Process with engine
    try {
      const result = this.engine.processAudioArray(audioData);

      if (result.success && result.score !== undefined) {
        this.updateSimilarityDisplay(result.score);
        this.addTestResult(result);
      }
    } catch (error) {
      console.error("Audio processing error:", error);
    }
  }

  /**
   * Handle file upload for testing
   */
  async handleFileUpload(event) {
    const files = Array.from(event.target.files);

    for (const file of files) {
      await this.processAudioFile(file);
    }
  }

  /**
   * Process uploaded audio file
   */
  async processAudioFile(file) {
    if (!this.isInitialized) {
      this.showMessage("Please initialize the engine first", "warning");
      return;
    }

    try {
      const arrayBuffer = await file.arrayBuffer();
      const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
      const audioData = audioBuffer.getChannelData(0);

      // Process with engine
      const result = this.engine.processAudioArray(audioData);

      if (result.success) {
        this.addFileTestResult(file.name, result);
        this.showMessage(
          `Processed ${file.name}: ${(result.score * 100).toFixed(
            1
          )}% similarity`,
          "success"
        );
      } else {
        this.showMessage(
          `Failed to process ${file.name}: ${result.error}`,
          "error"
        );
      }
    } catch (error) {
      console.error("File processing error:", error);
      this.showMessage(
        `Error processing ${file.name}: ${error.message}`,
        "error"
      );
    }
  }

  /**
   * Load preset test files from server
   */
  async loadPresetFiles() {
    try {
      const response = await fetch("/api/test-audio");
      const data = await response.json();

      const filesList = document.getElementById("presetFilesList");
      filesList.innerHTML = "";

      if (data.testFiles && data.testFiles.length > 0) {
        data.testFiles.forEach((file) => {
          const fileItem = document.createElement("div");
          fileItem.className = "file-item";
          fileItem.innerHTML = `
                        <span>${file.name}</span>
                        <button class="btn btn-outline btn-sm" onclick="app.processPresetFile('${file.path}', '${file.name}')">
                            Test
                        </button>
                    `;
          filesList.appendChild(fileItem);
        });
      } else {
        filesList.innerHTML =
          '<p class="text-muted">No preset test files available</p>';
      }
    } catch (error) {
      console.error("Failed to load preset files:", error);
    }
  }

  /**
   * Process preset audio file
   */
  async processPresetFile(filePath, fileName) {
    try {
      const response = await fetch(filePath);
      const arrayBuffer = await response.arrayBuffer();
      const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
      const audioData = audioBuffer.getChannelData(0);

      const result = this.engine.processAudioArray(audioData);

      if (result.success) {
        this.addFileTestResult(fileName, result);
        this.showMessage(
          `Processed ${fileName}: ${(result.score * 100).toFixed(
            1
          )}% similarity`,
          "success"
        );
      } else {
        this.showMessage(`Failed to process ${fileName}`, "error");
      }
    } catch (error) {
      console.error("Preset file processing error:", error);
      this.showMessage(
        `Error processing ${fileName}: ${error.message}`,
        "error"
      );
    }
  }

  /**
   * Update audio level meters
   */
  updateAudioLevels(audioData) {
    // Calculate RMS level
    let sum = 0;
    for (let i = 0; i < audioData.length; i++) {
      sum += audioData[i] * audioData[i];
    }
    const rms = Math.sqrt(sum / audioData.length);
    const db = rms > 0 ? 20 * Math.log10(rms) : -60;

    // Update meter
    const meter = document.getElementById("inputLevelMeter");
    const text = document.getElementById("inputLevelText");

    const percentage = Math.max(0, Math.min(100, ((db + 60) / 60) * 100));
    meter.style.width = `${percentage}%`;
    text.textContent = db > -60 ? `${db.toFixed(1)} dB` : "-∞ dB";
  }

  /**
   * Update similarity score display
   */
  updateSimilarityDisplay(score) {
    const scoreElement = document.querySelector(".score-value");
    const circle = document.querySelector(".score-circle");
    const confidence = document.querySelector(".confidence-fill");

    const percentage = Math.round(score * 100);
    scoreElement.textContent = `${percentage}%`;

    // Update circle gradient
    const degrees = percentage * 3.6; // 360 degrees = 100%
    circle.style.background = `conic-gradient(var(--primary-color) ${degrees}deg, var(--border-color) ${degrees}deg)`;

    // Update confidence (simple heuristic)
    const confidenceValue =
      score > 0.5 ? Math.min(100, score * 120) : score * 80;
    confidence.style.width = `${confidenceValue}%`;
    document.querySelector(
      ".confidence-text"
    ).textContent = `Confidence: ${Math.round(confidenceValue)}%`;
  }

  /**
   * Add test result to log
   */
  addTestResult(result) {
    const testResult = {
      timestamp: new Date(),
      score: result.score,
      processingTime: result.processingTimeMs || 0,
      type: "live",
    };

    this.testResults.push(testResult);
    this.updateStats();
    this.updateResultsLog();
  }

  /**
   * Add file test result
   */
  addFileTestResult(fileName, result) {
    const testResult = {
      timestamp: new Date(),
      fileName: fileName,
      score: result.score,
      processingTime: result.processingTimeMs || 0,
      type: "file",
    };

    this.testResults.push(testResult);
    this.updateStats();
    this.updateFileResults(testResult);
  }

  /**
   * Update statistics
   */
  updateStats() {
    this.stats.totalTests = this.testResults.length;

    if (this.testResults.length > 0) {
      this.stats.avgSimilarity =
        this.testResults.reduce((sum, r) => sum + r.score, 0) /
        this.testResults.length;
      this.stats.avgProcessingTime =
        this.testResults.reduce((sum, r) => sum + r.processingTime, 0) /
        this.testResults.length;
    }
  }

  /**
   * Update results log display
   */
  updateResultsLog() {
    const logContainer = document.getElementById("resultsLog");

    if (this.testResults.length === 0) {
      logContainer.innerHTML =
        '<p class="log-empty">No test results yet. Start testing to see analysis data here.</p>';
      return;
    }

    // Show recent results (last 10)
    const recentResults = this.testResults.slice(-10).reverse();

    logContainer.innerHTML = recentResults
      .map(
        (result) => `
            <div class="log-entry">
                <div class="log-header">
                    <strong>${
                      result.type === "live" ? "🎤 Live" : "📁 File"
                    } Test</strong>
                    <span class="log-time">${result.timestamp.toLocaleTimeString()}</span>
                </div>
                <div class="log-details">
                    ${
                      result.fileName
                        ? `<div>File: ${result.fileName}</div>`
                        : ""
                    }
                    <div>Similarity: ${(result.score * 100).toFixed(1)}%</div>
                    <div>Processing: ${result.processingTime.toFixed(1)}ms</div>
                </div>
            </div>
        `
      )
      .join("");
  }

  /**
   * Update file test results display
   */
  updateFileResults(result) {
    const resultsContainer = document.getElementById("fileAnalysisResults");

    const resultElement = document.createElement("div");
    resultElement.className = "analysis-result";
    resultElement.innerHTML = `
            <div class="result-header">
                <h4>📁 ${result.fileName}</h4>
                <span class="result-score">${(result.score * 100).toFixed(
                  1
                )}%</span>
            </div>
            <div class="result-details">
                <span>Processing Time: ${result.processingTime.toFixed(
                  1
                )}ms</span>
                <span>Timestamp: ${result.timestamp.toLocaleTimeString()}</span>
            </div>
        `;

    resultsContainer.appendChild(resultElement);
  }

  /**
   * Update UI elements
   */
  updateUI() {
    // Update stats display
    document.getElementById("totalTests").textContent = this.stats.totalTests;
    document.getElementById("avgSimilarity").textContent = `${(
      this.stats.avgSimilarity * 100
    ).toFixed(1)}%`;
    document.getElementById(
      "processingTime"
    ).textContent = `${this.stats.avgProcessingTime.toFixed(1)}ms`;

    // Update uptime
    const uptime = Math.floor((Date.now() - this.stats.startTime) / 1000);
    document.getElementById("engineUptime").textContent = `${uptime}s`;

    // Update performance metrics
    this.performanceMonitor.update();
  }

  /**
   * Show/hide sections and panels
   */
  showTestingSection() {
    document.getElementById("testingSection").classList.add("active");
    document.getElementById("setupSection").style.display = "none";
  }

  showTestPanel(panelId) {
    // Hide all panels
    document.querySelectorAll(".test-panel").forEach((panel) => {
      panel.classList.remove("active");
    });

    // Show selected panel
    document.getElementById(panelId).classList.add("active");
  }

  /**
   * Step management
   */
  setStepActive(stepNumber) {
    document
      .querySelectorAll(".step")
      .forEach((step) => step.classList.remove("active"));
    document.getElementById(`step${stepNumber}`).classList.add("active");
    this.currentStep = stepNumber;
  }

  setStepCompleted(stepNumber) {
    document.getElementById(`step${stepNumber}`).classList.add("completed");
    document.getElementById(`step${stepNumber}`).classList.remove("active");
  }

  /**
   * Status management
   */
  updateStatus(message, status) {
    const statusElement = document.getElementById("engineStatus");
    const dot = statusElement.querySelector(".status-dot");
    const text = statusElement.querySelector(".status-text");

    text.textContent = message;

    dot.className = "status-dot";
    if (status === "ready") {
      dot.classList.add("ready");
    } else if (status === "error") {
      dot.classList.add("error");
    }
  }

  /**
   * Message management
   */
  showMessage(message, type = "info") {
    const container = document.getElementById("messageContainer");

    const messageElement = document.createElement("div");
    messageElement.className = `message ${type}`;
    messageElement.textContent = message;

    container.appendChild(messageElement);

    // Auto-remove after 5 seconds
    setTimeout(() => {
      messageElement.remove();
    }, 5000);
  }

  /**
   * Loading overlay
   */
  showLoading(message = "Loading...") {
    const overlay = document.getElementById("loadingOverlay");
    const text = overlay.querySelector(".loading-text");
    text.textContent = message;
    overlay.classList.remove("hidden");
  }

  hideLoading() {
    document.getElementById("loadingOverlay").classList.add("hidden");
  }

  /**
   * Results management
   */
  clearResults() {
    this.testResults = [];
    this.stats.totalTests = 0;
    this.stats.avgSimilarity = 0;
    this.stats.avgProcessingTime = 0;
    this.updateResultsLog();
    this.showMessage("Results cleared", "info");
  }

  clearLog() {
    this.testResults = [];
    this.updateResultsLog();
    this.showMessage("Log cleared", "info");
  }

  exportResults() {
    const data = {
      engineInfo: {
        version: "1.0.0",
        initialized: this.isInitialized,
        masterCalls: this.masterCalls.map((call) => call.name),
      },
      statistics: this.stats,
      results: this.testResults,
    };

    const blob = new Blob([JSON.stringify(data, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);

    const a = document.createElement("a");
    a.href = url;
    a.download = `huntmaster-test-results-${
      new Date().toISOString().split("T")[0]
    }.json`;
    a.click();

    URL.revokeObjectURL(url);
    this.showMessage("Results exported successfully", "success");
  }
}

// Initialize app when DOM is loaded
document.addEventListener("DOMContentLoaded", () => {
  window.app = new HuntmasterApp();
});

// Make app globally available for preset file testing
window.processPresetFile = (path, name) => {
  if (window.app) {
    window.app.processPresetFile(path, name);
  }
};
