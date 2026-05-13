#!/bin/bash
# Quick coverage check - runs tests and shows coverage summary
# Usage: ./scripts/quick_coverage.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

echo "=== Quick Coverage Check ==="

cd "${BUILD_DIR}"

# Check if coverage build exists
if [ ! -f "CMakeCache.txt" ] || ! grep -q "ENABLE_COVERAGE:BOOL=ON" CMakeCache.txt 2>/dev/null; then
    echo "Configuring with coverage..."
    cmake -DENABLE_COVERAGE=ON -DENABLE_BINANCE_API=ON -DBUILD_TESTS=ON ..
fi

# Build
echo "Building..."
make -j$(nproc)

# Run tests
echo "Running tests..."
ctest --output-on-failure

# Quick coverage summary
echo ""
echo "=== Coverage Summary ==="
if command -v lcov &> /dev/null; then
    lcov --capture --directory . --output-file /tmp/coverage.info --quiet --ignore-errors source 2>/dev/null || true
    lcov --remove /tmp/coverage.info '/usr/*' '*/binapi/*' '*/tests/*' --output-file /tmp/coverage_filtered.info --quiet --ignore-errors source 2>/dev/null || true
    lcov --summary /tmp/coverage_filtered.info 2>/dev/null || echo "Coverage data not available"
    rm -f /tmp/coverage.info /tmp/coverage_filtered.info
else
    echo "lcov not installed. Install with: sudo apt-get install lcov"
fi
