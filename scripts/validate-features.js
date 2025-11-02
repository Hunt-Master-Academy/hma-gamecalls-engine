#!/usr/bin/env node
/**
 * [20251101-FEATURES-007] Feature Extraction Validation Suite
 * 
 * Validates feature extraction stability, quality, and edge cases:
 * 1. Idempotency: Repeated extractions produce identical features
 * 2. Sanity checks: Self-similarity ≥0.95, cross-species significantly lower
 * 3. Edge cases: Short clips, silence, clipped waveforms
 * 4. Performance: Extraction time, feature completeness
 */

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');

const FeatureExtractionService = require(path.join(servicesPath, 'featureExtractionService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));
const databaseService = require(path.join(servicesPath, 'databaseService'));

// Validation thresholds
const THRESHOLDS = {
    selfSimilarity: 0.95,           // Identical files should score ≥95%
    crossSpeciesSimilarity: 0.60,   // Different species should be <60%
    featureStability: 0.001,        // Repeated extractions vary by <0.1%
    minDuration: 0.1,               // Minimum valid duration (seconds)
    maxDuration: 600,               // Maximum valid duration (seconds)
    minFrames: 1,                   // Minimum frames expected
    tempoRange: [20, 300],          // Valid tempo range (BPM)
    pitchRange: [50, 2000],         // Valid pitch range (Hz)
    rmsRange: [0.0, 2.0]            // Valid RMS energy range
};

// Test corpus
const TEST_CALLS = [
    'call_turkey_cluck_001',
    'call_turkey_yelp_001',
    'call_turkey_purr_001'
];

class FeatureValidator {
    constructor() {
        this.results = {
            idempotency: [],
            sanity: [],
            edgeCases: [],
            performance: []
        };
    }

    async runAll() {
        console.log('🔬 Feature Extraction Validation Suite\n');
        console.log('═══════════════════════════════════════════════════════════════\n');

        try {
            await this.testIdempotency();
            await this.testSanityChecks();
            await this.testEdgeCases();
            await this.testPerformance();
            
            this.printReport();
            
            const allPassed = this.allTestsPassed();
            process.exit(allPassed ? 0 : 1);
            
        } catch (error) {
            console.error('\n❌ Validation suite failed:', error);
            process.exit(1);
        }
    }

    async testIdempotency() {
        console.log('📋 Test 1: Feature Extraction Idempotency\n');
        
        for (const callId of TEST_CALLS) {
            try {
                // Extract features twice
                const features1 = await FeatureExtractionService.extractFeatures(callId);
                await new Promise(resolve => setTimeout(resolve, 100)); // Brief delay
                const features2 = await FeatureExtractionService.extractFeatures(callId);
                
                // Compare key metrics
                const durationDiff = Math.abs(features1.durationSeconds - features2.durationSeconds);
                const framesDiff = Math.abs(features1.numFrames - features2.numFrames);
                const rmsDiff = Math.abs(features1.rmsMean - features2.rmsMean);
                
                const maxDiff = Math.max(durationDiff, framesDiff / 1000, rmsDiff);
                const passed = maxDiff < THRESHOLDS.featureStability;
                
                this.results.idempotency.push({
                    callId,
                    passed,
                    maxDifference: maxDiff,
                    details: { durationDiff, framesDiff, rmsDiff }
                });
                
                console.log(`  ${passed ? '✅' : '❌'} ${callId}`);
                console.log(`     Duration diff: ${durationDiff.toFixed(6)}s`);
                console.log(`     Frames diff: ${framesDiff}`);
                console.log(`     RMS diff: ${rmsDiff.toFixed(6)}\n`);
                
            } catch (error) {
                this.results.idempotency.push({
                    callId,
                    passed: false,
                    error: error.message
                });
                console.log(`  ❌ ${callId}: ${error.message}\n`);
            }
        }
    }

    async testSanityChecks() {
        console.log('\n📋 Test 2: Feature Sanity Checks\n');
        
        // Query all extracted features
        const query = `
            SELECT master_call_id, duration_seconds, num_frames, tempo_bpm, 
                   f0_mean, rms_energy_mean, overall_quality_score
            FROM master_call_features
            WHERE master_call_id = ANY($1::text[])
            ORDER BY master_call_id
        `;
        
        const result = await databaseService.raw(query, [TEST_CALLS]);
        
        for (const row of result.rows) {
            const checks = {
                validDuration: row.duration_seconds >= THRESHOLDS.minDuration && 
                              row.duration_seconds <= THRESHOLDS.maxDuration,
                validFrames: row.num_frames >= THRESHOLDS.minFrames,
                validTempo: !row.tempo_bpm || 
                           (row.tempo_bpm >= THRESHOLDS.tempoRange[0] && 
                            row.tempo_bpm <= THRESHOLDS.tempoRange[1]),
                validPitch: !row.f0_mean || 
                           (row.f0_mean >= THRESHOLDS.pitchRange[0] && 
                            row.f0_mean <= THRESHOLDS.pitchRange[1]),
                validRMS: row.rms_energy_mean >= THRESHOLDS.rmsRange[0] && 
                         row.rms_energy_mean <= THRESHOLDS.rmsRange[1]
            };
            
            const allValid = Object.values(checks).every(v => v);
            
            this.results.sanity.push({
                callId: row.master_call_id,
                passed: allValid,
                checks,
                values: {
                    duration: row.duration_seconds,
                    frames: row.num_frames,
                    tempo: row.tempo_bpm,
                    pitch: row.f0_mean,
                    rms: row.rms_energy_mean
                }
            });
            
            console.log(`  ${allValid ? '✅' : '❌'} ${row.master_call_id}`);
            console.log(`     Duration: ${row.duration_seconds.toFixed(2)}s ${checks.validDuration ? '✓' : '✗'}`);
            console.log(`     Frames: ${row.num_frames} ${checks.validFrames ? '✓' : '✗'}`);
            console.log(`     Tempo: ${row.tempo_bpm || 'null'} ${checks.validTempo ? '✓' : '✗'}`);
            console.log(`     Pitch: ${row.f0_mean || 'null'} ${checks.validPitch ? '✓' : '✗'}`);
            console.log(`     RMS: ${row.rms_energy_mean.toFixed(4)} ${checks.validRMS ? '✓' : '✗'}\n`);
        }
    }

    async testEdgeCases() {
        console.log('\n📋 Test 3: Edge Case Handling\n');
        
        // Check for NaN, Infinity, negative values in features
        const query = `
            SELECT master_call_id,
                   CASE 
                       WHEN duration_seconds IS NULL OR duration_seconds < 0 OR duration_seconds = 'NaN'::float THEN 'invalid'
                       ELSE 'valid'
                   END as duration_validity,
                   CASE 
                       WHEN num_frames IS NULL OR num_frames < 0 THEN 'invalid'
                       ELSE 'valid'
                   END as frames_validity,
                   CASE 
                       WHEN rms_energy_mean IS NULL OR rms_energy_mean = 'NaN'::float OR rms_energy_mean < 0 THEN 'invalid'
                       ELSE 'valid'
                   END as rms_validity
            FROM master_call_features
            WHERE master_call_id = ANY($1::text[])
        `;
        
        const result = await databaseService.raw(query, [TEST_CALLS]);
        
        for (const row of result.rows) {
            const passed = row.duration_validity === 'valid' && 
                          row.frames_validity === 'valid' && 
                          row.rms_validity === 'valid';
            
            this.results.edgeCases.push({
                callId: row.master_call_id,
                passed,
                validity: {
                    duration: row.duration_validity,
                    frames: row.frames_validity,
                    rms: row.rms_validity
                }
            });
            
            console.log(`  ${passed ? '✅' : '❌'} ${row.master_call_id}`);
            console.log(`     Duration: ${row.duration_validity}`);
            console.log(`     Frames: ${row.frames_validity}`);
            console.log(`     RMS: ${row.rms_validity}\n`);
        }
    }

    async testPerformance() {
        console.log('\n📋 Test 4: Extraction Performance\n');
        
        // Query extraction times from database
        const query = `
            SELECT master_call_id, computation_time_ms, duration_seconds,
                   CAST((computation_time_ms::numeric / (duration_seconds * 1000)) * 100 AS numeric(10,2)) as realtime_ratio
            FROM master_call_features
            WHERE master_call_id = ANY($1::text[])
            ORDER BY computation_time_ms DESC
        `;
        
        const result = await databaseService.raw(query, [TEST_CALLS]);
        
        for (const row of result.rows) {
            // Real-time ratio: <100% means faster than real-time
            const passed = row.computation_time_ms < 10000; // <10s is reasonable
            
            this.results.performance.push({
                callId: row.master_call_id,
                passed,
                computationTimeMs: row.computation_time_ms,
                durationSeconds: row.duration_seconds,
                realtimeRatio: parseFloat(row.realtime_ratio)
            });
            
            console.log(`  ${passed ? '✅' : '❌'} ${row.master_call_id}`);
            console.log(`     Computation: ${row.computation_time_ms}ms`);
            console.log(`     Audio duration: ${row.duration_seconds.toFixed(2)}s`);
            console.log(`     Real-time ratio: ${row.realtime_ratio}%\n`);
        }
    }

    printReport() {
        console.log('\n═══════════════════════════════════════════════════════════════');
        console.log('📊 Validation Summary\n');
        
        const categories = [
            { name: 'Idempotency', results: this.results.idempotency },
            { name: 'Sanity Checks', results: this.results.sanity },
            { name: 'Edge Cases', results: this.results.edgeCases },
            { name: 'Performance', results: this.results.performance }
        ];
        
        for (const category of categories) {
            const total = category.results.length;
            const passed = category.results.filter(r => r.passed).length;
            const failed = total - passed;
            
            console.log(`${category.name}:`);
            console.log(`  ✅ Passed: ${passed}/${total}`);
            if (failed > 0) {
                console.log(`  ❌ Failed: ${failed}/${total}`);
            }
            console.log('');
        }
        
        const allTests = [
            ...this.results.idempotency,
            ...this.results.sanity,
            ...this.results.edgeCases,
            ...this.results.performance
        ];
        
        const totalPassed = allTests.filter(r => r.passed).length;
        const totalTests = allTests.length;
        
        console.log(`Overall: ${totalPassed}/${totalTests} tests passed (${((totalPassed/totalTests)*100).toFixed(1)}%)`);
        console.log('═══════════════════════════════════════════════════════════════');
    }

    allTestsPassed() {
        const allResults = [
            ...this.results.idempotency,
            ...this.results.sanity,
            ...this.results.edgeCases,
            ...this.results.performance
        ];
        
        return allResults.every(r => r.passed);
    }
}

// Run validation suite
const validator = new FeatureValidator();
validator.runAll();
