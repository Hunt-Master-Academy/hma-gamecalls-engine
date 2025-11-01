#!/usr/bin/env node

/**
 * [20251102-TEST-001] Quick test to verify similarity score extraction
 */

const SessionsService = require('./src/services/sessionsService');

async function testSimilarityExtraction() {
    console.log('🧪 Testing Similarity Score Extraction');
    console.log('=' .repeat(50));
    
    try {
        // Create session with a master call
        console.log('\n➕ Creating session...');
        const session = await SessionsService.createSession('call_turkey_putt_putts', {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        console.log(`✅ Session created: ${session.id}`);
        console.log(`   Native Session ID: ${session.nativeSessionId}`);
        
        // Start the session (change to recording state)
        console.log('\n▶️  Starting session...');
        await SessionsService.startSession(session.id);
        console.log('✅ Session started (recording)');
        
        // Process a few chunks of synthetic audio
        console.log('\n🔊 Processing audio chunks...');
        for (let i = 0; i < 5; i++) {
            const audioBuffer = new Float32Array(32768);
            // Fill with random audio data
            for (let j = 0; j < audioBuffer.length; j++) {
                audioBuffer[j] = (Math.random() - 0.5);
            }
            
            const result = await SessionsService.processAudioData(session.id, audioBuffer);
            console.log(`   Chunk ${i + 1}: similarity=${result.similarityScore?.toFixed(4) || 'N/A'}`);
        }
        
        // Finalize and check if similarity is extracted
        console.log('\n🏁 Finalizing session...');
        const analysis = await SessionsService.finalizeSession(session.id);
        
        console.log('\n📊 Finalization Result:');
        console.log(`   sessionId: ${analysis.sessionId}`);
        console.log(`   similarity: ${analysis.similarity}`);
        console.log(`   finalize.similarityAtFinalize: ${analysis.analysis?.finalize?.similarityAtFinalize}`);
        console.log(`   finalized: ${analysis.analysis?.finalized}`);
        console.log(`   valid: ${analysis.analysis?.valid}`);
        
        // Cleanup
        await SessionsService.deleteSession(session.id);
        
        if (analysis.similarity !== undefined && analysis.similarity > 0) {
            console.log('\n✅ SUCCESS: Similarity score extracted correctly!');
            console.log(`   Final similarity: ${(analysis.similarity * 100).toFixed(2)}%`);
        } else {
            console.log('\n❌ FAILURE: Similarity score not extracted or is 0');
            console.log('   Raw analysis:', JSON.stringify(analysis, null, 2));
        }
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

testSimilarityExtraction();
