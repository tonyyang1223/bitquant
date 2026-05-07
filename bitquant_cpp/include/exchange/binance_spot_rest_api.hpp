/**
 * @file binance_spot_rest_api.hpp
 * @brief Binance Spot REST API client
 *
 * Wraps binapi library for Spot trading:
 * - Market data: ping, time, klines, price, exchange info
 * - Trading: send_order, cancel_order, query_order
 * - Account: query_account, query_open_orders
 * - User stream: start/keep/close user data stream
 */

#pragma once

#include "core/types.hpp"
#include "exchange/binance_mapping.hpp"
#include <string>
#include <vector>
#include <memory>

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
 * @brief Binance Spot REST API client
 *
 * Wraps binapi library for Spot trading:
 * - Market data: ping, time, klines, price, exchange info
 * - Trading: send_order, cancel_order, query_order
 * - Account: query_account, query_open_orders
 * - User stream: start/keep/close user data stream
 */
class BinanceSpotRestApi {
public:
    BinanceSpotRestApi();
    ~BinanceSpotRestApi();

    // Non-copyable
    BinanceSpotRestApi(const BinanceSpotRestApi&) = delete;
    BinanceSpotRestApi& operator=(const BinanceSpotRestApi&) = delete;

    //=========================================================================
    // Initialization
    //=========================================================================

    /**
     * @brief Initialize REST API client
     * @param host API host (e.g., "api.binance.com")
     * @param port API port (e.g., "443")
     * @param api_key API key (optional for public data)
     * @param api_secret API secret (optional for public data)
     * @param timeout_ms Request timeout in milliseconds
     * @return true on success
     */
    bool init(const std::string& host,
              const std::string& port,
              const std::string& api_key = "",
              const std::string& api_secret = "",
              size_t timeout_ms = 10000);

    //=========================================================================
    // Market Data
    //=========================================================================

    /**
     * @brief Ping Binance server
     * @return true if ping successful
     */
    bool ping();

    /**
     * @brief Get server time
     * @return Server timestamp in milliseconds, 0 on error
     */
    int64_t get_server_time();

    /**
     * @brief Get current price
     * @param symbol Trading pair (e.g., "BTCUSDT")
     * @return Current price, 0 on error
     */
    double get_price(const std::string& symbol);

    /**
     * @brief Get kline/candlestick data
     * @param symbol Trading pair
     * @param interval Kline interval
     * @param limit Number of klines (max 1000)
     * @return Vector of BarData
     */
    std::vector<BarData> get_klines(const std::string& symbol,
                                     Interval interval,
                                     size_t limit = 500);

    /**
     * @brief Get exchange info (contracts)
     * @return Vector of ContractData
     */
    std::vector<ContractData> get_exchange_info();

    //=========================================================================
    // Trading
    //=========================================================================

    /**
     * @brief Send order
     * @param req Order request
     * @return Order ID on success, empty string on failure
     */
    std::string send_order(const OrderRequest& req);

    /**
     * @brief Cancel order by symbol and order ID
     * @param symbol Trading pair
     * @param order_id Order ID (client order ID)
     * @return true on success
     */
    bool cancel_order(const std::string& symbol, const std::string& order_id);

    /**
     * @brief Cancel order using CancelRequest (IExchange interface)
     * @param req Cancel request
     * @return true on success
     */
    bool cancel_order(const CancelRequest& req);

    /**
     * @brief Query order status by symbol and order ID
     * @param symbol Trading pair
     * @param order_id Order ID (client order ID)
     * @return OrderData if found
     */
    std::optional<OrderData> query_order(const std::string& symbol,
                                          const std::string& order_id);

    /**
     * @brief Query order using OrderQueryRequest (IExchange interface)
     * @param req Order query request
     * @return OrderData if found
     */
    std::optional<OrderData> query_order(const OrderQueryRequest& req);

    /**
     * @brief Query all open orders
     * @param symbol Trading pair (empty for all symbols)
     * @return Vector of OrderData
     */
    std::vector<OrderData> query_open_orders(const std::string& symbol = "");

    //=========================================================================
    // Account
    //=========================================================================

    /**
     * @brief Query account balances
     * @return Vector of AccountData (one per asset)
     */
    std::vector<AccountData> query_account();

    /**
     * @brief Query account balance (single aggregated result, IExchange interface)
     * @return Aggregated AccountData if available
     */
    std::optional<AccountData> query_account_single();

    //=========================================================================
    // User Data Stream
    //=========================================================================

    /**
     * @brief Start user data stream
     * @return Listen key on success, empty on failure
     */
    std::string start_user_stream();

    /**
     * @brief Keep user data stream alive
     * @param listen_key Listen key from start_user_stream
     * @return true on success
     */
    bool keep_user_stream(const std::string& listen_key);

    /**
     * @brief Close user data stream
     * @param listen_key Listen key
     * @return true on success
     */
    bool close_user_stream(const std::string& listen_key);

    //=========================================================================
    // Utility
    //=========================================================================

    /**
     * @brief Get time offset (local - server)
     */
    int64_t time_offset() const noexcept { return time_offset_; }

    /**
     * @brief Get last error message
     */
    const std::string& last_error() const noexcept { return last_error_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string api_key_;
    std::string api_secret_;
    std::string host_;
    std::string port_;
    size_t timeout_ms_ = 10000;
    int64_t time_offset_ = 0;
    std::string last_error_;
};

} // namespace bitquant
