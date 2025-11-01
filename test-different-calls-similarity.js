#!/usr/bin/env node
// [20251102-DEBUG-002] Test different call types to diagnose why they show 100% similarity

const gameCallsEngine = require('./bindings/node-api/build/Release/gamecalls_engine.node');

async function compareCall(masterCallId, testCallId) {
    console.log(`\n========== Comparing: ${masterCallId} vs ${testCallId} ==========`);
    
    try {
        // Create session for test call
        const createResult = gameCallsEngine.createSession({ sampleRate: 44100 });
        if (createResult.error) throw new Error(`createSession failed: ${createResult.error}`);
        const { sessionId } = createResult;
        
        // Start session
        const startResult = gameCallsEngine.startSession(sessionId);
        if (startResult.error) throw new Error(`startSession failed: ${startResult.error}`);
        
        // Load master call (the reference we're comparing against)
        const loadMasterResult = gameCallsEngine.loadMasterCall(sessionId, masterCallId);
        if (loadMasterResult.error) throw new Error(`loadMasterCall failed: ${loadMasterResult.error}`);
        
        const masterFeatures = gameCallsEngine.getMasterFeatureCount(sessionId);
        console.log(`Master call "${masterCallId}": ${masterFeatures.count} feature frames`);
        
        // Now load the TEST call audio and process it
        const loadTestResult = gameCallsEngine.loadMasterCall(sessionId, testCallId);
        if (loadTestResult.error) throw new Error(`loadMasterCall for test failed: ${loadTestResult.error}`);
        
        // Get the test call's audio data
        const testAudioResult = gameCallsEngine.getMasterCallAudio(sessionId, testCallId);
        if (testAudioResult.error) {
            // If we can't get audio, process silence chunks instead
            console.log(`⚠️  Could not load audio for ${testCallId}, using silence`);
            for (let i = 0; i < 5; i++) {
                const audioChunk = new Float32Array(8192).fill(0.0);
                gameCallsEngine.processAudioChunk(sessionId, audioChunk);
            }
        } else {
            // Process the test audio in chunks
            const testAudio = testAudioResult.audio;
            console.log(`Test call "${testCallId}": ${testAudio.length} samples`);
            
            const chunkSize = 8192;
            for (let offset = 0; offset < testAudio.length; offset += chunkSize) {
                const chunk = testAudio.slice(offset, offset + chunkSize);
                gameCallsEngine.processAudioChunk(sessionId, chunk);
            }
        }
        
        const sessionFeatures = gameCallsEngine.getSessionFeatureCount(sessionId);
        console.log(`Session features extracted: ${sessionFeatures.count} frames`);
        
        // Finalize to get similarity
        const finalizeResult = gameCallsEngine.finalizeSession(sessionId);
        if (finalizeResult.error) throw new Error(`finalizeSession failed: ${finalizeResult.error}`);
        
        const similarity = finalizeResult.analysis.finalize.similarityAtFinalize;
        console.log(`\n📊 FINAL SIMILARITY: ${(similarity * 100).toFixed(2)}%`);
        
        // Cleanup
        gameCallsEngine.destroySession(sessionId);
        
        return similarity;
        
    } catch (error) {
        console.error(`❌ Comparison failed: ${error.message}`);
        return -1;
    }
}

async function runDiagnostics() {
    console.log('=== Similarity Component Diagnostics ===');
    console.log('Testing if different call types show inappropriate high similarity\n');
    
    // Test 1: Same call to itself (should be ~100%)
    console.log('\n━━━ TEST 1: Self-similarity (should be ~100%) ━━━');
    const selfSim = await compareCall('call_turkey_putt_putts', 'call_turkey_putt_putts');
    
    // Test 2: Completely different calls (should be <30%)
    console.log('\n━━━ TEST 2: Different call types (should be <30%) ━━━');
    const diffSim1 = await compareCall('call_turkey_kee_kee_run', 'call_turkey_gobbling');
    const diffSim2 = await compareCall('call_turkey_kee_kee_run', 'call_turkey_cutting');
    const diffSim3 = await compareCall('call_turkey_gobbling', 'call_turkey_cutting');
    
    // Test 3: Similar calls (should be 40-70%)
    console.log('\n━━━ TEST 3: Similar call types (should be 40-70%) ━━━');
    const simSim1 = await compareCall('call_turkey_plain_yelp', 'call_turkey_excited_yelps');
    const simSim2 = await compareCall('call_turkey_putts', 'call_turkey_putt_putts');
    
    // Analysis
    console.log('\n\n═══════════════════════════════════════════════════════');
    console.log('                  DIAGNOSTIC SUMMARY                  ');
    console.log('═══════════════════════════════════════════════════════\n');
    
    console.log('Self-Similarity Test:');
    console.log(`  Putt vs Putt: ${(selfSim * 100).toFixed(1)}% ${selfSim > 0.85 ? '✅ PASS' : '❌ FAIL'}`);
    
    console.log('\nDifferent Call Types Test:');
    console.log(`  Kee Kee vs Gobbling: ${(diffSim1 * 100).toFixed(1)}% ${diffSim1 < 0.30 ? '✅ PASS' : '❌ FAIL (too high!)'}`);
    console.log(`  Kee Kee vs Cutting: ${(diffSim2 * 100).toFixed(1)}% ${diffSim2 < 0.30 ? '✅ PASS' : '❌ FAIL (too high!)'}`);
    console.log(`  Gobbling vs Cutting: ${(diffSim3 * 100).toFixed(1)}% ${diffSim3 < 0.30 ? '✅ PASS' : '❌ FAIL (too high!)'}`);
    
    console.log('\nSimilar Call Types Test:');
    console.log(`  Plain Yelp vs Excited Yelps: ${(simSim1 * 100).toFixed(1)}% ${simSim1 > 0.40 && simSim1 < 0.80 ? '✅ PASS' : '⚠️  CHECK'}`);
    console.log(`  Putts vs Putt Putts: ${(simSim2 * 100).toFixed(1)}% ${simSim2 > 0.40 && simSim2 < 0.80 ? '✅ PASS' : '⚠️  CHECK'}`);
    
    console.log('\n═══════════════════════════════════════════════════════');
    
    // Count failures
    const failures = [
        selfSim < 0.85,
        diffSim1 >= 0.30,
        diffSim2 >= 0.30,
        diffSim3 >= 0.30
    ].filter(f => f).length;
    
    if (failures === 0) {
        console.log('\n✅ ALL TESTS PASSED - Algorithm is working correctly!');
        console.log('The pairwise matrix issue must be elsewhere (data loading, etc.)');
    } else {
        console.log(`\n❌ ${failures} TEST(S) FAILED - Algorithm needs tuning`);
        console.log('\nLikely issues:');
        if (selfSim < 0.85) {
            console.log('  • Self-similarity too low: Feature extraction non-deterministic');
        }
        if (diffSim1 >= 0.30 || diffSim2 >= 0.30 || diffSim3 >= 0.30) {
            console.log('  • Different calls too similar: Algorithm not discriminating');
            console.log('    - Check DTW normalization (making everything similar?)');
            console.log('    - Check blending weights (subsequence dominance?)');
            console.log('    - Check offset search gamma/scaling');
        }
    }
    
    console.log('\n💡 Check docker logs for component breakdown:');
    console.log('   docker logs hma-gamecalls-engine 2>&1 | grep "Similarity components"');
}

// Run diagnostics
runDiagnostics().then(() => {
    process.exit(0);
}).catch(err => {
    console.error('❌ Diagnostics failed:', err);
    process.exit(1);
});
