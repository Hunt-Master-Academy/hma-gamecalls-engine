#!/bin/bash
# [20251102-DOCKER-003] Run pairwise similarity analysis inside GameCalls engine container
# This ensures proper Docker network connectivity (redis:6379, postgres:5432, minio:9000)

set -e

echo "🐳 Running pairwise similarity analysis inside GameCalls engine container..."
echo ""

# Execute the pairwise script inside the running container
docker exec -it hma-gamecalls-engine node /app/scripts/pairwise-similarity-analysis.js

echo ""
echo "✅ Analysis complete!"
