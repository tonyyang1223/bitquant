/**
 * @file test_bar_generator.cpp
 * @brief Unit tests for BarGenerator
 */

#include "data/bar_generator.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cmath>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_interval_to_milliseconds() {
    std::cout << "[Test] interval_to_milliseconds" << std::endl;

    if (time_utils::interval_to_milliseconds(Interval::MINUTE_1) != 60000) return false;
    if (time_utils::interval_to_milliseconds(Interval::MINUTE_5) != 300000) return false;
    if (time_utils::interval_to_milliseconds(Interval::HOUR_1) != 3600000) return false;
    if (time_utils::interval_to_milliseconds(Interval::DAILY) != 86400000) return false;

    std::cout << "  PASS: interval_to_milliseconds works correctly" << std::endl;
    return true;
}

bool test_interval_to_minutes() {
    std::cout << "[Test] interval_to_minutes" << std::endl;

    if (time_utils::interval_to_minutes(Interval::MINUTE_1) != 1) return false;
    if (time_utils::interval_to_minutes(Interval::MINUTE_5) != 5) return false;
    if (time_utils::interval_to_minutes(Interval::HOUR_1) != 60) return false;

    std::cout << "  PASS: interval_to_minutes works correctly" << std::endl;
    return true;
}

bool test_align_to_interval() {
    std::cout << "[Test] align_to_interval" << std::endl;

    // Test minute alignment
    int64_t ts = 1700000000000;  // Some timestamp
    int64_t aligned = time_utils::align_to_interval(ts, Interval::MINUTE_1);
    // Should be aligned to minute boundary
    if (aligned % 60000 != 0) return false;

    std::cout << "  PASS: align_to_interval works" << std::endl;
    return true;
}

bool test_format_timestamp() {
    std::cout << "[Test] format_timestamp" << std::endl;

    int64_t ts = 1700000000000;  // 2023-11-14 22:13:20 UTC
    std::string formatted = time_utils::format_timestamp(ts);

    if (formatted.empty()) return false;

    std::cout << "  PASS: format_timestamp works" << std::endl;
    return true;
}

bool test_bar_generator_construction() {
    std::cout << "[Test] BarGenerator construction" << std::endl;

    BarGenerator bg(Interval::MINUTE_1, [](const BarData&) {});
    if (bg.interval() != Interval::MINUTE_1) return false;
    if (bg.has_data()) return false;

    std::cout << "  PASS: BarGenerator constructed correctly" << std::endl;
    return true;
}

bool test_bar_generator_construction_with_window() {
    std::cout << "[Test] BarGenerator construction with window" << std::endl;

    BarGenerator bg(Interval::MINUTE_5, 5, [](const BarData&) {});
    if (bg.window_size() != 5) return false;

    std::cout << "  PASS: BarGenerator with window constructed correctly" << std::endl;
    return true;
}

bool test_bar_generator_on_tick() {
    std::cout << "[Test] BarGenerator on_tick" << std::endl;

    int bar_count = 0;
    BarGenerator bg(Interval::MINUTE_1, [&bar_count](const BarData&) {
        bar_count++;
    });

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;

    bg.on_tick(tick);
    if (!bg.has_data()) return false;

    auto bar = bg.get_current_bar();
    if (!bar.has_value()) return false;
    if (bar->open_price != 50000.0) return false;
    if (bar->high_price != 50000.0) return false;
    if (bar->low_price != 50000.0) return false;
    if (bar->close_price != 50000.0) return false;

    std::cout << "  PASS: on_tick works correctly" << std::endl;
    return true;
}

bool test_bar_generator_update_bar() {
    std::cout << "[Test] BarGenerator update bar" << std::endl;

    BarGenerator bg(Interval::MINUTE_1, [](const BarData&) {});

    // First tick
    TickData tick1;
    tick1.symbol = "BTCUSDT";
    tick1.exchange = Exchange::BINANCE;
    tick1.last_price = 50000.0;
    tick1.volume = 1.0;
    tick1.datetime = 1700000000000;
    bg.on_tick(tick1);

    // Second tick - higher price
    TickData tick2;
    tick2.symbol = "BTCUSDT";
    tick2.exchange = Exchange::BINANCE;
    tick2.last_price = 51000.0;
    tick2.volume = 1.0;
    tick2.datetime = 1700000001000;  // 1 second later
    bg.on_tick(tick2);

    auto bar = bg.get_current_bar();
    if (!bar.has_value()) return false;
    if (bar->high_price != 51000.0) return false;

    // Third tick - lower price
    TickData tick3;
    tick3.symbol = "BTCUSDT";
    tick3.exchange = Exchange::BINANCE;
    tick3.last_price = 49000.0;
    tick3.volume = 1.0;
    tick3.datetime = 1700000002000;  // 2 seconds later
    bg.on_tick(tick3);

    bar = bg.get_current_bar();
    if (!bar.has_value()) return false;
    if (bar->low_price != 49000.0) return false;
    if (bar->close_price != 49000.0) return false;

    std::cout << "  PASS: update bar works correctly" << std::endl;
    return true;
}

