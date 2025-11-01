#!/usr/bin/env node
/**
 * [20251229-TEST-001] Standalone smoke test for GameCalls Engine real C++ integration
 * Tests complete processing pipeline: database → MinIO → C++ engine → results
 */

// [20251229-TEST-002] Load environment variables
require('dotenv').config();

const SessionsService = require('../src/services/sessionsService');
const GameCallsService = require('../src/services/gameCallsService');
const fs = require('fs');
const path = require('path');

// ANSI color codes for output
const colors = {
    reset: '\x1b[0m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    cyan: '\x1b[36m'
};

function log(message, color = 'reset') {
    console.log(`${colors[color]}${message}${colors.reset}`);
}

function logSection(title) {
    console.log('\n' + '='.repeat(60));
    log(title, 'cyan');
    console.log('='.repeat(60));
}

async function runTest() {
    logSection('🚀 GameCalls Engine Real C++ Integration Test');
    
    let testsFailed = 0;
    let testsPassed = 0;
    let sessionId = null;
    
    try {
        // Test 1: Load master call from database
        logSection('📋 Test 1: Load Master Call from Database');
        const response = await GameCallsService.listCalls({ page: 1, pageSize: 10 });
        
        if (!response || !response.calls || response.calls.length === 0) {
            throw new Error('No master calls found in database');
        }
        
        log(`✓ Found ${response.calls.length} master calls (total: ${response.total})`, 'green');
        
        // Use first master call
        const masterCall = response.calls[0];
        log(`✓ Selected master call: ${masterCall.name || masterCall.species}`, 'green');
        log(`  ID: ${masterCall.id}`, 'blue');
        log(`  Species: ${masterCall.species}`, 'blue');
        log(`  Call type: ${masterCall.call_type || 'N/A'}`, 'blue');
        log(`  Duration: ${masterCall.duration_seconds}s`, 'blue');
        testsPassed++;
        
        // Test 2: Create session with real C++ engine
        logSection('🔧 Test 2: Create Session with Real Engine');
        sessionId = await SessionsService.createSession(masterCall.id, {
            userId: 'test-user-001',
            sessionType: 'practice'
        });
        
        if (!sessionId) {
            throw new Error('Session creation returned null');
        }
        
        log(`✓ Session created: ${sessionId}`, 'green');
        testsPassed++;
        
        // Test 3: Start processing
        logSection('▶️  Test 3: Start Processing');
        await SessionsService.startProcessing(sessionId);
        log('✓ Processing started', 'green');
        testsPassed++;
        
        // Test 4: Process audio data (simulate real audio input)
        logSection('🎵 Test 4: Process Audio Data');
        
        // Generate test audio: 0.5 seconds of synthetic audio
        const sampleRate = 44100;
        const duration = 0.5; // seconds
        const numSamples = Math.floor(sampleRate * duration);
        const audioBuffer = new Float32Array(numSamples);
        
        // Generate sine wave at 440 Hz (A4 note - common in bird calls)
        const frequency = 440;
        for (let i = 0; i < numSamples; i++) {
            audioBuffer[i] = Math.sin(2 * Math.PI * frequency * i / sampleRate) * 0.3;
        }
        
        log(`✓ Generated ${numSamples} samples of test audio (${duration}s at ${sampleRate} Hz)`, 'blue');
        
        const result = await SessionsService.processAudioData(sessionId, audioBuffer);
        
        if (!result || !result.similarity) {
            throw new Error('processAudioData returned invalid result structure');
        }
        
        log('✓ Audio processed successfully', 'green');
        log('  Similarity scores:', 'blue');
        log(`    Overall: ${result.similarity.overall?.toFixed(4) || 'N/A'}`, 'blue');
        log(`    MFCC: ${result.similarity.mfcc?.toFixed(4) || 'N/A'}`, 'blue');
        log(`    Volume: ${result.similarity.volume?.toFixed(4) || 'N/A'}`, 'blue');
        log(`    Timing: ${result.similarity.timing?.toFixed(4) || 'N/A'}`, 'blue');
        log(`    Pitch: ${result.similarity.pitch?.toFixed(4) || 'N/A'}`, 'blue');
        log(`  Confidence: ${result.similarity.confidence?.toFixed(4) || 'N/A'}`, 'blue');
        log(`  Is Reliable: ${result.similarity.isReliable}`, 'blue');
        log(`  Is Match: ${result.similarity.isMatch}`, 'blue');
        log(`  Samples Analyzed: ${result.similarity.samplesAnalyzed || 'N/A'}`, 'blue');
        
        // Critical check: Verify this is NOT mock data
        if (result.similarity.overall === 0.85 && result.similarity.confidence === 0.92) {
            testsFailed++;
            log('✗ CRITICAL: Mock engine detected! Expected real C++ engine output', 'red');
            throw new Error('Mock engine is still being used instead of real C++ engine');
        }
        
        log('✓ Real C++ engine confirmed (not mock data)', 'green');
        testsPassed++;
        
        // Test 5: Finalize session
        logSection('🏁 Test 5: Finalize Session');
        const finalizeResult = await SessionsService.finalizeSession(sessionId);
        
        if (!finalizeResult) {
            throw new Error('finalizeSession returned null');
        }
        
        log('✓ Session finalized', 'green');
        log('  Enhanced analysis:', 'blue');
        log(`    Pitch: ${JSON.stringify(finalizeResult.pitch || {})}`, 'blue');
        log(`    Harmonic: ${JSON.stringify(finalizeResult.harmonic || {})}`, 'blue');
        log(`    Cadence: ${JSON.stringify(finalizeResult.cadence || {})}`, 'blue');
        testsPassed++;
        
        // Test 6: Stop session and get final analysis
        logSection('⏹️  Test 6: Stop Session');
        const finalAnalysis = await SessionsService.stopSession(sessionId);
        
        if (!finalAnalysis) {
            throw new Error('stopSession returned null');
        }
        
        log('✓ Session stopped', 'green');
        log('  Final analysis:', 'blue');
        log(`    Overall Score: ${finalAnalysis.overallScore?.toFixed(4) || 'N/A'}`, 'blue');
        log(`    Segments: ${finalAnalysis.segments?.length || 0}`, 'blue');
        log(`    Enhanced pitch: ${JSON.stringify(finalAnalysis.enhanced?.pitch || {})}`, 'blue');
        log(`    Enhanced harmonic: ${JSON.stringify(finalAnalysis.enhanced?.harmonic || {})}`, 'blue');
        log(`    Enhanced cadence: ${JSON.stringify(finalAnalysis.enhanced?.cadence || {})}`, 'blue');
        testsPassed++;
        
        sessionId = null; // Session is now stopped
        
    } catch (error) {
        log(`✗ Test failed: ${error.message}`, 'red');
        if (error.stack) {
            log(error.stack, 'red');
        }
        testsFailed++;
        
        // Cleanup if session still active
        if (sessionId) {
            try {
                await SessionsService.stopSession(sessionId);
                log('✓ Cleanup: Session stopped', 'yellow');
            } catch (cleanupError) {
                log(`✗ Cleanup failed: ${cleanupError.message}`, 'red');
            }
        }
    }
    
    // Final summary
    logSection('📊 Test Summary');
    log(`Tests Passed: ${testsPassed}`, testsPassed > 0 ? 'green' : 'reset');
    log(`Tests Failed: ${testsFailed}`, testsFailed > 0 ? 'red' : 'reset');
    
    if (testsFailed === 0) {
        log('\n🎉 ALL TESTS PASSED - Real C++ engine integration successful!', 'green');
        process.exit(0);
    } else {
        log('\n❌ TESTS FAILED - See errors above', 'red');
        process.exit(1);
    }
}

// Run the test
runTest().catch(error => {
    log(`\n❌ Fatal error: ${error.message}`, 'red');
    if (error.stack) {
        log(error.stack, 'red');
    }
    process.exit(1);
});
