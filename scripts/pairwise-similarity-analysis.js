#!/usr/bin/env node
// [20251030-ANALYSIS-001] Pairwise similarity matrix for turkey calls

require('dotenv').config({ path: '../backend/.env' });

// [20251102-DOCKER-005] Auto-detect Docker vs host environment
// When NODE_ENV=production, assume running in Docker with internal networking
if (!process.env.REDIS_HOST) {
    if (process.env.NODE_ENV === 'production') {
        // Running inside Docker container - use internal hostnames
        process.env.REDIS_HOST = 'redis';
        process.env.DB_HOST = 'postgres';
        process.env.MINIO_ENDPOINT = 'minio';
    } else {
        // Running on host - use localhost
        process.env.REDIS_HOST = 'localhost';
    }
}
if (!process.env.REDIS_PORT) {
    process.env.REDIS_PORT = '6379';
}

// [20251102-DOCKER-006] Adjust module paths for Docker vs host
const path = require('path');
let servicesPath;
if (process.env.NODE_ENV === 'production') {
    // Docker: scripts at /app/scripts, services at /app/src/services
    servicesPath = path.join(__dirname, '../src/services');
} else {
    // Host: scripts at ../scripts, services at ../backend/src/services
    servicesPath = path.join(__dirname, '../backend/src/services');
}

const SessionsService = require(path.join(servicesPath, 'sessionsService'));
const GameCallsService = require(path.join(servicesPath, 'gameCallsService'));
const MinIOService = require(path.join(servicesPath, 'minioService'));
const fs = require('fs');
const os = require('os');

function bufferToFloat32(buffer) {
    if (buffer.length <= 44) {
        return new Float32Array(0);
    }

    const data = buffer.subarray(44); // skip WAV header
    const sampleCount = Math.floor(data.length / 2);
    const float32 = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
        const sample = data.readInt16LE(i * 2);
        float32[i] = sample / 32768;
    }

    return float32;
}

