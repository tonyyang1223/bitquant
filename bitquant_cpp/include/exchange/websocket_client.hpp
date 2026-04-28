/**
 * @file websocket_client.hpp
 * @brief WebSocket client for real-time data streaming
 *
 * Based on howtrader's WebSocket design:
 * - Async connection management
 * - Subscribe/unsubscribe streams
 * - Automatic reconnection
 * - Callback-based data handling
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace bitquant {

//=============================================================================
// WebSocket Callback Types
//=============================================================================

/// Message callback
using WsMessageCallback = std::function<void(const std::string&)>;

/// Connection callback
using WsConnectionCallback = std::function<void(bool connected)>;

/// Error callback
using WsErrorCallback = std::function<void(const std::string& error)>;

//=============================================================================
// WebSocket Configuration
//=============================================================================

/**
 * @brief WebSocket configuration
 */
struct WebSocketConfig {
    std::string host;
    std::string port = "443";
    bool use_ssl = true;
    size_t reconnect_interval_ms = 5000;
    size_t ping_interval_ms = 30000;
    size_t max_reconnect_attempts = 5;
};

//=============================================================================
// WebSocket Client
//=============================================================================

/**
 * @brief WebSocket client for real-time data
 *
 * Provides async WebSocket connection with:
 * - Automatic reconnection
 * - Subscription management
 * - Thread-safe message handling
 *
 * Usage:
 * @code
 * WebSocketClient ws(config);
 * ws.on_message([](const std::string& msg) { ... });
 * ws.connect();
 * ws.subscribe("btcusdt@aggTrade");
 * // ...
 * ws.close();
 * @endcode
 */
class WebSocketClient {
public:
    WebSocketClient();
    explicit WebSocketClient(const WebSocketConfig& config);
    ~WebSocketClient();

    //=========================================================================
    // Callback Registration
    //=========================================================================

    void on_message(WsMessageCallback callback);
    void on_connect(WsConnectionCallback callback);
    void on_disconnect(WsConnectionCallback callback);
    void on_error(WsErrorCallback callback);

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to WebSocket server
     * @return true if connection initiated
     */
    bool connect();

    /**
     * @brief Close connection
     */
    void close();

    /**
     * @brief Check if connected
     */
    bool is_connected() const;

    //=========================================================================
    // Subscription Management
    //=========================================================================

    /**
     * @brief Subscribe to stream
     * @param stream Stream name (e.g., "btcusdt@aggTrade")
     */
    void subscribe(const std::string& stream);

    /**
     * @brief Subscribe to multiple streams
     */
    void subscribe(const std::vector<std::string>& streams);

    /**
     * @brief Unsubscribe from stream
     */
    void unsubscribe(const std::string& stream);

    /**
     * @brief Unsubscribe from multiple streams
     */
    void unsubscribe(const std::vector<std::string>& streams);

    /**
     * @brief Get active subscriptions
     */
    std::vector<std::string> get_subscriptions() const;

    //=========================================================================
    // Message Sending
    //=========================================================================

    /**
     * @brief Send raw message
     */
    bool send(const std::string& message);

    /**
     * @brief Send JSON message
     */
    bool send_json(const std::string& method, const std::vector<std::string>& params);

private:
    //=========================================================================
    // Internal Implementation
    //=========================================================================

    struct Impl;
    std::unique_ptr<Impl> impl_;

    void run_io_loop();
    void handle_reconnect();
    void send_ping();
};

//=============================================================================
// Binance WebSocket Helper
//=============================================================================

/**
 * @brief Binance WebSocket stream names
 */
namespace BinanceStream {

/// Aggregate trade stream
std::string agg_trade(const std::string& symbol);

/// Trade stream
std::string trade(const std::string& symbol);

/// Kline/Candlestick stream
std::string kline(const std::string& symbol, const std::string& interval);

/// Mini ticker stream
std::string mini_ticker(const std::string& symbol);

/// All mini tickers stream
std::string mini_ticker_all();

/// Ticker stream
std::string ticker(const std::string& symbol);

/// All tickers stream
std::string ticker_all();

/// Book ticker stream
std::string book_ticker(const std::string& symbol);

/// Partial book depth stream
std::string depth(const std::string& symbol, int levels = 5);

/// Diff depth stream
std::string depth_diff(const std::string& symbol);

/// Combine multiple streams
std::string combine(const std::vector<std::string>& streams);

} // namespace BinanceStream

} // namespace bitquant
