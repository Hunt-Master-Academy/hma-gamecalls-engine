#!/usr/bin/env node
/**
 * [20251102-TEST-005] Self-Similarity Test with Longer Calls
 * 
 * Tests self-similarity with longer duration calls to ensure
 * sufficient frames for proper alignment and scoring.
 */

require('dotenv').config({ path: '../backend/.env' });

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

const TEST_TIMEOUT = 180000;
const timeoutId = setTimeout(() => {
    console.error('\n❌ Test TIMEOUT');
    process.exit(1);
}, TEST_TIMEOUT);

async function loadAudioChunks(audioFilePath, chunkSize = 32768) {
    const fileHandle = await fs.open(audioFilePath, 'r');
    const buffer = Buffer.alloc(chunkSize);
    const chunks = [];
    
    let bytesRead;
    let offset = 44;
    
    while ((bytesRead = await fileHandle.read(buffer, 0, chunkSize, offset)) && bytesRead.bytesRead > 0) {
        offset += bytesRead.bytesRead;
        
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

async function testLongerCallSelfSimilarity() {
    console.log('🎯 Self-Similarity Test with Longer Calls');
    console.log('='.repeat(70));
    console.log('Hypothesis: Longer calls provide more frames for better alignment\n');

    try {
        // Get calls sorted by duration (longest first)
        const { calls } = await GameCallsService.listCalls({
            species: 'wild_turkey',
            pageSize: 5,
            sortBy: 'duration_seconds',
            sortOrder: 'desc' // LONGEST first
        });

        if (!calls || calls.length < 2) {
            throw new Error('Need at least 2 calls');
        }

        const testCall = calls[0]; // Longest call
        console.log(`📊 Testing: ${testCall.id}`);
        console.log(`   Type: ${testCall.call_type || 'UNKNOWN'}`);
        console.log(`   Duration: ${testCall.duration_seconds}s (longest available)\n`);

        // Create session
        const session = await SessionsService.createSession(testCall.id, {
            enableEnhanced: true,
            sampleRate: 44100
        });
        
        const sessionId = session.id;
        await SessionsService.startSession(sessionId);
        console.log(`✅ Session created and started: ${sessionId}\n`);

        // Download SAME audio
        const audioPath = testCall.audio_file_path || testCall.audioFilePath || testCall.audio_path;
        const audioKey = audioPath.replace('master-calls/', '');
        const stream = await MinIOService.getObjectStream('gamecalls-master-calls', audioKey);
        
        const tempFile = `/tmp/test-long-${Date.now()}.wav`;
        const writeStream = require('fs').createWriteStream(tempFile);
        
        await new Promise((resolve, reject) => {
            stream.pipe(writeStream);
            stream.on('error', reject);
            writeStream.on('finish', resolve);
            writeStream.on('error', reject);
        });

        const audioChunks = await loadAudioChunks(tempFile, 32768);
        console.log(`✅ Prepared ${audioChunks.length} chunks\n`);
        console.log(`Processing identical audio through engine...`);

        const results = [];
        for (let i = 0; i < audioChunks.length; i++) {
            const chunk = audioChunks[i];
            
            try {
                const result = await SessionsService.processAudioData(sessionId, chunk);
                const similarity = result.similarity?.overall || 0;
                
                results.push({
                    chunk: i + 1,
                    similarity,
                    confidence: result.similarity?.confidence || 0
                });

                if (i % 5 === 0 || i === audioChunks.length - 1) {
                    console.log(`  Chunk ${(i + 1).toString().padStart(2)}/${audioChunks.length}: ${(similarity * 100).toFixed(1)}%`);
                }

            } catch (error) {
                console.log(`  ❌ Chunk ${i + 1} failed: ${error.message}`);
                break;
            }
        }

        // Analysis
        const maxSim = Math.max(...results.map(r => r.similarity));
        const finalSim = results[results.length - 1]?.similarity || 0;
        const avgSim = results.reduce((sum, r) => sum + r.similarity, 0) / results.length;
        
        // Check last 25% of chunks (most data available)
        const lastQuarter = results.slice(Math.floor(results.length * 0.75));
        const avgLastQuarter = lastQuarter.reduce((sum, r) => sum + r.similarity, 0) / lastQuarter.length;

        console.log(`\n${'='.repeat(70)}`);
        console.log('📊 Results\n');
        console.log(`  Peak Similarity: ${(maxSim * 100).toFixed(2)}%`);
        console.log(`  Final Similarity: ${(finalSim * 100).toFixed(2)}%`);
        console.log(`  Average (all): ${(avgSim * 100).toFixed(2)}%`);
        console.log(`  Average (last 25%): ${(avgLastQuarter * 100).toFixed(2)}%`);

        console.log(`\n  📈 Progression (every 10th chunk):`);
        for (let i = 0; i < results.length; i += Math.max(1, Math.floor(results.length / 10))) {
            const r = results[i];
            const simPct = (r.similarity * 100).toFixed(1);
            const bar = '█'.repeat(Math.floor(r.similarity * 40));
            console.log(`    Chunk ${r.chunk.toString().padStart(2)}: ${simPct.padStart(5)}% ${bar}`);
        }

        console.log(`\n  ✅ Validation:`);
        if (avgLastQuarter >= 0.90) {
            console.log(`    🎉 EXCELLENT: Last quarter avg ${(avgLastQuarter * 100).toFixed(1)}% >= 90%`);
            console.log(`    ✅ Self-similarity working correctly with sufficient data`);
        } else if (avgLastQuarter >= 0.80) {
            console.log(`    ✅ GOOD: Last quarter avg ${(avgLastQuarter * 100).toFixed(1)}% >= 80%`);
            console.log(`    ⚠️  Close to target, may need minor tuning`);
        } else {
            console.log(`    ❌ FAIL: Last quarter avg ${(avgLastQuarter * 100).toFixed(1)}% < 80%`);
            console.log(`    ⚠️  Algorithm issue persists even with long audio`);
        }

        await fs.unlink(tempFile);

        clearTimeout(timeoutId);
        process.exit(avgLastQuarter >= 0.80 ? 0 : 1);

    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error(error.stack);
        clearTimeout(timeoutId);
        process.exit(1);
    }
}

testLongerCallSelfSimilarity();
