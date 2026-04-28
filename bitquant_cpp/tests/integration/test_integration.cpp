/**
 * @file test_integration.cpp
 * @brief Integration tests for the complete trading platform
 *
 * Tests the full workflow:
 * - Data loading
 * - Multi-strategy execution
 * - Order matching
 * - Performance calculation
 */

#include "engine/trading_engine.hpp"
#include "engine/strategy.hpp"
#include "data/array_manager.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <fstream>

using namespace bitquant;

//=============================================================================
// Test Strategies
//=============================================================================

class SimpleBuySellStrategy : public IStrategy {
public:
    int buy_bar = 1;
    int sell_bar = 10;
    int bar_count = 0;

    void on_init() override { inited_ = true; }
    void on_start() override { trading_ = true; }

    void on_bar(const BarData& bar) override {
        ++bar_count;

        if (bar_count == buy_bar) {
            buy(bar.close_price, 1.0);
        } else if (bar_count == sell_bar) {
            sell(bar.close_price, position());
        }
    }
};

class MovingAverageStrategy : public IStrategy {
public:
    int fast_period = 5;
    int slow_period = 10;
    ArrayManager am{100};
    double prev_diff = 0;

    void on_init() override { inited_ = true; }
    void on_start() override { trading_ = true; }

    void on_bar(const BarData& bar) override {
        am.update_bar(bar);

        if (!am.inited() || am.count() < slow_period) {
            return;
        }

        double fast_ma = am.sma(fast_period);
        double slow_ma = am.sma(slow_period);
        double diff = fast_ma - slow_ma;

        if (prev_diff <= 0 && diff > 0 && position() <= 0) {
            buy(bar.close_price, 1.0);
        } else if (prev_diff >= 0 && diff < 0 && position() > 0) {
            sell(bar.close_price, position());
        }

        prev_diff = diff;
    }
};

//=============================================================================
// Test Data Generation
//=============================================================================

std::vector<BarData> generate_test_data(int count, double start_price = 100.0) {
    std::vector<BarData> bars;
    bars.reserve(count);

    for (int i = 0; i < count; ++i) {
        BarData bar;
        bar.datetime = i * 60000;  // 1 minute bars
        bar.open_price = start_price + i * 0.1;
        bar.high_price = bar.open_price + 0.5;
        bar.low_price = bar.open_price - 0.5;
        bar.close_price = bar.open_price + (i % 2 == 0 ? 0.2 : -0.1);
        bar.volume = 1000.0;
        bars.push_back(bar);
    }

    return bars;
}

//=============================================================================
// Test Functions
//=============================================================================

bool test_broker_basic() {
    std::cout << "Testing: Broker basic functionality... ";

    BrokerConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;

    Broker broker(config);

    auto data = generate_test_data(20);
    broker.set_data(data);

    auto strategy = std::make_unique<SimpleBuySellStrategy>();
    broker.set_strategy(std::move(strategy));

    broker.run();

    const auto& perf = broker.performance();

    assert(perf.initial_capital == 100000.0);
    assert(perf.total_trades >= 2);

    std::cout << "PASSED\n";
    return true;
}

bool test_performance_calculation() {
    std::cout << "Testing: Performance calculation... ";

    BrokerConfig config;
    config.initial_cash = 1'000'000.0;

    Broker broker(config);

    auto data = generate_test_data(100);
    broker.set_data(data);

    auto strategy = std::make_unique<SimpleBuySellStrategy>();
    strategy->buy_bar = 1;
    strategy->sell_bar = 50;
    broker.set_strategy(std::move(strategy));

    broker.run();

    const auto& equity = broker.equity_curve();
    assert(equity.size() == 100);

    const auto& perf = broker.performance();
    assert(perf.total_trades >= 2);

    std::cout << "PASSED\n";
    return true;
}

bool test_multiple_trades() {
    std::cout << "Testing: Multiple trades... ";

    BrokerConfig config;
    config.initial_cash = 100000.0;

    Broker broker(config);

    // Create data with clear trend changes
    std::vector<BarData> data;
    double price = 100.0;
    for (int i = 0; i < 200; ++i) {
        BarData bar;
        bar.datetime = i * 60000;
        bar.open_price = price;
        bar.high_price = price + 1;
        bar.low_price = price - 1;
        bar.close_price = price;
        bar.volume = 1000.0;

        // Price movement pattern
        if (i < 50) price += 0.2;       // Uptrend
        else if (i < 100) price -= 0.2;  // Downtrend
        else if (i < 150) price += 0.2;  // Uptrend
        else price -= 0.2;                // Downtrend

        data.push_back(bar);
    }

    broker.set_data(data);

    auto strategy = std::make_unique<MovingAverageStrategy>();
    strategy->fast_period = 5;
    strategy->slow_period = 10;
    broker.set_strategy(std::move(strategy));

    broker.run();

    const auto& perf = broker.performance();

    std::cout << "PASSED (trades: " << perf.total_trades << ")\n";
    return true;
}

