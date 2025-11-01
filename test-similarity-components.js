#!/usr/bin/env node
// [20251102-DEBUG-001] Test to analyze similarity calculation components
// This script compares the same call to itself to verify algorithm behavior

const gameCallsEngine = require('./backend/build/Release/gamecalls_engine.node');

async function testSimilarityComponents() {
    console.log('=== Similarity Component Analysis ===\n');
    
    const callId = 'call_turkey_putt_putts';
    console.log(`Testing call: ${callId} (comparing to itself - should be ~100%)\n`);
    
    try {
        // Create session
        const createResult = gameCallsEngine.createSession({ sampleRate: 44100 });
        if (createResult.error) {
            throw new Error(`createSession failed: ${createResult.error}`);
        }
        const { sessionId } = createResult;
        console.log(`✓ Session created: ${sessionId}`);
        
        // Start session
        const startResult = gameCallsEngine.startSession(sessionId);
        if (startResult.error) {
            throw new Error(`startSession failed: ${startResult.error}`);
        }
        console.log(`✓ Session started`);
        
        // Load master call
        const loadResult = gameCallsEngine.loadMasterCall(sessionId, callId);
        if (loadResult.error) {
            throw new Error(`loadMasterCall failed: ${loadResult.error}`);
        }
        console.log(`✓ Master call loaded: ${callId}`);
        
        // Get master call feature count
        const masterFeaturesResult = gameCallsEngine.getMasterFeatureCount(sessionId);
        console.log(`✓ Master call features: ${masterFeaturesResult.count} frames\n`);
        
        // Process the SAME audio multiple times to simulate comparing to itself
        console.log('Processing audio chunks...');
        for (let i = 0; i < 5; i++) {
            const audioChunk = new Float32Array(8192); // 8192 samples per chunk
            
            // Fill with silence for simplicity (real test would use actual audio)
            for (let j = 0; j < audioChunk.length; j++) {
                audioChunk[j] = 0.0;
            }
            
            const processResult = gameCallsEngine.processAudioChunk(sessionId, audioChunk);
            if (processResult.error) {
                throw new Error(`processAudioChunk ${i+1} failed: ${processResult.error}`);
            }
            
            // Get intermediate similarity after each chunk
            const scoreResult = gameCallsEngine.getSimilarityScore(sessionId);
            if (!scoreResult.error) {
                console.log(`  Chunk ${i+1}: similarity = ${(scoreResult.score * 100).toFixed(2)}%`);
            }
        }
        
        console.log('\n✓ Audio processing complete');
        
        // Get session feature count
        const sessionFeaturesResult = gameCallsEngine.getSessionFeatureCount(sessionId);
        console.log(`✓ Session features: ${sessionFeaturesResult.count} frames`);
        
        // Finalize and get detailed analysis
        console.log('\nFinalizing session...');
        const finalizeResult = gameCallsEngine.finalizeSession(sessionId);
        if (finalizeResult.error) {
            throw new Error(`finalizeSession failed: ${finalizeResult.error}`);
        }
        
        const { analysis } = finalizeResult;
        console.log('\n=== Finalized Analysis ===');
        console.log(`Similarity at finalize: ${(analysis.finalize.similarityAtFinalize * 100).toFixed(2)}%`);
        console.log(`Segment: ${analysis.finalize.segmentStartMs}ms - ${analysis.finalize.segmentDurationMs}ms`);
        console.log(`Normalization scalar: ${analysis.finalize.normalizationScalar.toFixed(3)}`);
        console.log(`Loudness deviation: ${(analysis.finalize.loudnessDeviation * 100).toFixed(2)}%`);
        console.log(`Valid: ${analysis.valid}, Finalized: ${analysis.finalized}`);
        
        // Log pitch/harmonic/cadence details
        console.log('\n=== Component Analysis ===');
        console.log(`Pitch: ${analysis.pitch.pitchHz.toFixed(2)} Hz (confidence: ${(analysis.pitch.confidence * 100).toFixed(1)}%, grade: ${analysis.pitch.grade})`);
        console.log(`Harmonic: ${analysis.harmonic.fundamental.toFixed(2)} Hz (confidence: ${(analysis.harmonic.confidence * 100).toFixed(1)}%, grade: ${analysis.harmonic.grade})`);
        console.log(`Cadence: ${analysis.cadence.tempoBPM.toFixed(2)} BPM (confidence: ${(analysis.cadence.confidence * 100).toFixed(1)}%, grade: ${analysis.cadence.grade})`);
        
        // Cleanup
        const destroyResult = gameCallsEngine.destroySession(sessionId);
        console.log(`\n✓ Session destroyed`);
        
        // Analysis
        console.log('\n=== Issue Analysis ===');
        const finalSim = analysis.finalize.similarityAtFinalize;
        
        if (finalSim > 0.95) {
            console.log('✅ PASS: Self-similarity is high (>95%) - algorithm working correctly');
        } else if (finalSim > 0.80) {
            console.log('⚠️  WARNING: Self-similarity moderate (80-95%) - algorithm may need tuning');
        } else {
            console.log('❌ FAIL: Self-similarity too low (<80%) - algorithm has serious issues');
            console.log('\nPossible causes:');
            console.log('1. MFCC feature extraction producing different results each time');
            console.log('2. DTW normalization too aggressive (making everything similar)');
            console.log('3. Blending weights favoring wrong components');
            console.log('4. Distance metric not appropriate for audio features');
        }
        
    } catch (error) {
        console.error('❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

// Run the test
console.log('Starting similarity component analysis...\n');
testSimilarityComponents().then(() => {
    console.log('\n✓ Analysis complete');
    process.exit(0);
}).catch(err => {
    console.error('\n❌ Analysis failed:', err);
    process.exit(1);
});
