#!/usr/bin/env node
/**
 * [20251101-TEST-005] Cross-Similarity Test
 * Tests that DIFFERENT calls produce LOW similarity scores
 * Expected: Different call types should be <30%, similar types 40-70%
 */

const fs = require('fs').promises;
const path = require('path');
const servicesPath = process.env.NODE_ENV === 'production'
    ? path.join(__dirname, '../src/services')
    : path.join(__dirname, '../backend/src/services');

const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));
const SessionsService = require(path.join(servicesPath, 'sessionsService'));
const minioService = require(path.join(servicesPath, 'minioService'));

async function testCrossSimilarity() {
    console.log('🔀 Cross-Similarity Test (Different Calls Should Score Low)');
    console.log('======================================================================\n');
    
    try {
        // Test pairs: [masterCall, testCall, expectedRange]
        const testPairs = [
            { master: 'call_turkey_cluck_001', test: 'call_turkey_yelp_001', expected: '<50%', reason: 'Same species, different call type' },
            { master: 'call_turkey_cluck_001', test: 'call_turkey_purr_001', expected: '<50%', reason: 'Same species, different call type' },
            { master: 'call_turkey_yelp_001', test: 'call_turkey_purr_001', expected: '<50%', reason: 'Same species, different call type' }
        ];

        const results = [];

        for (const pair of testPairs) {
            console.log(`──────────────────────────────────────────────────────────────────────`);
            console.log(`Testing: ${pair.master} vs ${pair.test}`);
            console.log(`  Expected: ${pair.expected} (${pair.reason})\n`);

            try {
                // STEP 1: Create session with master call
                const session = await SessionsService.createSession(pair.master, {
                    sampleRate: 44100,
                    enableEnhancedAnalysis: false
                });
                console.log(`  ✅ Session created with master: ${pair.master}`);

                // STEP 2: Start session
                await SessionsService.startSession(session.id);
                console.log(`  ✅ Session started`);

                // STEP 3: Get test audio file
                const testCall = await GameCallsService.getCall(pair.test);
                const testAudioPath = await minioService.downloadToTemp(testCall.audioFile);
                console.log(`  ✅ Downloaded test audio: ${pair.test}`);

                // STEP 4: Load test audio
                const audioChunks = await loadAudioFile(testAudioPath);
                console.log(`  ✅ Loaded ${audioChunks.length} chunks\n`);

                // STEP 5: Process test audio against master
                const chunkResults = [];
                for (let i = 0; i < audioChunks.length; i++) {
                    const result = await SessionsService.processAudioData(session.id, audioChunks[i]);
                    const similarity = result.similarity?.overall || 0;
                    chunkResults.push(similarity);
                    
                    const simPct = (similarity * 100).toFixed(1);
                    const bar = '█'.repeat(Math.floor(similarity * 30));
                    console.log(`    Chunk ${String(i + 1).padStart(2)}/${audioChunks.length}: ${simPct.padStart(5)}% ${bar}`);
                }

                // STEP 6: Analyze results
                const maxSimilarity = Math.max(...chunkResults);
                const avgSimilarity = chunkResults.reduce((sum, s) => sum + s, 0) / chunkResults.length;

                console.log(`\n  📊 Cross-Similarity Analysis:`);
                console.log(`    Peak Similarity: ${(maxSimilarity * 100).toFixed(1)}%`);
                console.log(`    Average Similarity: ${(avgSimilarity * 100).toFixed(1)}%`);

                // Validate expectations
                let status = '✅ PASS';
                let reason = 'Expected low similarity for different calls';
                
                if (pair.expected === '<50%' && maxSimilarity < 0.5) {
                    status = '✅ PASS';
                } else if (maxSimilarity >= 0.5) {
                    status = '⚠️  WARN';
                    reason = 'Higher than expected - may indicate over-generosity';
                }

                console.log(`\n  ${status}: Peak ${(maxSimilarity * 100).toFixed(1)}% ${reason}\n`);

                results.push({
                    master: pair.master,
                    test: pair.test,
                    peak: maxSimilarity,
                    avg: avgSimilarity,
                    passed: maxSimilarity < 0.5
                });

                // Cleanup
                await SessionsService.destroySession(session.id);
                await fs.unlink(testAudioPath);
                console.log(`  🧹 Cleanup complete\n`);

            } catch (error) {
                console.error(`  ❌ Test failed: ${error.message}\n`);
                results.push({
                    master: pair.master,
                    test: pair.test,
                    peak: -1,
                    avg: -1,
                    passed: false,
                    error: error.message
                });
            }
        }

        // Summary
        console.log('======================================================================');
        console.log('📊 Cross-Similarity Test Summary\n');
        console.log(`  Total Pairs Tested: ${results.length}`);
        console.log(`  Passed: ${results.filter(r => r.passed).length}`);
        console.log(`  Warnings: ${results.filter(r => !r.passed && r.peak >= 0).length}\n`);

        console.log('  Detailed Results:');
        for (const result of results) {
            if (result.error) {
                console.log(`    ❌ ${result.master} vs ${result.test}: ERROR - ${result.error}`);
            } else {
                const status = result.passed ? '✅' : '⚠️';
                console.log(`    ${status} ${result.master} vs ${result.test}: Peak=${(result.peak * 100).toFixed(1)}%, Avg=${(result.avg * 100).toFixed(1)}%`);
            }
        }

        console.log('\n  🎯 Interpretation:');
        const allPassed = results.every(r => r.passed);
        if (allPassed) {
            console.log('    ✅ Algorithm correctly discriminates between different call types');
        } else {
            console.log('    ⚠️  Some pairs show higher similarity than expected');
            console.log('    ⚠️  May need to tune discriminative features or weights');
        }

        process.exit(allPassed ? 0 : 1);

    } catch (error) {
        console.error('❌ Test suite failed:', error);
        process.exit(1);
    }
}

/**
 * Load audio file and split into chunks for processing
 */
async function loadAudioFile(audioFilePath, maxChunkSize = 5000000) {
    const fileStats = await fs.stat(audioFilePath);
    const fileSize = fileStats.size - 44; // Exclude WAV header
    
    const fileHandle = await fs.open(audioFilePath, 'r');
    const buffer = Buffer.alloc(fileSize);
    
    // Read entire audio data (skip 44-byte WAV header)
    const { bytesRead } = await fileHandle.read(buffer, 0, fileSize, 44);
    await fileHandle.close();
    
    // Convert Int16 PCM to Float32 normalized [-1.0, 1.0]
    const float32Data = new Float32Array(bytesRead / 2);
    for (let i = 0; i < float32Data.length; i++) {
        const sample = buffer.readInt16LE(i * 2);
        float32Data[i] = sample / 32768.0;
    }
    
    // Split into chunks if needed
    if (float32Data.length <= maxChunkSize) {
        return [float32Data];
    } else {
        const chunks = [];
        for (let offset = 0; offset < float32Data.length; offset += maxChunkSize) {
            const end = Math.min(offset + maxChunkSize, float32Data.length);
            chunks.push(float32Data.slice(offset, end));
        }
        return chunks;
    }
}

// Run tests
testCrossSimilarity().catch(console.error);
