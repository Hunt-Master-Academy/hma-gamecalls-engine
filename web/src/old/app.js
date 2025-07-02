// Huntmaster Web Application
let audioContext;
let mediaRecorder;
let recordedChunks = [];
let isRecording = false;
let analyser;
// let waveformCanvas;
// let waveformCtx;
let animationId;

// Engine-related variables
let engine = null;
let engineId = null;
let engineReady = false;
let recordedBuffer = null;

// Master call data
let masterCallAudioData = null;
let masterCallWaveformData = null;
let masterCallDuration = 0;

// Helper for logging to the page and console for easier debugging
function log(message) {
  const logElement = document.getElementById("debugLog"); // Add <pre id="debugLog"></pre> to your HTML
  if (logElement) {
    const timestamp = new Date().toLocaleTimeString();
    logElement.textContent += `[${timestamp}] ${message}\n`;
    logElement.scrollTop = logElement.scrollHeight; // Auto-scroll
  }
  console.log(message);
}
// Safely load WASM
log("App script loaded. Awaiting WASM module...");
// Check if module exists
if (typeof HuntmasterEngine === "undefined") {
  log(
    "ERROR: HuntmasterEngine is not defined. Check if huntmaster_engine.js loaded correctly."
  );
  document.getElementById("engineStatus").textContent =
    "Error: WASM module not found";
} else {
  log("HuntmasterEngine found. Attempting to load...");

  HuntmasterEngine()
    .then((Module) => {
      log("Module promise resolved. Checking Module object...");
      log(`Module type: ${typeof Module}`);
      log(`Module keys: ${Object.keys(Module).slice(0, 20).join(", ")}...`);

      // Check for specific functions we need
      log(`_createEngine exists: ${typeof Module._createEngine}`);
      log(`_malloc exists: ${typeof Module._malloc}`);
      log(`HEAP8 exists: ${typeof Module.HEAP8}`);
      log(`HEAPF32 exists: ${typeof Module.HEAPF32}`);

      if (Module.ready) {
        log("Module has 'ready' promise. Waiting...");
        Module.ready.then(() => {
          log("Module.ready resolved");
          initializeApp(Module);
        });
      } else {
        log("No Module.ready, initializing directly");
        initializeApp(Module);
      }
    })
    .catch((err) => {
      log(`FATAL: Failed to initialize WASM engine: ${err}`);
      log(`Error stack: ${err.stack}`);
      document.getElementById("engineStatus").textContent = "Engine Load Error";
    });
}

function initializeApp(Module) {
  log("WASM module promise resolved. Initializing application.");
  engine = Module;

  if (!engine.HEAPF32) {
    log("ERROR: engine.HEAPF32 is undefined after init. Cannot continue.");
    document.getElementById("engineStatus").textContent = "Engine Memory Error";
    return;
  }

  try {
    log("Creating engine instance...");
    engineId = engine._createEngine();
    log(`Raw engineId return value: ${engineId}`);
  } catch (e) {
    log(`ERROR: engine._createEngine() threw an exception: ${e}`);
    document.getElementById("engineStatus").textContent =
      "Engine Creation Error";
    return;
  }

  if (!engineId || engineId <= 0 || isNaN(engineId)) {
    log("ERROR: Invalid engine ID returned from _createEngine()");
    document.getElementById("engineStatus").textContent = "Invalid Engine ID";
    return;
  }

  // Success
  log(`✅ Engine instance created with ID: ${engineId}. Engine is ready.`);
  engineReady = true;
  document.getElementById("engineStatus").textContent = "Engine Ready!";

  log("Engine module loaded:");
  console.log(engine);
  log(`Total Memory: ${engine.HEAPF32.length * 4} bytes`);
  log(`_malloc defined: ${typeof engine._malloc === "function"}`);
  log(`_free defined: ${typeof engine._free === "function"}`);
  log(
    `DOM Ready Check - masterCallSelect exists: ${!!document.getElementById(
      "masterCallSelect"
    )}`
  );

  audioContext = new (window.AudioContext || window.webkitAudioContext)();
  setupEventListeners();
  setupWaveformDisplay();

  // Enable UI
  document.getElementById("startBtn").disabled = false;
  document.getElementById("masterCallSelect").disabled = false;
}

