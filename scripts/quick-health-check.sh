#!/bin/bash

# ==============================================================================
# Huntmaster Engine Quick Health Check
# ==============================================================================
# Automated deployment validation script
# Run this before each deployment to catch common issues
# ==============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
SERVER_URL="http://localhost:8080"
TIMEOUT=10

echo -e "${BLUE}🔍 Huntmaster Engine Health Check${NC}"
echo "=================================================="

# Function to check HTTP endpoint
check_endpoint() {
    local url=$1
    local description=$2
    local expected_status=${3:-200}

    echo -n "Testing $description... "

    if curl -f -s -m $TIMEOUT "$url" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ OK${NC}"
        return 0
    else
        echo -e "${RED}❌ FAILED${NC}"
        return 1
    fi
}

# Function to check file size
check_file_size() {
    local url=$1
    local description=$2
    local min_size=$3

    echo -n "Checking $description size... "

    local size=$(curl -s -I "$url" 2>/dev/null | grep -i content-length | cut -d' ' -f2 | tr -d '\r\n')

    if [[ -n "$size" ]] && [[ "$size" -gt "$min_size" ]]; then
        echo -e "${GREEN}✅ OK (${size} bytes)${NC}"
        return 0
    else
        echo -e "${RED}❌ FAILED (size: ${size:-0})${NC}"
        return 1
    fi
}

# Initialize counters
TOTAL_TESTS=0
PASSED_TESTS=0

# Test 1: Web Server
echo -e "\n${YELLOW}📡 Web Server Tests${NC}"
((TOTAL_TESTS++))
if check_endpoint "$SERVER_URL/" "Web server"; then
    ((PASSED_TESTS++))
fi

# Test 2: WASM Files
echo -e "\n${YELLOW}⚡ WebAssembly Files${NC}"
((TOTAL_TESTS++))
if check_file_size "$SERVER_URL/dist/huntmaster_engine.wasm" "WASM module" 100000; then
    ((PASSED_TESTS++))
fi

((TOTAL_TESTS++))
if check_file_size "$SERVER_URL/dist/huntmaster_engine.js" "WASM JavaScript" 40000; then
    ((PASSED_TESTS++))
fi

# Test 3: HTML Interfaces
echo -e "\n${YELLOW}🌐 HTML Interfaces${NC}"
interfaces=("alpha_test.html" "test_minimal.html" "user_test.html" "index.html")
for interface in "${interfaces[@]}"; do
    ((TOTAL_TESTS++))
    if check_endpoint "$SERVER_URL/$interface" "$interface"; then
        ((PASSED_TESTS++))
    fi
done

# Test 4: Master Call Files
echo -e "\n${YELLOW}🦌 Master Call Audio Files${NC}"
master_calls=("buck_grunt" "doe_grunt" "fawn_bleat" "buck_bawl" "doe_bleat")
for call in "${master_calls[@]}"; do
    ((TOTAL_TESTS++))
    if check_file_size "$SERVER_URL/data/master_calls/${call}.wav" "${call}.wav" 10000; then
        ((PASSED_TESTS++))
    fi
done

# Test 5: CSS and JavaScript Assets
echo -e "\n${YELLOW}🎨 Asset Files${NC}"
((TOTAL_TESTS++))
if check_endpoint "$SERVER_URL/src/app.js" "Main application JavaScript"; then
    ((PASSED_TESTS++))
fi

((TOTAL_TESTS++))
if check_endpoint "$SERVER_URL/src/masterCalls.js" "Master calls configuration"; then
    ((PASSED_TESTS++))
fi

# Performance Tests
echo -e "\n${YELLOW}⚡ Performance Tests${NC}"
echo -n "Testing WASM load speed... "
WASM_LOAD_TIME=$(curl -o /dev/null -s -w "%{time_total}" "$SERVER_URL/dist/huntmaster_engine.wasm")
if (( $(echo "$WASM_LOAD_TIME < 5.0" | bc -l) )); then
    echo -e "${GREEN}✅ OK (${WASM_LOAD_TIME}s)${NC}"
    ((PASSED_TESTS++))
else
    echo -e "${RED}❌ SLOW (${WASM_LOAD_TIME}s)${NC}"
fi
((TOTAL_TESTS++))

# Memory and Resource Tests
echo -e "\n${YELLOW}💾 Resource Validation${NC}"
echo -n "Testing master call directory access... "
if curl -f -s "$SERVER_URL/data/master_calls/" > /dev/null 2>&1; then
    echo -e "${GREEN}✅ OK${NC}"
    ((PASSED_TESTS++))
else
    echo -e "${RED}❌ FAILED${NC}"
fi
((TOTAL_TESTS++))

# Advanced Tests
echo -e "\n${YELLOW}🔬 Advanced Validation${NC}"
echo -n "Testing CORS headers... "
CORS_HEADER=$(curl -s -I "$SERVER_URL/dist/huntmaster_engine.wasm" | grep -i "access-control-allow-origin" || echo "")
if [[ -n "$CORS_HEADER" ]] || curl -f -s "$SERVER_URL/dist/huntmaster_engine.wasm" > /dev/null; then
    echo -e "${GREEN}✅ OK${NC}"
    ((PASSED_TESTS++))
else
    echo -e "${YELLOW}⚠️  No CORS headers (may cause issues)${NC}"
fi
((TOTAL_TESTS++))

# Calculate results
PASS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))

echo ""
echo "=================================================="
echo -e "${BLUE}📊 Health Check Results${NC}"
echo "=================================================="
echo "Tests Passed: $PASSED_TESTS/$TOTAL_TESTS"
echo "Pass Rate: $PASS_RATE%"

if [[ $PASS_RATE -ge 90 ]]; then
    echo -e "${GREEN}🚀 STATUS: READY FOR DEPLOYMENT${NC}"
    echo "All critical systems operational"
    exit 0
elif [[ $PASS_RATE -ge 75 ]]; then
    echo -e "${YELLOW}⚠️  STATUS: DEPLOYMENT WITH CAUTION${NC}"
    echo "Some non-critical issues detected"
    exit 1
else
    echo -e "${RED}🛑 STATUS: DO NOT DEPLOY${NC}"
    echo "Critical issues detected - fix before deployment"
    exit 2
fi
