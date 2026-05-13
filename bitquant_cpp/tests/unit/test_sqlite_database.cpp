/**
 * @file test_sqlite_database.cpp
 * @brief Unit tests for SQLiteDatabase
 */

#include "database/sqlite_database.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>

using namespace bitquant;

#define EXPECT_TRUE(x) do { if (!(x)) { std::cerr << "FAIL: " << #x << " is false" << std::endl; return false; } } while(0)
#define EXPECT_EQ(a, b) do { if ((a) != (b)) { std::cerr << "FAIL: " << (a) << " != " << (b) << std::endl; return false; } } while(0)
#define EXPECT_NEAR(a, b, eps) do { if (std::abs((a) - (b)) > eps) { std::cerr << "FAIL: " << (a) << " != " << (b) << " (eps=" << eps << ")" << std::endl; return false; } } while(0)
#define EXPECT_GT(a, b) do { if ((a) <= (b)) { std::cerr << "FAIL: " << (a) << " <= " << (b) << std::endl; return false; } } while(0)

//=============================================================================
// Test Functions
//=============================================================================

bool test_create_database() {
    std::cout << "[Test] Create Database" << std::endl;

    auto db = create_sqlite_database(":memory:");
    EXPECT_TRUE(db->is_connected());

    std::cout << "  PASS: Database created" << std::endl;
    return true;
}

bool test_save_and_load_bar_data() {
    std::cout << "[Test] Save and Load Bar Data" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Create test bar data
    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.exchange = Exchange::BINANCE;
    bar.interval = Interval::MINUTE_1;
    bar.datetime = 1234567890000;
    bar.open_price = 50000.0;
    bar.high_price = 51000.0;
    bar.low_price = 49500.0;
    bar.close_price = 50500.0;
    bar.volume = 1000.0;
    bar.turnover = 50000000.0;

    // Save bar
    EXPECT_TRUE(db->save_bar(bar));

    // Load bar
    auto loaded = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 100);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_NEAR(loaded[0].open_price, 50000.0, 0.01);
    EXPECT_NEAR(loaded[0].high_price, 51000.0, 0.01);
    EXPECT_NEAR(loaded[0].low_price, 49500.0, 0.01);
    EXPECT_NEAR(loaded[0].close_price, 50500.0, 0.01);
    EXPECT_NEAR(loaded[0].volume, 1000.0, 0.01);

    std::cout << "  PASS: Bar data saved and loaded correctly" << std::endl;
    return true;
}

bool test_save_and_load_tick_data() {
    std::cout << "[Test] Save and Load Tick Data" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Create test tick data
    TickData tick;
    tick.symbol = "ETHUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.datetime = 1234567890000;
    tick.last_price = 3000.0;
    tick.volume = 500.0;
    tick.bid_price_1 = 2999.0;
    tick.bid_volume_1 = 100.0;
    tick.ask_price_1 = 3001.0;
    tick.ask_volume_1 = 150.0;

    // Save tick
    EXPECT_TRUE(db->save_tick(tick));

    // Load tick
    auto loaded = db->load_ticks("ETHUSDT", Exchange::BINANCE, 100);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_NEAR(loaded[0].last_price, 3000.0, 0.01);
    EXPECT_NEAR(loaded[0].volume, 500.0, 0.01);

    std::cout << "  PASS: Tick data saved and loaded correctly" << std::endl;
    return true;
}

bool test_save_multiple_bars() {
    std::cout << "[Test] Save Multiple Bars" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Create multiple bars
    std::vector<BarData> bars;
    for (int i = 0; i < 10; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1234567890000 + i * 60000;
        bar.open_price = 50000.0 + i * 100.0;
        bar.high_price = 50100.0 + i * 100.0;
        bar.low_price = 49900.0 + i * 100.0;
        bar.close_price = 50050.0 + i * 100.0;
        bar.volume = 100.0;
        bars.push_back(bar);
    }

    EXPECT_EQ(db->save_bars(bars), 10);

    // Load all bars
    auto loaded = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 100);
    EXPECT_EQ(loaded.size(), 10);

    // Check order
    EXPECT_NEAR(loaded[0].open_price, 50000.0, 0.01);
    EXPECT_NEAR(loaded[9].open_price, 50900.0, 0.01);

    std::cout << "  PASS: Multiple bars saved and loaded correctly" << std::endl;
    return true;
}

