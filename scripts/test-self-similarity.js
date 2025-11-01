#!/usr/bin/env node
/**
 * [20251102-TEST-004] Self-Similarity Test (Perfect Match Validation)
 * 
 * Critical Test: Master call compared against itself should yield ~100% similarity
 * 
 * This validates:
 * 1. MFCC feature extraction is deterministic
 * 2. Similarity algorithm properly recognizes identical calls
 * 3. No over-penalization in scoring
 * 4. Algorithm fixes maintain high self-similarity
 * 
 * Expected: >95% similarity (accounting for floating-point precision)
 * Failure: <90% indicates algorithm is over-penalizing or MFCC non-deterministic
 */

require('dotenv').config({ path: '../backend/.env' });

// Auto-detect environment
if (!process.env.REDIS_HOST) {
    process.env.REDIS_HOST = process.env.NODE_ENV === 'production' ? 'redis' : 'localhost';
}
if (!process.env.REDIS_PORT) {
    process.env.REDIS_PORT = '6379';
}

const path = require('path');
const fs = require('fs').promises;
const servicesPath = process.env.NODE_ENV === 'production'
    ? path.join(__dirname, '../src/services')
    : path.join(__dirname, '../backend/src/services');

const SessionsService = require(path.join(servicesPath, 'sessionsService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));
const MinIOService = require(path.join(servicesPath, 'minioService'));

const TEST_TIMEOUT = 180000; // 3 minutes
const timeoutId = setTimeout(() => {
    console.error('\n❌ Test TIMEOUT after 3 minutes');
    process.exit(1);
}, TEST_TIMEOUT);

/**
 * Load audio file and convert to Float32Array chunks
 */
async function loadAudioChunks(audioFilePath, chunkSize = 32768) {
    const fileHandle = await fs.open(audioFilePath, 'r');
    const buffer = Buffer.alloc(chunkSize);
    const chunks = [];
    
    let bytesRead;
    let offset = 44; // Skip WAV header
    
    while ((bytesRead = await fileHandle.read(buffer, 0, chunkSize, offset)) && bytesRead.bytesRead > 0) {
        offset += bytesRead.bytesRead;
        
        // Convert Int16 PCM to Float32 normalized [-1.0, 1.0]
        const float32Chunk = new Float32Array(bytesRead.bytesRead / 2);
        for (let i = 0; i < float32Chunk.length; i++) {
            const sample = buffer.readInt16LE(i * 2);
            float32Chunk[i] = sample / 32768.0;
        }
        
        chunks.push(float32Chunk);
    }
    
    await fileHandle.close();
    return chunks;
}

async function testSelfSimilarity() {
    console.log('🎯 Self-Similarity Test (Perfect Match Validation)');
    console.log('='.repeat(70));
    console.log('Testing: Master call vs ITSELF should yield ~100% similarity\n');

    const results = [];

    try {
        // Get sample turkey calls for testing
        const { calls } = await GameCallsService.listCalls({
            species: 'wild_turkey',
            pageSize: 5,
            sortBy: 'duration_seconds',
            sortOrder: 'asc'
        });

        if (!calls || calls.length < 3) {
            throw new Error('Need at least 3 calls for testing');
        }

        console.log(`📊 Testing ${Math.min(3, calls.length)} calls for self-similarity\n`);

        // Test first 3 calls
        for (const call of calls.slice(0, 3)) {
            console.log('─'.repeat(70));
            console.log(`Testing: ${call.id}`);
            console.log(`  Type: ${call.call_type || 'UNKNOWN'}`);
            console.log(`  Duration: ${call.duration_seconds}s`);

            try {
                // STEP 1: Create session with master call
                const session = await SessionsService.createSession(call.id, {
                    enableEnhanced: true,
                    sampleRate: 44100,
                    bufferSize: 32768
                });
                
                const sessionId = session.id;
                console.log(`\n  ✅ Session created: ${sessionId}`);

                // Start recording mode
                await SessionsService.startSession(sessionId);
                console.log(`  ✅ Session started (recording mode)`);

                // STEP 2: Download THE SAME audio file (simulating user playing back master)
                const audioPath = call.audio_file_path || call.audioFilePath || call.audio_path;
                if (!audioPath) {
                    throw new Error(`No audio path found for ${call.id}`);
                }
                
                const audioKey = audioPath.replace('master-calls/', '');
                const stream = await MinIOService.getObjectStream('gamecalls-master-calls', audioKey);
                
                const tempFile = `/tmp/self-test-${Date.now()}.wav`;
                const writeStream = require('fs').createWriteStream(tempFile);
                
                await new Promise((resolve, reject) => {
                    stream.pipe(writeStream);
                    stream.on('error', reject);
                    writeStream.on('finish', resolve);
                    writeStream.on('error', reject);
                });

                console.log(`  ✅ Downloaded audio: ${tempFile}`);

                // STEP 3: Stream the SAME audio back to engine
                const audioChunks = await loadAudioChunks(tempFile, 32768);
                console.log(`  ✅ Prepared ${audioChunks.length} chunks\n`);

                const chunkResults = [];
                console.log(`  📊 Processing chunks (same audio vs master):`);

                for (let i = 0; i < audioChunks.length; i++) {
                    const chunk = audioChunks[i];
                    
                    try {
                        const result = await SessionsService.processAudioData(sessionId, chunk);
                        const similarity = result.similarity?.overall || 0;
                        
                        chunkResults.push({
                            chunkIndex: i,
                            similarity,
                            confidence: result.similarity?.confidence || 0,
                            readiness: result.readiness || 'not_ready'
                        });

                        const simPct = (similarity * 100).toFixed(1);
                        const bar = '█'.repeat(Math.floor(similarity * 30));
                        console.log(`    Chunk ${(i + 1).toString().padStart(2)}/${audioChunks.length}: ${simPct.padStart(5)}% ${bar}`);

                    } catch (error) {
                        console.log(`    ❌ Chunk ${i + 1} failed: ${error.message}`);
                        break;
                    }
                }

                // STEP 4: Analyze self-similarity
                const maxSimilarity = Math.max(...chunkResults.map(r => r.similarity));
                const avgSimilarity = chunkResults.reduce((sum, r) => sum + r.similarity, 0) / chunkResults.length;
                const finalSimilarity = chunkResults[chunkResults.length - 1]?.similarity || 0;

                console.log(`\n  📊 Self-Similarity Analysis:`);
                console.log(`    Peak Similarity: ${(maxSimilarity * 100).toFixed(1)}%`);
                console.log(`    Average Similarity: ${(avgSimilarity * 100).toFixed(1)}%`);
                console.log(`    Final Similarity: ${(finalSimilarity * 100).toFixed(1)}%`);

                // CRITICAL VALIDATION
                const EXPECTED_MIN = 0.90; // Should be at least 90% for identical audio
                const IDEAL_MIN = 0.95;    // Ideally >95%

                console.log(`\n  ✅ Validation Results:`);
                
                if (maxSimilarity >= IDEAL_MIN) {
                    console.log(`    🎉 EXCELLENT: Peak ${(maxSimilarity * 100).toFixed(1)}% >= ${IDEAL_MIN * 100}% (identical audio recognized)`);
                } else if (maxSimilarity >= EXPECTED_MIN) {
                    console.log(`    ✅ PASS: Peak ${(maxSimilarity * 100).toFixed(1)}% >= ${EXPECTED_MIN * 100}% (acceptable self-similarity)`);
                } else {
                    console.log(`    ❌ FAIL: Peak ${(maxSimilarity * 100).toFixed(1)}% < ${EXPECTED_MIN * 100}% (algorithm over-penalizing)`);
                }

                results.push({
                    callId: call.id,
                    callType: call.call_type,
                    duration: call.duration_seconds,
                    maxSimilarity,
                    avgSimilarity,
                    finalSimilarity,
                    passed: maxSimilarity >= EXPECTED_MIN
                });

                // Cleanup
                await fs.unlink(tempFile);
                console.log(`\n  🧹 Cleanup complete`);

            } catch (error) {
                console.log(`\n  ❌ Test failed for ${call.id}: ${error.message}`);
                results.push({
                    callId: call.id,
                    callType: call.call_type,
                    error: error.message,
                    passed: false
                });
            }

            console.log('');
        }

        // FINAL SUMMARY
        console.log('='.repeat(70));
        console.log('📊 Self-Similarity Test Summary\n');

        const passed = results.filter(r => r.passed).length;
        const failed = results.filter(r => !r.passed).length;

        console.log(`  Total Tests: ${results.length}`);
        console.log(`  Passed: ${passed}`);
        console.log(`  Failed: ${failed}\n`);

        console.log('  Detailed Results:');
        results.forEach(r => {
            if (r.error) {
                console.log(`    ❌ ${r.callId} (${r.callType}): ERROR - ${r.error}`);
            } else {
                const status = r.passed ? '✅' : '❌';
                console.log(`    ${status} ${r.callId} (${r.callType}): Peak=${(r.maxSimilarity * 100).toFixed(1)}%, Avg=${(r.avgSimilarity * 100).toFixed(1)}%`);
            }
        });

        console.log('\n  🎯 Critical Insight:');
        if (passed === results.length) {
            console.log('    ✅ Algorithm correctly recognizes identical audio (self-similarity >90%)');
            console.log('    ✅ MFCC features are deterministic');
            console.log('    ✅ No over-penalization in similarity scoring');
        } else {
            console.log('    ❌ Algorithm may be over-penalizing or MFCC non-deterministic');
            console.log('    ⚠️  Expected: Identical audio should yield >90% similarity');
            console.log('    ⚠️  Check: subsequence matcher penalties, DTW normalization, blending weights');
        }

        clearTimeout(timeoutId);
        process.exit(failed === 0 ? 0 : 1);

    } catch (error) {
        console.error('\n❌ Test suite failed:', error.message);
        console.error(error.stack);
        clearTimeout(timeoutId);
        process.exit(1);
    }
}

testSelfSimilarity();
