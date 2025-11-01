#!/bin/bash
# [20251102-INFRA-001] Build script with timeout to prevent indefinite hangs

set -e

BUILD_TIMEOUT=${BUILD_TIMEOUT:-600}  # 10 minutes default
REBUILD=${REBUILD:-false}

echo "🔨 Building hma-gamecalls-engine with ${BUILD_TIMEOUT}s timeout..."

cd /home/xbyooki/projects/hma-infra/docker

if [ "$REBUILD" = "true" ]; then
    echo "   Using --no-cache for clean rebuild"
    timeout ${BUILD_TIMEOUT} docker-compose build --no-cache hma-gamecalls-engine || {
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "❌ Build TIMEOUT after ${BUILD_TIMEOUT}s"
            exit 124
        else
            echo "❌ Build FAILED with exit code $EXIT_CODE"
            exit $EXIT_CODE
        fi
    }
else
    echo "   Using cache"
    timeout ${BUILD_TIMEOUT} docker-compose build hma-gamecalls-engine || {
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "❌ Build TIMEOUT after ${BUILD_TIMEOUT}s"
            exit 124
        else
            echo "❌ Build FAILED with exit code $EXIT_CODE"
            exit $EXIT_CODE
        fi
    }
fi

echo "✅ Build completed successfully"
