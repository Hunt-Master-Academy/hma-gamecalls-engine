#!/usr/bin/env node
/**
 * [20251102-DEBUG-001] Deep Dive Self-Similarity Debug
 * 
 * Investigates why identical audio yields low similarity:
 * 1. Check master call feature count vs user audio feature count
 * 2. Monitor similarity component breakdown (DTW, offset, mean, subsequence)
 * 3. Verify audio normalization is consistent
 * 4. Check if readiness affects final score
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

const TEST_TIMEOUT = 120000;
const timeoutId = setTimeout(() => {
    console.error('\n❌ Test TIMEOUT after 2 minutes');
    process.exit(1);
}, TEST_TIMEOUT);

async function loadAudioChunks(audioFilePath, chunkSize = 32768) {
    const fileHandle = await fs.open(audioFilePath, 'r');
    const buffer = Buffer.alloc(chunkSize);
    const chunks = [];
    
    let bytesRead;
    let offset = 44; // Skip WAV header
    
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

async function debugSelfSimilarity() {
    console.log('🔍 Deep Dive Self-Similarity Debug');
    console.log('='.repeat(70));

    try {
        // Get ONE call for focused debugging
        const { calls } = await GameCallsService.listCalls({
            species: 'wild_turkey',
            pageSize: 1,
            sortBy: 'duration_seconds',
            sortOrder: 'asc' // Start with shortest for faster iteration
        });

        if (!calls || calls.length === 0) {
            throw new Error('No calls available for testing');
        }

        const call = calls[0];
        console.log(`\n📊 Debug Call: ${call.id}`);
        console.log(`   Type: ${call.call_type || 'UNKNOWN'}`);
        console.log(`   Duration: ${call.duration_seconds}s\n`);

        // STEP 1: Create session with master
        console.log('─'.repeat(70));
        console.log('STEP 1: Create Session with Master Call');
        console.log('─'.repeat(70));
        
        const session = await SessionsService.createSession(call.id, {
            enableEnhanced: true,
            sampleRate: 44100,
            bufferSize: 32768
        });
        
        const sessionId = session.id;
        const sessionDetails = await SessionsService.getSession(sessionId);
        
        console.log(`✅ Session ID: ${sessionId}`);
        console.log(`✅ Native Session ID: ${sessionDetails.nativeSessionId}`);
        console.log(`✅ Master Call: ${sessionDetails.masterCallId}`);
        console.log(`✅ Sample Rate: ${sessionDetails.sampleRate} Hz`);
        console.log(`✅ Status: ${sessionDetails.status}`);

        // Start recording
        await SessionsService.startSession(sessionId);
        console.log(`✅ Recording mode activated\n`);

        // STEP 2: Download SAME audio
        console.log('─'.repeat(70));
        console.log('STEP 2: Load SAME Audio for Comparison');
        console.log('─'.repeat(70));
        
        const audioPath = call.audio_file_path || call.audioFilePath || call.audio_path;
        if (!audioPath) {
            throw new Error(`No audio path for ${call.id}`);
        }
        
        const audioKey = audioPath.replace('master-calls/', '');
        const stream = await MinIOService.getObjectStream('gamecalls-master-calls', audioKey);
        
        const tempFile = `/tmp/debug-self-${Date.now()}.wav`;
        const writeStream = require('fs').createWriteStream(tempFile);
        
        await new Promise((resolve, reject) => {
            stream.pipe(writeStream);
            stream.on('error', reject);
            writeStream.on('finish', resolve);
            writeStream.on('error', reject);
        });

        console.log(`✅ Downloaded: ${tempFile}`);
        
        // Get file stats
        const stats = await fs.stat(tempFile);
        console.log(`✅ File size: ${stats.size} bytes`);
        
        // Load chunks
        const audioChunks = await loadAudioChunks(tempFile, 32768);
        console.log(`✅ Prepared ${audioChunks.length} chunks (32768 samples each)\n`);

        // STEP 3: Process chunks with detailed logging
        console.log('─'.repeat(70));
        console.log('STEP 3: Stream Audio & Monitor Component Breakdown');
        console.log('─'.repeat(70));
        console.log('');

        const results = [];

        for (let i = 0; i < audioChunks.length; i++) {
            const chunk = audioChunks[i];
            
            console.log(`Chunk ${i + 1}/${audioChunks.length}:`);
            console.log(`  Samples: ${chunk.length}`);
            console.log(`  Min: ${Math.min(...chunk).toFixed(4)}, Max: ${Math.max(...chunk).toFixed(4)}`);
            console.log(`  Mean: ${(chunk.reduce((a, b) => a + b, 0) / chunk.length).toFixed(4)}`);
            
            try {
                const result = await SessionsService.processAudioData(sessionId, chunk);
                
                console.log(`  → Similarity: ${(result.similarity?.overall * 100 || 0).toFixed(2)}%`);
                console.log(`  → Confidence: ${(result.similarity?.confidence * 100 || 0).toFixed(2)}%`);
                console.log(`  → Readiness: ${result.readiness || 'unknown'}`);
                
                // Try to get detailed session metrics if available
                const updatedSession = await SessionsService.getSession(sessionId);
                if (updatedSession.metrics) {
                    console.log(`  → Metrics:`);
                    if (updatedSession.lastOffsetComponent !== undefined) {
                        console.log(`     - Offset: ${updatedSession.lastOffsetComponent?.toFixed(4) || 'N/A'}`);
                    }
                    if (updatedSession.lastDTWComponent !== undefined) {
                        console.log(`     - DTW: ${updatedSession.lastDTWComponent?.toFixed(4) || 'N/A'}`);
                    }
                    if (updatedSession.lastMeanComponent !== undefined) {
                        console.log(`     - Mean: ${updatedSession.lastMeanComponent?.toFixed(4) || 'N/A'}`);
                    }
                    if (updatedSession.lastSubsequenceComponent !== undefined) {
                        console.log(`     - Subsequence: ${updatedSession.lastSubsequenceComponent?.toFixed(4) || 'N/A'}`);
                    }
                }
                
                results.push({
                    chunk: i + 1,
                    samples: chunk.length,
                    similarity: result.similarity?.overall || 0,
                    confidence: result.similarity?.confidence || 0,
                    readiness: result.readiness,
                    metrics: updatedSession.metrics
                });
                
            } catch (error) {
                console.log(`  ❌ ERROR: ${error.message}`);
                break;
            }
            
            console.log('');
        }

        // STEP 4: Analysis
        console.log('─'.repeat(70));
        console.log('STEP 4: Analysis & Diagnosis');
        console.log('─'.repeat(70));
        console.log('');

        const maxSim = Math.max(...results.map(r => r.similarity));
        const finalSim = results[results.length - 1]?.similarity || 0;
        const avgSim = results.reduce((sum, r) => sum + r.similarity, 0) / results.length;

        console.log(`📊 Similarity Statistics:`);
        console.log(`   Peak: ${(maxSim * 100).toFixed(2)}%`);
        console.log(`   Final: ${(finalSim * 100).toFixed(2)}%`);
        console.log(`   Average: ${(avgSim * 100).toFixed(2)}%`);
        console.log('');

        console.log(`📈 Progression:`);
        results.forEach(r => {
            const simPct = (r.similarity * 100).toFixed(1);
            const bar = '█'.repeat(Math.floor(r.similarity * 50));
            console.log(`   Chunk ${r.chunk.toString().padStart(2)}: ${simPct.padStart(5)}% ${bar}`);
        });
        console.log('');

        console.log(`🔍 Diagnosis:`);
        if (maxSim < 0.50) {
            console.log(`   ❌ CRITICAL: Peak similarity ${(maxSim * 100).toFixed(1)}% is dangerously low`);
            console.log(`   ⚠️  Possible causes:`);
            console.log(`      1. Master features not loaded correctly into session`);
            console.log(`      2. User audio features extracted with different parameters`);
            console.log(`      3. Similarity components over-penalizing identical audio`);
            console.log(`      4. DTW alignment failing on identical sequences`);
        } else if (maxSim < 0.90) {
            console.log(`   ⚠️  WARNING: Peak similarity ${(maxSim * 100).toFixed(1)}% below expected >90%`);
            console.log(`   Possible causes:`);
            console.log(`      1. Algorithm penalties too aggressive (check gamma, coverage uplift)`);
            console.log(`      2. Blending weights favoring lower-scoring components`);
            console.log(`      3. Need more audio data to reach optimal alignment`);
        } else {
            console.log(`   ✅ GOOD: Peak similarity ${(maxSim * 100).toFixed(1)}% meets expectations`);
        }

        // Check if similarity increases over time (expected for self-similarity)
        const firstHalf = results.slice(0, Math.floor(results.length / 2));
        const secondHalf = results.slice(Math.floor(results.length / 2));
        const avgFirst = firstHalf.reduce((sum, r) => sum + r.similarity, 0) / firstHalf.length;
        const avgSecond = secondHalf.reduce((sum, r) => sum + r.similarity, 0) / secondHalf.length;

        console.log('');
        console.log(`📈 Temporal Pattern:`);
        console.log(`   First half avg: ${(avgFirst * 100).toFixed(2)}%`);
        console.log(`   Second half avg: ${(avgSecond * 100).toFixed(2)}%`);
        if (avgSecond > avgFirst) {
            console.log(`   ✅ Similarity improving over time (expected for identical audio)`);
        } else {
            console.log(`   ⚠️  Similarity not improving (unexpected for identical audio)`);
        }

        // Cleanup
        await fs.unlink(tempFile);

        clearTimeout(timeoutId);
        process.exit(maxSim >= 0.90 ? 0 : 1);

    } catch (error) {
        console.error('\n❌ Debug failed:', error.message);
        console.error(error.stack);
        clearTimeout(timeoutId);
        process.exit(1);
    }
}

debugSelfSimilarity();
