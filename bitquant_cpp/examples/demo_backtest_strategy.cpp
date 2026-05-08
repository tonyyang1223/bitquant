/**
 * @file demo_backtest_strategy.cpp
 * @brief Simple backtest demo using BinanceSpotGateway for historical data
 *
 * Demonstrates:
 * - Fetching historical data from Binance API
 * - Creating a simple SMA crossover strategy
 * - Running backtest with Broker
 * - Displaying performance results
 */

#include "engine/broker.hpp"
#include "engine/strategy.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "data/array_manager.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

//=============================================================================
// Simple SMA Crossover Strategy
//=============================================================================

/**
 * @brief Simple moving average crossover strategy
 *
 * Logic:
 * - Buy when short SMA crosses above long SMA (golden cross)
 * - Sell when short SMA crosses below long SMA (death cross)
 */
class SimpleSMAStrategy : public IStrategy {
public:
    int short_period = 10;
    int long_period = 20;

    SimpleSMAStrategy() {
        parameters = {"short_period", "long_period"};
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

        double short_ma = am_.sma(short_period);
        double long_ma = am_.sma(long_period);

        static double prev_diff = 0.0;
        double diff = short_ma - long_ma;

        // Golden cross - buy signal
        if (prev_diff <= 0.0 && diff > 0.0) {
            if (is_flat()) {
                buy(bar.close_price, 1.0);
            }
        }
        // Death cross - sell signal
        else if (prev_diff >= 0.0 && diff < 0.0) {
            if (is_long()) {
                sell(bar.close_price, position());
            }
        }

        prev_diff = diff;
    }

private:
    ArrayManager am_{100};
};

//=============================================================================
// Helper Functions
//=============================================================================

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

void print_results(const PerformanceMetrics& metrics) {
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n=== Backtest Results ===\n";
    std::cout << "Initial Capital:  $" << std::setprecision(0) << metrics.initial_capital << "\n";
    std::cout << "Final Capital:    $" << metrics.final_capital << "\n";
    std::cout << "Total Return:     " << std::setprecision(2) << (metrics.total_return * 100) << "%\n";
    std::cout << "Annualized Return: " << (metrics.annualized_return * 100) << "%\n";
    std::cout << "Sharpe Ratio:     " << metrics.sharpe_ratio << "\n";
    std::cout << "Max Drawdown:     " << (metrics.max_drawdown * 100) << "%\n";
    std::cout << "Win Rate:         " << (metrics.win_rate * 100) << "%\n";
    std::cout << "Total Trades:     " << metrics.total_trades << "\n";
    std::cout << "Winning Trades:   " << metrics.winning_trades << "\n";
    std::cout << "Losing Trades:    " << metrics.losing_trades << "\n";
    std::cout << "Total Commission: $" << metrics.total_commission << "\n";
}

//=============================================================================
// Main
//=============================================================================

int main() {
    print_header("Simple Backtest Strategy Demo");

    try {
        // Step 1: Fetch historical data from Binance
        std::cout << "[1] Fetching historical data from Binance...\n";

        BinanceSpotGateway gateway;
        GatewayConfig config;
        config.api_key = "";      // Public API, no key needed
        config.api_secret = "";

        if (!gateway.connect(config)) {
            std::cerr << "Failed to connect to Binance: " << gateway.last_error() << "\n";
            return 1;
        }

        // Fetch 500 hourly bars for BTCUSDT
        HistoryRequest req;
        req.symbol = "BTCUSDT";
        req.interval = Interval::HOUR_1;
        req.limit = 500;

        auto bars = gateway.get_bars(req);
        gateway.close();

        if (bars.empty()) {
            std::cerr << "No historical data received.\n";
            return 1;
        }

        std::cout << "    Fetched " << bars.size() << " bars (BTCUSDT 1H)\n";
        std::cout << "    Date range: ";
        if (!bars.empty()) {
            std::time_t first_t = bars.front().datetime / 1000;
            std::time_t last_t = bars.back().datetime / 1000;
            std::cout << std::put_time(std::gmtime(&first_t), "%Y-%m-%d") << " to ";
            std::cout << std::put_time(std::gmtime(&last_t), "%Y-%m-%d") << "\n";
        }

        // Step 2: Create Broker with backtest configuration
        std::cout << "\n[2] Configuring Broker...\n";

        BrokerConfig broker_config;
        broker_config.initial_cash = 100'000.0;    // $100,000 initial capital
        broker_config.commission_rate = 0.001;      // 0.1% commission
        broker_config.slippage_rate = 0.0005;       // 0.05% slippage
        broker_config.leverage = 1.0;               // No leverage
        broker_config.allow_short = false;          // Spot trading, no short

        Broker broker(broker_config);

        std::cout << "    Capital: $100,000\n";
        std::cout << "    Commission: 0.1%\n";
        std::cout << "    Slippage: 0.05%\n";

        // Step 3: Set historical data
        std::cout << "\n[3] Loading historical data...\n";
        broker.set_data(bars);

        // Step 4: Create and set strategy
        std::cout << "\n[4] Adding SimpleSMAStrategy...\n";

        auto strategy = std::make_unique<SimpleSMAStrategy>();
        strategy->short_period = 10;
        strategy->long_period = 20;

        std::cout << "    Short SMA period: 10\n";
        std::cout << "    Long SMA period: 20\n";

        broker.set_strategy(std::move(strategy));

        // Step 5: Run backtest
        std::cout << "\n[5] Running backtest...\n";
        broker.run();

        // Step 6: Display results
        const auto& metrics = broker.performance();
        print_results(metrics);

        print_header("Demo Complete");
        std::cout << "Backtest completed successfully!\n";
        std::cout << "The SimpleSMAStrategy demonstrates basic SMA crossover logic.\n";
        std::cout << "Results are for demonstration only, not investment advice.\n";

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}