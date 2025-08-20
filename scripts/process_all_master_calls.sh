#!/bin/bash
# filepath: scripts/process_all_master_calls.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT_DIR="$PROJECT_ROOT/data/master_calls"
OUTPUT_DIR="$PROJECT_ROOT/data/processed_calls"
TOOL_PATH="$PROJECT_ROOT/build/tools/process_master_calls"

echo "Master Call Processing Pipeline"
echo "=================================="
echo "Input:  $INPUT_DIR"
echo "Output: $OUTPUT_DIR"
echo ""

# Build the processing tool if needed
if [ ! -f "$TOOL_PATH" ]; then
    echo "Building processing tool..."
    cd "$PROJECT_ROOT"
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make process_master_calls -j$(nproc)
fi

# Create backup of existing processed files
if [ -d "$OUTPUT_DIR" ]; then
    BACKUP_DIR="${OUTPUT_DIR}_backup_$(date +%Y%m%d_%H%M%S)"
    echo "Backing up existing processed files to: $BACKUP_DIR"
    cp -r "$OUTPUT_DIR" "$BACKUP_DIR"
fi

# Process all master calls
echo "Processing master calls..."
"$TOOL_PATH" "$INPUT_DIR" "$OUTPUT_DIR"

# Validate results
echo ""
echo "📊 Processing Results:"
echo "====================="
echo "MFC files:      $(find $OUTPUT_DIR/mfc -name "*.mfc" 2>/dev/null | wc -l)"
echo "Waveform files: $(find $OUTPUT_DIR/waveforms -name "*.json" 2>/dev/null | wc -l)"
echo "Metadata files: $(find $OUTPUT_DIR/metadata -name "*.json" 2>/dev/null | wc -l)"

# Generate index file
echo ""
echo "Generating master call index..."
python3 "$SCRIPT_DIR/generate_call_index.py" "$OUTPUT_DIR"

echo ""
echo "[SUCCESS] Master call processing complete!"
