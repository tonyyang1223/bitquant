/**
 * @file demo_multi_strategy.cpp
 * @brief Demonstration of multi-strategy trading with TradingEngine
 *
 * Shows:
 * - TradingEngine configuration
 * - Multiple strategy management
 * - Backtesting with performance reporting
 * - Risk management integration
 */

#include "engine/trading_engine.hpp"
#include "engine/strategy.hpp"
#include "data/array_manager.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

//=============================================================================
// Demo Strategies
//=============================================================================

/**
 * @brief Simple moving average crossover strategy
 */
class SMAStrategy : public IStrategy {
public:
    int fast_period = 10;
    int slow_period = 20;

    SMAStrategy() {
        parameters = {"fast_period", "slow_period"};
    }

    void on_init() override {
        am_ = ArrayManager(100);
        inited_ = true;
    }

    void on_start() override {
        trading_ = true;
    }

    void on_bar(const BarData& bar) override {
        am_.update_bar(bar);

        if (!am_.inited()) {
            return;
        }

        double fast_ma = am_.sma(fast_period);
        double slow_ma = am_.sma(slow_period);

        static double prev_diff = 0;
        double diff = fast_ma - slow_ma;

        // Crossover signal
        if (prev_diff <= 0 && diff > 0) {
            // Golden cross - buy
            if (position() <= 0) {
                buy(bar.close_price, 1.0);
            }
        } else if (prev_diff >= 0 && diff < 0) {
            // Death cross - sell
            if (position() > 0) {
                sell(bar.close_price, position());
            }
        }

        prev_diff = diff;
    }

private:
    ArrayManager am_{100};
};

/**
 * @brief RSI mean reversion strategy
 */
class RSIStrategy : public IStrategy {
public:
    int rsi_period = 14;
    double oversold = 30.0;
    double overbought = 70.0;

    RSIStrategy() {
        parameters = {"rsi_period", "oversold", "overbought"};
    }

    void on_init() override {
        am_ = ArrayManager(100);
        inited_ = true;
    }

    void on_start() override {
        trading_ = true;
    }

    void on_bar(const BarData& bar) override {
        am_.update_bar(bar);

        if (!am_.inited()) {
            return;
        }

        double rsi_val = am_.rsi(rsi_period);

        if (rsi_val < oversold && position() <= 0) {
            // Oversold - buy
            buy(bar.close_price, 1.0);
        } else if (rsi_val > overbought && position() > 0) {
            // Overbought - sell
            sell(bar.close_price, position());
        }
    }

private:
    ArrayManager am_{100};
};

/**
 * @brief Bollinger Bands strategy
 */
class BollingerStrategy : public IStrategy {
public:
    int period = 20;
    double std_dev = 2.0;

    BollingerStrategy() {
        parameters = {"period", "std_dev"};
    }

    void on_init() override {
        am_ = ArrayManager(100);
        inited_ = true;
    }

    void on_start() override {
        trading_ = true;
    }

    void on_bar(const BarData& bar) override {
        am_.update_bar(bar);

        if (!am_.inited()) {
            return;
        }

        auto [upper, middle, lower] = am_.bollinger(period, std_dev);

        if (bar.close_price <= lower && position() <= 0) {
            // Price below lower band - buy
            buy(bar.close_price, 1.0);
        } else if (bar.close_price >= upper && position() > 0) {
            // Price above upper band - sell
            sell(bar.close_price, position());
        }
    }

private:
    ArrayManager am_{100};
};

//=============================================================================
// Demo Functions
//=============================================================================

