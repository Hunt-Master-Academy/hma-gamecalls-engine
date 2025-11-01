#!/usr/bin/env node
/**
 * [20251102-DEBUG-001] Session Destroy Verification Test
 * Tests create → destroy → create pattern with JS-visible feedback
 */

const binding = require('./bindings/node-api/lib/index');

console.log('🔍 Session Destroy Verification Test');
console.log('====================================\n');

async function testSessionLifecycle() {
    try {
        // Initial state
        console.log('📊 Initial state:');
        let info = await binding.getActiveSessionsInfo();
        console.log(`   Active wrappers: ${info.activeWrappers}`);
        console.log(`   Next wrapper ID: ${info.nextWrapperId}\n`);
        
        // Create Session A
        console.log('➕ Creating Session A...');
        const sessionA = await binding.createSession('call_turkey_putt_putts', {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        console.log(`   ✅ Created wrapper session: ${sessionA.sessionId || sessionA}`);
        const sessionAId = sessionA.sessionId || sessionA;
        
        info = await binding.getActiveSessionsInfo();
        console.log(`   Active wrappers after create: ${info.activeWrappers}`);
        console.log(`   Next wrapper ID: ${info.nextWrapperId}\n`);
        
        // Destroy Session A
        console.log('🗑️  Destroying Session A...');
        const destroyResult = await binding.destroySession(sessionAId);
        console.log(`   Destroyed: ${destroyResult.destroyed || destroyResult}`);
        console.log(`   C++ sessions destroyed: ${destroyResult.cppSessionsDestroyed || 'N/A'}`);
        console.log(`   Active wrappers after destroy: ${destroyResult.activeWrappers || 'N/A'}\n`);
        
        info = await binding.getActiveSessionsInfo();
        console.log(`   📊 State after destroy:`);
        console.log(`   Active wrappers: ${info.activeWrappers}`);
        console.log(`   Next wrapper ID: ${info.nextWrapperId}\n`);
        
        // Create Session B
        console.log('➕ Creating Session B...');
        const sessionB = await binding.createSession('call_turkey_putt_putts', {
            sampleRate: 44100,
            enableEnhancedAnalysis: true
        });
        console.log(`   ✅ Created wrapper session: ${sessionB.sessionId || sessionB}`);
        const sessionBId = sessionB.sessionId || sessionB;
        
        info = await binding.getActiveSessionsInfo();
        console.log(`   Active wrappers after create: ${info.activeWrappers}`);
        console.log(`   Next wrapper ID: ${info.nextWrapperId}\n`);
        
        // Analysis
        console.log('📈 Analysis:');
        const cppDestroyed = destroyResult.cppSessionsDestroyed || 0;
        if (cppDestroyed === 0) {
            console.log('   ❌ PROBLEM: No C++ sessions were destroyed!');
            console.log('   This means destroySession() is not calling engine->destroySession()');
        } else if (cppDestroyed === 1) {
            console.log('   ✅ GOOD: 1 C++ session was destroyed as expected');
        } else {
            console.log(`   ⚠️  WARNING: ${cppDestroyed} C++ sessions destroyed (expected 1)`);
        }
        
        // Note: We can't easily check native session ID from JS wrapper,
        // but we can check if wrapper IDs are reused
        if (sessionAId !== sessionBId) {
            console.log(`   ℹ️  Wrapper IDs differ (A=${sessionAId}, B=${sessionBId}) - this is expected`);
        }
        
        // Cleanup
        console.log('\n🧹 Cleanup...');
        const cleanupResult = await binding.destroySession(sessionBId);
        console.log(`   Destroyed Session B: ${cleanupResult.destroyed || cleanupResult}`);
        console.log(`   C++ sessions destroyed: ${cleanupResult.cppSessionsDestroyed || 'N/A'}`);
        console.log(`   Active wrappers: ${cleanupResult.activeWrappers || 'N/A'}\n`);
        
        console.log('✅ Test complete\n');
        
    } catch (error) {
        console.error('❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

testSessionLifecycle();
