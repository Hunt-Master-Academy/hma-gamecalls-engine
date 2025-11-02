#!/usr/bin/env node
/**
 * Build Version Verification Script
 * 
 * Verifies that the C++ engine binary has been rebuilt by checking:
 * 1. Build timestamp from getEngineInfo()
 * 2. Fix version identifier
 * 3. Binary file timestamp
 * 
 * Run this after rebuilding to confirm the new code is active.
 */

const path = require('path');
const fs = require('fs');

console.log('🔍 Build Version Verification');
console.log('=' .repeat(70));

try {
    // Load the binding
    const bindingPath = path.join(__dirname, '../bindings/node-api/build/Release/gamecalls_engine.node');
    console.log(`\n📦 Binding Path: ${bindingPath}`);
    
    // Check if file exists
    if (!fs.existsSync(bindingPath)) {
        console.error('❌ Binding file not found!');
        process.exit(1);
    }
    
    // Get file stats
    const stats = fs.statSync(bindingPath);
    console.log(`📅 File Modified: ${stats.mtime.toISOString()}`);
    console.log(`📏 File Size: ${(stats.size / 1024 / 1024).toFixed(2)} MB`);
    
    // Load and get engine info
    const binding = require(bindingPath);
    console.log('\n✅ Binding loaded successfully');
    
    // Call getEngineInfo
    const info = binding.getEngineInfo ? binding.getEngineInfo() : null;
    
    if (info) {
        console.log('\n🔧 Engine Info:');
        console.log(`   Version: ${info.version || 'N/A'}`);
        console.log(`   Build Type: ${info.buildType || 'N/A'}`);
        console.log(`   Build Timestamp: ${info.buildTimestamp || 'N/A'}`);
        console.log(`   Fix Version: ${info.fixVersion || 'N/A'}`);
        console.log(`   C++ Standard: ${info.cppStandard || 'N/A'}`);
        
        // Check for FIX-033
        if (info.fixVersion === 'FIX-033') {
            console.log('\n✅ FIX-033 is active (pragmatic override + build timestamp)');
        } else {
            console.log(`\n⚠️  Expected FIX-033, got: ${info.fixVersion || 'none'}`);
        }
    } else {
        console.log('\n⚠️  getEngineInfo() not available or returned null');
    }
    
    console.log('\n' + '='.repeat(70));
    console.log('✅ Verification complete');
    
} catch (error) {
    console.error('\n❌ Error:', error.message);
    console.error(error.stack);
    process.exit(1);
}
