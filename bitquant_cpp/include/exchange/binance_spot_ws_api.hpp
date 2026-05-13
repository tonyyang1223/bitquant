/**
 * @file binance_spot_ws_api.hpp
 * @brief Binance Spot WebSocket API client
 *
 * Implements real-time data streaming via WebSocket:
 * - Market data: ticker, depth, kline
 * - User data: order updates, account changes
 *
 * Based on howtrader's BinanceSpotWebsocketApi design
 */

#pragma once

#include "exchange/websocket_client.hpp"
#include "core/types.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace flatjson {
    class fjson;  // Forward declaration
}

namespace bitquant {

// Forward declaration
class BinanceSpotGateway;

//=============================================================================
// WebSocket API Callbacks
//=============================================================================

/// Tick data callback
using WsTickCallback = std::function<void(const TickData&)>;

/// Bar data callback
using WsBarCallback = std::function<void(const BarData&)>;

/// Order update callback (from user data stream)
using WsOrderCallback = std::function<void(const flatjson::fjson&)>;

//=============================================================================
// Binance Spot WebSocket API
//=============================================================================

/**
 * @brief Binance Spot WebSocket client
 *
 * Handles two types of connections:
 * 1. Market data stream: Public data (ticker, depth, kline)
 * 2. User data stream: Private data (order updates, balance changes)
 *
 * Usage:
 * @code
 * BinanceSpotWsApi ws(gateway);
 * ws.on_tick([](const TickData& tick) { ... });
 * ws.connect_market_stream();
 * ws.subscribe_ticker("BTCUSDT");
 * // For user data:
 * ws.connect_user_stream(listen_key);
 * @endcode
 */
class BinanceSpotWsApi {
public:
    explicit BinanceSpotWsApi(BinanceSpotGateway* gateway);
    ~BinanceSpotWsApi();

    // Non-copyable
    BinanceSpotWsApi(const BinanceSpotWsApi&) = delete;
    BinanceSpotWsApi& operator=(const BinanceSpotWsApi&) = delete;

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to market data stream
     * @param proxy_host Proxy host (optional)
     * @param proxy_port Proxy port (optional)
     * @return true on success
     */
    bool connect_market_stream(const std::string& proxy_host = "", int proxy_port = 0);

    /**
     * @brief Connect to user data stream
     * @param listen_key Listen key from REST API
     * @return true on success
     */
    bool connect_user_stream(const std::string& listen_key);

    /**
     * @brief Close all connections
     */
    void close();

    /**
     * @brief Check if connected
     */
    bool is_connected() const;

    //=========================================================================
    // Market Data Subscriptions
    //=========================================================================

    /**
     * @brief Subscribe to ticker updates
     * @param symbol Trading pair (e.g., "BTCUSDT")
     */
    void subscribe_ticker(const std::string& symbol);

    /**
     * @brief Subscribe to depth updates
     * @param symbol Trading pair
     */
    void subscribe_depth(const std::string& symbol);

    /**
     * @brief Subscribe to kline updates
     * @param symbol Trading pair
     * @param interval Kline interval (e.g., "1m", "5m", "1h")
     */
    void subscribe_kline(const std::string& symbol, const std::string& interval);

    /**
     * @brief Unsubscribe from ticker
     */
    void unsubscribe_ticker(const std::string& symbol);

    //=========================================================================
    // Callback Registration
    //=========================================================================

    void on_tick(WsTickCallback callback);
    void on_bar(WsBarCallback callback);
    void on_order(WsOrderCallback callback);

    //=========================================================================
    // Data Access
    //=========================================================================

    /**
     * @brief Get last tick for symbol
     */
    std::optional<TickData> get_tick(const std::string& symbol) const;

private:
    //=========================================================================
    // Message Processing
    //=========================================================================

    void on_message(const std::string& msg);
    void process_message(const flatjson::fjson& data);

    // Market data handlers
    void process_ticker(const flatjson::fjson& data);
    void process_depth(const flatjson::fjson& data);
    void process_kline(const flatjson::fjson& data);

    // User data handlers
    void process_order_update(const flatjson::fjson& data);
    void process_account_update(const flatjson::fjson& data);

    //=========================================================================
    // Members
    //=========================================================================

    BinanceSpotGateway* gateway_;

    std::unique_ptr<WebSocketClient> ws_client_;
    std::unique_ptr<WebSocketClient> user_ws_client_;

    // Tick cache
    std::unordered_map<std::string, TickData> ticks_;
    mutable std::mutex ticks_mutex_;

    // Callbacks
    WsTickCallback tick_callback_;
    WsBarCallback bar_callback_;
    WsOrderCallback order_callback_;

    // Request ID counter
    int req_id_ = 0;

    // Symbol subscriptions
    std::vector<std::string> subscribed_symbols_;
};

} // namespace bitquant