bool test_bar_generator_get_current_bar() {
    std::cout << "[Test] BarGenerator get_current_bar" << std::endl;

    BarGenerator bg(Interval::MINUTE_1, [](const BarData&) {});

    // No data yet
    auto bar = bg.get_current_bar();
    if (bar.has_value()) return false;

    // Add tick
    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;
    bg.on_tick(tick);

    bar = bg.get_current_bar();
    if (!bar.has_value()) return false;

    std::cout << "  PASS: get_current_bar works correctly" << std::endl;
    return true;
}

bool test_bar_generator_has_data() {
    std::cout << "[Test] BarGenerator has_data" << std::endl;

    BarGenerator bg(Interval::MINUTE_1, [](const BarData&) {});
    if (bg.has_data()) return false;

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;
    bg.on_tick(tick);

    if (!bg.has_data()) return false;

    std::cout << "  PASS: has_data works correctly" << std::endl;
    return true;
}

bool test_bar_generator_reset() {
    std::cout << "[Test] BarGenerator reset" << std::endl;

    BarGenerator bg(Interval::MINUTE_1, [](const BarData&) {});

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;
    bg.on_tick(tick);

    if (!bg.has_data()) return false;

    bg.reset();
    if (bg.has_data()) return false;

    std::cout << "  PASS: reset works correctly" << std::endl;
    return true;
}

bool test_bar_generator_on_bar() {
    std::cout << "[Test] BarGenerator on_bar (synthesis mode)" << std::endl;

    int synthesized_count = 0;
    BarGenerator bg(Interval::MINUTE_5, 5, [&synthesized_count](const BarData&) {
        synthesized_count++;
    });

    // Feed 5 1-minute bars
    for (int i = 0; i < 5; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1700000000000 + i * 60000;
        bar.open_price = 50000.0 + i * 100;
        bar.high_price = 50100.0 + i * 100;
        bar.low_price = 49900.0 + i * 100;
        bar.close_price = 50050.0 + i * 100;
        bar.volume = 10.0;
        bg.on_bar(bar);
    }

    // Should have synthesized a 5-minute bar
    if (synthesized_count != 1) return false;

    std::cout << "  PASS: on_bar synthesis works correctly" << std::endl;
    return true;
}

bool test_multi_bar_generator_construction() {
    std::cout << "[Test] MultiBarGenerator construction" << std::endl;

    MultiBarGenerator mbg;
    // Should not throw

    std::cout << "  PASS: MultiBarGenerator constructed correctly" << std::endl;
    return true;
}

bool test_multi_bar_generator_add_generator() {
    std::cout << "[Test] MultiBarGenerator add_generator" << std::endl;

    MultiBarGenerator mbg;
    mbg.add_generator(Interval::MINUTE_1, [](const BarData&) {});
    mbg.add_generator(Interval::MINUTE_5, [](const BarData&) {});
    // Should not throw

    std::cout << "  PASS: add_generator works" << std::endl;
    return true;
}

bool test_multi_bar_generator_on_tick() {
    std::cout << "[Test] MultiBarGenerator on_tick" << std::endl;

    int bar_count_1m = 0;
    int bar_count_5m = 0;

    MultiBarGenerator mbg;
    mbg.add_generator(Interval::MINUTE_1, [&bar_count_1m](const BarData&) {
        bar_count_1m++;
    });
    mbg.add_generator(Interval::MINUTE_5, [&bar_count_5m](const BarData&) {
        bar_count_5m++;
    });

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;

    mbg.on_tick(tick);
    // Should not throw

    std::cout << "  PASS: on_tick works" << std::endl;
    return true;
}

bool test_multi_bar_generator_reset() {
    std::cout << "[Test] MultiBarGenerator reset" << std::endl;

    MultiBarGenerator mbg;
    mbg.add_generator(Interval::MINUTE_1, [](const BarData&) {});

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.volume = 1.0;
    tick.datetime = 1700000000000;
    mbg.on_tick(tick);

    mbg.reset();
    // Should not throw

    std::cout << "  PASS: reset works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BarGenerator Unit Tests                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_interval_to_milliseconds,
        test_interval_to_minutes,
        test_align_to_interval,
        test_format_timestamp,
        test_bar_generator_construction,
        test_bar_generator_construction_with_window,
        test_bar_generator_on_tick,
        test_bar_generator_update_bar,
        test_bar_generator_get_current_bar,
        test_bar_generator_has_data,
        test_bar_generator_reset,
        test_bar_generator_on_bar,
        test_multi_bar_generator_construction,
        test_multi_bar_generator_add_generator,
        test_multi_bar_generator_on_tick,
        test_multi_bar_generator_reset
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