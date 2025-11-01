#!/bin/bash
# Monitor container resources during testing

CONTAINER_NAME="hma-gamecalls-engine"
LOG_FILE="/tmp/container-resource-monitor.log"
INTERVAL=2  # seconds

echo "🔍 Container Resource Monitor"
echo "=============================="
echo "Container: ${CONTAINER_NAME}"
echo "Interval: ${INTERVAL}s"
echo "Log file: ${LOG_FILE}"
echo ""

# Get container PID
CONTAINER_PID=$(docker inspect -f '{{.State.Pid}}' ${CONTAINER_NAME})
if [ -z "$CONTAINER_PID" ] || [ "$CONTAINER_PID" == "0" ]; then
    echo "❌ Container not running"
    exit 1
fi

echo "Container PID: ${CONTAINER_PID}"

# Find node process inside container
NODE_PID=$(docker exec ${CONTAINER_NAME} pgrep -f "node src/index.js" | head -1)
if [ -z "$NODE_PID" ]; then
    echo "❌ Node process not found"
    exit 1
fi

echo "Node PID (inside container): ${NODE_PID}"
echo ""
echo "Monitoring started. Press Ctrl+C to stop."
echo ""

# Write header
echo "timestamp,mem_usage_mb,mem_percent,cpu_percent,fd_count,sessions_active" > ${LOG_FILE}

while true; do
    # Get Docker stats
    STATS=$(docker stats ${CONTAINER_NAME} --no-stream --format "{{.MemUsage}},{{.MemPerc}},{{.CPUPerc}}" 2>/dev/null)
    
    if [ -z "$STATS" ]; then
        echo "❌ Failed to get container stats"
        exit 1
    fi
    
    # Parse memory (e.g., "123.4MiB / 7.775GiB")
    MEM_USAGE=$(echo $STATS | cut -d',' -f1 | cut -d' ' -f1 | sed 's/MiB//')
    MEM_PERCENT=$(echo $STATS | cut -d',' -f2 | sed 's/%//')
    CPU_PERCENT=$(echo $STATS | cut -d',' -f3 | sed 's/%//')
    
    # Get file descriptor count
    FD_COUNT=$(docker exec ${CONTAINER_NAME} ls -1 /proc/${NODE_PID}/fd 2>/dev/null | wc -l)
    
    # Get active session count (from Redis or logs - simplified to 0 for now)
    SESSIONS=0
    
    # Timestamp
    TIMESTAMP=$(date +%s)
    
    # Log to file
    echo "${TIMESTAMP},${MEM_USAGE},${MEM_PERCENT},${CPU_PERCENT},${FD_COUNT},${SESSIONS}" >> ${LOG_FILE}
    
    # Display to console
    echo "[$(date '+%H:%M:%S')] Mem: ${MEM_USAGE} MB (${MEM_PERCENT}%) | CPU: ${CPU_PERCENT}% | FDs: ${FD_COUNT}"
    
    sleep ${INTERVAL}
done
