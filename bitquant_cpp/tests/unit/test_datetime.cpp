/**
 * @file test_datetime.cpp
 * @brief Unit tests for datetime utilities
 */

#include "utils/datetime.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_timestamp_to_string() {
    std::cout << "[Test] timestamp_to_string" << std::endl;

    // Known timestamp: 2024-01-01 00:00:00 UTC
    int64_t ts = 1704067200000;  // 2024-01-01 00:00:00 UTC
    std::string result = timestamp_to_string(ts);

    if (result.empty()) return false;
    // Should contain year
    if (result.find("2024") == std::string::npos) return false;

    std::cout << "  PASS: timestamp_to_string works" << std::endl;
    return true;
}

bool test_parse_datetime() {
    std::cout << "[Test] parse_datetime" << std::endl;

    std::string dt_str = "2024-01-01 00:00:00";
    int64_t ts = parse_datetime(dt_str);

    if (ts <= 0) return false;

    std::cout << "  PASS: parse_datetime works" << std::endl;
    return true;
}

bool test_current_timestamp_ms() {
    std::cout << "[Test] current_timestamp_ms" << std::endl;

    int64_t ts1 = current_timestamp_ms();
    int64_t ts2 = current_timestamp_ms();

    if (ts1 <= 0) return false;
    // Second call should be >= first
    if (ts2 < ts1) return false;

    std::cout << "  PASS: current_timestamp_ms works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          DateTime Unit Tests                              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_timestamp_to_string,
        test_parse_datetime,
        test_current_timestamp_ms
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}