function setupEventListeners() {
  const select = document.getElementById("masterCallSelect");
  const loadBtn = document.getElementById("loadMasterBtn");
  loadBtn.disabled = !engineReady;
  // Only one listener for the load button
  loadBtn.addEventListener("click", loadMasterCall);

  // Enable/disable load button based on selection
  select.addEventListener("change", () => {
    loadBtn.disabled = select.value === "";
  });

  document.getElementById("startBtn").addEventListener("click", startRecording);
  document.getElementById("stopBtn").addEventListener("click", stopRecording);
  document
    .getElementById("playbackBtn")
    .addEventListener("click", playRecording);
}

function showStatus(message, type = "info") {
  const statusElement = document.getElementById("status");
  statusElement.textContent = message;

  // Optional: Add color coding based on type
  switch (type) {
    case "success":
      statusElement.style.color = "#4CAF50";
      break;
    case "error":
      statusElement.style.color = "#f44336";
      break;
    case "info":
    default:
      statusElement.style.color = "#2196F3";
      break;
  }
}

// Add these helper functions for waveform processing
function downsampleWaveform(audioData, targetLength) {
  const sourceLength = audioData.length;
  const ratio = sourceLength / targetLength;
  const result = new Float32Array(targetLength);

  for (let i = 0; i < targetLength; i++) {
    const start = Math.floor(i * ratio);
    const end = Math.floor((i + 1) * ratio);

    // Get the max absolute value in this segment for better visualization
    let maxVal = 0;
    for (let j = start; j < end && j < sourceLength; j++) {
      const absVal = Math.abs(audioData[j]);
      if (absVal > maxVal) {
        maxVal = absVal;
      }
    }
    result[i] = audioData[start] >= 0 ? maxVal : -maxVal;
  }

  return result;
}

// Mock master call data
const masterCallData = {
  buck_grunt: {
    frames: 83,
    coeffs: 13,
    data: new Float32Array(83 * 13).fill(0.1),
  },
  doe_grunt: {
    frames: 90,
    coeffs: 13,
    data: new Float32Array(90 * 13).fill(0.15),
  },
  fawn_bleat: {
    frames: 75,
    coeffs: 13,
    data: new Float32Array(75 * 13).fill(0.2),
  },
};