bool test_trading_engine_backtest() {
    std::cout << "Testing: TradingEngine backtest... ";

    TradingEngineConfig config;
    config.mode = EngineMode::BACKTESTING;
    config.initial_capital = 1'000'000.0;
    config.commission_rate = 0.001;

    TradingEngine engine;
    engine.configure(config);

    auto data = generate_test_data(100);
    engine.load_data("TEST", data);

    auto strategy = std::make_unique<SimpleBuySellStrategy>();
    engine.add_strategy("TestStrategy", std::move(strategy), "TEST", {});

    engine.run_backtest();

    const auto& perf = engine.get_performance();
    assert(perf.initial_capital == 1'000'000.0);

    std::cout << "PASSED\n";
    return true;
}

bool test_risk_manager_order_limit() {
    std::cout << "Testing: RiskManager order limit... ";

    RiskConfig config;
    config.active = true;
    config.order_size_limit = 10.0;

    RiskManager rm(config);

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.volume = 5.0;  // Within limit
    req.price = 50000.0;

    assert(rm.check_risk(req, ""));

    req.volume = 20.0;  // Over limit
    assert(!rm.check_risk(req, ""));

    std::cout << "PASSED\n";
    return true;
}

bool test_event_engine() {
    std::cout << "Testing: EventEngine... ";

    EventEngine engine(1);

    int tick_count = 0;
    int bar_count = 0;

    engine.register_handler(EVENT_TICK, [&](const Event& e) {
        (void)e;
        ++tick_count;
    });

    engine.register_handler(EVENT_BAR, [&](const Event& e) {
        (void)e;
        ++bar_count;
    });

    engine.start();

    // Send events
    TickData tick;
    tick.symbol = "BTCUSDT";
    engine.put(Event(EVENT_TICK, tick));

    BarData bar;
    bar.symbol = "BTCUSDT";
    engine.put(Event(EVENT_BAR, bar));

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    engine.stop();

    assert(tick_count >= 1);
    assert(bar_count >= 1);

    std::cout << "PASSED\n";
    return true;
}

bool test_array_manager() {
    std::cout << "Testing: ArrayManager indicators... ";

    ArrayManager am(100);

    // Add data
    for (int i = 0; i < 50; ++i) {
        BarData bar;
        bar.close_price = 100.0 + i * 0.5;
        am.update_bar(bar);
    }

    assert(am.inited());
    assert(am.count() == 50);

    // Test SMA
    double sma = am.sma(10);
    assert(sma > 0);

    // Test EMA
    double ema = am.ema(10);
    assert(ema > 0);

    // Test RSI
    double rsi = am.rsi(14);
    assert(rsi >= 0 && rsi <= 100);

    // Test MACD
    auto [macd, signal, hist] = am.macd();
    (void)macd; (void)signal; (void)hist;

    // Test Bollinger
    auto [upper, middle, lower] = am.bollinger(20, 2.0);
    assert(upper > lower);

    std::cout << "PASSED\n";
    return true;
}

//=============================================================================
// Performance Benchmark
//=============================================================================

void benchmark_backtest(int num_bars) {
    std::cout << "\n=== Performance Benchmark ===\n";
    std::cout << "Bars: " << num_bars << "\n";

    // Generate large dataset
    auto data = generate_test_data(num_bars);

    BrokerConfig config;
    config.initial_cash = 1'000'000.0;

    Broker broker(config);
    broker.set_data(data);

    auto strategy = std::make_unique<MovingAverageStrategy>();
    broker.set_strategy(std::move(strategy));

    auto start = std::chrono::high_resolution_clock::now();
    broker.run();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << (num_bars * 1000.0 / duration.count()) << " bars/sec\n";

    const auto& perf = broker.performance();
    std::cout << "Trades: " << perf.total_trades << "\n";
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BitQuant C++ Integration Tests                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    try {
        if (test_broker_basic()) ++passed; else ++failed;
        if (test_performance_calculation()) ++passed; else ++failed;
        if (test_multiple_trades()) ++passed; else ++failed;
        if (test_trading_engine_backtest()) ++passed; else ++failed;
        if (test_risk_manager_order_limit()) ++passed; else ++failed;
        if (test_event_engine()) ++passed; else ++failed;
        if (test_array_manager()) ++passed; else ++failed;

        // Benchmark
        benchmark_backtest(10000);
        benchmark_backtest(50000);
        benchmark_backtest(100000);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              Test Results: " << passed << " passed, " << failed << " failed              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}
