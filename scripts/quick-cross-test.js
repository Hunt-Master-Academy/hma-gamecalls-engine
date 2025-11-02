#!/usr/bin/env node
const path = require('path');
const fs = require('fs').promises;

const servicesPath = path.join(__dirname, '../backend/src/services');
const SessionsService = require(path.join(servicesPath, 'sessionsService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));
const MinIOService = require(path.join(servicesPath, 'minioService'));

async function loadAudioChunk(filePath) {
    const fileHandle = await fs.open(filePath, 'r');
    const buffer = Buffer.alloc(65536);
    await fileHandle.read(buffer, 0, 65536, 44);
    await fileHandle.close();
    
    const float32 = new Float32Array(32768);
    for (let i = 0; i < 32768; i++) {
        float32[i] = buffer.readInt16LE(i * 2) / 32768.0;
    }
    return float32;
}

async function test() {
    console.log('🔀 Quick Cross-Similarity Test\n');
    
    // Test 1: Cluck master vs Yelp test
    const session1 = await SessionsService.createSession('call_turkey_cluck_001', { sampleRate: 44100 });
    await SessionsService.startSession(session1.id);
    
    const yelpCall = await GameCallsService.getCall('call_turkey_yelp_001');
    const yelpKey = (yelpCall.audio_file_path || yelpCall.audioFilePath).replace('master-calls/', '');
    const yelpStream = await MinIOService.getObjectStream('gamecalls-master-calls', yelpKey);
    const yelpFile = '/tmp/test-yelp.wav';
    await new Promise((resolve, reject) => {
        const writeStream = require('fs').createWriteStream(yelpFile);
        yelpStream.pipe(writeStream);
        yelpStream.on('error', reject);
        writeStream.on('finish', resolve);
    });
    
    const yelpChunk = await loadAudioChunk(yelpFile);
    const result1 = await SessionsService.processAudioData(session1.id, yelpChunk);
    
    console.log('Test 1: Cluck master vs Yelp test');
    console.log(`  Similarity: ${(result1.similarity?.overall * 100 || 0).toFixed(1)}%`);
    console.log(`  Expected: <50% (different call types)`);
    console.log(`  Status: ${result1.similarity?.overall < 0.5 ? '✅ PASS' : '⚠️ WARN'}\n`);
    
    await SessionsService.destroySession(session1.id);
    
    // Test 2: Yelp master vs Yelp test (should be high)
    const session2 = await SessionsService.createSession('call_turkey_yelp_001', { sampleRate: 44100 });
    await SessionsService.startSession(session2.id);
    
    const result2 = await SessionsService.processAudioData(session2.id, yelpChunk);
    
    console.log('Test 2: Yelp master vs Yelp test (self-similarity)');
    console.log(`  Similarity: ${(result2.similarity?.overall * 100 || 0).toFixed(1)}%`);
    console.log(`  Expected: >85% (same call)`);
    console.log(`  Status: ${result2.similarity?.overall > 0.85 ? '✅ PASS' : '❌ FAIL'}\n`);
    
    await SessionsService.destroySession(session2.id);
    
    process.exit(0);
}

test().catch(err => {
    console.error('Error:', err.message);
    process.exit(1);
});