async function loadMasterCall() {
  console.log(
    `[${new Date().toLocaleTimeString()}] loadMasterCall() triggered.`
  );

  if (!engine || !engineId) {
    console.error("Engine not initialized!");
    showStatus("Engine not initialized. Please refresh the page.", "error");
    return;
  }

  const selectElement = document.getElementById("masterCallSelect");
  const selectedCall = selectElement.value;

  if (!selectedCall) {
    showStatus("Please select a master call.", "error");
    return;
  }

  console.log(
    `[${new Date().toLocaleTimeString()}] Attempting to load master call: ${selectedCall}`
  );

  try {
    // First, load the audio file for playback and visualization
    const audioResponse = await fetch(`master_calls/${selectedCall}.wav`);
    const audioBlob = await audioResponse.blob();
    const arrayBuffer = await audioBlob.arrayBuffer();

    // Decode audio for playback and waveform visualization
    if (!audioContext) {
      audioContext = new (window.AudioContext || window.webkitAudioContext)();
    }

    const audioBuffer = await audioContext.decodeAudioData(
      arrayBuffer.slice(0)
    ); // Clone the buffer
    masterCallDuration = audioBuffer.duration;

    // Extract audio data for waveform
    masterCallAudioData = audioBuffer.getChannelData(0);

    // Prepare waveform data (downsampled for visualization)
    const canvasWidth = waveformCanvas.width;
    masterCallWaveformData = downsampleWaveform(
      masterCallAudioData,
      canvasWidth
    );

    // Draw the master call waveform
    drawMasterCallWaveform();

    // Enable the play master call button
    document.getElementById("playMasterCallBtn").disabled = false;

    // Now load into the engine for comparison
    const response = await fetch(`master_calls/${selectedCall}.json`);
    const masterCallData = await response.json();

    if (
      !masterCallData.mfcc_features ||
      !Array.isArray(masterCallData.mfcc_features)
    ) {
      throw new Error("Invalid master call data format");
    }

    const flatFeatures = masterCallData.mfcc_features.flat();
    const numFrames = masterCallData.mfcc_features.length;
    const numCoefficients = masterCallData.mfcc_features[0].length;

    const featuresPtr = engine._malloc(flatFeatures.length * 4);
    engine.HEAPF32.set(flatFeatures, featuresPtr / 4);

    const result = engine._loadMasterCall(
      engineId,
      featuresPtr,
      numFrames,
      numCoefficients
    );
    engine._free(featuresPtr);

    if (result === 1) {
      showStatus(
        `Master call "${selectedCall}" loaded successfully!`,
        "success"
      );
      console.log(
        `[${new Date().toLocaleTimeString()}] ✅ Master call loaded: ${numFrames} frames, ${numCoefficients} coefficients per frame`
      );
    } else {
      throw new Error(`Failed to load master call. Error code: ${result}`);
    }
  } catch (error) {
    console.error("Error loading master call:", error);
    showStatus(`Error loading master call: ${error.message}`, "error");
  }
}

function drawMasterCallWaveform() {
  if (!masterCallWaveformData) return;

  const width = waveformCanvas.width;
  const height = waveformCanvas.height;
  const centerY = height / 2;

  // Clear canvas
  waveformCtx.fillStyle = "#f0f0f0";
  waveformCtx.fillRect(0, 0, width, height);

  // Draw center line
  waveformCtx.strokeStyle = "#ccc";
  waveformCtx.lineWidth = 1;
  waveformCtx.beginPath();
  waveformCtx.moveTo(0, centerY);
  waveformCtx.lineTo(width, centerY);
  waveformCtx.stroke();

  // Draw master call waveform
  waveformCtx.strokeStyle = "rgba(100, 100, 200, 0.5)"; // Blue with transparency
  waveformCtx.lineWidth = 2;
  waveformCtx.beginPath();

  const barWidth = width / masterCallWaveformData.length;

  for (let i = 0; i < masterCallWaveformData.length; i++) {
    const x = i * barWidth;
    const normalizedValue = masterCallWaveformData[i];
    const y = centerY - normalizedValue * centerY * 0.8;

    if (i === 0) {
      waveformCtx.moveTo(x, y);
    } else {
      waveformCtx.lineTo(x, y);
    }
  }

  waveformCtx.stroke();
}

async function playMasterCall() {
  if (!masterCallAudioData || !audioContext) {
    showStatus("No master call loaded.", "error");
    return;
  }

  try {
    // Create a new buffer source each time
    const source = audioContext.createBufferSource();
    const audioBuffer = audioContext.createBuffer(
      1,
      masterCallAudioData.length,
      audioContext.sampleRate
    );
    audioBuffer.getChannelData(0).set(masterCallAudioData);

    source.buffer = audioBuffer;
    source.connect(audioContext.destination);
    source.start();

    showStatus("Playing master call...", "info");

    // Update status when playback ends
    source.onended = () => {
      showStatus("Master call playback finished.", "success");
    };
  } catch (error) {
    console.error("Error playing master call:", error);
    showStatus("Error playing master call.", "error");
  }
}

