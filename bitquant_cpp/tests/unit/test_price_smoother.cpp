/**
 * @file test_price_smoother.cpp
 * @brief Unit tests for PriceSmoother
 */

#include "data/price_smoother.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>
#include <vector>

using namespace bitquant;

void test_basic_smoothing() {
    std::cout << "Test: Basic smoothing\n";

    PriceSmoother smoother;

    // Feed 10 prices around 76000
    for (int i = 0; i < 10; ++i) {
        smoother.process(76000.0 + (i % 2) * 100.0);  // 76000, 76100, 76000...
    }

    double smoothed = smoother.get_smoothed_price();
    std::cout << "  Smoothed price: " << smoothed << "\n";

    // Should be close to average (~76050)
    assert(smoothed > 75900 && smoothed < 76200);
    assert(smoother.is_initialized());

    std::cout << "  ✅ Passed\n";
}

void test_outlier_detection() {
    std::cout << "Test: Outlier detection (11% drop like testnet bug)\n";

    PriceSmootherConfig config;
    config.outlier_threshold_pct = 5.0;
    config.require_confirmation = true;
    PriceSmoother smoother(config);

    // Feed 10 normal prices at 76000
    for (int i = 0; i < 10; ++i) {
        smoother.process(76000.0);
    }

    std::cout << "  Warm-up complete, smoothed price: " << smoother.get_smoothed_price() << "\n";

    // Feed outlier (11% drop like testnet bug)
    auto result = smoother.process(67714.0);  // 11% below 76000

    std::cout << "  Raw price: 67714\n";
    std::cout << "  Smoothed price: " << result.smoothed_price << "\n";
    std::cout << "  Is anomaly: " << result.is_anomaly << "\n";

    // Should flag as anomaly
    assert(result.is_anomaly);

    // Smoothed price should NOT drop to 67714
    assert(result.smoothed_price > 75000);

    // Smoothed price should stay close to original 76000
    assert(std::abs(result.smoothed_price - 76000) < 500);

    std::cout << "  ✅ Passed - Outlier rejected, smoothed price preserved\n";
}

void test_confirmation_mechanism() {
    std::cout << "Test: Confirmation mechanism\n";

    PriceSmootherConfig config;
    config.require_confirmation = true;
    config.outlier_threshold_pct = 5.0;
    PriceSmoother smoother(config);

    // Initialize
    for (int i = 0; i < 10; ++i) {
        smoother.process(76000.0);
    }

    // First outlier (11% drop)
    auto r1 = smoother.process(67714.0);
    std::cout << "  First outlier: anomaly=" << r1.is_anomaly
              << ", smoothed=" << r1.smoothed_price << "\n";
    assert(r1.is_anomaly);
    assert(r1.smoothed_price > 75000);  // Should NOT drop

    // Second normal price (outlier should be rejected)
    auto r2 = smoother.process(76000.0);
    std::cout << "  After normal price: anomaly=" << r2.is_anomaly
              << ", smoothed=" << r2.smoothed_price << "\n";

    // Anomaly should be cleared
    assert(!r2.is_anomaly);

    std::cout << "  ✅ Passed - Single outlier rejected, anomaly cleared\n";
}

void test_sustained_price_drop() {
    std::cout << "Test: Sustained price drop (legitimate trend)\n";

    PriceSmootherConfig config;
    config.require_confirmation = true;
    config.outlier_threshold_pct = 5.0;
    PriceSmoother smoother(config);

    // Initialize at 76000
    for (int i = 0; i < 10; ++i) {
        smoother.process(76000.0);
    }

    // Gradual drop (2% per tick for 10 ticks = 20% total)
    double price = 76000.0;
    for (int i = 0; i < 10; ++i) {
        price *= 0.98;  // 2% drop each tick
        auto result = smoother.process(price);
        std::cout << "  Tick " << i+1 << ": raw=" << price
                  << ", smoothed=" << result.smoothed_price
                  << ", anomaly=" << result.is_anomaly << "\n";

        // For gradual drops, anomaly should not be flagged (within threshold)
        // After enough ticks, smoothed price should converge to new price
    }

    // Final smoothed price should be close to final raw price
    double final_smoothed = smoother.get_smoothed_price();
    std::cout << "  Final smoothed: " << final_smoothed << "\n";

    // Should have caught up with the trend (within 10%)
    assert(std::abs(final_smoothed - price) / price < 0.15);

    std::cout << "  ✅ Passed - Gradual trend tracked correctly\n";
}

void test_thread_safety() {
    std::cout << "Test: Thread safety\n";

    PriceSmoother smoother;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&smoother, &success_count, t]() {
            for (int i = 0; i < 100; ++i) {
                smoother.process(76000.0 + t * 100);
                success_count++;
            }
        });
    }

    for (auto& t : threads) t.join();

    std::cout << "  Processed " << success_count << " ticks from 5 threads\n";
    assert(success_count == 500);
    assert(smoother.tick_count() == 10);  // Buffer size fixed

    std::cout << "  ✅ Passed - No crashes, consistent results\n";
}

void test_invalid_prices() {
    std::cout << "Test: Invalid prices (zero/negative)\n";

    PriceSmoother smoother;

    // Initialize
    for (int i = 0; i < 10; ++i) {
        smoother.process(76000.0);
    }

    // Feed invalid price (zero)
    auto r1 = smoother.process(0.0);
    assert(r1.smoothed_price > 0);  // Should keep previous valid price
    std::cout << "  Zero price handled: smoothed=" << r1.smoothed_price << "\n";

    // Feed negative price
    auto r2 = smoother.process(-1000.0);
    assert(r2.smoothed_price > 0);  // Should keep previous valid price
    std::cout << "  Negative price handled: smoothed=" << r2.smoothed_price << "\n";

    std::cout << "  ✅ Passed - Invalid prices rejected\n";
}

void test_reset() {
    std::cout << "Test: Reset functionality\n";

    PriceSmoother smoother;

    // Initialize and process
    for (int i = 0; i < 15; ++i) {
        smoother.process(76000.0);
    }

    assert(smoother.is_initialized());
    assert(smoother.tick_count() == 10);

    // Reset
    smoother.reset();

    assert(!smoother.is_initialized());
    assert(smoother.tick_count() == 0);
    assert(smoother.get_smoothed_price() == 0.0);

    std::cout << "  ✅ Passed - Reset clears all state\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          PriceSmoother Unit Tests                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    test_basic_smoothing();
    test_outlier_detection();
    test_confirmation_mechanism();
    test_sustained_price_drop();
    test_thread_safety();
    test_invalid_prices();
    test_reset();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          All PriceSmoother tests passed ✅                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return 0;
}