/**
 * @file binance_usdt_ws_api.hpp
 * @brief Binance USDT-M Futures WebSocket API
 *
 * Implements WebSocket API for Binance USDT-M perpetual futures.
 * Uses fstream.binance.com endpoints.
 *
 * Features:
 * - Market data streams (ticker, depth, kline, trade)
 * - User data stream (account, order updates)
 * - Automatic reconnection
 * - Multiple symbol subscription
 *
 * Reference: howtrader/gateway/binance/binance_usdt_gateway.py (WebSocket classes)
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

namespace bitquant {

// Forward declarations
class BinanceUsdtGateway;

//=============================================================================
// WebSocket Configuration
//=============================================================================

struct BinanceUsdtWsConfig {
    std::string host = "fstream.binance.com";
    std::string port = "443";
    int reconnect_interval_ms = 1000;
    int max_reconnect_attempts = 10;
    int ping_interval_ms = 30000;
    int pong_timeout_ms = 10000;
};

//=============================================================================
// BinanceUsdtWsApi - Market Data WebSocket
//=============================================================================

/**
 * @brief Binance USDT-M Futures Market Data WebSocket API
 *
 * Handles public market data streams:
 * - Ticker (24hr statistics)
 * - Depth (order book)
 * - Kline (candlestick)
 * - Trade (individual trades)
 * - AggTrade (aggregated trades)
 */
class BinanceUsdtWsApi {
public:
    // Callback types
    using TickCallback = std::function<void(const TickData&)>;
    using BarCallback = std::function<void(const BarData&)>;
    using DepthCallback = std::function<void(const std::string&, const std::vector<std::pair<double, double>>& bids, const std::vector<std::pair<double, double>>& asks)>;
    using TradeCallback = std::function<void(const std::string&, double, double, bool)>;
    using LogCallback = std::function<void(const std::string&)>;
    using ConnectedCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param gateway Parent gateway
     */
    explicit BinanceUsdtWsApi(BinanceUsdtGateway* gateway = nullptr);

    /**
     * @brief Destructor
     */
    ~BinanceUsdtWsApi();

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to market data stream
     * @param config WebSocket configuration
     */
    void connect(const BinanceUsdtWsConfig& config = BinanceUsdtWsConfig{});

    /**
     * @brief Stop WebSocket connection
     */
    void stop();

    /**
     * @brief Check if connected
     */
    bool is_connected() const { return connected_.load(); }

    //=========================================================================
    // Subscription
    //=========================================================================

    /**
     * @brief Subscribe to ticker updates
     * @param symbol Symbol (e.g., "BTCUSDT")
     */
    void subscribe_ticker(const std::string& symbol);

    /**
     * @brief Subscribe to depth updates
     * @param symbol Symbol
     * @param levels Depth levels (5, 10, or 20)
     */
    void subscribe_depth(const std::string& symbol, int levels = 5);

    /**
     * @brief Subscribe to kline updates
     * @param symbol Symbol
     * @param interval Kline interval (e.g., "1m", "5m", "1h")
     */
    void subscribe_kline(const std::string& symbol, const std::string& interval);

    /**
     * @brief Subscribe to trade updates
     * @param symbol Symbol
     */
    void subscribe_trade(const std::string& symbol);

    /**
     * @brief Subscribe to aggregated trade updates
     * @param symbol Symbol
     */
    void subscribe_agg_trade(const std::string& symbol);

    /**
     * @brief Unsubscribe from symbol
     * @param symbol Symbol
     */
    void unsubscribe(const std::string& symbol);

    //=========================================================================
    // Callbacks
    //=========================================================================