function drawWaveform() {
  if (!analyser || !isRecording) return;

  const bufferLength = analyser.fftSize;
  const dataArray = new Float32Array(bufferLength);
  analyser.getFloatTimeDomainData(dataArray);

  const width = waveformCanvas.width;
  const height = waveformCanvas.height;
  const centerY = height / 2;

  // Clear canvas and redraw master call waveform if available
  waveformCtx.fillStyle = "#f0f0f0";
  waveformCtx.fillRect(0, 0, width, height);

  // Draw master call waveform in background if loaded
  if (masterCallWaveformData) {
    waveformCtx.strokeStyle = "rgba(100, 100, 200, 0.3)"; // More transparent blue
    waveformCtx.lineWidth = 2;
    waveformCtx.beginPath();

    const barWidth = width / masterCallWaveformData.length;

    for (let i = 0; i < masterCallWaveformData.length; i++) {
      const x = i * barWidth;
      const normalizedValue = masterCallWaveformData[i];
      const y = centerY - normalizedValue * centerY * 0.8;

      if (i === 0) {
        waveformCtx.moveTo(x, y);
      } else {
        waveformCtx.lineTo(x, y);
      }
    }

    waveformCtx.stroke();
  }

  // Draw center line
  waveformCtx.strokeStyle = "#ccc";
  waveformCtx.lineWidth = 1;
  waveformCtx.beginPath();
  waveformCtx.moveTo(0, centerY);
  waveformCtx.lineTo(width, centerY);
  waveformCtx.stroke();

  // Draw live waveform
  waveformCtx.strokeStyle = "#00aa00"; // Green for live recording
  waveformCtx.lineWidth = 2;
  waveformCtx.beginPath();

  const sliceWidth = width / bufferLength;
  let x = 0;

  for (let i = 0; i < bufferLength; i++) {
    const v = dataArray[i];
    const y = centerY - v * centerY * 0.8;

    if (i === 0) {
      waveformCtx.moveTo(x, y);
    } else {
      waveformCtx.lineTo(x, y);
    }

    x += sliceWidth;
  }

  waveformCtx.stroke();

  // Add legend
  waveformCtx.font = "12px Arial";
  waveformCtx.fillStyle = "rgba(100, 100, 200, 0.8)";
  waveformCtx.fillText("Master Call", 10, 20);
  waveformCtx.fillStyle = "#00aa00";
  waveformCtx.fillText("Your Recording", 10, 35);

  animationId = requestAnimationFrame(drawWaveform);
}

async function startRecording() {
  try {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });

    // Create audio nodes
    const source = audioContext.createMediaStreamSource(stream);

    // Set up analyser for waveform visualization
    analyser = audioContext.createAnalyser();
    analyser.fftSize = 2048;
    analyser.smoothingTimeConstant = 0.8;

    source.connect(analyser);

    // Start recording
    mediaRecorder = new MediaRecorder(stream);
    recordedChunks = []; // Use the correct variable name

    mediaRecorder.ondataavailable = (event) => {
      recordedChunks.push(event.data);
    };

    mediaRecorder.onstop = async () => {
      const audioBlob = new Blob(recordedChunks, { type: "audio/wav" });
      await processRecording(audioBlob);
    };

    // Start session for real-time processing
    const sessionResult = engine._startSession(engineId);
    if (sessionResult !== 1) {
      throw new Error(`Failed to start session. Error code: ${sessionResult}`);
    }

    mediaRecorder.start();
    isRecording = true;

    // Start waveform animation
    drawWaveform();

    showStatus("Recording...", "info");
    document.getElementById("startBtn").disabled = true;
    document.getElementById("stopBtn").disabled = false;
  } catch (err) {
    console.error("Error accessing microphone:", err);
    showStatus("Error: Could not access microphone", "error");
  }
}

function updateSimilarityDisplay(percentage) {
  const scoreElement = document.getElementById("similarityScore");
  scoreElement.textContent = `${percentage.toFixed(1)}%`;
  scoreElement.style.display = "block";

  // Color code based on score
  if (percentage >= 80) {
    scoreElement.style.color = "#00aa00"; // Green
    scoreElement.style.backgroundColor = "#e8f5e9";
  } else if (percentage >= 60) {
    scoreElement.style.color = "#ff9800"; // Orange
    scoreElement.style.backgroundColor = "#fff3e0";
  } else {
    scoreElement.style.color = "#f44336"; // Red
    scoreElement.style.backgroundColor = "#ffebee";
  }
}