bool test_time_range_filter() {
    std::cout << "[Test] Time Range Filter" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Create bars with different timestamps
    std::vector<BarData> bars;
    for (int i = 0; i < 20; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1000000 + i * 60000;  // 1 minute intervals
        bar.open_price = 50000.0;
        bar.close_price = 50000.0;
        bar.high_price = 50100.0;
        bar.low_price = 49900.0;
        bar.volume = 100.0;
        bars.push_back(bar);
    }

    EXPECT_EQ(db->save_bars(bars), 20);

    // Load specific time range (bars 5-14)
    int64_t start_time = 1000000 + 5 * 60000;  // Bar 5
    int64_t end_time = 1000000 + 14 * 60000;   // Bar 14
    auto loaded = db->load_bars_by_time("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, start_time, end_time);

    EXPECT_EQ(loaded.size(), 10);  // Bars 5-14 inclusive

    std::cout << "  PASS: Time range filter works correctly" << std::endl;
    return true;
}

bool test_delete_bar_data() {
    std::cout << "[Test] Delete Bar Data" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Create and save bars
    std::vector<BarData> bars;
    for (int i = 0; i < 5; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1000000 + i * 60000;
        bar.open_price = 50000.0;
        bar.close_price = 50000.0;
        bar.high_price = 50100.0;
        bar.low_price = 49900.0;
        bar.volume = 100.0;
        bars.push_back(bar);
    }

    EXPECT_EQ(db->save_bars(bars), 5);

    // Delete bars
    EXPECT_GT(db->delete_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1), 0);

    // Verify deletion
    auto loaded = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 100);
    EXPECT_EQ(loaded.size(), 0);

    std::cout << "  PASS: Bar data deleted correctly" << std::endl;
    return true;
}

bool test_bar_overview() {
    std::cout << "[Test] Bar Overview" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Add bars
    std::vector<BarData> bars;
    for (int i = 0; i < 15; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1000000 + i * 60000;
        bar.open_price = 50000.0;
        bar.close_price = 50000.0;
        bar.high_price = 50100.0;
        bar.low_price = 49900.0;
        bar.volume = 100.0;
        bars.push_back(bar);
    }

    EXPECT_EQ(db->save_bars(bars), 15);

    // Get overview
    auto overview = db->get_bar_overview();
    EXPECT_EQ(overview.size(), 1);
    EXPECT_EQ(overview[0].symbol, "BTCUSDT");
    EXPECT_EQ(overview[0].count, 15);

    std::cout << "  PASS: Bar overview works correctly" << std::endl;
    return true;
}

bool test_strategy_state() {
    std::cout << "[Test] Strategy State" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Save strategy state
    std::string state = R"({"position": 1.0, "entry_price": 50000.0})";
    EXPECT_TRUE(db->save_strategy_state("test_strategy", state));

    // Load strategy state
    auto loaded = db->load_strategy_state("test_strategy");
    EXPECT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value(), state);

    // Delete strategy state
    EXPECT_TRUE(db->delete_strategy_state("test_strategy"));

    // Verify deletion
    auto deleted = db->load_strategy_state("test_strategy");
    EXPECT_TRUE(!deleted.has_value());

    std::cout << "  PASS: Strategy state works correctly" << std::endl;
    return true;
}

bool test_transaction() {
    std::cout << "[Test] Transaction" << std::endl;

    auto db = create_sqlite_database(":memory:");

    // Begin transaction
    EXPECT_TRUE(db->begin_transaction());

    // Save bar
    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.exchange = Exchange::BINANCE;
    bar.interval = Interval::MINUTE_1;
    bar.datetime = 1234567890000;
    bar.open_price = 50000.0;
    bar.close_price = 50500.0;
    bar.high_price = 51000.0;
    bar.low_price = 49500.0;
    bar.volume = 100.0;

    EXPECT_TRUE(db->save_bar(bar));

    // Commit
    EXPECT_TRUE(db->commit());

    // Verify
    auto loaded = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 100);
    EXPECT_EQ(loaded.size(), 1);

    std::cout << "  PASS: Transaction works correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          SQLite Database Unit Tests                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_create_database,
        test_save_and_load_bar_data,
        test_save_and_load_tick_data,
        test_save_multiple_bars,
        test_time_range_filter,
        test_delete_bar_data,
        test_bar_overview,
        test_strategy_state,
        test_transaction
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
