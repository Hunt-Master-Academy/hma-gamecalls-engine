#!/usr/bin/env node
// [20251028-BINDINGS-046] Test script for GameCalls Engine Node-API bindings
// Validates core functionality: session creation, audio processing, finalization

const gameCallsEngine = require('../lib/index');
const fs = require('fs');
const path = require('path');

// ANSI color codes
const colors = {
    reset: '\x1b[0m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m'
};

function log(message, color = 'reset') {
    console.log(`${colors[color]}${message}${colors.reset}`);
}

async function runTests() {
    log('\n=== GameCalls Engine Node-API Bindings Test ===\n', 'blue');
    
    let sessionId = null;
    let passed = 0;
    let failed = 0;
    
    try {
        // Test 1: Engine initialization
        log('Test 1: Engine Initialization', 'yellow');
        await gameCallsEngine.initialize();
        log('✓ Engine initialized successfully\n', 'green');
        passed++;
        
        // Test 2: Get engine info
        log('Test 2: Engine Info', 'yellow');
        const info = await gameCallsEngine.getEngineInfo();
        log(`✓ Version: ${info.version}`, 'green');
        log(`✓ Build Type: ${info.buildType}`, 'green');
        log(`✓ C++ Standard: ${info.cppStandard}\n`, 'green');
        passed++;
        
        // Test 3: Create session
        log('Test 3: Session Creation', 'yellow');
        const masterCallPath = path.join(__dirname, '../../../test_data/master_elk_bugle.wav');
        
        if (!fs.existsSync(masterCallPath)) {
            log(`⚠ Master call not found: ${masterCallPath}`, 'yellow');
            log('⚠ Skipping session-dependent tests\n', 'yellow');
            return;
        }
        
        sessionId = await gameCallsEngine.createSession(masterCallPath, {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        log(`✓ Session created: ID ${sessionId}\n`, 'green');
        passed++;
        
        // Test 4: Process audio buffer
        log('Test 4: Audio Processing', 'yellow');
        const bufferSize = 4096;
        const audioBuffer = new Float32Array(bufferSize);
        
        // Generate test sine wave at 440 Hz
        for (let i = 0; i < bufferSize; i++) {
            audioBuffer[i] = Math.sin(2 * Math.PI * 440 * i / 44100) * 0.5;
        }
        
        const results = await gameCallsEngine.processAudio(sessionId, audioBuffer);
        log(`✓ Similarity Score: ${results.similarityScore.toFixed(3)}`, 'green');
        log(`✓ Confidence: ${results.confidence.toFixed(3)}`, 'green');
        log(`✓ Readiness: ${results.readiness}`, 'green');
        log(`✓ Pitch: ${results.pitch.pitch.toFixed(2)} Hz`, 'green');
        log(`✓ RMS Level: ${results.levels.rms.toFixed(4)}\n`, 'green');
        passed++;
        
        // Test 5: Get similarity score
        log('Test 5: Similarity Score Query', 'yellow');
        const score = await gameCallsEngine.getSimilarityScore(sessionId);
        log(`✓ Score: ${score.score.toFixed(3)}`, 'green');
        log(`✓ Confidence: ${score.confidence.toFixed(3)}\n`, 'green');
        passed++;
        
        // Test 6: Finalize session
        log('Test 6: Session Finalization', 'yellow');
        const finalAnalysis = await gameCallsEngine.finalizeSession(sessionId);
        log(`✓ Overall Score: ${finalAnalysis.overallScore.toFixed(3)}`, 'green');
        log(`✓ Segment: ${finalAnalysis.segment.startMs}-${finalAnalysis.segment.endMs}ms`, 'green');
        log(`✓ Duration: ${finalAnalysis.segment.durationMs}ms`, 'green');
        log(`✓ Processing Time: ${finalAnalysis.processingTimeMs.toFixed(2)}ms\n`, 'green');
        passed++;
        
        // Test 7: Destroy session
        log('Test 7: Session Cleanup', 'yellow');
        await gameCallsEngine.destroySession(sessionId);
        log('✓ Session destroyed successfully\n', 'green');
        sessionId = null;
        passed++;
        
    } catch (error) {
        log(`✗ Test failed: ${error.message}`, 'red');
        log(error.stack, 'red');
        failed++;
    } finally {
        // Cleanup
        if (sessionId !== null) {
            try {
                await gameCallsEngine.destroySession(sessionId);
            } catch (e) {
                // Ignore cleanup errors
            }
        }
    }
    
    // Summary
    log('\n=== Test Summary ===', 'blue');
    log(`Passed: ${passed}`, passed > 0 ? 'green' : 'reset');
    log(`Failed: ${failed}`, failed > 0 ? 'red' : 'reset');
    log(`Total: ${passed + failed}\n`, 'blue');
    
    process.exit(failed > 0 ? 1 : 0);
}

// Run tests
runTests().catch(error => {
    log(`\nFatal error: ${error.message}`, 'red');
    console.error(error);
    process.exit(1);
});