async function processFinalAudio(audioBlob) {
  console.log("Processing final audio for similarity scoring...");

  try {
    const arrayBuffer = await audioBlob.arrayBuffer();
    const audioContext = new (window.AudioContext ||
      window.webkitAudioContext)();
    const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

    const audioData = audioBuffer.getChannelData(0);
    const sampleRate = audioBuffer.sampleRate;

    console.log(
      `Audio details: ${audioData.length} samples at ${sampleRate}Hz`
    );

    // Normalize audio data
    let maxVal = 0;
    for (let i = 0; i < audioData.length; i++) {
      maxVal = Math.max(maxVal, Math.abs(audioData[i]));
    }

    if (maxVal > 0) {
      for (let i = 0; i < audioData.length; i++) {
        audioData[i] = audioData[i] / maxVal;
      }
    }

    console.log(`Audio normalized. Max value was: ${maxVal}`);

    // Start session
    const sessionResult = engine._startSession(engineId);
    if (sessionResult !== 1) {
      throw new Error(`Failed to start session. Error code: ${sessionResult}`);
    }

    // Process in chunks
    const chunkSize = 4096;
    for (let offset = 0; offset < audioData.length; offset += chunkSize) {
      const remainingSamples = audioData.length - offset;
      const currentChunkSize = Math.min(chunkSize, remainingSamples);

      const chunk = audioData.slice(offset, offset + currentChunkSize);

      const chunkPtr = engine._malloc(currentChunkSize * 4);
      engine.HEAPF32.set(chunk, chunkPtr / 4);

      const processResult = engine._processAudioChunk(
        engineId,
        chunkPtr,
        currentChunkSize,
        sampleRate
      );

      engine._free(chunkPtr);

      if (processResult !== 1) {
        console.warn(
          `Warning: Chunk processing returned code ${processResult}`
        );
      }
    }

    // Get similarity score
    const score = engine._getSimilarityScore(engineId);

    // Get additional debug info
    const featureCount = engine._getSessionFeatureCount(engineId);
    console.log(`Session processed ${featureCount} feature frames`);

    // End session
    engine._endSession(engineId);

    // Convert score to percentage (DTW returns lower scores for better matches)
    // We need to invert and scale the score
    let similarityPercentage;
    if (score < 0) {
      similarityPercentage = 0;
    } else {
      // Empirically determined scaling - adjust based on testing
      // Lower DTW scores = better match
      // Typical good matches might be 0-50, poor matches 100+
      const maxExpectedScore = 100; // Adjust based on your testing
      similarityPercentage = Math.max(
        0,
        Math.min(100, 100 - (score / maxExpectedScore) * 100)
      );
    }

    console.log(
      `Raw DTW score: ${score}, Similarity: ${similarityPercentage}%`
    );

    showStatus(
      `Similarity Score: ${similarityPercentage.toFixed(1)}%`,
      similarityPercentage > 70 ? "success" : "info"
    );

    updateSimilarityDisplay(similarityPercentage);
  } catch (error) {
    console.error("Error processing audio:", error);
    showStatus("Error processing audio for similarity scoring.", "error");
  }
}

function stopRecording() {
  if (mediaRecorder && isRecording) {
    mediaRecorder.stop();
    isRecording = false;

    // Stop the waveform animation
    if (animationId) {
      cancelAnimationFrame(animationId);
      animationId = null;
    }

    showStatus("Processing...", "info");
    document.getElementById("startBtn").disabled = false;
    document.getElementById("stopBtn").disabled = true;

    // End the engine session
    engine._endSession(engineId);
  }
}

