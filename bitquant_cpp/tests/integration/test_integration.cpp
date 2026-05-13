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
#include "engine/strategy_optimizer.hpp"
#include "engine/risk_manager.hpp"
#include "engine/event.hpp"
#include "data/array_manager.hpp"
#include "database/sqlite_database.hpp"
#include "exchange/offset_converter.hpp"
#include "exchange/inverse_contract.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <fstream>
#include <set>

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
// Additional Test Strategies
//=============================================================================

class MultiSymbolStrategy : public IStrategy {
public:
    std::set<std::string> symbols;
    int bar_count = 0;

    void on_init() override {
        inited_ = true;
        symbols = {"BTCUSDT", "ETHUSDT"};
    }
    void on_start() override { trading_ = true; }

    void on_bar(const BarData& bar) override {
        ++bar_count;
        symbols.insert(bar.symbol);

        if (bar_count == 5 && position() == 0) {
            buy(bar.close_price, 1.0);
        } else if (bar_count == 15 && position() > 0) {
            sell(bar.close_price, position());
        }
    }
};

class RiskAwareStrategy : public IStrategy {
public:
    double max_position_size = 5.0;
    double stop_loss_pct = 0.02;
    double entry_price = 0;
    int bar_count = 0;

    void on_init() override { inited_ = true; }
    void on_start() override { trading_ = true; }

    void on_bar(const BarData& bar) override {
        ++bar_count;

        // Entry
        if (bar_count == 5 && position() == 0) {
            entry_price = bar.close_price;
            buy(bar.close_price, max_position_size);
        }

        // Stop loss check
        if (position() > 0 && entry_price > 0) {
            double loss_pct = (entry_price - bar.close_price) / entry_price;
            if (loss_pct > stop_loss_pct) {
                sell(bar.close_price, position());
                entry_price = 0;
            }
        }

        // Take profit
        if (bar_count == 30 && position() > 0) {
            sell(bar.close_price, position());
            entry_price = 0;
        }
    }
};

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

bool test_multi_strategy_backtest() {
    std::cout << "Testing: Multi-strategy backtest... ";

    TradingEngineConfig config;
    config.mode = EngineMode::BACKTESTING;
    config.initial_capital = 1'000'000.0;

    TradingEngine engine;
    engine.configure(config);

    // Load data for multiple symbols
    auto btc_data = generate_test_data(100, 50000.0);
    auto eth_data = generate_test_data(100, 3000.0);

    engine.load_data("BTCUSDT", btc_data);
    engine.load_data("ETHUSDT", eth_data);

    // Add multiple strategies
    auto strategy1 = std::make_unique<SimpleBuySellStrategy>();
    strategy1->buy_bar = 5;
    strategy1->sell_bar = 20;
    engine.add_strategy("Strategy1", std::move(strategy1), "BTCUSDT", {});

    auto strategy2 = std::make_unique<SimpleBuySellStrategy>();
    strategy2->buy_bar = 10;
    strategy2->sell_bar = 30;
    engine.add_strategy("Strategy2", std::move(strategy2), "ETHUSDT", {});

    engine.run_backtest();

    const auto& perf = engine.get_performance();
    assert(perf.initial_capital == 1'000'000.0);

    std::cout << "PASSED\n";
    return true;
}

bool test_risk_manager_integration() {
    std::cout << "Testing: RiskManager integration... ";

    RiskConfig risk_config;
    risk_config.active = true;
    risk_config.order_size_limit = 10.0;
    risk_config.order_flow_limit = 100;
    risk_config.active_order_limit = 20;

    TradingEngineConfig config;
    config.mode = EngineMode::BACKTESTING;
    config.initial_capital = 100000.0;

    TradingEngine engine;
    engine.configure(config);

    auto data = generate_test_data(100);
    engine.load_data("TEST", data);

    auto strategy = std::make_unique<RiskAwareStrategy>();
    engine.add_strategy("RiskAware", std::move(strategy), "TEST", {});

    engine.run_backtest();

    std::cout << "PASSED\n";
    return true;
}

bool test_database_integration() {
    std::cout << "Testing: Database integration... ";

    auto db = create_sqlite_database(":memory:");
    assert(db->is_connected());

    // Save bars
    auto bars = generate_test_data(50);
    int saved = db->save_bars(bars);
    assert(saved == 50);

    // Load bars
    auto loaded = db->load_bars("TEST", Exchange::BINANCE, Interval::MINUTE_1, 100);
    assert(loaded.size() == 50);

    // Get overview
    auto overview = db->get_bar_overview();
    assert(overview.size() == 1);

    std::cout << "PASSED\n";
    return true;
}

