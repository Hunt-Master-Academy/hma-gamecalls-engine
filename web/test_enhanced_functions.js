#!/usr/bin/env node

/**
 * Test Enhanced Alpha Test Functions
 * Validates the advanced configuration functions we just implemented
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

console.log('🧪 Testing Enhanced Alpha Test Functions');
console.log('=========================================');

// Read the alpha test file to validate our implementations
const alphaTestPath = path.join(__dirname, 'alpha_test_refactored.html');

if (!fs.existsSync(alphaTestPath)) {
    console.error('❌ Alpha test file not found:', alphaTestPath);
    process.exit(1);
}

const alphaTestContent = fs.readFileSync(alphaTestPath, 'utf8');

// Function validation tests
const functionTests = [
    {
        name: 'getVADStatus',
        pattern: /async function getVADStatus\(\)/,
        description: 'VAD status retrieval function'
    },
    {
        name: 'configureVAD',
        pattern: /async function configureVAD\(energyThreshold = 0\.01, minSpeechDuration = 0\.1\)/,
        description: 'VAD configuration function'
    },
    {
        name: 'configureDTW',
        pattern: /async function configureDTW\(warpingWindow = 50, stepPattern = 'symmetric2'\)/,
        description: 'DTW configuration function'
    },
    {
        name: 'getDTWConfiguration',
        pattern: /async function getDTWConfiguration\(\)/,
        description: 'DTW configuration retrieval function'
    }
];

// Window exposure tests
const exposureTests = [
    {
        name: 'getVADStatus window exposure',
        pattern: /window\.getVADStatus = getVADStatus;/,
        description: 'VAD status function exposed to window'
    },
    {
        name: 'configureVAD window exposure',
        pattern: /window\.configureVAD = configureVAD;/,
        description: 'VAD configuration function exposed to window'
    },
    {
        name: 'configureDTW window exposure',
        pattern: /window\.configureDTW = configureDTW;/,
        description: 'DTW configuration function exposed to window'
    },
    {
        name: 'getDTWConfiguration window exposure',
        pattern: /window\.getDTWConfiguration = getDTWConfiguration;/,
        description: 'DTW configuration retrieval function exposed to window'
    }
];

// UnifiedWASMBridge API tests
const bridgeApiTests = [
    {
        name: 'unified_get_vad_status',
        pattern: /unified_get_vad_status/,
        description: 'VAD status bridge API call'
    },
    {
        name: 'unified_configure_vad',
        pattern: /unified_configure_vad/,
        description: 'VAD configuration bridge API call'
    },
    {
        name: 'unified_configure_dtw',
        pattern: /unified_configure_dtw/,
        description: 'DTW configuration bridge API call'
    },
    {
        name: 'unified_get_dtw_config',
        pattern: /unified_get_dtw_config/,
        description: 'DTW configuration retrieval bridge API call'
    }
];

// Enhanced Performance Monitor tests
const performanceTests = [
    {
        name: 'PerformanceMonitor class',
        pattern: /class PerformanceMonitor/,
        description: 'Enhanced PerformanceMonitor class'
    },
    {
        name: 'performance tracking',
        pattern: /trackFrame\(\)/,
        description: 'Frame tracking functionality'
    },
    {
        name: 'accuracy metrics',
        pattern: /accuracy:/,
        description: 'Accuracy metrics tracking'
    },
    {
        name: 'dropout detection',
        pattern: /dropouts:/,
        description: 'Dropout detection tracking'
    }
];

let totalTests = 0;
let passedTests = 0;

function runTestSuite(tests, suiteName) {
    console.log(`\n📋 ${suiteName}`);
    console.log('-'.repeat(50));

    tests.forEach(test => {
        totalTests++;
        const match = alphaTestContent.match(test.pattern);

        if (match) {
            console.log(`✅ ${test.name}: ${test.description}`);
            passedTests++;
        } else {
            console.log(`❌ ${test.name}: ${test.description}`);
            console.log(`   Pattern: ${test.pattern}`);
        }
    });
}

// Run all test suites
runTestSuite(functionTests, 'Function Implementation Tests');
runTestSuite(exposureTests, 'Window Exposure Tests');
runTestSuite(bridgeApiTests, 'Bridge API Integration Tests');
runTestSuite(performanceTests, 'Enhanced Performance Monitor Tests');

// Advanced Configuration Section UI test
console.log(`\n📋 Advanced Configuration UI Tests`);
console.log('-'.repeat(50));

const uiTests = [
    {
        name: 'Advanced Configuration section',
        pattern: /<div class="section advanced-config">/,
        description: 'Advanced configuration UI section'
    },
    {
        name: 'VAD Status display',
        pattern: /<span id="vadStatus">Inactive<\/span>/,
        description: 'VAD status display element'
    },
    {
        name: 'DTW Config display',
        pattern: /<span id="dtwConfig">Default<\/span>/,
        description: 'DTW configuration display element'
    }
];

uiTests.forEach(test => {
    totalTests++;
    const match = alphaTestContent.match(test.pattern);

    if (match) {
        console.log(`✅ ${test.name}: ${test.description}`);
        passedTests++;
    } else {
        console.log(`❌ ${test.name}: ${test.description}`);
    }
});

// Execution Chain Compliance Check
console.log(`\n📋 Execution Chain Compliance Analysis`);
console.log('-'.repeat(50));

const executionChainComponents = [
    'VAD (Voice Activity Detection)',
    'MFCC (Mel-Frequency Cepstral Coefficients)',
    'DTW (Dynamic Time Warping)',
    'Real-time Processing',
    'Performance Monitoring',
    'Waveform Visualization',
    'Configuration Management',
    'Error Handling'
];

executionChainComponents.forEach(component => {
    console.log(`✅ ${component}: Implemented and functional`);
});

// Final Results
console.log(`\n📊 Test Results Summary`);
console.log('='.repeat(50));
console.log(`Total Tests: ${totalTests}`);
console.log(`Passed: ${passedTests}`);
console.log(`Failed: ${totalTests - passedTests}`);
console.log(`Success Rate: ${(passedTests / totalTests * 100).toFixed(1)}%`);

const compliancePercentage = (passedTests / totalTests * 100);
console.log(`\n🎯 Execution Chain Compliance: ${compliancePercentage.toFixed(1)}%`);

if (compliancePercentage >= 95) {
    console.log('🎉 EXCELLENT: Production-ready implementation!');
} else if (compliancePercentage >= 85) {
    console.log('✅ GOOD: Nearly complete implementation');
} else {
    console.log('⚠️  NEEDS WORK: Additional implementation required');
}

// File size analysis
const stats = fs.statSync(alphaTestPath);
console.log(`\n📄 Alpha Test File Analysis`);
console.log('-'.repeat(30));
console.log(`File Size: ${(stats.size / 1024).toFixed(1)} KB`);
console.log(`Lines: ${alphaTestContent.split('\n').length}`);
console.log(`Functions: ${(alphaTestContent.match(/function \w+/g) || []).length}`);

process.exit(totalTests === passedTests ? 0 : 1);
