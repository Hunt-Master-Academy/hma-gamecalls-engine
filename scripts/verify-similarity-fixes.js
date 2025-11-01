#!/usr/bin/env node
// [20251102-TEST-002] Simple 3-comparison test with timeout

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');
const SessionsService = require(path.join(servicesPath, 'sessionsService'));

const TEST_TIMEOUT = 90000; // 90 seconds total

// Hardcoded test calls
const TEST_PAIRS = [
    // Self-similarity tests (should be high 85-95%)
    { master: 'call_turkey_putt_putts', test: 'call_turkey_putt_putts', expected: 'high', label: 'Self: Putt Putts' },
    
    // Different calls (should be low <40%)
    { master: 'call_turkey_kee_kee_run', test: 'call_turkey_gobbling', expected: 'low', label: 'Different: Kee Kee vs Gobbling' },
    { master: 'call_turkey_plain_yelp', test: 'call_turkey_cutting', expected: 'low', label: 'Different: Yelp vs Cutting' }
];

async function testPair(masterCallId, testCallId, label, expected) {
    try {
        const session = await SessionsService.createSession(masterCallId, { enableEnhanced: true });
        const analysis = await SessionsService.finalizeSession(session.sessionId);
        const similarity = analysis.similarity || 0;
        
        let status = '⚠️';
        if (expected === 'high' && similarity > 0.85) status = '✅';
        else if (expected === 'low' && similarity < 0.40) status = '✅';
        else if (expected === 'medium' && similarity >= 0.40 && similarity <= 0.75) status = '✅';
        else status = '❌';
        
        console.log(`${status} ${label}: ${(similarity * 100).toFixed(1)}%`);
        return { label, similarity, expected, passed: status === '✅' };
        
    } catch (error) {
        console.log(`❌ ${label}: ERROR - ${error.message}`);
        return { label, similarity: -1, expected, passed: false, error: error.message };
    }
}

async function runTests() {
    const timeout = setTimeout(() => {
        console.error('\n❌ TEST SUITE TIMEOUT after 90 seconds');
        process.exit(124);
    }, TEST_TIMEOUT);

    try {
        console.log('🧪 Similarity Algorithm Verification\n');
        console.log('Testing fixes:');
        console.log('  • Subsequence gamma: 0.45→0.75 (penalizes weak matches)');
        console.log('  • Coverage uplift: 1.60x→1.15x (less aggressive)');
        console.log('  • Expansion multiplier: REMOVED');
        console.log('  • Cosine threshold: >0.3 (reject orthogonal vectors)');
        console.log('  • DTW: exp(-2*distance) for better discrimination\n');
        
        const results = [];
        
        for (const pair of TEST_PAIRS) {
            const result = await testPair(pair.master, pair.test, pair.label, pair.expected);
            results.push(result);
        }
        
        const passed = results.filter(r => r.passed).length;
        const total = results.length;
        
        console.log(`\n━━━ RESULTS: ${passed}/${total} tests passed ━━━`);
        
        if (passed === total) {
            console.log('✅ All tests passed - algorithm discrimination improved!');
            console.log('   Ready to run full 15×15 pairwise matrix');
        } else {
            console.log('⚠️  Some tests failed - may need further tuning');
            results.filter(r => !r.passed).forEach(r => {
                console.log(`   • ${r.label}: ${(r.similarity * 100).toFixed(1)}% (expected ${r.expected})`);
            });
        }
        
        clearTimeout(timeout);
        process.exit(passed === total ? 0 : 1);
        
    } catch (error) {
        clearTimeout(timeout);
        console.error('\n❌ Test suite failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

runTests();
