#!/usr/bin/env node
/**
 * [20251101-FEATURES-003] Feature Extraction CLI
 * Extract and store acoustic features for master calls
 * 
 * Usage:
 *   node scripts/extract-features.js --all                    # Extract all missing
 *   node scripts/extract-features.js --call call_turkey_cluck_001
 *   node scripts/extract-features.js --species turkey         # All turkey calls
 */

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');

const FeatureExtractionService = require(path.join(servicesPath, 'featureExtractionService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));

async function main() {
    const args = process.argv.slice(2);
    
    console.log('🔬 Feature Extraction Tool\n');
    console.log('═══════════════════════════════════════════════════════════════\n');
    
    try {
        if (args.includes('--all')) {
            // Extract features for all calls missing them
            console.log('Extracting features for all master calls without features...\n');
            const results = await FeatureExtractionService.extractMissingFeatures();
            
            console.log('\n═══════════════════════════════════════════════════════════════');
            console.log('📊 Batch Extraction Summary\n');
            console.log(`  Total Calls: ${results.total}`);
            console.log(`  ✅ Successful: ${results.successful}`);
            console.log(`  ❌ Failed: ${results.failed}`);
            
            if (results.errors.length > 0) {
                console.log('\n  Errors:');
                results.errors.forEach(err => {
                    console.log(`    • ${err.callId}: ${err.error}`);
                });
            }
            
        } else if (args.includes('--call')) {
            // Extract single call
            const callIdIndex = args.indexOf('--call') + 1;
            const callId = args[callIdIndex];
            
            if (!callId) {
                throw new Error('--call requires a call ID');
            }
            
            console.log(`Extracting features for ${callId}...\n`);
            const features = await FeatureExtractionService.extractFeatures(callId);
            
            console.log('\n✅ Feature Extraction Complete!\n');
            console.log('Feature Summary:');
            console.log(`  MFCC Dimensions: ${features.mfccMean.length}`);
            console.log(`  RMS Energy: ${features.rmsMean.toFixed(4)} ± ${features.rmsStd.toFixed(4)}`);
            
            if (features.durationSeconds != null) {
                console.log(`  Duration: ${Number(features.durationSeconds).toFixed(2)}s`);
            }
            if (features.numFrames != null) {
                console.log(`  Frames: ${features.numFrames}`);
            }
            
            if (features.f0Mean != null) {
                console.log(`  Pitch (F0): ${features.f0Mean.toFixed(1)} Hz (${features.f0Min.toFixed(1)} - ${features.f0Max.toFixed(1)} Hz)`);
            }
            
            if (features.tempoBpm != null) {
                console.log(`  Tempo: ${features.tempoBpm.toFixed(1)} BPM`);
            }
            
        } else if (args.includes('--species')) {
            // Extract all calls for a species
            const speciesIndex = args.indexOf('--species') + 1;
            const species = args[speciesIndex];
            
            if (!species) {
                throw new Error('--species requires a species name');
            }
            
            console.log(`Finding ${species} calls...\n`);
            const callsResult = await GameCallsService.listCalls({ 
                species, 
                pageSize: 100 
            });
            
            const callIds = callsResult.calls.map(c => c.id);
            console.log(`Found ${callIds.length} ${species} calls\n`);
            
            const results = await FeatureExtractionService.batchExtract(callIds);
            
            console.log('\n═══════════════════════════════════════════════════════════════');
            console.log(`📊 ${species} Feature Extraction Summary\n`);
            console.log(`  Total Calls: ${results.total}`);
            console.log(`  ✅ Successful: ${results.successful}`);
            console.log(`  ❌ Failed: ${results.failed}`);
            
        } else {
            // Show usage
            console.log('Usage:');
            console.log('  node scripts/extract-features.js --all');
            console.log('  node scripts/extract-features.js --call <callId>');
            console.log('  node scripts/extract-features.js --species <speciesName>');
            console.log('\nExamples:');
            console.log('  node scripts/extract-features.js --all');
            console.log('  node scripts/extract-features.js --call call_turkey_cluck_001');
            console.log('  node scripts/extract-features.js --species turkey');
            process.exit(1);
        }
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n❌ Error:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

main();
