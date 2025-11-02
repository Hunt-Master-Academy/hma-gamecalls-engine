#!/usr/bin/env node
/**
 * [20251101-V1.0-SMOKE] Component Score Instrumentation
 * 
 * Displays per-component scores for any audio comparison to debug scoring logic.
 * 
 * Usage: node smoke-test-components.js <masterCallId>
 * 
 * Prints:
 * - MFCC score (DTW)
 * - Volume score
 * - Timing score
 * - Pitch score
 * - Overall weighted score
 * - Confidence and readiness
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

async function loadAudioFile(audioFilePath) {
    const fileStats = await fs.stat(audioFilePath);
    const fileSize = fileStats.size - 44; // Exclude WAV header
    
    const fileHandle = await fs.open(audioFilePath, 'r');
    const buffer = Buffer.alloc(fileSize);
    
    const { bytesRead } = await fileHandle.read(buffer, 0, fileSize, 44);
    await fileHandle.close();
    
    const float32Data = new Float32Array(bytesRead / 2);
    for (let i = 0; i < float32Data.length; i++) {
        const sample = buffer.readInt16LE(i * 2);
        float32Data[i] = sample / 32768.0;
    }
    
    return float32Data;
}

async function runSmokeTest(masterCallId) {
    console.log(`\n🔬 Component Score Smoke Test`);
    console.log(`======================================================================`);
    console.log(`Master Call: ${masterCallId}\n`);

    // Get master call metadata
    const call = await GameCallsService.getCall(masterCallId);
    if (!call) {
        throw new Error(`Master call not found: ${masterCallId}`);
    }

    console.log(`  Species: ${call.species}`);
    console.log(`  Type: ${call.call_type}`);
    console.log(`  Duration: ${call.duration_seconds}s\n`);

    // Create session
    const session = await SessionsService.createSession(masterCallId, {
        enableEnhanced: true,
        sampleRate: 44100,
        bufferSize: 32768
    });
    console.log(`✅ Session created: ${session.id}`);

    await SessionsService.startSession(session.id);
    console.log(`✅ Session started\n`);

    // Download master audio
    const audioPath = call.audio_file_path || call.audioFilePath || call.audio_path;
    const audioKey = audioPath.replace('master-calls/', '');
    const stream = await MinIOService.getObjectStream('gamecalls-master-calls', audioKey);
    
    const tempFile = `/tmp/smoke-${Date.now()}.wav`;
    const writeStream = require('fs').createWriteStream(tempFile);
    
    await new Promise((resolve, reject) => {
        stream.pipe(writeStream);
        stream.on('error', reject);
        writeStream.on('finish', resolve);
        writeStream.on('error', reject);
    });

    console.log(`📥 Downloaded: ${tempFile}\n`);

    // Process full audio in one chunk
    const audioData = await loadAudioFile(tempFile);
    console.log(`📊 Processing ${audioData.length} samples...\n`);

    const result = await SessionsService.processAudioData(session.id, audioData);

    // Display component scores
    console.log(`🎯 Component Scores:`);
    console.log(`   MFCC Similarity:    ${((result.similarity?.mfcc || 0) * 100).toFixed(2)}%`);
    console.log(`   Volume Match:       ${((result.similarity?.volume || 0) * 100).toFixed(2)}%`);
    console.log(`   Timing Accuracy:    N/A (see overall)`);
    console.log(`   Pitch Similarity:   N/A (disabled)`);
    console.log(`   ────────────────────────────────`);
    console.log(`   OVERALL SCORE:      ${((result.similarity?.overall || 0) * 100).toFixed(2)}%`);
    console.log(`   Confidence:         ${((result.confidence || 0) * 100).toFixed(2)}%`);
    console.log(`   Readiness:          ${result.readiness || 'unknown'}`);

    // Cleanup
    await SessionsService.endSession(session.id);
    await fs.unlink(tempFile);

    console.log(`\n✅ Smoke test complete\n`);
}

// Main
const masterCallId = process.argv[2] || 'call_turkey_cluck_001';
runSmokeTest(masterCallId)
    .then(() => process.exit(0))
    .catch(err => {
        console.error(`\n❌ Smoke test failed:`, err.message);
        process.exit(1);
    });
