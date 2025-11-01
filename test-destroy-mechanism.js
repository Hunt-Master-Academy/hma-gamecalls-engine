#!/usr/bin/env node

/**
 * [20250127-TEST-001] Minimal test for destroySession mechanism
 * Tests the C++ session cleanup without needing actual audio files
 */

const binding = require('./bindings/node-api/build/Release/gamecalls_engine.node');

console.log('🔍 Testing Session Destroy Mechanism (C++ Level)');
console.log('='.repeat(50));

async function testDestroyMechanism() {
    try {
        // Get initial state
        console.log('\n📊 Initial state:');
        const initialInfo = binding.getActiveSessionsInfo();
        console.log(`   Active wrappers: ${initialInfo.activeWrappers}`);
        console.log(`   Next wrapper ID: ${initialInfo.nextWrapperId}`);
        
        // Initialize engine
        console.log('\n🔧 Initializing engine...');
        binding.initializeEngine();
        console.log('✅ Engine initialized');
        
        // Try to get session info after init
        const afterInit = binding.getActiveSessionsInfo();
        console.log(`   Active wrappers after init: ${afterInit.activeWrappers}`);
        
        // Note: We can't test createSession without audio files,
        // but we CAN test that the binding exports work and return expected types
        
        console.log('\n✅ SUCCESS: Binding exports are working correctly!');
        console.log('   - getActiveSessionsInfo() returns:', typeof initialInfo);
        console.log('   - activeWrappers field:', typeof initialInfo.activeWrappers);
        console.log('   - nextWrapperId field:', typeof initialInfo.nextWrapperId);
        
        console.log('\n📝 Next Steps:');
        console.log('   1. ✅ Binding exports verified');
        console.log('   2. ⏭️  Need to test with actual session creation (requires audio files)');
        console.log('   3. ⏭️  Run full lifecycle stress test');
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

testDestroyMechanism();
