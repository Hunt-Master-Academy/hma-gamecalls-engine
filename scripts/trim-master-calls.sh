#!/bin/bash
# [20251101-V1.0-PREP] Master Call Silence Trimming Pipeline
#
# Purpose: Remove leading/trailing silence from master calls to improve self-similarity scores
# Method: Use ffmpeg silenceremove filter with conservative thresholds
# Output: Trimmed WAV files ready for re-upload to MinIO
#
# Usage: ./trim-master-calls.sh <input_dir> <output_dir>
#
# Example: ./trim-master-calls.sh ./master_calls ./master_calls_trimmed

set -e

INPUT_DIR="${1:-/app/data/master_calls}"
OUTPUT_DIR="${2:-/app/data/master_calls_trimmed}"

# Silence detection thresholds
SILENCE_THRESHOLD="-40dB"  # Conservative: only remove very quiet parts
MIN_SILENCE_DURATION="0.1"  # 100ms minimum silence to remove

echo "🔧 Master Call Trimming Pipeline"
echo "========================================================================"
echo "Input:  $INPUT_DIR"
echo "Output: $OUTPUT_DIR"
echo "Threshold: $SILENCE_THRESHOLD (duration: ${MIN_SILENCE_DURATION}s)"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Counter
PROCESSED=0
FAILED=0

# Process each WAV file
for input_file in "$INPUT_DIR"/*.wav; do
    if [ ! -f "$input_file" ]; then
        echo "⚠️  No WAV files found in $INPUT_DIR"
        exit 1
    fi
    
    filename=$(basename "$input_file")
    output_file="$OUTPUT_DIR/$filename"
    
    echo "📝 Processing: $filename"
    
    # Use ffmpeg silenceremove filter
    # - Remove silence from start and end
    # - Preserve audio quality (no re-encoding, just trimming)
    if ffmpeg -i "$input_file" \
        -af "silenceremove=start_periods=1:start_duration=${MIN_SILENCE_DURATION}:start_threshold=${SILENCE_THRESHOLD}:stop_periods=-1:stop_duration=${MIN_SILENCE_DURATION}:stop_threshold=${SILENCE_THRESHOLD}" \
        -c:a pcm_s16le \
        -y "$output_file" 2>&1 | grep -q "Output"; then
        
        # Get file sizes
        INPUT_SIZE=$(stat -f%z "$input_file" 2>/dev/null || stat -c%s "$input_file")
        OUTPUT_SIZE=$(stat -f%z "$output_file" 2>/dev/null || stat -c%s "$output_file")
        REDUCTION=$((100 - (OUTPUT_SIZE * 100 / INPUT_SIZE)))
        
        echo "   ✅ Trimmed: ${INPUT_SIZE} → ${OUTPUT_SIZE} bytes (${REDUCTION}% reduction)"
        ((PROCESSED++))
    else
        echo "   ❌ FAILED"
        ((FAILED++))
    fi
    
    echo ""
done

echo "========================================================================"
echo "✅ Processing complete"
echo "   Processed: $PROCESSED"
echo "   Failed: $FAILED"
echo ""
echo "Next steps:"
echo "1. Review trimmed files: ls -lh $OUTPUT_DIR"
echo "2. Listen to samples to verify quality"
echo "3. Upload to MinIO: mc cp $OUTPUT_DIR/*.wav minio/gamecalls-master-calls-trimmed/"
echo ""
