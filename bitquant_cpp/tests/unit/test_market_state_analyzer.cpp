/**
 * @file test_market_state_analyzer.cpp
 * @brief Unit tests for MarketStateAnalyzer
 */

#include "data/market_state_analyzer.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

using namespace bitquant;

void test_initialization() {
    std::cout << "Test: Initialization\n";

    MarketStateAnalyzer analyzer;
    assert(analyzer.state() == MarketState::WAITING);
    assert(!analyzer.is_initialized());
    assert(analyzer.bar_count() == 0);

    std::cout << "  ✅ Passed\n";
}

void test_warmup() {
    std::cout << "Test: Warm-up period\n";

    MarketStateConfig config;
    config.period = 10;
    config.confirmation_bars = 1;  // Use 1 for faster confirmation
    MarketStateAnalyzer analyzer(config);

    // Feed 9 bars (not enough)
    for (int i = 0; i < 9; ++i) {
        analyzer.update(100.0, 99.0, 100.0);
    }
    assert(analyzer.state() == MarketState::WAITING);
    assert(!analyzer.is_initialized());

    // Feed 10th bar (buffer becomes full, but confirmation mechanism requires 1 more bar)
    analyzer.update(100.0, 99.0, 100.0);
    assert(analyzer.is_initialized());
    // First bar with full buffer starts confirmation, but state is not yet confirmed
    std::cout << "  After 10 bars: state=" << static_cast<int>(analyzer.state()) << "\n";

    // Feed 11th bar to confirm state
    analyzer.update(100.0, 99.0, 100.0);
    std::cout << "  After 11 bars: state=" << static_cast<int>(analyzer.state()) << "\n";
    assert(analyzer.state() != MarketState::WAITING);

    std::cout << "  ✅ Passed - Analyzer initialized after period bars\n";
}

void test_consolidation_detection() {
    std::cout << "Test: Consolidation detection\n";

    MarketStateConfig config;
    config.period = 10;
    config.width_threshold_pct = 5.0;
    config.confirmation_bars = 1;  // Faster confirmation for test
    MarketStateAnalyzer analyzer(config);

    // Feed stable prices (low volatility)
    for (int i = 0; i < 15; ++i) {
        analyzer.update(101.0, 99.0, 100.0);
    }

    std::cout << "  Channel width: " << analyzer.channel_width_pct() << "%\n";
    assert(analyzer.state() == MarketState::CONSOLIDATION);

    std::cout << "  ✅ Passed - Low volatility detected as consolidation\n";
}

void test_trending_detection() {
    std::cout << "Test: Trending detection\n";

    MarketStateConfig config;
    config.period = 10;
    config.width_threshold_pct = 3.0;  // Lower threshold for test
    config.confirmation_bars = 1;
    MarketStateAnalyzer analyzer(config);

    // Feed prices with trend (increasing)
    for (int i = 0; i < 10; ++i) {
        double price = 100.0 + i * 2.0;  // Rising from 100 to 118
        analyzer.update(price + 1.0, price - 1.0, price);
    }
    assert(analyzer.is_initialized());

    // Need confirmation bars
    for (int i = 0; i < 5; ++i) {
        double price = 120.0 + i * 2.0;
        analyzer.update(price + 1.0, price - 1.0, price);
    }

    std::cout << "  Channel width: " << analyzer.channel_width_pct() << "%\n";
    assert(analyzer.state() == MarketState::TRENDING);

    std::cout << "  ✅ Passed - High volatility detected as trending\n";
}

