// Huntmaster Web Application
let engine = null;
let engineId = null;
let audioContext = null;
let mediaRecorder = null;
let audioChunks = [];
let isRecording = false;
let recordedBuffer = null;
let engineReady = false;

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

function loadMasterCall() {
  log("loadMasterCall() triggered.");
  const callType = document.getElementById("masterCallSelect").value;
  const masterData = masterCallData[callType];

  // Detailed check and logging to help debug the "Engine not ready" issue
  log(`- Checking engine state:`);
  log(`  - engine object exists: ${!!engine}`);
  if (engine) {
    log(`  - engine.HEAPF32 exists: ${!!engine.HEAPF32}`);
  }
  log(`  - engineId is valid: ${engineId > 0} (ID: ${engineId})`);

  if (!engine || !engine.HEAPF32 || !engineId || engineId <= 0) {
    console.error("WASM engine not initialized or HEAPF32 not available");
    document.getElementById("status").textContent = "Error: Engine not ready";
    return;
  }

  log(`Attempting to load master call: ${callType}`);
  if (!masterData) {
    console.error("Invalid master call type selected:", callType);
    document.getElementById("status").textContent =
      "Error: Invalid master call";
    return;
  }

  try {
    const dataSize = masterData.data.length * 4; // 4 bytes per float
    const dataPtr = engine._malloc(dataSize);
    engine.HEAPF32.set(masterData.data, dataPtr / 4);

    const result = engine._loadMasterCall(
      engineId,
      dataPtr,
      masterData.frames,
      masterData.coeffs
    );

    engine._free(dataPtr);

    if (result) {
      document.getElementById(
        "status"
      ).textContent = `Loaded master call: ${callType}`;
    } else {
      console.error("Engine returned failure from _loadMasterCall");
      document.getElementById("status").textContent =
        "Error loading master call";
    }
  } catch (err) {
    console.error("Exception during loadMasterCall:", err);
    document.getElementById("status").textContent =
      "Exception: Failed to load call";
  }
}

async function startRecording() {
  try {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });

    const sessionId = engine._startSession(engineId, 44100, 1024, 512);

    mediaRecorder = new MediaRecorder(stream);
    audioChunks = [];

    mediaRecorder.ondataavailable = (event) => {
      audioChunks.push(event.data);
    };

    mediaRecorder.onstop = async () => {
      const audioBlob = new Blob(audioChunks, { type: "audio/wav" });
      await processRecording(audioBlob);
    };

    const source = audioContext.createMediaStreamSource(stream);
    const processor = audioContext.createScriptProcessor(1024, 1, 1);

    processor.onaudioprocess = (e) => {
      if (!isRecording) return;

      const inputData = e.inputBuffer.getChannelData(0);

      let maxLevel = 0;
      for (let i = 0; i < inputData.length; i++) {
        maxLevel = Math.max(maxLevel, Math.abs(inputData[i]));
      }
      updateLevelMeter(maxLevel);

      const dataPtr = engine._malloc(inputData.length * 4);
      engine.HEAPF32.set(inputData, dataPtr / 4);
      engine._processAudioChunk(engineId, dataPtr, inputData.length);
      engine._free(dataPtr);

      updateWaveform(inputData);
    };

    source.connect(processor);
    processor.connect(audioContext.destination);

    mediaRecorder.start();
    isRecording = true;

    document.getElementById("status").textContent = "Recording...";
    document.getElementById("startBtn").disabled = true;
    document.getElementById("stopBtn").disabled = false;
  } catch (err) {
    console.error("Error accessing microphone:", err);
    document.getElementById("status").textContent =
      "Error: Could not access microphone";
  }
}

function stopRecording() {
  if (mediaRecorder && isRecording) {
    mediaRecorder.stop();
    isRecording = false;

    document.getElementById("status").textContent = "Processing...";
    document.getElementById("startBtn").disabled = false;
    document.getElementById("stopBtn").disabled = true;

    const score = engine._getSimilarityScore(engineId);
    displayScore(score);

    engine._endSession(engineId);
  }
}

async function processRecording(audioBlob) {
  const arrayBuffer = await audioBlob.arrayBuffer();
  recordedBuffer = await audioContext.decodeAudioData(arrayBuffer);

  document.getElementById("playbackBtn").disabled = false;
  document.getElementById("status").textContent =
    "Recording complete. Click play to hear your attempt.";
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

  waveformCanvas.width = waveformCanvas.offsetWidth;
  waveformCanvas.height = waveformCanvas.offsetHeight;

  waveformCtx.fillStyle = "#f0f0f0";
  waveformCtx.fillRect(0, 0, waveformCanvas.width, waveformCanvas.height);
}

function updateWaveform(audioData) {
  waveformData.push(...audioData);

  const maxSamples = 44100 * 3;
  if (waveformData.length > maxSamples) {
    waveformData = waveformData.slice(-maxSamples);
  }

  waveformCtx.fillStyle = "#f0f0f0";
  waveformCtx.fillRect(0, 0, waveformCanvas.width, waveformCanvas.height);

  waveformCtx.strokeStyle = "#4CAF50";
  waveformCtx.lineWidth = 2;
  waveformCtx.beginPath();

  const step = Math.ceil(waveformData.length / waveformCanvas.width);
  const amplitude = waveformCanvas.height / 2;

  for (let i = 0; i < waveformCanvas.width; i++) {
    const dataIndex = i * step;
    const value = waveformData[dataIndex] || 0;
    const y = amplitude + value * amplitude;

    if (i === 0) {
      waveformCtx.moveTo(i, y);
    } else {
      waveformCtx.lineTo(i, y);
    }
  }

  waveformCtx.stroke();
}

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
  waveformData = [];
  setupWaveformDisplay(); // Clear and redraw empty waveform
}

document.getElementById("resetBtn").addEventListener("click", resetUI);

// Initial UI setup
resetUI();
