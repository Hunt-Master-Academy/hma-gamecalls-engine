#!/bin/bash
# [20251031-FIX-001] Batch pairwise similarity analysis runner
# Workaround for C++ resource leak after ~220 sessions
# Runs analysis in batches of 3 master calls, restarting container between batches

set -e

BATCH_SIZE=3
RESULTS_DIR="/tmp/pairwise-batches"
FINAL_MATRIX="/tmp/turkey-call-similarity-matrix-complete.csv"
CONTAINER_NAME="hma-gamecalls-engine"

echo "🔬 Batch Pairwise Similarity Analysis Runner"
echo "============================================="
echo "Batch size: ${BATCH_SIZE} master calls per container restart"
echo ""

# Clean up previous batch results
rm -rf "${RESULTS_DIR}"
mkdir -p "${RESULTS_DIR}"

# Get list of all turkey calls from database
echo "📋 Fetching turkey call list..."
CALL_LIST=$(docker exec ${CONTAINER_NAME} node -e "
const GameCallsService = require('/app/src/services/gameCallsService');
(async () => {
  const { calls } = await GameCallsService.listCalls({ species: 'wild_turkey', pageSize: 20 });
  console.log(JSON.stringify(calls.map(c => ({ id: c.id, name: c.name }))));
})();
" 2>/dev/null | grep '^\[' )

# Parse call list
CALL_COUNT=$(echo ${CALL_LIST} | jq '. | length')
echo "Found ${CALL_COUNT} turkey calls"
echo ""

# Calculate number of batches needed
NUM_BATCHES=$(( (CALL_COUNT + BATCH_SIZE - 1) / BATCH_SIZE ))
echo "Will process in ${NUM_BATCHES} batches"
echo ""

# Process each batch
for ((batch=0; batch<${NUM_BATCHES}; batch++)); do
  START_IDX=$((batch * BATCH_SIZE))
  END_IDX=$(( (batch + 1) * BATCH_SIZE - 1 ))
  
  if [ ${END_IDX} -ge ${CALL_COUNT} ]; then
    END_IDX=$((CALL_COUNT - 1))
  fi
  
  echo "=========================================="
  echo "📦 BATCH $((batch + 1))/${NUM_BATCHES}"
  echo "Processing calls ${START_IDX} to ${END_IDX}"
  echo "=========================================="
  
  # Clean temp directories before batch
  echo "🧹 Cleaning temp directories..."
  docker exec ${CONTAINER_NAME} sh -c "rm -rf /tmp/gamecalls-* 2>/dev/null || true"
  
  # Extract call IDs for this batch
  BATCH_CALLS=$(echo ${CALL_LIST} | jq -r ".[${START_IDX}:$((END_IDX + 1))] | map(.id) | join(\",\")")
  
  echo "Calls in this batch: ${BATCH_CALLS}"
  echo ""
  
  # Run pairwise analysis for this batch
  echo "▶️  Running analysis..."
  docker exec ${CONTAINER_NAME} node /app/scripts/pairwise-similarity-analysis.js \
    --batch-mode \
    --master-calls="${BATCH_CALLS}" \
    --output="/tmp/pairwise-batch-${batch}.json" \
    2>&1 | grep -E "(Master:|similarity:|ERROR|comparing)" || true
  
  # Copy batch results to host
  docker cp ${CONTAINER_NAME}:/tmp/pairwise-batch-${batch}.json "${RESULTS_DIR}/" 2>/dev/null || \
    echo "⚠️  No results file for batch ${batch}"
  
  echo ""
  echo "✅ Batch $((batch + 1)) complete"
  
  # Restart container between batches to clear C++ resources
  if [ ${batch} -lt $((NUM_BATCHES - 1)) ]; then
    echo "🔄 Restarting container to clear C++ resources..."
    docker restart ${CONTAINER_NAME} > /dev/null
    sleep 8  # Wait for container to be ready
    echo "✅ Container restarted"
  fi
  
  echo ""
done

echo "=========================================="
echo "🔗 MERGING BATCH RESULTS"
echo "=========================================="

# Merge all batch results into final matrix
docker exec ${CONTAINER_NAME} node << 'MERGE_SCRIPT'
const fs = require('fs');
const path = require('path');

const NUM_BATCHES = parseInt(process.env.NUM_BATCHES || '0');
const FINAL_MATRIX = process.env.FINAL_MATRIX || '/tmp/turkey-call-similarity-matrix-complete.csv';

// Load all batch results
const batches = [];
for (let i = 0; i < NUM_BATCHES; i++) {
  const file = `/tmp/pairwise-batch-${i}.json`;
  if (fs.existsSync(file)) {
    const data = JSON.parse(fs.readFileSync(file, 'utf8'));
    batches.push(data);
    console.log(`Loaded batch ${i}: ${Object.keys(data).length} master calls`);
  } else {
    console.warn(`⚠️  Batch ${i} results not found`);
  }
}

// Merge into single matrix
const mergedMatrix = {};
batches.forEach(batch => {
  Object.assign(mergedMatrix, batch);
});

console.log(`\nMerged matrix: ${Object.keys(mergedMatrix).length} master calls`);

// Convert to CSV
const callNames = Object.keys(mergedMatrix).sort();
let csv = ',' + callNames.join(',') + '\n';

callNames.forEach(masterCall => {
  const row = [masterCall];
  callNames.forEach(testCall => {
    const similarity = mergedMatrix[masterCall]?.[testCall];
    row.push(similarity !== undefined ? (typeof similarity === 'number' ? similarity.toFixed(4) : similarity) : 'N/A');
  });
  csv += row.join(',') + '\n';
});

fs.writeFileSync(FINAL_MATRIX, csv);
console.log(`\n✅ Final matrix saved to: ${FINAL_MATRIX}`);
console.log(`\nMatrix preview:`);
console.log(csv.split('\n').slice(0, 5).join('\n'));
MERGE_SCRIPT

# Copy final matrix to host
docker cp ${CONTAINER_NAME}:${FINAL_MATRIX} "${RESULTS_DIR}/" 2>/dev/null || \
  echo "⚠️  Failed to copy final matrix"

echo ""
echo "=========================================="
echo "✅ BATCH PROCESSING COMPLETE!"
echo "=========================================="
echo "Results directory: ${RESULTS_DIR}"
echo "Final matrix: ${RESULTS_DIR}/$(basename ${FINAL_MATRIX})"
echo ""
echo "To visualize the matrix:"
echo "  python3 -c 'import pandas as pd, seaborn as sns, matplotlib.pyplot as plt; df = pd.read_csv(\"${RESULTS_DIR}/$(basename ${FINAL_MATRIX})\", index_col=0); sns.heatmap(df.astype(float), annot=True, cmap=\"YlGnBu\", vmin=0, vmax=1); plt.title(\"Turkey Call Similarity Matrix\"); plt.tight_layout(); plt.savefig(\"${RESULTS_DIR}/similarity-heatmap.png\"); print(\"Heatmap saved to ${RESULTS_DIR}/similarity-heatmap.png\")'"
echo ""