    void set_tick_callback(TickCallback cb) { tick_callback_ = std::move(cb); }
    void set_bar_callback(BarCallback cb) { bar_callback_ = std::move(cb); }
    void set_depth_callback(DepthCallback cb) { depth_callback_ = std::move(cb); }
    void set_trade_callback(TradeCallback cb) { trade_callback_ = std::move(cb); }
    void set_log_callback(LogCallback cb) { log_callback_ = std::move(cb); }
    void set_connected_callback(ConnectedCallback cb) { connected_callback_ = std::move(cb); }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void on_connected();
    void on_disconnected();
    void on_message(const std::string& message);
    void process_stream_message(const std::string& stream, const std::string& data);
    void process_ticker(const std::string& symbol, const std::string& data);
    void process_depth(const std::string& symbol, const std::string& data);
    void process_kline(const std::string& symbol, const std::string& data);
    void process_trade(const std::string& symbol, const std::string& data);
    void process_agg_trade(const std::string& symbol, const std::string& data);
    void resubscribe_all();
    void write_log(const std::string& msg);
    std::string to_lower_symbol(const std::string& symbol);

    //=========================================================================
    // Members
    //=========================================================================

    BinanceUsdtGateway* gateway_;
    std::string gateway_name_ = "BINANCE_USDT";

    BinanceUsdtWsConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};

    // Subscriptions
    std::map<std::string, std::vector<std::string>> subscriptions_;  // symbol -> streams
    std::mutex subscriptions_mutex_;

    // Tick cache
    std::unordered_map<std::string, TickData> last_ticks_;
    std::mutex ticks_mutex_;

    // Callbacks
    TickCallback tick_callback_;
    BarCallback bar_callback_;
    DepthCallback depth_callback_;
    TradeCallback trade_callback_;
    LogCallback log_callback_;
    ConnectedCallback connected_callback_;
};

//=============================================================================
// BinanceUsdtUserWsApi - User Data WebSocket
//=============================================================================

/**
 * @brief Binance USDT-M Futures User Data WebSocket API
 *
 * Handles private user data streams:
 * - Account updates (balance changes)
 * - Order updates (new, filled, cancelled)
 * - Position updates
 */
class BinanceUsdtUserWsApi {
public:
    // Callback types
    using AccountCallback = std::function<void(const AccountData&)>;
    using PositionCallback = std::function<void(const PositionData&)>;
    using OrderCallback = std::function<void(const OrderData&)>;
    using LogCallback = std::function<void(const std::string&)>;

    /**
     * @brief Constructor
     * @param gateway Parent gateway
     */
    explicit BinanceUsdtUserWsApi(BinanceUsdtGateway* gateway = nullptr);

    /**
     * @brief Destructor
     */
    ~BinanceUsdtUserWsApi();

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to user data stream
     * @param listen_key Listen key from REST API
     * @param config WebSocket configuration
     */
    void connect(const std::string& listen_key, const BinanceUsdtWsConfig& config = BinanceUsdtWsConfig{});

    /**
     * @brief Stop WebSocket connection
     */
    void stop();

    /**
     * @brief Check if connected
     */
    bool is_connected() const { return connected_.load(); }

    //=========================================================================
    // Callbacks
    //=========================================================================

    void set_account_callback(AccountCallback cb) { account_callback_ = std::move(cb); }
    void set_position_callback(PositionCallback cb) { position_callback_ = std::move(cb); }
    void set_order_callback(OrderCallback cb) { order_callback_ = std::move(cb); }
    void set_log_callback(LogCallback cb) { log_callback_ = std::move(cb); }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void on_connected();
    void on_disconnected();
    void on_message(const std::string& message);
    void process_account_update(const std::string& data);
    void process_order_update(const std::string& data);
    void write_log(const std::string& msg);

    //=========================================================================
    // Members
    //=========================================================================

    BinanceUsdtGateway* gateway_;
    std::string gateway_name_ = "BINANCE_USDT";
    std::string listen_key_;

    BinanceUsdtWsConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};

    // Callbacks
    AccountCallback account_callback_;
    PositionCallback position_callback_;
    OrderCallback order_callback_;
    LogCallback log_callback_;
};

} // namespace bitquant