#!/usr/bin/env node
/**
 * Session Lifecycle Stress Test
 * Tests create → process → destroy cycle to find resource exhaustion point
 */

const SessionsService = require('./src/services/sessionsService');

const TEST_ITERATIONS = 100;
const CHUNKS_PER_SESSION = 5;
const CHUNK_SIZE = 32768;

function generateTestAudio(size, frequency = 440) {
    const buffer = new Float32Array(size);
    for (let i = 0; i < size; i++) {
        buffer[i] = Math.sin(2 * Math.PI * frequency * i / 44100) * 0.5;
    }
    return buffer;
}

async function runLifecycleTest() {
    console.log('🧪 Session Lifecycle Stress Test');
    console.log('================================\n');
    console.log(`Iterations: ${TEST_ITERATIONS}`);
    console.log(`Chunks per session: ${CHUNKS_PER_SESSION}`);
    console.log(`Total chunks: ${TEST_ITERATIONS * CHUNKS_PER_SESSION}\n`);
    
    const testBuffer = generateTestAudio(CHUNK_SIZE);
    const stats = {
        successfulSessions: 0,
        successfulChunks: 0,
        failures: [],
        startTime: Date.now()
    };
    
    for (let i = 0; i < TEST_ITERATIONS; i++) {
        let sessionId = null;
        
        try {
            // Create session
            const session = await SessionsService.createSession('call_turkey_putt_putts', {
                userId: 'stress-test'
            });
            sessionId = session.id;
            
            // Start session
            await SessionsService.startSession(sessionId);
            
            // Process multiple chunks
            for (let chunk = 0; chunk < CHUNKS_PER_SESSION; chunk++) {
                const result = await SessionsService.processAudioData(sessionId, testBuffer);
                stats.successfulChunks++;
                
                if ((stats.successfulChunks % 50) === 0) {
                    const elapsed = ((Date.now() - stats.startTime) / 1000).toFixed(1);
                    const rate = (stats.successfulChunks / (Date.now() - stats.startTime) * 1000).toFixed(1);
                    console.log(`✅ Chunk ${stats.successfulChunks}: similarity=${result.similarity.overall.toFixed(4)} (${elapsed}s, ${rate} chunks/s)`);
                }
            }
            
            // Stop and delete session
            await SessionsService.stopSession(sessionId);
            await SessionsService.deleteSession(sessionId);
            
            stats.successfulSessions++;
            
            if ((i + 1) % 10 === 0) {
                console.log(`📊 Session ${i + 1}/${TEST_ITERATIONS} complete`);
            }
            
        } catch (error) {
            stats.failures.push({
                iteration: i + 1,
                totalChunks: stats.successfulChunks,
                sessionId: sessionId,
                error: error.message
            });
            
            console.error(`\n❌ FAILURE at iteration ${i + 1}`);
            console.error(`   Total chunks processed: ${stats.successfulChunks}`);
            console.error(`   Error: ${error.message}\n`);
            
            // Try to cleanup
            if (sessionId) {
                try {
                    await SessionsService.deleteSession(sessionId);
                } catch (cleanupError) {
                    console.error(`   Cleanup also failed: ${cleanupError.message}`);
                }
            }
            
            // Stop on first failure to analyze
            break;
        }
    }
    
    // Report results
    const elapsed = ((Date.now() - stats.startTime) / 1000).toFixed(1);
    const avgRate = (stats.successfulChunks / (Date.now() - stats.startTime) * 1000).toFixed(1);
    
    console.log('\n📊 Test Results');
    console.log('===============');
    console.log(`✅ Successful sessions: ${stats.successfulSessions}/${TEST_ITERATIONS}`);
    console.log(`✅ Successful chunks: ${stats.successfulChunks}`);
    console.log(`❌ Failures: ${stats.failures.length}`);
    console.log(`⏱️  Total time: ${elapsed}s`);
    console.log(`📈 Average rate: ${avgRate} chunks/s`);
    
    if (stats.failures.length > 0) {
        console.log('\n❌ Failure Details:');
        stats.failures.forEach((f, idx) => {
            console.log(`\nFailure ${idx + 1}:`);
            console.log(`  Iteration: ${f.iteration}`);
            console.log(`  Total chunks at failure: ${f.totalChunks}`);
            console.log(`  Session ID: ${f.sessionId}`);
            console.log(`  Error: ${f.error}`);
        });
        
        process.exit(1);
    } else {
        console.log('\n✅ ALL TESTS PASSED!');
        process.exit(0);
    }
}

// Run test
runLifecycleTest().catch(error => {
    console.error('Fatal error:', error);
    process.exit(1);
});
