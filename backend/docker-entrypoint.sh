#!/bin/sh
# [20251031-FIX-002] GameCalls Engine startup script with temp cleanup

echo "🧹 Cleaning orphaned temp directories..."
rm -rf /tmp/gamecalls-* 2>/dev/null || true
echo "✅ Temp cleanup complete"

echo "🚀 Starting GameCalls Engine..."
exec node src/index.js
