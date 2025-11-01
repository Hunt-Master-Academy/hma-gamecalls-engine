#!/usr/bin/env node
// [20251102-TEST-001] Direct C++ engine test with debug logging

const engine = require('./bindings/node-api/lib/index');
const SessionsService = require('./src/services/sessionsService');
const fs = require('fs');

async function main() {
    console.log('🧪 Direct C++ Engine Test with Debug Logging\n');
    
    // Use SessionsService to create a session (it handles MinIO download)
    console.log('📋 Creating session via SessionsService (handles MinIO download)...');
    
    let session;
    try {
        session = await SessionsService.createSession('call_turkey_putt_putts', {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        console.log('✅ Session created:', session.id);
        console.log('   Native Session ID:', session.nativeSessionId);
        console.log('   Master call path:', session.masterCallPath, '\n');
    } catch (error) {
        console.error('❌ Session creation failed:', error.message);
        process.exit(1);
    }
    
    // Create test audio buffer (sine wave)
    const bufferSize = 32768;
    const audioBuffer = new Float32Array(bufferSize);
    for (let i = 0; i < bufferSize; i++) {
        audioBuffer[i] = Math.sin(2 * Math.PI * 440 * i / 44100) * 0.5;
    }
    console.log('🎵 Test audio buffer: 440Hz sine wave,', bufferSize, 'samples\n');
    
    // Process audio
    console.log('🔊 Processing audio through C++ engine...');
    console.log('   (Check logs above for detailed C++ debug output)\n');
    
    try {
        const result = await engine.processAudio(session.nativeSessionId, audioBuffer);
        console.log('✅ SUCCESS! Audio processed');
        console.log('\n📊 Results:');
        console.log('   Similarity:', result.similarityScore);
        console.log('   Confidence:', result.confidence);
        console.log('   Readiness:', result.readiness);
        console.log('   Pitch:', result.pitch);
        console.log('   Levels:', result.levels);
    } catch (error) {
        console.error('❌ PROCESSING FAILED!');
        console.error('   Error:', error.message);
        console.error('\n📋 Full stack:');
        console.error(error.stack);
        process.exit(1);
    } finally {
        await SessionsService.deleteSession(session.id);
        console.log('\n🗑️  Cleanup complete');
    }
}

main().catch(error => {
    console.error('Fatal error:', error);
    process.exit(1);
});
