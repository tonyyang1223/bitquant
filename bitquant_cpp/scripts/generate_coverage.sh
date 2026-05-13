#!/bin/bash
# Generate code coverage report for BitQuant C++ project
# Usage: ./scripts/generate_coverage.sh [output_dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
OUTPUT_DIR="${1:-${PROJECT_DIR}/coverage_report}"

echo "=== BitQuant Code Coverage Report Generator ==="
echo "Project directory: ${PROJECT_DIR}"
echo "Build directory: ${BUILD_DIR}"
echo "Output directory: ${OUTPUT_DIR}"

# Check if lcov and genhtml are installed
if ! command -v lcov &> /dev/null; then
    echo "Error: lcov is not installed. Please install it:"
    echo "  sudo apt-get install lcov"
    exit 1
fi

if ! command -v genhtml &> /dev/null; then
    echo "Error: genhtml is not installed. Please install it:"
    echo "  sudo apt-get install lcov"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "${BUILD_DIR}" ]; then
    mkdir -p "${BUILD_DIR}"
fi

# Configure with coverage enabled
cd "${BUILD_DIR}"
cmake -DENABLE_COVERAGE=ON -DENABLE_BINANCE_API=ON -DBUILD_TESTS=ON ..

# Build project
echo "Building project with coverage flags..."
make -j$(nproc)

# Run tests
echo "Running tests..."
ctest --output-on-failure

# Generate coverage data
echo "Generating coverage data..."
lcov --capture --directory . --output-file coverage.info --ignore-errors source

# Filter out system headers and external libraries
echo "Filtering coverage data..."
lcov --remove coverage.info '/usr/*' '*/binapi/*' '*/tests/*' --output-file coverage_filtered.info --ignore-errors source

# Generate HTML report
echo "Generating HTML report..."
genhtml coverage_filtered.info --output-directory "${OUTPUT_DIR}" --title "BitQuant C++ Coverage Report" --legend --show-details

# Print summary
echo ""
echo "=== Coverage Summary ==="
lcov --summary coverage_filtered.info

echo ""
echo "Coverage report generated at: ${OUTPUT_DIR}"
echo "Open ${OUTPUT_DIR}/index.html to view the report"

# Cleanup intermediate files
rm -f coverage.info coverage_filtered.info