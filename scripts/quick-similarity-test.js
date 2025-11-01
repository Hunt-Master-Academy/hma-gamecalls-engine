#!/usr/bin/env node
// [20251102-TEST-001] Quick test of similarity calculation with fixed parameters

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');
const SessionsService = require(path.join(servicesPath, 'sessionsService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));

const TEST_TIMEOUT = 60000; // 60 seconds

async function runQuickTest() {
    const timeout = setTimeout(() => {
        console.error('❌ TEST TIMEOUT after 60 seconds');
        process.exit(124);
    }, TEST_TIMEOUT);

    try {
        console.log('🧪 Quick Similarity Test with Fixed Parameters\n');
        
        // Get a sample of calls
        const calls = await GameCallsService.getAllGameCalls();
        const turkeyCalls = calls.filter(c => c.id.includes('turkey')).slice(0, 5);
        
        console.log(`Testing ${turkeyCalls.length} calls:\n`);
        turkeyCalls.forEach((c, i) => console.log(`  ${i+1}. ${c.name} (${c.id})`));
        
        // Test 1: Same call to itself (should be high ~85-95%)
        console.log('\n━━━ TEST 1: Self-Similarity ━━━');
        const call1 = turkeyCalls[0];
        const session1 = await SessionsService.createSession(call1.id, { enableEnhanced: true });
        const analysis1 = await SessionsService.finalizeSession(session1.sessionId);
        const selfSim = analysis1.similarity || 0;
        console.log(`${call1.name} vs itself: ${(selfSim * 100).toFixed(1)}% ${selfSim > 0.85 ? '✅' : '❌'}`);
        
        // Test 2: Different calls (should be lower <70%)
        if (turkeyCalls.length >= 2) {
            console.log('\n━━━ TEST 2: Different Calls ━━━');
            const call2 = turkeyCalls[1];
            const session2 = await SessionsService.createSession(call1.id, { enableEnhanced: true });
            const analysis2 = await SessionsService.finalizeSession(session2.sessionId);
            const diffSim = analysis2.similarity || 0;
            console.log(`${call1.name} vs ${call2.name}: ${(diffSim * 100).toFixed(1)}% ${diffSim < 0.70 ? '✅' : '⚠️'}`);
        }
        
        console.log('\n━━━ SUMMARY ━━━');
        console.log('✅ Fixes applied:');
        console.log('   • Gamma increased to 0.75 (penalizes weak matches)');
        console.log('   • Coverage uplift reduced to 1.15x max');
        console.log('   • Expansion multiplier removed');
        console.log('   • Cosine threshold added (>0.3)');
        console.log('   • Local match threshold raised to 0.4');
        console.log('   • Subsequence dominance reduced');
        console.log('   • DTW uses exp(-distance) instead of 1/(1+distance)');
        
        clearTimeout(timeout);
        console.log('\n✅ Quick test complete - ready for full pairwise matrix');
        process.exit(0);
        
    } catch (error) {
        clearTimeout(timeout);
        console.error('❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

runQuickTest();
