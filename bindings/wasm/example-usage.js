// File: example-usage.js
// Example of using the WASM engine in JavaScript

async function initializeHuntmaster() {
    // Load the WASM module
    const Module = await import('./huntmaster.js');
    await Module.ready;
    
    // Create engine instance
    const engine = new Module.HuntmasterEngine();
    
    // Initialize with typical web audio settings
    const success = engine.initialize(48000, 512, 13);
    if (!success) {
        throw new Error('Failed to initialize engine');
    }
    
    // Load a master call
    const masterAudio = await fetch('sounds/buck_grunt.wav')
        .then(r => r.arrayBuffer())
        .then(buffer => audioContext.decodeAudioData(buffer));
    
    const audioData = masterAudio.getChannelData(0);
    engine.loadMasterCall('buck_grunt', audioData);
    
    // Process real-time audio
    const processAudio = (inputBuffer) => {
        const result = engine.processAudioArray(inputBuffer);
        
        if (result.success) {
            console.log(`Similarity: ${result.score}, Time: ${result.processingTimeMs}ms`);
        }
    };
    
    // Enable streaming mode for continuous processing
    engine.enableStreaming(true);
    
    // In audio worklet or script processor
    const audioCallback = (audioData) => {
        if (engine.enqueueAudioBuffer(audioData)) {
            const results = engine.dequeueResults();
            results.forEach(chunk => {
                if (chunk.containsVoice) {
                    console.log(`Voice detected at frame ${chunk.frameIndex}`);
                }
            });
        }
    };
    
    // Monitor performance
    setInterval(() => {
        const stats = engine.getPerformanceStats();
        console.log('Performance:', stats);
    }, 5000);
    
    // Handle memory pressure
    if ('memory' in performance) {
        performance.memory.addEventListener('pressure', () => {
            engine.onMemoryPressure();
        });
    }
    
    return engine;
}

// SharedArrayBuffer example for audio workers
async function setupAudioWorker() {
    // Create shared buffer (requires COOP/COEP headers)
    const sharedBuffer = new SharedArrayBuffer(4096 * 4);
    
    const worker = new Worker('audio-worker.js');
    worker.postMessage({ 
        cmd: 'init', 
        buffer: sharedBuffer 
    });
    
    // In the worker
    const Module = await import('./huntmaster.js');
    const audioWorker = new Module.HuntmasterAudioWorker();
    
    audioWorker.initialize(
        Module.HEAPU8.buffer.byteOffset + sharedBuffer,
        sharedBuffer.byteLength
    );
    
    // Process audio in worker
    audioWorker.processSharedBuffer();
}