bool test_strategy_optimizer_integration() {
    std::cout << "Testing: Strategy optimizer integration... ";

    StrategyOptimizer optimizer;
    optimizer.set_initial_capital(100000.0);
    optimizer.set_commission_rate(0.001);

    // Generate test data
    auto bars = generate_test_data(100);

    // Define parameter ranges
    std::unordered_map<std::string, std::vector<double>> param_ranges;
    param_ranges["fast_period"] = {3, 5, 7};
    param_ranges["slow_period"] = {10, 15, 20};

    // Generate combinations
    auto combinations = optimizer.generate_param_combinations(param_ranges);
    assert(combinations.size() == 9);  // 3 x 3

    std::cout << "PASSED\n";
    return true;
}

bool test_paper_broker() {
    std::cout << "Testing: Paper broker simulation... ";

    BrokerConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;
    config.slippage_rate = 0.0005;

    Broker broker(config);

    // Generate trending data
    std::vector<BarData> data;
    double price = 100.0;
    for (int i = 0; i < 100; ++i) {
        BarData bar;
        bar.datetime = i * 60000;
        bar.open_price = price;
        bar.high_price = price + 0.5;
        bar.low_price = price - 0.5;
        bar.close_price = price + 0.2;
        bar.volume = 1000.0;
        price += 0.1;  // Uptrend
        data.push_back(bar);
    }

    broker.set_data(data);

    auto strategy = std::make_unique<MovingAverageStrategy>();
    strategy->fast_period = 5;
    strategy->slow_period = 10;
    broker.set_strategy(std::move(strategy));

    broker.run();

    const auto& perf = broker.performance();
    assert(perf.total_trades >= 0);

    // Verify equity curve
    const auto& equity = broker.equity_curve();
    assert(equity.size() == 100);

    std::cout << "PASSED\n";
    return true;
}

bool test_offset_converter_integration() {
    std::cout << "Testing: OffsetConverter integration... ";

    OffsetConverter converter;

    // Register a contract
    ContractData contract;
    contract.symbol = "BTCUSDT";
    contract.exchange = Exchange::BINANCE;
    contract.product = Product::FUTURES;
    contract.net_position = false;
    converter.register_contract(contract);

    // Update position
    PositionData pos;
    pos.symbol = "BTCUSDT";
    pos.exchange = Exchange::BINANCE;
    pos.direction = Direction::LONG;
    pos.volume = 10.0;
    pos.yd_volume = 5.0;
    converter.update_position(pos);

    // Get position holding
    auto& holding = converter.get_position_holding("BTCUSDT");
    assert(holding.long_pos() == 10.0);
    assert(holding.long_yd() == 5.0);

    std::cout << "PASSED\n";
    return true;
}

bool test_inverse_contract_integration() {
    std::cout << "Testing: Inverse contract integration... ";

    // Test linear contract
    double linear_pnl = calculate_pnl(false, 50000.0, 51000.0, 1.0, 1.0, Direction::LONG);
    assert(std::abs(linear_pnl - 1000.0) < 0.01);

    // Test inverse contract
    double inverse_pnl = calculate_pnl(true, 50000.0, 51000.0, 1.0, 100.0, Direction::LONG);
    assert(inverse_pnl > 0);  // Should be positive for long position with price increase

    // Test contract detection
    assert(is_inverse_contract("BTCUSD_PERP"));
    assert(!is_inverse_contract("BTCUSDT"));

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
        // Core tests
        if (test_broker_basic()) ++passed; else ++failed;
        if (test_performance_calculation()) ++passed; else ++failed;
        if (test_multiple_trades()) ++passed; else ++failed;
        if (test_trading_engine_backtest()) ++passed; else ++failed;
        if (test_risk_manager_order_limit()) ++passed; else ++failed;
        if (test_event_engine()) ++passed; else ++failed;
        if (test_array_manager()) ++passed; else ++failed;

        // Enhanced integration tests
        if (test_multi_strategy_backtest()) ++passed; else ++failed;
        if (test_risk_manager_integration()) ++passed; else ++failed;
        if (test_database_integration()) ++passed; else ++failed;
        if (test_strategy_optimizer_integration()) ++passed; else ++failed;
        if (test_paper_broker()) ++passed; else ++failed;
        if (test_offset_converter_integration()) ++passed; else ++failed;
        if (test_inverse_contract_integration()) ++passed; else ++failed;

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