async function generatePairwiseMatrix() {
    console.log('🔬 Turkey Call Pairwise Similarity Analysis\n');
    
    // [20251031-BATCH-001] Parse command line arguments for batch mode
    const args = process.argv.slice(2);
    const batchMode = args.includes('--batch-mode');
    const masterCallsArg = args.find(a => a.startsWith('--master-calls='));
    const outputArg = args.find(a => a.startsWith('--output='));
    
    let masterCallIds = null;
    let outputFile = null;
    
    if (batchMode) {
        if (!masterCallsArg || !outputArg) {
            throw new Error('Batch mode requires --master-calls=id1,id2,id3 and --output=/path/to/output.json');
        }
        masterCallIds = masterCallsArg.split('=')[1].split(',');
        outputFile = outputArg.split('=')[1];
        console.log(`📦 BATCH MODE: Processing ${masterCallIds.length} master calls`);
        console.log(`   Output: ${outputFile}\n`);
    }
    
    try {
        // Get all turkey calls sorted by duration (longest first for reliability)
        const { calls } = await GameCallsService.listCalls({
            species: 'wild_turkey',
            pageSize: 20,
            sortBy: 'duration_seconds',
            sortOrder: 'desc'
        });

        if (!calls || calls.length === 0) {
            throw new Error('No turkey calls found in database');
        }

        // [20251031-BATCH-002] Filter to batch master calls if specified
        let masterCalls = calls;
        if (batchMode && masterCallIds) {
            masterCalls = calls.filter(c => masterCallIds.includes(c.id));
            console.log(`Filtered to ${masterCalls.length} master calls for this batch\n`);
        }

        console.log(`Found ${calls.length} turkey calls\n`);

        // Debug: Check first call structure
        if (calls.length > 0) {
            console.log('Sample call structure:', {
                id: calls[0].id,
                name: calls[0].name,
                audio_file_path: calls[0].audio_file_path
            });
        }

        // Initialize results matrix
        const matrix = [];
        const callNames = calls.map(c => c.name);
        const callTypes = calls.map(c => c.call_type);
        
        // [20251031-BATCH-003] In batch mode, only process specified master calls
        const batchResults = {};
        
        console.log('🎯 Starting pairwise comparisons...\n');

        // Compare each call to every other call (including self for baseline)
        for (let i = 0; i < masterCalls.length; i++) {
            const row = [];
            const masterCall = masterCalls[i];
            
            console.log(`\n[${i + 1}/${masterCalls.length}] Master: ${masterCall.name} (${masterCall.call_type})`);
            
            // Initialize batch results for this master call
            if (batchMode) {
                batchResults[masterCall.name] = {};
            }
            
            for (let j = 0; j < calls.length; j++) {
                const testCall = calls[j];
                
                let sessionId = null;
                try {
                    // Create session with master call ID as first parameter
                    const session = await SessionsService.createSession(masterCall.id, {
                        userId: 'test-pairwise-analysis'
                    });
                    sessionId = session.id;

                    // Start session
                    await SessionsService.startSession(sessionId);

                    // Download test call audio from MinIO
                    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'pairwise-'));
                    const tempFile = path.join(tempDir, `${testCall.id}.wav`);
                    
                    // [20251030-ANALYSIS-003] Normalize object key to handle legacy master-calls prefix
                    const objectKey = (testCall.audio_file_path || '').replace(/^master-calls\//, '');
                    const stream = await MinIOService.getObjectStream('gamecalls-master-calls', objectKey);
                    const writeStream = fs.createWriteStream(tempFile);
                    
                    await new Promise((resolve, reject) => {
                        stream.pipe(writeStream);
                        stream.on('error', reject);
                        writeStream.on('finish', resolve);
                        writeStream.on('error', reject);
                    });

                    const fileBuffer = fs.readFileSync(tempFile);
                    fs.rmSync(tempDir, { recursive: true, force: true });

                    const floatSamples = bufferToFloat32(fileBuffer);
                    if (floatSamples.length === 0) {
                        throw new Error('Audio file contained no PCM samples');
                    }

                    const chunkSize = 8192 * 4;
                    let finalSimilarity = 0;
                    let chunksProcessed = 0;

                    for (let offset = 0; offset < floatSamples.length; offset += chunkSize) {
                        const end = Math.min(offset + chunkSize, floatSamples.length);
                        const chunk = floatSamples.subarray(offset, end);

                        if (chunk.length > 0) {
                            const result = await SessionsService.processAudioData(sessionId, chunk);
                            finalSimilarity = result.similarity.overall;
                            chunksProcessed++;
                        }
                    }

                    // Finalize to get comprehensive analysis
                    const analysis = await SessionsService.finalizeSession(sessionId);
                    const finalScore = analysis.similarity || finalSimilarity;

                    // Stop session
                    await SessionsService.stopSession(sessionId);
                    
                    // [20251102-FIX-001] CRITICAL: Destroy session to prevent C++ resource exhaustion
                    // Without this, sessions accumulate and hit the 1000 session limit after ~217 comparisons
                    await SessionsService.deleteSession(sessionId);

                    row.push({
                        score: finalScore,
                        chunks: chunksProcessed,
                        valid: analysis.valid
                    });
                    
                    // [20251031-BATCH-004] Store result in batch results object
                    if (batchMode) {
                        batchResults[masterCall.name][testCall.name] = finalScore;
                    }

                    const emoji = i === j ? '🎯' : (finalScore > 0.8 ? '✅' : finalScore > 0.5 ? '⚠️' : '❌');
                    console.log(`   ${emoji} vs ${testCall.name}: ${(finalScore * 100).toFixed(1)}% (${chunksProcessed} chunks)`);

                } catch (error) {
                    console.error(`   ❌ Error comparing with ${testCall.name}:`, error.message);
                    if (error?.stack) {
                        console.error(error.stack.split('\n').slice(1, 3).join('\n'));
                    }
                    
                    // [20251102-FIX-002] Cleanup session on error to prevent leaks
                    if (sessionId) {
                        try {
                            await SessionsService.deleteSession(sessionId);
                            console.log(`   🗑️  Cleaned up failed session ${sessionId}`);
                        } catch (cleanupError) {
                            console.error(`   ⚠️  Failed to cleanup session: ${cleanupError.message}`);
                        }
                    }
                    
                    // [20251031-BATCH-005] Store error in batch results
                    if (batchMode) {
                        batchResults[masterCall.name][testCall.name] = 'ERROR';
                    }
                    
                    row.push({ score: -1, chunks: 0, valid: false, error: error.message });
                }
            }
            
            matrix.push(row);
        }
        
        // [20251031-BATCH-006] In batch mode, save results and exit
        if (batchMode) {
            const fs = require('fs');
            fs.writeFileSync(outputFile, JSON.stringify(batchResults, null, 2));
            console.log(`\n✅ Batch results saved to: ${outputFile}`);
            console.log(`   Processed ${Object.keys(batchResults).length} master calls`);
            return; // Skip full matrix generation
        }

        // Generate analysis report (full mode only)
        console.log('\n\n' + '='.repeat(80));
        console.log('📊 PAIRWISE SIMILARITY MATRIX RESULTS');
        console.log('='.repeat(80) + '\n');

        // Print matrix as table
        console.log('Similarity Scores (Master → Test):');
        console.log('-'.repeat(80));
        
        // Header row
        const header = 'Master Call'.padEnd(25) + ' | ' + 
                      calls.map((c, i) => `${i + 1}`.padStart(5)).join(' ');
        console.log(header);
        console.log('-'.repeat(80));

        // Data rows
        for (let i = 0; i < calls.length; i++) {
            const rowLabel = `${i + 1}. ${callNames[i]}`.padEnd(25);
            const rowData = matrix[i].map(cell => 
                cell.score >= 0 ? (cell.score * 100).toFixed(0).padStart(5) : '  ERR'
            ).join(' ');
            console.log(`${rowLabel} | ${rowData}`);
        }

        console.log('\n' + '-'.repeat(80));
        console.log('Legend:');
        calls.forEach((c, i) => {
            console.log(`  ${i + 1}. ${c.name} (${c.call_type})`);
        });

        // Identify high-similarity pairs (potential confusion)
        console.log('\n' + '='.repeat(80));
        console.log('🔍 CONFUSION ANALYSIS (Non-self high similarities)');
        console.log('='.repeat(80) + '\n');

        const confusionPairs = [];
        for (let i = 0; i < calls.length; i++) {
            for (let j = i + 1; j < calls.length; j++) {
                const score = matrix[i][j].score;
                if (score >= 0 && score > 0.7) { // High similarity threshold
                    confusionPairs.push({
                        call1: callNames[i],
                        type1: callTypes[i],
                        call2: callNames[j],
                        type2: callTypes[j],
                        similarity: score
                    });
                }
            }
        }

        if (confusionPairs.length > 0) {
            confusionPairs.sort((a, b) => b.similarity - a.similarity);
            console.log('High-similarity pairs (>70%, may confuse students):\n');
            confusionPairs.forEach((pair, idx) => {
                console.log(`${idx + 1}. ${pair.call1} ↔️ ${pair.call2}`);
                console.log(`   Similarity: ${(pair.similarity * 100).toFixed(1)}%`);
                console.log(`   Types: ${pair.type1} vs ${pair.type2}`);
                console.log(`   ${pair.type1 === pair.type2 ? '⚠️  SAME TYPE - Expected' : '❌ DIFFERENT TYPES - Unexpected!'}\n`);
            });
        } else {
            console.log('✅ No high-similarity pairs found. Good call separation!\n');
        }

        // Identify outliers (low self-similarity)
        console.log('='.repeat(80));
        console.log('🚨 OUTLIER ANALYSIS (Low self-similarity = potential issues)');
        console.log('='.repeat(80) + '\n');

        const outliers = [];
        for (let i = 0; i < calls.length; i++) {
            const selfScore = matrix[i][i].score;
            if (selfScore >= 0 && selfScore < 0.85) { // Self-similarity should be high
                outliers.push({
                    call: callNames[i],
                    type: callTypes[i],
                    selfScore: selfScore
                });
            }
        }

        if (outliers.length > 0) {
            console.log('Calls with low self-similarity (<85%):\n');
            outliers.forEach((outlier, idx) => {
                console.log(`${idx + 1}. ${outlier.call} (${outlier.type})`);
                console.log(`   Self-similarity: ${(outlier.selfScore * 100).toFixed(1)}%`);
                console.log(`   ⚠️  May indicate: audio quality issues, background noise, or variable call pattern\n`);
            });
        } else {
            console.log('✅ All calls have strong self-similarity (≥85%). Good quality!\n');
        }

        // Generate CSV for external visualization (Excel, Python, etc.)
        const csvPath = '/tmp/turkey-call-similarity-matrix.csv';
        let csv = ',' + calls.map(c => c.name).join(',') + '\n';
        for (let i = 0; i < calls.length; i++) {
            csv += callNames[i] + ',' + matrix[i].map(cell => 
                cell.score >= 0 ? cell.score.toFixed(4) : 'ERROR'
            ).join(',') + '\n';
        }
        fs.writeFileSync(csvPath, csv);
        console.log(`\n💾 CSV matrix saved to: ${csvPath}`);
        console.log('   Import into spreadsheet or Python for heatmap visualization');

        // Generate heatmap instructions
        console.log('\n' + '='.repeat(80));
        console.log('📈 HEATMAP GENERATION INSTRUCTIONS');
        console.log('='.repeat(80) + '\n');
        console.log('To visualize the similarity matrix as a heatmap:\n');
        console.log('Python (matplotlib/seaborn):');
        console.log('  import pandas as pd');
        console.log('  import seaborn as sns');
        console.log('  df = pd.read_csv("/tmp/turkey-call-similarity-matrix.csv", index_col=0)');
        console.log('  sns.heatmap(df, annot=True, cmap="YlGnBu", vmin=0, vmax=1)');
        console.log('  plt.title("Turkey Call Pairwise Similarity")');
        console.log('  plt.show()');
        console.log('\nExcel:');
        console.log('  1. Open CSV in Excel');
        console.log('  2. Select data range → Insert → Chart → Heat Map');
        console.log('  3. Use conditional formatting for color scale\n');

        console.log('='.repeat(80));
        console.log('✅ Analysis complete!');
        console.log('='.repeat(80) + '\n');

    } catch (error) {
        console.error('❌ Fatal error:', error);
        process.exit(1);
    }
}

generatePairwiseMatrix().catch(console.error);