void test_state_transition_with_confirmation() {
    std::cout << "Test: State transition with confirmation\n";

    MarketStateConfig config;
    config.period = 5;
    config.width_threshold_pct = 5.0;
    config.confirmation_bars = 2;
    MarketStateAnalyzer analyzer(config);

    // Warm up with consolidation
    for (int i = 0; i < 10; ++i) {
        analyzer.update(101.0, 99.0, 100.0);
    }
    assert(analyzer.state() == MarketState::CONSOLIDATION);

    // Single bar of trend (should not transition yet)
    analyzer.update(110.0, 90.0, 100.0);
    std::cout << "  After 1 trend bar: state=" << static_cast<int>(analyzer.state()) << "\n";
    // Should still be CONSOLIDATION due to confirmation requirement
    assert(analyzer.state() == MarketState::CONSOLIDATION);

    // Second bar of trend (should transition now)
    analyzer.update(110.0, 90.0, 100.0);
    std::cout << "  After 2 trend bars: state=" << static_cast<int>(analyzer.state()) << "\n";
    assert(analyzer.state() == MarketState::TRENDING);

    std::cout << "  ✅ Passed - Confirmation mechanism works\n";
}

void test_reset() {
    std::cout << "Test: Reset functionality\n";

    MarketStateConfig config;
    config.period = 5;
    MarketStateAnalyzer analyzer(config);

    // Initialize
    for (int i = 0; i < 10; ++i) {
        analyzer.update(100.0, 99.0, 100.0);
    }
    assert(analyzer.is_initialized());
    assert(analyzer.bar_count() == 5);  // Ring buffer size

    // Reset
    analyzer.reset();
    assert(!analyzer.is_initialized());
    assert(analyzer.bar_count() == 0);
    assert(analyzer.state() == MarketState::WAITING);

    std::cout << "  ✅ Passed - Reset clears all state\n";
}

void test_channel_values() {
    std::cout << "Test: Channel value calculations\n";

    MarketStateConfig config;
    config.period = 3;
    MarketStateAnalyzer analyzer(config);

    // Feed known values
    analyzer.update(102.0, 98.0, 100.0);
    analyzer.update(102.0, 98.0, 101.0);
    analyzer.update(102.0, 98.0, 102.0);

    double middle = analyzer.middle_band();
    double upper = analyzer.upper_band();
    double lower = analyzer.lower_band();
    double width = analyzer.channel_width_pct();

    std::cout << "  Middle: " << middle << "\n";
    std::cout << "  Upper: " << upper << "\n";
    std::cout << "  Lower: " << lower << "\n";
    std::cout << "  Width: " << width << "%\n";

    // Middle should be (100+101+102)/3 = 101
    assert(std::abs(middle - 101.0) < 0.01);

    // Upper should be middle + 2*stddev, Lower should be middle - 2*stddev
    // stddev = sqrt(((100-101)^2 + (101-101)^2 + (102-101)^2)/3) = sqrt(2/3) ≈ 0.816
    double expected_stddev = std::sqrt(2.0/3.0);
    double expected_upper = 101.0 + 2.0 * expected_stddev;
    double expected_lower = 101.0 - 2.0 * expected_stddev;

    assert(std::abs(upper - expected_upper) < 0.01);
    assert(std::abs(lower - expected_lower) < 0.01);

    // Width should be (upper - lower) / middle * 100 ≈ 3.24%
    double expected_width = (expected_upper - expected_lower) / 101.0 * 100.0;
    assert(std::abs(width - expected_width) < 0.1);

    std::cout << "  ✅ Passed - Channel values calculated correctly\n";
}

void test_thread_safety() {
    std::cout << "Test: Thread safety\n";

    MarketStateConfig config;
    config.period = 10;
    MarketStateAnalyzer analyzer(config);
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&analyzer, &success_count, t]() {
            for (int i = 0; i < 20; ++i) {
                double price = 100.0 + t + (i % 3);
                analyzer.update(price + 1.0, price - 1.0, price);
                success_count++;
            }
        });
    }

    for (auto& t : threads) t.join();

    std::cout << "  Processed " << success_count << " updates from 5 threads\n";
    assert(success_count == 100);
    assert(analyzer.is_initialized());

    std::cout << "  ✅ Passed - No crashes, thread-safe\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          MarketStateAnalyzer Unit Tests                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    test_initialization();
    test_warmup();
    test_consolidation_detection();
    test_trending_detection();
    test_state_transition_with_confirmation();
    test_reset();
    test_channel_values();
    test_thread_safety();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          All MarketStateAnalyzer tests passed ✅           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return 0;
}
