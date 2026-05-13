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
// Connection State
//=============================================================================

/**
 * @brief WebSocket connection state
 */
enum class ConnectionState {
    IDLE,              // Not connected
    CONNECTING,        // Attempting to connect
    CONNECTED,         // Successfully connected
    RECONNECT_PENDING, // Waiting to reconnect
    FAILED             // Connection failed (max attempts reached)
};

/**
 * @brief Convert ConnectionState to string
 */
inline const char* connection_state_to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::IDLE: return "IDLE";
        case ConnectionState::CONNECTING: return "CONNECTING";
        case ConnectionState::CONNECTED: return "CONNECTED";
        case ConnectionState::RECONNECT_PENDING: return "RECONNECT_PENDING";
        case ConnectionState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

/// State change callback
using WsStateCallback = std::function<void(ConnectionState state)>;

/// Reconnect attempt callback
using WsReconnectCallback = std::function<void(int attempt, int max_attempts)>;

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

    // Reconnection settings
    size_t reconnect_interval_ms = 1000;      // Initial retry interval
    size_t max_reconnect_interval_ms = 30000; // Max retry interval (30s)
    size_t max_reconnect_attempts = 10;       // 0 = unlimited
    double backoff_multiplier = 1.5;           // Exponential backoff factor

    // Heartbeat settings
    size_t ping_interval_ms = 30000;          // Ping every 30s
    size_t pong_timeout_ms = 10000;           // Wait 10s for pong
    size_t heartbeat_check_interval_ms = 5000; // Check every 5s
    size_t message_timeout_ms = 60000;        // Message timeout (60s)

    // Connection timeout
    size_t connect_timeout_ms = 10000;        // 10s connection timeout

    // Statistics interval (for binapi)
    size_t stat_interval_sec = 10;            // Stats every 10 seconds
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

    /**
     * @brief Get current connection state
     */
    ConnectionState state() const;

    /**
     * @brief Register state change callback
     */
    void on_state_change(WsStateCallback callback);

    /**
     * @brief Register reconnect attempt callback
     */
    void on_reconnect_attempt(WsReconnectCallback callback);

    /**
     * @brief Trigger immediate reconnection
     */
    void reconnect_now();

    /**
     * @brief Reset reconnect attempt counter
     */
    void reset_reconnect_attempts();

    /**
     * @brief Get number of reconnect attempts
     */
    int get_reconnect_attempts() const;

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

    // Main I/O loop
    void run_io_loop();

    // Connection management
    void set_state(ConnectionState new_state);
    void attempt_connection();
    void on_connection_established();
    void on_connection_lost();

    // Reconnection logic
    void start_reconnect();
    void stop_reconnect();
    void reconnect_loop();
    void trigger_reconnect();
    size_t calculate_backoff_time() const;
    void resubscribe_all();
    void handle_reconnect();

    // Monitor thread
    void start_monitor();
    void stop_monitor();
    void monitor_loop();

    // Heartbeat logic
    void start_heartbeat();
    void stop_heartbeat();
    void heartbeat_loop();
    void check_connection_health();
    void send_ping();
    void on_pong_received();
    void update_last_message_time();

    // WebSocket message handling (binapi callbacks)
    void on_websocket_message(const char* channel, const char* data, std::size_t size);
    void do_subscribe(const std::string& stream);
    void do_unsubscribe(const std::string& stream);
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
