/**
 * @file binance_api.hpp
 * @brief Binance API wrapper for fetching market data
 *
 * Wraps binapi library to provide a simple interface for:
 * - Fetching kline/candlestick data
 * - Getting exchange info
 * - Price ticker
 *
 * Port of Python binance_api/binance.py
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations for binapi
namespace boost {
namespace asio {
    class io_context;
}
}

namespace binapi {
namespace rest {
    class api;
}
}

namespace bitquant {

/**
 * @brief Binance API configuration
 */
struct BinanceConfig {
    std::string host = "api.binance.com";
    std::string port = "443";
    std::string api_key;      // Optional for public data
    std::string api_secret;   // Optional for public data
    size_t timeout_ms = 10000;
    bool testnet = false;
};

/**
 * @brief Kline interval enum
 */
enum class KlineInterval {
    MIN_1, MIN_3, MIN_5, MIN_15, MIN_30,
    HOUR_1, HOUR_2, HOUR_4, HOUR_6, HOUR_8, HOUR_12,
    DAY_1, DAY_3,
    WEEK_1,
    MONTH_1
};

/**
 * @brief Convert KlineInterval to Binance string
 */
const char* kline_interval_to_string(KlineInterval interval);

/**
 * @brief Binance API client for market data
 *
 * Provides synchronous interface for fetching market data.
 * Thread-safe after initialization.
 *
 * Usage:
 * @code
 * BinanceApiClient client;
 * client.init();
 * auto klines = client.get_klines("BTCUSDT", KlineInterval::MIN_1, 1000);
 * for (const auto& bar : klines) {
 *     std::cout << bar.close_price << std::endl;
 * }
 * @endcode
 */
class BinanceApiClient {
public:
    BinanceApiClient();
    explicit BinanceApiClient(const BinanceConfig& config);
    ~BinanceApiClient();

    // Non-copyable
    BinanceApiClient(const BinanceApiClient&) = delete;
    BinanceApiClient& operator=(const BinanceApiClient&) = delete;

    // Movable
    BinanceApiClient(BinanceApiClient&&) noexcept;
    BinanceApiClient& operator=(BinanceApiClient&&) noexcept;

    /**
     * @brief Initialize the client (connect to exchange)
     * @return true on success
     */
    bool init();

    /**
     * @brief Check connectivity to Binance
     */
    bool ping();

    /**
     * @brief Get server time
     * @return Server timestamp in milliseconds, 0 on error
     */
    uint64_t get_server_time();

    /**
     * @brief Get kline/candlestick data
     * @param symbol Trading pair (e.g., "BTCUSDT")
     * @param interval Kline interval
     * @param limit Number of klines (max 1000)
     * @return Vector of BarData
     */
    std::vector<BarData> get_klines(
        const std::string& symbol,
        KlineInterval interval,
        size_t limit = 500
    );

    // Note: binapi doesn't support time range klines directly
    // For historical data fetching, use fetch_historical_data()

    /**
     * @brief Get current price
     * @param symbol Trading pair
     * @return Current price, 0 on error
     */
    double get_price(const std::string& symbol);

    /**
     * @brief Fetch historical data (like Python version)
     *
     * Fetches data in batches, saves to CSV files.
     * Compatible with Python binance.py behavior.
     *
     * @param symbol Trading pair
     * @param interval Kline interval
     * @param total_bars Total bars to fetch
     * @param output_dir Output directory for CSV files
     * @return Total bars fetched
     */
    size_t fetch_historical_data(
        const std::string& symbol,
        KlineInterval interval,
        size_t total_bars,
        const std::string& output_dir = "."
    );

    /**
     * @brief Get last error message
     */
    const std::string& last_error() const { return last_error_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    BinanceConfig config_;
    std::string last_error_;
};

/**
 * @brief Save klines to CSV file
 *
 * CSV format: open_time,open,high,low,close,volume
 * Compatible with Python version format.
 *
 * @param klines Kline data
 * @param filename Output filename
 * @return true on success
 */
bool save_klines_to_csv(
    const std::vector<BarData>& klines,
    const std::string& filename
);

/**
 * @brief Load klines from CSV file
 * @param filename Input filename
 * @return Vector of BarData
 */
std::vector<BarData> load_klines_from_csv(const std::string& filename);

} // namespace bitquant
