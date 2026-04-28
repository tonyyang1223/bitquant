/**
 * @file demo_binance.cpp
 * @brief Demo for Binance API - port from Python binance_api/binance.py
 *
 * Demonstrates:
 * 1. Ping Binance server
 * 2. Get server time
 * 3. Fetch kline data
 * 4. Get current price
 * 5. Fetch historical data (like Python version)
 */

#include "exchange/binance_api.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

void print_bar(const BarData& bar) {
    std::cout << "Time: " << bar.datetime
              << " | O: " << std::fixed << std::setprecision(2) << bar.open_price
              << " | H: " << bar.high_price
              << " | L: " << bar.low_price
              << " | C: " << bar.close_price
              << " | V: " << std::setprecision(4) << bar.volume
              << std::endl;
}

int main() {
    std::cout << "=== Binance API Demo (C++) ===" << std::endl;
    std::cout << "Port from Python binance_api/binance.py" << std::endl;
    std::cout << std::endl;

    // Create client
    BinanceConfig config;
    config.host = "api.binance.com";
    config.port = "443";
    config.timeout_ms = 10000;

    BinanceApiClient client(config);

    // Initialize
    std::cout << "[1] Initializing connection..." << std::endl;
    if (!client.init()) {
        std::cerr << "Failed to initialize: " << client.last_error() << std::endl;
        return 1;
    }
    std::cout << "Connected to Binance!" << std::endl;
    std::cout << std::endl;

    // Ping
    std::cout << "[2] Pinging Binance..." << std::endl;
    if (client.ping()) {
        std::cout << "Ping successful!" << std::endl;
    } else {
        std::cerr << "Ping failed: " << client.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get server time
    std::cout << "[3] Getting server time..." << std::endl;
    uint64_t server_time = client.get_server_time();
    if (server_time > 0) {
        std::time_t t = server_time / 1000;
        std::cout << "Server time: " << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S UTC") << std::endl;
    } else {
        std::cerr << "Failed: " << client.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get current price
    std::cout << "[4] Getting BTCUSDT price..." << std::endl;
    double price = client.get_price("BTCUSDT");
    if (price > 0) {
        std::cout << "Current BTCUSDT price: $" << std::fixed << std::setprecision(2) << price << std::endl;
    } else {
        std::cerr << "Failed: " << client.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get klines
    std::cout << "[5] Fetching recent 10 klines (BTCUSDT 1m)..." << std::endl;
    auto klines = client.get_klines("BTCUSDT", KlineInterval::MIN_1, 10);
    if (!klines.empty()) {
        std::cout << "Fetched " << klines.size() << " klines:" << std::endl;
        for (const auto& bar : klines) {
            print_bar(bar);
        }
    } else {
        std::cerr << "Failed: " << client.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Save to CSV
    if (!klines.empty()) {
        std::cout << "[6] Saving klines to CSV..." << std::endl;
        if (save_klines_to_csv(klines, "klines_demo.csv")) {
            std::cout << "Saved to klines_demo.csv" << std::endl;
        } else {
            std::cerr << "Failed to save CSV" << std::endl;
        }
    }
    std::cout << std::endl;

    // Optional: Fetch historical data (like Python version)
    // Uncomment to test (will fetch 2000 bars)
    /*
    std::cout << "[7] Fetching historical data (like Python version)..." << std::endl;
    size_t total = client.fetch_historical_data("BTCUSDT", KlineInterval::MIN_1, 2000);
    std::cout << "Total bars fetched: " << total << std::endl;
    */

    std::cout << "=== Demo Complete ===" << std::endl;
    return 0;
}
