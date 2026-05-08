/**
 * @file demo_binance_spot.cpp
 * @brief Demo for Binance Spot Gateway - demonstrates IExchange interface
 *
 * This demo tests the BinanceSpotGateway functionality:
 * 1. Create BinanceSpotGateway
 * 2. Connect (public API only, no trading)
 * 3. Ping server
 * 4. Get server time
 * 5. Get BTCUSDT price
 * 6. Fetch klines
 * 7. Get exchange info
 * 8. Get BTCUSDT contract details
 * 9. Close connection
 */

#include "exchange/binance_spot_gateway.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

int main() {
    std::cout << "=== Binance Spot Gateway Demo ===" << std::endl;
    std::cout << std::endl;

    // Create gateway
    BinanceSpotGateway gateway;

    // Connect (public API only, no trading)
    GatewayConfig config;
    config.api_key = "";  // Empty for public data only
    config.api_secret = "";

    std::cout << "[1] Connecting to Binance..." << std::endl;
    if (!gateway.connect(config)) {
        std::cerr << "Connection failed: " << gateway.last_error() << std::endl;
        return 1;
    }
    std::cout << "Connected!" << std::endl;
    std::cout << std::endl;

    // Ping
    std::cout << "[2] Ping..." << std::endl;
    if (gateway.ping()) {
        std::cout << "Ping successful!" << std::endl;
    } else {
        std::cerr << "Ping failed: " << gateway.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get server time
    std::cout << "[3] Server time..." << std::endl;
    int64_t time = gateway.get_server_time();
    if (time > 0) {
        std::time_t t = time / 1000;
        std::cout << "Server time: " << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S UTC") << std::endl;
    } else {
        std::cerr << "Failed: " << gateway.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get price
    std::cout << "[4] BTCUSDT price..." << std::endl;
    double price = gateway.get_price("BTCUSDT");
    if (price > 0) {
        std::cout << "Current price: $" << std::fixed << std::setprecision(2) << price << std::endl;
    } else {
        std::cerr << "Failed: " << gateway.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get klines
    std::cout << "[5] Fetching klines..." << std::endl;
    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.interval = Interval::MINUTE_1;
    req.limit = 10;

    auto bars = gateway.get_bars(req);
    if (!bars.empty()) {
        std::cout << "Fetched " << bars.size() << " bars:" << std::endl;
        for (const auto& bar : bars) {
            std::cout << "  O:" << std::fixed << std::setprecision(2) << bar.open_price
                      << " H:" << bar.high_price
                      << " L:" << bar.low_price
                      << " C:" << bar.close_price
                      << " V:" << std::setprecision(4) << bar.volume
                      << std::endl;
        }
    } else {
        std::cerr << "Failed to fetch bars: " << gateway.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Get contracts
    std::cout << "[6] Exchange info..." << std::endl;
    auto contracts = gateway.get_exchange_info();
    std::cout << "Total contracts: " << contracts.size() << std::endl;
    std::cout << std::endl;

    // Find BTCUSDT contract
    std::cout << "[7] BTCUSDT contract details..." << std::endl;
    auto btc_contract = gateway.get_contract("BTCUSDT");
    if (btc_contract) {
        std::cout << "BTCUSDT:" << std::endl;
        std::cout << "  Symbol: " << btc_contract->symbol << std::endl;
        std::cout << "  Exchange: " << static_cast<int>(btc_contract->exchange) << std::endl;
        std::cout << "  min_volume: " << btc_contract->min_volume << std::endl;
        std::cout << "  pricetick: " << btc_contract->pricetick << std::endl;
    } else {
        std::cerr << "Failed to get BTCUSDT contract: " << gateway.last_error() << std::endl;
    }
    std::cout << std::endl;

    // Close
    std::cout << "[8] Closing connection..." << std::endl;
    gateway.close();
    std::cout << "Done!" << std::endl;

    return 0;
}