async function processRecording(audioBlob) {
  const arrayBuffer = await audioBlob.arrayBuffer();
  recordedBuffer = await audioContext.decodeAudioData(arrayBuffer);

  document.getElementById("playbackBtn").disabled = false;
  showStatus("Recording complete. Processing similarity...", "info");

  // Process the audio for similarity scoring
  await processFinalAudio(audioBlob);
}

function playRecording() {
  if (recordedBuffer) {
    const source = audioContext.createBufferSource();
    source.buffer = recordedBuffer;
    source.connect(audioContext.destination);
    source.start();
  }
}

function updateLevelMeter(level) {
  const percentage = Math.min(100, level * 100);
  document.getElementById("levelBar").style.width = percentage + "%";

  const levelBar = document.getElementById("levelBar");
  if (percentage > 90) {
    levelBar.style.backgroundColor = "#f44336";
  } else if (percentage > 70) {
    levelBar.style.backgroundColor = "#ff9800";
  } else {
    levelBar.style.backgroundColor = "#4CAF50";
  }
}

function displayScore(score) {
  const percentage = Math.round(score * 10000) / 100;
  document.getElementById("score").textContent = percentage + "%";

  const scoreElement = document.getElementById("score");
  if (percentage > 1) {
    scoreElement.style.color = "#4CAF50";
  } else if (percentage > 0.5) {
    scoreElement.style.color = "#ff9800";
  } else {
    scoreElement.style.color = "#f44336";
  }
}

let waveformCanvas = null;
let waveformCtx = null;
let waveformData = [];

function setupWaveformDisplay() {
  waveformCanvas = document.getElementById("waveform");
  waveformCtx = waveformCanvas.getContext("2d");

  // Set canvas size to match display size
  const rect = waveformCanvas.getBoundingClientRect();
  waveformCanvas.width = rect.width;
  waveformCanvas.height = rect.height;

  // Initial clear
  waveformCtx.fillStyle = "#f0f0f0";
  waveformCtx.fillRect(0, 0, waveformCanvas.width, waveformCanvas.height);
}

// function updateWaveform(audioData) {
//   waveformData.push(...audioData);

//   const maxSamples = 44100 * 3;
//   if (waveformData.length > maxSamples) {
//     waveformData = waveformData.slice(-maxSamples);
//   }

//   waveformCtx.fillStyle = "#f0f0f0";
//   waveformCtx.fillRect(0, 0, waveformCanvas.width, waveformCanvas.height);

//   waveformCtx.strokeStyle = "#4CAF50";
//   waveformCtx.lineWidth = 2;
//   waveformCtx.beginPath();

//   const step = Math.ceil(waveformData.length / waveformCanvas.width);
//   const amplitude = waveformCanvas.height / 2;

//   for (let i = 0; i < waveformCanvas.width; i++) {
//     const dataIndex = i * step;
//     const value = waveformData[dataIndex] || 0;
//     const y = amplitude + value * amplitude;

//     if (i === 0) {
//       waveformCtx.moveTo(i, y);
//     } else {
//       waveformCtx.lineTo(i, y);
//     }
//   }

//   waveformCtx.stroke();
// }

window.addEventListener("beforeunload", () => {
  if (engine && engineId) {
    engine._destroyEngine(engineId);
  }
});

function resetUI() {
  document.getElementById("startBtn").disabled = false;
  document.getElementById("stopBtn").disabled = true;
  document.getElementById("playbackBtn").disabled = true;
  document.getElementById("status").textContent = "Ready to record";
  document.getElementById("score").textContent = "N/A";
  document.getElementById("score").style.color = "#000";
  document.getElementById("levelBar").style.width = "0%";
  document.getElementById("levelBar").style.backgroundColor = "#4CAF50";
  // waveformData = [];
  setupWaveformDisplay(); // Clear and redraw empty waveform
}

document.getElementById("resetBtn").addEventListener("click", resetUI);

// Initial UI setup
resetUI();