void print_separator(const std::string& title) {
    std::cout << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demo_engine_configuration() {
    print_separator("Trading Engine Configuration");

    // Using builder pattern
    auto engine = TradingEngineBuilder()
        .set_mode(EngineMode::BACKTESTING)
        .set_capital(1'000'000.0)
        .set_commission(0.001)
        .set_slippage(0.0005)
        .set_leverage(1.0)
        .allow_short(true)
        .enable_risk_manager(true)
        .build();

    std::cout << "Engine created with:\n";
    std::cout << "  Mode: Backtesting\n";
    std::cout << "  Capital: $1,000,000\n";
    std::cout << "  Commission: 0.1%\n";
    std::cout << "  Slippage: 0.05%\n";
    std::cout << "  Risk Manager: Enabled\n";
}

void demo_multi_strategy_backtest() {
    print_separator("Multi-Strategy Backtest");

    // Create engine
    TradingEngineConfig config;
    config.mode = EngineMode::BACKTESTING;
    config.initial_capital = 1'000'000.0;
    config.commission_rate = 0.001;
    config.slippage_rate = 0.0005;
    config.enable_risk_manager = true;

    TradingEngine engine;
    engine.configure(config);

    // Set log callback
    engine.set_log_callback([](const std::string&) {
        // Silent during backtest
    });

    // Load data
    std::cout << "Loading historical data...\n";
    size_t bars_loaded = engine.load_data_from_csv("BTCUSDT", "bitmex_btc_usd_1min_data.csv");
    std::cout << "Loaded " << bars_loaded << " bars\n";

    if (bars_loaded == 0) {
        std::cout << "No data available. Please ensure data file exists.\n";
        return;
    }

    // Add strategies
    std::cout << "\nAdding strategies...\n";

    auto sma_strategy = std::make_unique<SMAStrategy>();
    sma_strategy->fast_period = 10;
    sma_strategy->slow_period = 20;
    engine.add_strategy("SMA_Cross", std::move(sma_strategy), "BTCUSDT", {});

    auto rsi_strategy = std::make_unique<RSIStrategy>();
    rsi_strategy->rsi_period = 14;
    rsi_strategy->oversold = 30.0;
    rsi_strategy->overbought = 70.0;
    engine.add_strategy("RSI_Reversion", std::move(rsi_strategy), "BTCUSDT", {});

    auto boll_strategy = std::make_unique<BollingerStrategy>();
    boll_strategy->period = 20;
    boll_strategy->std_dev = 2.0;
    engine.add_strategy("Bollinger", std::move(boll_strategy), "BTCUSDT", {});

    std::cout << "Added 3 strategies:\n";
    for (const auto& name : engine.get_strategy_names()) {
        std::cout << "  - " << name << "\n";
    }

    // Run backtest
    std::cout << "\nRunning backtest...\n";
    engine.run_backtest();

    // Get results
    const auto& metrics = engine.get_performance();

    std::cout << "\n";
    std::cout << metrics.to_string();
}

void demo_risk_management() {
    print_separator("Risk Management Integration");

    // Create risk manager
    RiskConfig risk_config;
    risk_config.active = true;
    risk_config.order_flow_limit = 50;
    risk_config.order_size_limit = 100.0;
    risk_config.trade_limit = 1000.0;
    risk_config.active_order_limit = 50;
    risk_config.order_cancel_limit = 500;
    risk_config.prevent_self_trade = true;

    RiskManager risk_manager(risk_config);

    std::cout << "Risk Manager Configuration:\n";
    std::cout << "  Order flow limit: " << risk_config.order_flow_limit << " per window\n";
    std::cout << "  Order size limit: " << risk_config.order_size_limit << "\n";
    std::cout << "  Trade limit: " << risk_config.trade_limit << " per day\n";
    std::cout << "  Active order limit: " << risk_config.active_order_limit << "\n";
    std::cout << "  Cancel limit: " << risk_config.order_cancel_limit << " per symbol\n";
    std::cout << "  Self-trade prevention: " << (risk_config.prevent_self_trade ? "Yes" : "No") << "\n";

    // Test risk check
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.volume = 10.0;
    req.price = 50000.0;

    std::cout << "\nTest order: BUY 10 BTCUSDT @ $50,000\n";
    bool passed = risk_manager.check_risk(req, "BINANCE");
    std::cout << "Risk check: " << (passed ? "PASSED" : "REJECTED") << "\n";

    // Test oversized order
    req.volume = 200.0;
    std::cout << "\nTest order: BUY 200 BTCUSDT @ $50,000\n";
    passed = risk_manager.check_risk(req, "BINANCE");
    std::cout << "Risk check: " << (passed ? "PASSED" : "REJECTED") << "\n";
}

void demo_strategy_lifecycle() {
    print_separator("Strategy Lifecycle Management");

    TradingEngine engine;
    engine.configure(TradingEngineConfig{});

    std::cout << "Strategy lifecycle demo:\n\n";

    // Add strategy
    auto strategy = std::make_unique<SMAStrategy>();
    engine.add_strategy("Test_Strategy", std::move(strategy), "BTCUSDT", {});

    std::cout << "1. Added strategy: Test_Strategy\n";
    std::cout << "   Status: CREATED\n";

    // Initialize
    engine.init_all_strategies();
    std::cout << "2. Initialized strategy\n";
    std::cout << "   Status: READY\n";

    // Start
    engine.start_all_strategies();
    std::cout << "3. Started strategy\n";
    std::cout << "   Status: RUNNING\n";

    // Stop
    engine.stop_all_strategies();
    std::cout << "4. Stopped strategy\n";
    std::cout << "   Status: STOPPED\n";
}

void demo_engine_modes() {
    print_separator("Trading Engine Modes");

    std::cout << "TradingEngine supports three modes:\n\n";

    std::cout << "1. BACKTESTING Mode\n";
    std::cout << "   - Uses historical data\n";
    std::cout << "   - Order matching via Broker\n";
    std::cout << "   - Performance metrics calculation\n";
    std::cout << "   - No real exchange connection\n\n";

    std::cout << "2. PAPER Mode\n";
    std::cout << "   - Real-time data from exchange\n";
    std::cout << "   - Simulated order execution\n";
    std::cout << "   - No real money at risk\n";
    std::cout << "   - Good for strategy testing\n\n";

    std::cout << "3. LIVE Mode\n";
    std::cout << "   - Real-time data from exchange\n";
    std::cout << "   - Real order execution\n";
    std::cout << "   - Risk management enforced\n";
    std::cout << "   - Real money at risk!\n";
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║      BitQuant C++ Multi-Strategy Trading Engine Demo       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    try {
        demo_engine_configuration();
        demo_multi_strategy_backtest();
        demo_risk_management();
        demo_strategy_lifecycle();
        demo_engine_modes();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    print_separator("Demo Complete");
    std::cout << "All demonstrations completed successfully!\n";

    return 0;
}
