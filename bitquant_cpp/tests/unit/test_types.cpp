/**
 * @file test_types.cpp
 * @brief Unit tests for core types
 */

#include "core/types.hpp"
#include <iostream>
#include <cmath>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_exchange_to_string() {
    std::cout << "[Test] exchange_to_string" << std::endl;

    if (exchange_to_string(Exchange::BINANCE) != "BINANCE") return false;
    if (exchange_to_string(Exchange::OKX) != "OKX") return false;
    if (exchange_to_string(Exchange::LOCAL) != "LOCAL") return false;

    std::cout << "  PASS: exchange_to_string works correctly" << std::endl;
    return true;
}

bool test_exchange_from_string() {
    std::cout << "[Test] exchange_from_string" << std::endl;

    if (exchange_from_string("BINANCE") != Exchange::BINANCE) return false;
    if (exchange_from_string("binance") != Exchange::BINANCE) return false;
    if (exchange_from_string("OKX") != Exchange::OKX) return false;
    if (exchange_from_string("LOCAL") != Exchange::LOCAL) return false;
    if (exchange_from_string("UNKNOWN") != Exchange::LOCAL) return false;

    std::cout << "  PASS: exchange_from_string works correctly" << std::endl;
    return true;
}

bool test_interval_to_string() {
    std::cout << "[Test] interval_to_string" << std::endl;

    if (interval_to_string(Interval::TICK) != "TICK") return false;
    if (interval_to_string(Interval::MINUTE_1) != "1m") return false;
    if (interval_to_string(Interval::MINUTE_5) != "5m") return false;
    if (interval_to_string(Interval::HOUR_1) != "1h") return false;
    if (interval_to_string(Interval::DAILY) != "1d") return false;
    if (interval_to_string(Interval::WEEKLY) != "1w") return false;
    if (interval_to_string(Interval::MONTHLY) != "1M") return false;

    std::cout << "  PASS: interval_to_string works correctly" << std::endl;
    return true;
}

bool test_interval_from_string() {
    std::cout << "[Test] interval_from_string" << std::endl;

    if (interval_from_string("1m") != Interval::MINUTE_1) return false;
    if (interval_from_string("monthly") != Interval::MONTHLY) return false;
    if (interval_from_string("5m") != Interval::MINUTE_5) return false;
    if (interval_from_string("1h") != Interval::HOUR_1) return false;
    if (interval_from_string("1d") != Interval::DAILY) return false;
    if (interval_from_string("daily") != Interval::DAILY) return false;
    if (interval_from_string("1w") != Interval::WEEKLY) return false;
    if (interval_from_string("weekly") != Interval::WEEKLY) return false;

    std::cout << "  PASS: interval_from_string works correctly" << std::endl;
    return true;
}

bool test_direction_to_string() {
    std::cout << "[Test] direction_to_string" << std::endl;

    if (direction_to_string(Direction::LONG) != "LONG") return false;
    if (direction_to_string(Direction::SHORT) != "SHORT") return false;
    if (direction_to_string(Direction::NET) != "NET") return false;

    std::cout << "  PASS: direction_to_string works correctly" << std::endl;
    return true;
}

bool test_direction_from_string() {
    std::cout << "[Test] direction_from_string" << std::endl;

    if (direction_from_string("LONG") != Direction::LONG) return false;
    if (direction_from_string("long") != Direction::LONG) return false;
    if (direction_from_string("BUY") != Direction::LONG) return false;
    if (direction_from_string("buy") != Direction::LONG) return false;
    if (direction_from_string("SHORT") != Direction::SHORT) return false;
    if (direction_from_string("short") != Direction::SHORT) return false;
    if (direction_from_string("SELL") != Direction::SHORT) return false;
    if (direction_from_string("sell") != Direction::SHORT) return false;
    if (direction_from_string("NET") != Direction::NET) return false;

    std::cout << "  PASS: direction_from_string works correctly" << std::endl;
    return true;
}

bool test_performance_metrics_to_string() {
    std::cout << "[Test] PerformanceMetrics::to_string" << std::endl;

    PerformanceMetrics metrics;
    metrics.initial_capital = 100000.0;
    metrics.final_capital = 120000.0;
    metrics.total_return = 0.20;
    metrics.annualized_return = 0.25;
    metrics.max_drawdown_percent = 0.10;
    metrics.sharpe_ratio = 1.5;
    metrics.sortino_ratio = 2.0;
    metrics.calmar_ratio = 2.5;
    metrics.total_trades = 100;
    metrics.winning_trades = 60;
    metrics.losing_trades = 40;
    metrics.win_rate = 0.60;
    metrics.profit_factor = 1.8;
    metrics.avg_win = 500.0;
    metrics.avg_loss = 300.0;
    metrics.avg_trade = 200.0;
    metrics.max_consecutive_wins = 5;
    metrics.max_consecutive_losses = 3;
    metrics.total_commission = 1000.0;
    metrics.total_slippage = 500.0;
    metrics.total_turnover = 1000000.0;

    std::string report = metrics.to_string();
    if (report.empty()) return false;
    if (report.find("100000.00") == std::string::npos) return false;

    std::cout << "  PASS: PerformanceMetrics::to_string works correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Types Unit Tests                                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_exchange_to_string,
        test_exchange_from_string,
        test_interval_to_string,
        test_interval_from_string,
        test_direction_to_string,
        test_direction_from_string,
        test_performance_metrics_to_string
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