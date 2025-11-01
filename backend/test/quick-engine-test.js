#!/usr/bin/env node
/**
 * [20251229-TEST-004] Quick test of C++ engine with existing preprocessed call
 */
require('dotenv').config();

const path = require('path');
const bindingsPath = path.resolve(__dirname, '../../bindings/node-api/lib/index');
console.log('Loading bindings from:', bindingsPath);
const gameCallsEngine = require(bindingsPath);

async function test() {
    console.log('🧪 Testing C++ engine with preprocessed call: turkey_purr_001');
    
    try {
        console.log('DEBUG: Loaded engine module');
        console.log('DEBUG: Available methods:', Object.keys(gameCallsEngine));
        
        // Initialize engine
        console.log('DEBUG: About to initialize...');
        await gameCallsEngine.initialize();
        console.log('✓ Engine initialized');
        
        console.log('DEBUG: About to create session...');
        // Create session with existing preprocessed call ID
        const sessionId = await gameCallsEngine.createSession('turkey_purr_001', {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        console.log(`✓ Session created: ${sessionId}`);
        
        // Generate test audio (0.5s sine wave)
        const sampleRate = 44100;
        const duration = 0.5;
        const numSamples = Math.floor(sampleRate * duration);
        const audioBuffer = new Float32Array(numSamples);
        for (let i = 0; i < numSamples; i++) {
            audioBuffer[i] = Math.sin(2 * Math.PI * 440 * i / sampleRate) * 0.3;
        }
        
        // Process audio
        const result = await gameCallsEngine.processAudio(sessionId, audioBuffer);
        console.log('✓ Audio processed');
        console.log('  Similarity:', result);
        
        // Get similarity score
        const score = await gameCallsEngine.getSimilarityScore(sessionId);
        console.log('✓ Similarity score:', score);
        
        // Finalize
        const final = await gameCallsEngine.finalizeSession(sessionId);
        console.log('✓ Finalized:', final);
        
        // Cleanup
        await gameCallsEngine.destroySession(sessionId);
        console.log('✓ Session destroyed');
        
        console.log('\n🎉 SUCCESS - Real C++ engine is working!');
        process.exit(0);
        
    } catch (error) {
        console.error('❌ Error:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

test();
