/**
 * @file demo_strategy.cpp
 * @brief Demo strategy - port from Python demo_strategy.py
 *
 * This demonstrates how to create a trading strategy in C++.
 */

#include "engine/broker.hpp"
#include "engine/strategy.hpp"
#include "data/array_manager.hpp"
#include "utils/datetime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace bitquant {

/**
 * @brief Demo strategy using SMA crossover
 */
class DemoStrategy : public IStrategy {
public:
    // Strategy parameters
    int long_period = 110;
    int short_period = 70;

    DemoStrategy() : am_(1000) {}

    void on_start() override {
        std::cout << "策略开始运行.." << std::endl;
    }

    void on_stop() override {
        std::cout << "策略停止运行.." << std::endl;
    }

    void on_bar(const BarData& bar) override {
        // Update array manager
        am_.update_bar(bar);

        if (!am_.inited()) {
            return;
        }

        // Calculate indicators
        double sma_long = am_.sma(long_period);
        double sma_short = am_.sma(short_period);

        // Generate signals
        double current_pos = position();

        if (sma_short > sma_long && current_pos == 0) {
            // Buy signal
            buy(bar.close_price, 1.0);
            std::cout << "买入信号: " << timestamp_to_string(bar.datetime)
                      << " @ " << bar.close_price << std::endl;
        } else if (sma_short < sma_long && current_pos > 0) {
            // Sell signal
            sell(bar.close_price, 1.0);
            std::cout << "卖出信号: " << timestamp_to_string(bar.datetime)
                      << " @ " << bar.close_price << std::endl;
        }
    }

private:
    ArrayManager am_;
};

/**
 * @brief Load CSV data
 */
std::vector<BarData> load_csv(const std::string& filename) {
    std::vector<BarData> bars;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return bars;
    }

    std::string line;
    std::getline(file, line);  // Skip header

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 6) {
            BarData bar;
            bar.datetime = parse_datetime(tokens[0]);
            bar.open_price = std::stod(tokens[1]);
            bar.high_price = std::stod(tokens[2]);
            bar.low_price = std::stod(tokens[3]);
            bar.close_price = std::stod(tokens[4]);
            bar.volume = std::stod(tokens[5]);
            bars.push_back(bar);
        }
    }

    return bars;
}

} // namespace bitquant

int main(int argc, char* argv[]) {
    using namespace bitquant;

    std::cout << "=== BitQuant C++ Backtest Demo ===" << std::endl;

    // Default data file
    std::string data_file = "bitmex_btc_usd_1min_data.csv";
    if (argc > 1) {
        data_file = argv[1];
    }

    // Load data
    std::cout << "Loading data from: " << data_file << std::endl;
    auto data = load_csv(data_file);
    std::cout << "Loaded " << data.size() << " bars" << std::endl;

    if (data.empty()) {
        std::cerr << "No data loaded. Exiting." << std::endl;
        return 1;
    }

    // Create broker
    BrokerConfig config;
    config.initial_cash = 1'000'000.0;
    config.commission_rate = 0.002;
    config.slippage_rate = 0.0005;
    config.leverage = 1.0;

    Broker broker(config);

    // Create strategy
    auto strategy = std::make_unique<DemoStrategy>();
    strategy->long_period = 110;
    strategy->short_period = 70;

    // Configure broker
    broker.set_strategy(std::move(strategy));
    broker.set_data(data);

    // Run backtest
    std::cout << "\nRunning backtest..." << std::endl;
    broker.run();

    // Print results
    const auto& performance = broker.performance();
    std::cout << "\n" << performance.to_string() << std::endl;

    return 0;
}
