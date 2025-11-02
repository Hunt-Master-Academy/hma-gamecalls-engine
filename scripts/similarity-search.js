#!/usr/bin/env node
/**
 * [20251101-FEATURES-011] Feature-Based Similarity Search Demo
 * 
 * Demonstrates fast similarity search using pre-computed features:
 * 1. Filter by metadata (species, duration range)
 * 2. Compute feature vector distance (Euclidean)
 * 3. Rank and return top-N similar calls
 */

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');
const databaseService = require(path.join(servicesPath, 'databaseService'));

async function findSimilarCalls(targetCallId, options = {}) {
    const {
        maxResults = 5,
        sameSpeciesOnly = false,
        maxDurationDiff = 2.0,  // seconds
        minQualityScore = 0.0
    } = options;

    console.log(`\n🔍 Finding similar calls to: ${targetCallId}\n`);
    console.log('═══════════════════════════════════════════════════════════════\n');

    // Get target call features
    const targetQuery = `
        SELECT * FROM v_feature_search_candidates
        WHERE master_call_id = $1
    `;
    const targetResult = await databaseService.raw(targetQuery, [targetCallId]);
    
    if (targetResult.rows.length === 0) {
        throw new Error(`Call ${targetCallId} not found or has no extracted features`);
    }
    
    const target = targetResult.rows[0];
    console.log(`Target: ${target.name} (${target.species})`);
    console.log(`  Duration: ${target.duration_seconds?.toFixed(2)}s`);
    console.log(`  Tempo: ${target.tempo_bpm || 'N/A'}`);
    console.log(`  RMS Energy: ${target.rms_energy_mean?.toFixed(4) || 'N/A'}`);
    console.log(`  Pitch: ${target.f0_mean?.toFixed(1) || 'N/A'} Hz\n`);

    // Build similarity search query with filters
    const candidatesQuery = `
        WITH target AS (
            SELECT feature_vector, species, duration_seconds
            FROM v_feature_search_candidates
            WHERE master_call_id = $1
        ),
        candidates AS (
            SELECT 
                c.master_call_id,
                c.name,
                c.species,
                c.call_type,
                c.duration_seconds,
                c.tempo_bpm,
                c.rms_energy_mean,
                c.f0_mean,
                c.overall_quality_score,
                c.feature_vector,
                -- Euclidean distance in feature space
                sqrt(
                    power(c.feature_vector[1] - t.feature_vector[1], 2) +
                    power(c.feature_vector[2] - t.feature_vector[2], 2) +
                    power(c.feature_vector[3] - t.feature_vector[3], 2) +
                    power(c.feature_vector[4] - t.feature_vector[4], 2)
                ) as feature_distance,
                -- Duration similarity (0-1, where 1 = identical)
                1.0 - (abs(c.duration_seconds - t.duration_seconds) / GREATEST(c.duration_seconds, t.duration_seconds)) as duration_similarity
            FROM v_feature_search_candidates c
            CROSS JOIN target t
            WHERE c.master_call_id != $1  -- Exclude self
                ${sameSpeciesOnly ? 'AND c.species = t.species' : ''}
                AND abs(c.duration_seconds - t.duration_seconds) <= $2
                AND COALESCE(c.overall_quality_score, 0) >= $3
        )
        SELECT 
            master_call_id,
            name,
            species,
            call_type,
            duration_seconds,
            tempo_bpm,
            rms_energy_mean,
            f0_mean,
            overall_quality_score,
            feature_distance,
            duration_similarity,
            -- Combined similarity score (lower distance = higher similarity)
            (1.0 - feature_distance) * 0.7 + duration_similarity * 0.3 as combined_similarity
        FROM candidates
        ORDER BY feature_distance ASC
        LIMIT $4
    `;

    const result = await databaseService.raw(candidatesQuery, [
        targetCallId,
        maxDurationDiff,
        minQualityScore,
        maxResults
    ]);

    console.log(`Found ${result.rows.length} similar calls:\n`);

    for (let i = 0; i < result.rows.length; i++) {
        const call = result.rows[i];
        const similarity = (call.combined_similarity * 100).toFixed(1);
        
        console.log(`${i + 1}. ${call.name} (${call.species})`);
        console.log(`   Similarity: ${similarity}%`);
        console.log(`   Call Type: ${call.call_type || 'N/A'}`);
        console.log(`   Duration: ${call.duration_seconds?.toFixed(2)}s`);
        console.log(`   Feature Distance: ${call.feature_distance?.toFixed(4)}`);
        console.log(`   Duration Match: ${(call.duration_similarity * 100).toFixed(1)}%`);
        if (call.tempo_bpm) {
            console.log(`   Tempo: ${call.tempo_bpm.toFixed(1)} BPM`);
        }
        if (call.f0_mean) {
            console.log(`   Pitch: ${call.f0_mean.toFixed(1)} Hz`);
        }
        console.log('');
    }

    return result.rows;
}

async function main() {
    const args = process.argv.slice(2);
    
    if (args.length === 0) {
        console.log('Usage: node scripts/similarity-search.js <callId> [options]');
        console.log('\nOptions:');
        console.log('  --max-results <n>        Maximum results (default: 5)');
        console.log('  --same-species           Only search within same species');
        console.log('  --max-duration-diff <n>  Max duration difference in seconds (default: 2.0)');
        console.log('\nExample:');
        console.log('  node scripts/similarity-search.js call_turkey_cluck_001 --same-species --max-results 10');
        process.exit(1);
    }

    const callId = args[0];
    const options = {
        maxResults: parseInt(args[args.indexOf('--max-results') + 1]) || 5,
        sameSpeciesOnly: args.includes('--same-species'),
        maxDurationDiff: parseFloat(args[args.indexOf('--max-duration-diff') + 1]) || 2.0
    };

    try {
        await findSimilarCalls(callId, options);
        process.exit(0);
    } catch (error) {
        console.error('\n❌ Error:', error.message);
        process.exit(1);
    }
}

main();
