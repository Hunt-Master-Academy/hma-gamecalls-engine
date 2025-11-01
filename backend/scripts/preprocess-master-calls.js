#!/usr/bin/env node
/**
 * [20251229-PREPROCESS-011] CLI tool to preprocess master calls
 * Usage: node scripts/preprocess-master-calls.js [call_id]
 */

require('dotenv').config();

const preprocessor = require('../src/services/masterCallPreprocessor');
const databaseService = require('../src/services/databaseService');

async function main() {
    const args = process.argv.slice(2);
    const specificCallId = args[0];

    try {
        // Initialize database
        databaseService.initialize();

        if (specificCallId) {
            // Process specific master call
            console.log(`🎯 Processing specific master call: ${specificCallId}\n`);

            const query = 'SELECT * FROM master_calls WHERE id = $1';
            const result = await databaseService.raw(query, [specificCallId]);

            if (result.rows.length === 0) {
                console.error(`❌ Master call not found: ${specificCallId}`);
                process.exit(1);
            }

            const masterCall = result.rows[0];
            const processingResult = await preprocessor.preprocessMasterCall(masterCall);

            if (processingResult.success) {
                console.log(`\n✅ Successfully preprocessed: ${specificCallId}`);
                console.log(`   Normalized ID: ${processingResult.normalizedId}`);
                process.exit(0);
            } else {
                console.error(`\n❌ Failed to preprocess: ${specificCallId}`);
                console.error(`   Error: ${processingResult.error}`);
                process.exit(1);
            }

        } else {
            // Process all master calls
            const results = await preprocessor.preprocessAllMasterCalls();

            if (results.failed > 0) {
                console.log('\n⚠️  Some master calls failed to preprocess:');
                results.details
                    .filter(d => !d.success && d.error)
                    .forEach(d => {
                        console.log(`  - ${d.id}: ${d.error}`);
                    });
            }

            process.exit(results.failed > 0 ? 1 : 0);
        }

    } catch (error) {
        console.error('\n❌ Fatal error:', error.message);
        console.error(error.stack);
        process.exit(1);
    } finally {
        await databaseService.close();
    }
}

main();
