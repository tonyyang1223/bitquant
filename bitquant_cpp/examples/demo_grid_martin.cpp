/**
 * @file demo_grid_martin.cpp
 * @brief Grid Martin strategy backtest demo using BinanceSpotGateway
 *
 * Demonstrates:
 * - Fetching historical data from Binance API
 * - Configuring GridMartinStrategy with BTCUSDT parameters
 * - Running backtest with Broker
 * - Displaying performance results
 */

#include "engine/broker.hpp"
#include "strategy/grid_martin_strategy.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

//=============================================================================
// Helper Functions
//=============================================================================

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_strategy_config(double base_price, int grid_count, double grid_spacing,
                           double amount_per_grid, double initial_capital) {
    std::cout << "\n=== GridMartinStrategy Configuration ===\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Symbol:          BTCUSDT\n";
    std::cout << "  Base Price:      $" << std::setprecision(0) << base_price << "\n";
    std::cout << "  Grid Count:      " << grid_count << "\n";
    std::cout << "  Grid Spacing:    " << std::setprecision(1) << (grid_spacing * 100) << "%\n";
    std::cout << "  Amount per Grid: $" << std::setprecision(0) << amount_per_grid << "\n";
    std::cout << "  Initial Capital: $" << initial_capital << "\n";
}

void print_grid_levels(const std::vector<double>& levels) {
    std::cout << "\n=== Grid Levels ===\n";
    std::cout << std::fixed << std::setprecision(2);
    for (size_t i = 0; i < levels.size(); ++i) {
        std::cout << "  Grid " << std::setw(2) << i << ": $"
                  << std::setprecision(0) << levels[i] << "\n";
    }
}

void print_results(const PerformanceMetrics& metrics) {
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n=== Backtest Results ===\n";
    std::cout << "Initial Capital:    $" << std::setprecision(0) << metrics.initial_capital << "\n";
    std::cout << "Final Capital:      $" << metrics.final_capital << "\n";
    std::cout << "Total Return:       " << std::setprecision(2) << (metrics.total_return * 100) << "%\n";
    std::cout << "Annualized Return:  " << (metrics.annualized_return * 100) << "%\n";
    std::cout << "Sharpe Ratio:       " << metrics.sharpe_ratio << "\n";
    std::cout << "Max Drawdown:       " << (metrics.max_drawdown * 100) << "%\n";
    std::cout << "Win Rate:           " << (metrics.win_rate * 100) << "%\n";
    std::cout << "Total Trades:       " << metrics.total_trades << "\n";
    std::cout << "Winning Trades:     " << metrics.winning_trades << "\n";
    std::cout << "Losing Trades:      " << metrics.losing_trades << "\n";
    std::cout << "Total Commission:   $" << std::setprecision(2) << metrics.total_commission << "\n";
}

//=============================================================================
// Main
//=============================================================================

int main() {
    print_header("Grid Martin Strategy Backtest Demo");

    // Strategy parameters
    const double base_price = 85000.0;
    const int grid_count = 10;
    const double grid_spacing = 0.01;    // 1%
    const double amount_per_grid = 100.0; // $100 per grid
    const double initial_capital = 10000.0;

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
        broker_config.initial_cash = initial_capital;
        broker_config.commission_rate = 0.001;   // 0.1% commission
        broker_config.slippage_rate = 0.0005;    // 0.05% slippage
        broker_config.leverage = 1.0;            // No leverage
        broker_config.allow_short = false;       // Spot trading, no short

        Broker broker(broker_config);

        std::cout << "    Capital: $" << std::setprecision(0) << initial_capital << "\n";
        std::cout << "    Commission: 0.1%\n";
        std::cout << "    Slippage: 0.05%\n";

        // Step 3: Set historical data
        std::cout << "\n[3] Loading historical data...\n";
        broker.set_data(bars);

        // Step 4: Create and configure GridMartinStrategy
        std::cout << "\n[4] Configuring GridMartinStrategy...\n";

        auto strategy = std::make_unique<GridMartinStrategy>();
        strategy->base_price_ = base_price;
        strategy->grid_count_ = grid_count;
        strategy->grid_spacing_ = grid_spacing;
        strategy->amount_per_grid_ = amount_per_grid;
        strategy->symbol_ = "BTCUSDT";

        print_strategy_config(base_price, grid_count, grid_spacing,
                              amount_per_grid, initial_capital);

        // Show grid levels (calculated in constructor)
        // Note: grid_levels are calculated in on_init(), so we need to trigger it first
        // For display purposes, we manually calculate expected levels
        std::vector<double> expected_levels;
        for (int i = 0; i < grid_count; ++i) {
            double level = base_price * (1.0 - grid_spacing * (grid_count - i));
            expected_levels.push_back(level);
        }
        print_grid_levels(expected_levels);

        broker.set_strategy(std::move(strategy));

        // Step 5: Run backtest
        std::cout << "\n[5] Running backtest...\n";
        broker.run();

        // Step 6: Display results
        const auto& metrics = broker.performance();
        print_results(metrics);

        print_header("Demo Complete");
        std::cout << "Backtest completed successfully!\n";
        std::cout << "The GridMartinStrategy demonstrates grid-based trading.\n";
        std::cout << "Results are for demonstration only, not investment advice.\n";

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
