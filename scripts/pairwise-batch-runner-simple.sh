#!/bin/bash
# [20251031-FIX-003] Simplified batch pairwise runner using GameCallsService
# [20251031-CRITICAL] Reduced to 1 master call per batch due to C++ engine failures

set -e

BATCH_SIZE=1  # CRITICAL: Engine fails after ~15 comparisons, use 1 master per restart
RESULTS_DIR="/tmp/pairwise-batches"
FINAL_MATRIX="/tmp/turkey-call-similarity-matrix-complete.csv"
CONTAINER_NAME="hma-gamecalls-engine"

echo "🔬 Batch Pairwise Similarity Analysis Runner"
echo "============================================="
echo "⚠️  CRITICAL: Running in 1-master-per-batch mode due to C++ resource issues"
echo "Batch size: ${BATCH_SIZE} master call per container restart"
echo ""

# Clean up previous batch results
rm -rf "${RESULTS_DIR}"
mkdir -p "${RESULTS_DIR}"

# Get list of all turkey calls using GameCallsService
echo "📋 Fetching turkey call list..."
docker exec ${CONTAINER_NAME} node -e "
const GameCallsService = require('/app/src/services/gameCallsService');
(async () => {
  const { calls } = await GameCallsService.listCalls({ species: 'wild_turkey', pageSize: 20 });
  console.log(JSON.stringify(calls.map(c => ({ id: c.id, name: c.name }))));
})();
" 2>/dev/null | tail -1 > /tmp/call-list.json

CALL_COUNT=$(jq '. | length' /tmp/call-list.json)
echo "Found ${CALL_COUNT} turkey calls"
echo ""

# Calculate number of batches needed
NUM_BATCHES=$(( (CALL_COUNT + BATCH_SIZE - 1) / BATCH_SIZE ))
echo "Will process in ${NUM_BATCHES} batches"
echo ""

# Process each batch
for ((batch=0; batch<${NUM_BATCHES}; batch++)); do
  START_IDX=$((batch * BATCH_SIZE))
  END_IDX=$(( (batch + 1) * BATCH_SIZE ))
  
  if [ ${END_IDX} -gt ${CALL_COUNT} ]; then
    END_IDX=${CALL_COUNT}
  fi
  
  echo "=========================================="
  echo "📦 BATCH $((batch + 1))/${NUM_BATCHES}"
  echo "Processing calls ${START_IDX} to $((END_IDX - 1))"
  echo "=========================================="
  
  # Clean temp directories before batch
  echo "🧹 Cleaning temp directories..."
  docker exec ${CONTAINER_NAME} sh -c "rm -rf /tmp/gamecalls-* 2>/dev/null || true"
  
  # Extract call IDs for this batch
  BATCH_CALLS=$(jq -r ".[${START_IDX}:${END_IDX}] | map(.id) | join(\",\")" /tmp/call-list.json)
  
  echo "Calls in this batch: ${BATCH_CALLS}"
  echo ""
  
  # Run pairwise analysis for this batch
  echo "▶️  Running analysis..."
  docker exec ${CONTAINER_NAME} node /app/scripts/pairwise-similarity-analysis.js \
    --batch-mode \
    --master-calls="${BATCH_CALLS}" \
    --output="/tmp/pairwise-batch-${batch}.json" \
    2>&1 | grep -E "(^\[|Master:|similarity:|✅|❌|comparing)" || true
  
  # Copy batch results to host
  docker cp ${CONTAINER_NAME}:/tmp/pairwise-batch-${batch}.json "${RESULTS_DIR}/" 2>/dev/null && \
    echo "✅ Batch results copied" || \
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

# Simple merge using jq
echo "Merging ${NUM_BATCHES} batch files..."
jq -s 'reduce .[] as $item ({}; . * $item)' ${RESULTS_DIR}/pairwise-batch-*.json > ${RESULTS_DIR}/merged.json

# Convert to CSV
node << 'MERGE_SCRIPT'
const fs = require('fs');
const merged = JSON.parse(fs.readFileSync('/tmp/pairwise-batches/merged.json', 'utf8'));

const callNames = Object.keys(merged).sort();
let csv = ',' + callNames.join(',') + '\n';

callNames.forEach(masterCall => {
  const row = [masterCall];
  callNames.forEach(testCall => {
    const similarity = merged[masterCall]?.[testCall];
    row.push(similarity !== undefined ? (typeof similarity === 'number' ? similarity.toFixed(4) : similarity) : 'N/A');
  });
  csv += row.join(',') + '\n';
});

fs.writeFileSync('/tmp/pairwise-batches/turkey-call-similarity-matrix-complete.csv', csv);
console.log('\n✅ Final matrix saved');
console.log('\nMatrix preview:');
console.log(csv.split('\n').slice(0, 5).join('\n'));
MERGE_SCRIPT

echo ""
echo "=========================================="
echo "✅ BATCH PROCESSING COMPLETE!"
echo "=========================================="
echo "Results directory: ${RESULTS_DIR}"
echo "Final matrix: ${RESULTS_DIR}/turkey-call-similarity-matrix-complete.csv"
echo ""
echo "To visualize:"
echo "  python3 << 'PLOT'"
echo "import pandas as pd, seaborn as sns, matplotlib.pyplot as plt"
echo "df = pd.read_csv('${RESULTS_DIR}/turkey-call-similarity-matrix-complete.csv', index_col=0)"
echo "df = df.apply(pd.to_numeric, errors='coerce')"
echo "sns.heatmap(df, annot=True, cmap='YlGnBu', vmin=0, vmax=1, fmt='.2f')"
echo "plt.title('Turkey Call Similarity Matrix')"
echo "plt.tight_layout()"
echo "plt.savefig('${RESULTS_DIR}/similarity-heatmap.png', dpi=150)"
echo "print('Saved to ${RESULTS_DIR}/similarity-heatmap.png')"
echo "PLOT"
echo ""
