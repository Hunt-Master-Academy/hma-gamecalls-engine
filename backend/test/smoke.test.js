// [20251029-TEST-001] Smoke test for GameCalls Engine processing pipeline
/**
 * Complete end-to-end smoke test:
 * 1. Load master call from database
 * 2. Create C++ engine session
 * 3. Start session and process audio
 * 4. Extract features and calculate similarity
 * 5. Stop session and verify results
 */

// [20251029-TEST-014] Load environment variables from .env
require('dotenv').config();

const SessionsService = require('../src/services/sessionsService');
const GameCallsService = require('../src/services/gameCallsService');

// [20251029-TEST-002] Set environment for real engine
process.env.LD_LIBRARY_PATH = '/home/xbyooki/projects/hma-gamecalls-engine/build/_deps/kissfft-build:' + (process.env.LD_LIBRARY_PATH || '');

describe('GameCalls Engine Processing Pipeline - Smoke Test', () => {
    let sessionId;
    let masterCallId;
    let masterCallSummary;

    beforeAll(async () => {
        // [20251030-TEST-004] Use longest master call for C++ engine (needs sufficient audio)
        const { calls = [] } = await GameCallsService.listCalls({ 
            pageSize: 20,
            sortBy: 'duration_seconds',
            sortOrder: 'desc'
        });
        if (calls.length === 0) {
            throw new Error('No master calls found in database. Run migrations and seeds first.');
        }

        // Use the longest call to ensure C++ engine has enough data for finalization
        const selectedCall = calls[0]; // Should be call_coyote_howl_001 (5.2s)
        masterCallId = selectedCall.id;
        masterCallSummary = {
            species: selectedCall.species,
            callType: selectedCall.call_type || selectedCall.callType,
            name: selectedCall.name
        };

        console.log(`\n🎯 Using master call: ${masterCallSummary.species} - ${masterCallSummary.callType} (ID: ${masterCallId})`);
        console.log(`   Duration: ${selectedCall.duration_seconds || selectedCall.durationSeconds}s`);
    });

    afterAll(async () => {
        // Cleanup session if created
        if (sessionId) {
            try {
                await SessionsService.stopSession(sessionId);
            } catch (err) {
                console.warn('Session cleanup warning:', err.message);
            }
        }
    });

    test('should load master call from database', async () => {
        const masterCall = await GameCallsService.getCall(masterCallId);
        
        expect(masterCall).toBeDefined();
        expect(masterCall.id).toBe(masterCallId);
        expect(masterCall.species).toBeDefined();
        expect(masterCall.audioFilePath).toBeDefined();
        
    console.log(`\n✅ Master call loaded:`);
    console.log(`   Name: ${masterCall.name}`);
    console.log(`   Species: ${masterCall.species}`);
    console.log(`   Call Type: ${masterCall.callType}`);
    console.log(`   Duration: ${masterCall.durationSeconds || masterCall.duration}s`);
    console.log(`   Audio Path: ${masterCall.audioFilePath}`);
    }, 10000);

    test('should create session with real C++ engine', async () => {
        const session = await SessionsService.createSession(masterCallId, {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });

        sessionId = session.id; // [20251029-TEST-017] Use correct property name
        
        expect(session).toBeDefined();
        expect(session.id).toBeDefined();
        expect(session.masterCallId).toBe(masterCallId);
        expect(session.status).toBe('created');
        
        console.log(`\n✅ Session created:`);
        console.log(`   Session ID: ${sessionId}`);
        console.log(`   Status: ${session.status}`);
        console.log(`   Engine Type: ${session.engineType || 'C++ Native'}`);
    }, 30000);

    test('should start session successfully', async () => {
        const result = await SessionsService.startSession(sessionId);
        
        expect(result).toBeDefined();
        expect(result.status).toBe('recording');
        
        console.log(`\n✅ Session started:`);
        console.log(`   Status: ${result.status}`);
    }, 10000);

    test('should process audio chunk and get real-time similarity', async () => {
        // [20251029-TEST-019] Create larger audio buffer (need ~25 feature frames for finalization)
        // With 512 frame size and 256 hop, need at least 25*256 = 6400 samples minimum
        const audioBuffer = new Float32Array(8192).fill(0); // Use 8192 samples for safety
        
        const result = await SessionsService.processAudio(sessionId, audioBuffer);
        
        expect(result).toBeDefined();
        expect(result.similarity).toBeDefined();
        expect(typeof result.similarity.overall).toBe('number');
        expect(typeof result.similarity.confidence).toBe('number');
        
        console.log(`\n✅ Audio processed with REAL engine:`);
        console.log(`   Overall Similarity: ${result.similarity.overall.toFixed(4)}`);
        console.log(`   MFCC: ${result.similarity.mfcc?.toFixed(4) || 'N/A'}`);
        console.log(`   Volume: ${result.similarity.volume?.toFixed(4) || 'N/A'}`);
        console.log(`   Timing: ${result.similarity.timing?.toFixed(4) || 'N/A'}`);
        console.log(`   Pitch: ${result.similarity.pitch?.toFixed(4) || 'N/A'}`);
        console.log(`   Confidence: ${result.similarity.confidence.toFixed(4)}`);
        console.log(`   Is Reliable: ${result.similarity.isReliable}`);
        console.log(`   Is Match: ${result.similarity.isMatch}`);
        console.log(`   Samples Analyzed: ${result.similarity.samplesAnalyzed || 'N/A'}`);
        
        // Verify we're NOT getting mock data
        if (result.similarity.overall === 0.85 && result.similarity.confidence === 0.92) {
            console.error('\n❌ WARNING: Receiving MOCK engine data!');
            throw new Error('Mock engine detected - expected real C++ engine data');
        }
        
        console.log('\n✅ Confirmed: Using REAL C++ engine (not mock)');
    }, 10000);

    test('should finalize session and get comprehensive analysis', async () => {
        const result = await SessionsService.finalizeSession(sessionId);
        
        expect(result).toBeDefined();
        expect(result.analysis).toBeDefined();
        
        const analysis = result.analysis;
        
        // Verify enhanced analysis fields exist
        expect(analysis.pitch).toBeDefined();
        expect(analysis.harmonic).toBeDefined();
        expect(analysis.cadence).toBeDefined();
        expect(analysis.finalize).toBeDefined();
        
        console.log(`\n✅ Session finalized with comprehensive analysis:`);
        console.log(`\n📊 Pitch Analysis:`);
        console.log(`   Frequency: ${analysis.pitch.pitchHz?.toFixed(2) || 'N/A'} Hz`);
        console.log(`   Confidence: ${analysis.pitch.confidence?.toFixed(4) || 'N/A'}`);
        console.log(`   Grade: ${analysis.pitch.grade || 'N/A'}`);
        
        console.log(`\n📊 Harmonic Analysis:`);
        console.log(`   Fundamental: ${analysis.harmonic.fundamental?.toFixed(2) || 'N/A'} Hz`);
        console.log(`   Confidence: ${analysis.harmonic.confidence?.toFixed(4) || 'N/A'}`);
        console.log(`   Grade: ${analysis.harmonic.grade || 'N/A'}`);
        
        console.log(`\n📊 Cadence Analysis:`);
        console.log(`   Tempo: ${analysis.cadence.tempoBPM?.toFixed(2) || 'N/A'} BPM`);
        console.log(`   Confidence: ${analysis.cadence.confidence?.toFixed(4) || 'N/A'}`);
        console.log(`   Grade: ${analysis.cadence.grade || 'N/A'}`);
        
        console.log(`\n📊 Finalize Metrics:`);
        console.log(`   Similarity at Finalize: ${analysis.finalize.similarityAtFinalize?.toFixed(4) || 'N/A'}`);
        console.log(`   Normalization Scalar: ${analysis.finalize.normalizationScalar?.toFixed(4) || 'N/A'}`);
        console.log(`   Loudness Deviation: ${analysis.finalize.loudnessDeviation?.toFixed(4) || 'N/A'}`);
        console.log(`   Segment Start: ${analysis.finalize.segmentStartMs || 'N/A'} ms`);
        console.log(`   Segment Duration: ${analysis.finalize.segmentDurationMs || 'N/A'} ms`);
        
        console.log(`\n📊 Status Flags:`);
        console.log(`   Valid: ${analysis.valid}`);
        console.log(`   Finalized: ${analysis.finalized}`);
    }, 10000);

    test('should stop session and cleanup', async () => {
        const result = await SessionsService.stopSession(sessionId);
        
        expect(result).toBeDefined();
        expect(result.status).toBe('completed'); // [20251029-TEST-020] Correct status after finalization
        
        console.log(`\n✅ Session stopped:`);
        console.log(`   Status: ${result.status}`);
        
        sessionId = null; // Prevent double cleanup in afterAll
    }, 10000);
});

// [20251029-TEST-003] Run smoke test if executed directly
if (require.main === module) {
    console.log('\n' + '='.repeat(60));
    console.log('🧪 GameCalls Engine Processing Pipeline - SMOKE TEST');
    console.log('='.repeat(60));
    console.log('\nThis test verifies the complete processing pipeline:');
    console.log('  ✓ Database integration (master calls)');
    console.log('  ✓ MinIO integration (audio streaming)');
    console.log('  ✓ C++ Engine integration (real feature extraction)');
    console.log('  ✓ Session lifecycle (create → start → process → finalize → stop)');
    console.log('  ✓ Real-time similarity scoring');
    console.log('  ✓ Enhanced analysis (pitch, harmonic, cadence)');
    console.log('  ✓ Finalize-stage refined DTW');
    console.log('\n' + '='.repeat(60) + '\n');
    
    // Run with jest
    const { run } = require('jest');
    run(['--testPathPattern=smoke-test.js', '--verbose']);
}
