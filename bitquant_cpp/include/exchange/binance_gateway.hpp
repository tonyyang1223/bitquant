/**
 * @file binance_gateway.hpp
 * @brief Binance exchange gateway implementing IExchange interface
 *
 * Based on howtrader's BinanceGateway design:
 * - REST API for market data and trading
 * - WebSocket for real-time data streaming
 * - Order management
 * - Account/position query
 */

#pragma once

#include "i_exchange.hpp"
#include "core/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace bitquant {

//=============================================================================
// Binance Gateway Configuration
//=============================================================================

/**
 * @brief Binance gateway configuration
 */
struct BinanceGatewayConfig {
    std::string host = "api.binance.com";
    std::string port = "443";
    std::string api_key;
    std::string api_secret;
    size_t timeout_ms = 10000;
    bool testnet = false;
    bool use_websocket = true;
    std::string ws_host = "stream.binance.com";
    std::string ws_port = "9443";
};

//=============================================================================
// Binance Gateway
//=============================================================================

/**
 * @brief Binance exchange gateway
 *
 * Implements IExchange interface for Binance exchange.
 * Supports:
 * - REST API: market data, order management, account query
 * - WebSocket: real-time tick and bar data streaming
 *
 * Usage:
 * @code
 * BinanceGateway gateway;
 * gateway.connect(config_json);
 * gateway.subscribe_tick("BTCUSDT", on_tick);
 * gateway.send_order(order_req);
 * gateway.close();
 * @endcode
 */
class BinanceGateway : public IExchange {
public:
    BinanceGateway();
    explicit BinanceGateway(const BinanceGatewayConfig& config);
    ~BinanceGateway() override;

    //=========================================================================
    // IExchange Interface Implementation
    //=========================================================================

    std::string name() const override { return "Binance"; }
    Exchange exchange() const override { return Exchange::BINANCE; }
    std::string gateway_name() const override { return "BINANCE"; }

    std::vector<Exchange> supported_exchanges() const override {
        return { Exchange::BINANCE };
    }

    ExchangeCapabilities capabilities() const override {
        return {
            .market_data = true,
            .trading = true,
            .stop_order = false,
            .history_data = true,
            .websocket = true,
            .margin = true,
            .futures = true,
            .options = false
        };
    }

    //=========================================================================
    // Connection Management
    //=========================================================================

    bool connect(const std::string& config) override;
    void close() override;
    bool is_connected() const override;

    //=========================================================================
    // Market Data - Synchronous
    //=========================================================================

    std::optional<TickData> get_tick(const std::string& symbol) override;
    std::vector<BarData> get_bars(const HistoryRequest& req) override;
    double get_price(const std::string& symbol) override;
    std::optional<ContractData> get_contract(const std::string& symbol) override;

    //=========================================================================
    // Market Data - Asynchronous (WebSocket)
    //=========================================================================

    void subscribe_tick(const SubscribeRequest& req) override;
    void subscribe_bar(const std::string& symbol, Interval interval) override;
    void unsubscribe_tick(const std::string& symbol) override;

    //=========================================================================
    // Order Management
    //=========================================================================

    std::string send_order(const OrderRequest& req) override;
    bool cancel_order(const CancelRequest& req) override;
    void cancel_all_orders(const std::string& symbol = "") override;
    std::optional<OrderData> query_order(const std::string& orderid) override;

    //=========================================================================
    // Account Management
    //=========================================================================

    std::vector<PositionData> query_positions(const std::string& symbol = "") override;
    std::optional<AccountData> query_account() override;
    std::vector<OrderData> query_open_orders(const std::string& symbol = "") override;

    //=========================================================================
    // Utility Methods
    //=========================================================================

    /**
     * @brief Get server time
     */
    uint64_t get_server_time();

    /**
     * @brief Check connectivity (ping)
     */
    bool ping();

    /**
     * @brief Get exchange info
     */
    std::vector<ContractData> get_exchange_info();

    /**
     * @brief Fetch historical data to CSV (like Python version)
     */
    size_t fetch_historical_data(
        const std::string& symbol,
        Interval interval,
        size_t total_bars,
        const std::string& output_dir = "."
    );

    /**
     * @brief Get last error message
     */
    const std::string& last_error() const { return last_error_; }

private:
    //=========================================================================
    // Internal Implementation
    //=========================================================================

    /**
     * @brief Initialize REST API client
     */
    bool init_rest_api();

    /**
     * @brief Initialize WebSocket connection
     */
    bool init_websocket();

    /**
     * @brief WebSocket processing thread
     */
    void ws_thread_func();

    /**
     * @brief Convert Interval to Binance string
     */
    static std::string interval_to_string(Interval interval);

    /**
     * @brief Process incoming WebSocket message
     */
    void process_ws_message(const std::string& message);

    //=========================================================================
    // Member Variables
    //=========================================================================

    BinanceGatewayConfig config_;
    std::atomic<bool> connected_{false};
    std::string last_error_;

    // REST API implementation (PIMPL)
    struct RestImpl;
    std::unique_ptr<RestImpl> rest_impl_;

    // WebSocket implementation (PIMPL)
    struct WsImpl;
    std::unique_ptr<WsImpl> ws_impl_;

    // WebSocket thread
    std::thread ws_thread_;
    std::atomic<bool> ws_running_{false};

    // Subscriptions
    std::unordered_map<std::string, Interval> bar_subscriptions_;
    std::unordered_map<std::string, bool> tick_subscriptions_;

    // Contract cache
    std::unordered_map<std::string, ContractData> contracts_;

    // Tick cache
    std::unordered_map<std::string, TickData> last_ticks_;
};

//=============================================================================
// Registration
//=============================================================================

// Auto-register Binance gateway
extern bool binance_gateway_registered;

} // namespace bitquant