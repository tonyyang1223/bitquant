/**
 * @file websocket_client.cpp
 * @brief WebSocket client implementation using binapi WebSocket
 *
 * High-quality implementation with:
 * - Real WebSocket connection via binapi::ws::websockets
 * - Automatic reconnection with exponential backoff
 * - Heartbeat monitoring
 * - Subscription management with auto-restore
 * - Thread-safe operations
 */

#include "exchange/websocket_client.hpp"

// binapi WebSocket support
#include <binapi/websocket.hpp>
#include <binapi/types.hpp>
#include <binapi/flatjson.hpp>

// Boost ASIO
#include <boost/asio.hpp>

// Standard library
#include <iostream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <condition_variable>

namespace bitquant {

//=============================================================================
// WebSocketClient Implementation
//=============================================================================

struct WebSocketClient::Impl {
    WebSocketConfig config;

    // Boost ASIO and binapi WebSocket
    std::unique_ptr<boost::asio::io_context> io_context;
    std::unique_ptr<binapi::ws::websockets> websockets;

    // Thread management
    std::thread io_thread;
    std::thread monitor_thread;
    std::atomic<bool> running{false};
    std::atomic<bool> io_running{false};
    std::atomic<bool> monitor_running{false};

    // Connection state
    std::atomic<ConnectionState> state{ConnectionState::IDLE};
    std::atomic<int> reconnect_attempts{0};
    std::mutex state_mutex;
    std::condition_variable state_cv;

    // Callbacks
    WsMessageCallback message_callback;
    WsConnectionCallback connect_callback;
    WsConnectionCallback disconnect_callback;
    WsErrorCallback error_callback;
    WsStateCallback state_callback;
    WsReconnectCallback reconnect_callback;

    // Heartbeat tracking
    std::atomic<int64_t> last_message_time{0};
    std::atomic<int64_t> last_ping_time{0};
    std::atomic<bool> pong_received{false};
    std::mutex heartbeat_mutex;
    std::condition_variable heartbeat_cv;
    std::condition_variable monitor_cv;

    // Subscription entry
    struct SubscriptionEntry {
        std::string stream;
        binapi::ws::websockets::handle handle{nullptr};
    };

    // Active subscriptions
    std::vector<SubscriptionEntry> subscriptions;
    std::mutex subscriptions_mutex;
};

//=============================================================================
// Constructor/Destructor
//=============================================================================

WebSocketClient::WebSocketClient() : impl_(std::make_unique<Impl>()) {
}

WebSocketClient::WebSocketClient(const WebSocketConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

WebSocketClient::~WebSocketClient() {
    close();
}

//=============================================================================
// Callback Registration
//=============================================================================

void WebSocketClient::on_message(WsMessageCallback callback) {
    impl_->message_callback = std::move(callback);
}

void WebSocketClient::on_connect(WsConnectionCallback callback) {
    impl_->connect_callback = std::move(callback);
}

void WebSocketClient::on_disconnect(WsConnectionCallback callback) {
    impl_->disconnect_callback = std::move(callback);
}

void WebSocketClient::on_error(WsErrorCallback callback) {
    impl_->error_callback = std::move(callback);
}

void WebSocketClient::on_state_change(WsStateCallback callback) {
    impl_->state_callback = std::move(callback);
}

void WebSocketClient::on_reconnect_attempt(WsReconnectCallback callback) {
    impl_->reconnect_callback = std::move(callback);
}

//=============================================================================
// Connection Management
//=============================================================================

bool WebSocketClient::connect() {
    if (impl_->running) {
        return true;  // Already connected
    }

    std::cout << "[WebSocket] Connecting to " << impl_->config.host
              << ":" << impl_->config.port << std::endl;

    set_state(ConnectionState::CONNECTING);

    try {
        impl_->running = true;

        // Create io_context
        impl_->io_context = std::make_unique<boost::asio::io_context>();

        // Create websockets instance
        impl_->websockets = std::make_unique<binapi::ws::websockets>(
            *impl_->io_context,
            impl_->config.host,
            impl_->config.port,
            [this](const char* channel, const char* data, std::size_t size) {
                this->on_websocket_message(channel, data, size);
                return true;
            },
            nullptr,  // on_state_changed
            impl_->config.stat_interval_sec
        );

        // Start I/O thread
        impl_->io_thread = std::thread([this]() { run_io_loop(); });

        // Start monitor thread
        impl_->monitor_thread = std::thread([this]() { monitor_loop(); });

        set_state(ConnectionState::CONNECTED);
        on_connection_established();

        return true;

    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] Connection error: " << e.what() << std::endl;
        set_state(ConnectionState::FAILED);
        if (impl_->error_callback) {
            impl_->error_callback(e.what());
        }
        return false;
    }
}

void WebSocketClient::close() {
    if (!impl_->running) return;

    std::cout << "[WebSocket] Closing connection..." << std::endl;

    impl_->running = false;

    // Stop I/O context
    if (impl_->io_context) {
        impl_->io_context->stop();
    }

    // Notify monitor
    impl_->monitor_cv.notify_all();

    // Wait for threads
    if (impl_->io_thread.joinable()) {
        impl_->io_thread.join();
    }
    if (impl_->monitor_thread.joinable()) {
        impl_->monitor_thread.join();
    }

    // Clear subscriptions
    {
        std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);
        impl_->subscriptions.clear();
    }

    // Cleanup
    impl_->websockets.reset();
    impl_->io_context.reset();

    set_state(ConnectionState::IDLE);
    on_connection_lost();
}

bool WebSocketClient::is_connected() const {
    return impl_->running && impl_->state == ConnectionState::CONNECTED;
}

ConnectionState WebSocketClient::state() const {
    return impl_->state;
}

void WebSocketClient::reconnect_now() {
    impl_->reconnect_attempts = 0;
    impl_->monitor_cv.notify_all();
}

void WebSocketClient::reset_reconnect_attempts() {
    impl_->reconnect_attempts = 0;
}

int WebSocketClient::get_reconnect_attempts() const {
    return impl_->reconnect_attempts;
}

//=============================================================================
// I/O Loop
//=============================================================================

void WebSocketClient::run_io_loop() {
    impl_->io_running = true;

    try {
        boost::asio::io_context::work work(*impl_->io_context);
        impl_->io_context->run();
    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] I/O loop error: " << e.what() << std::endl;
        if (impl_->error_callback) {
            impl_->error_callback(std::string("I/O error: ") + e.what());
        }
    }

    impl_->io_running = false;

    // Connection lost, trigger reconnect if still running
    if (impl_->running) {
        set_state(ConnectionState::RECONNECT_PENDING);
    }
}

//=============================================================================
// Monitor Loop
//=============================================================================

void WebSocketClient::monitor_loop() {
    impl_->monitor_running = true;

    while (impl_->running) {
        // Wait for check interval or notification
        std::unique_lock<std::mutex> lock(impl_->state_mutex);
        impl_->monitor_cv.wait_for(lock,
            std::chrono::milliseconds(impl_->config.heartbeat_check_interval_ms));

        if (!impl_->running) break;

        check_connection_health();

        // Handle reconnection if needed
        if (impl_->state == ConnectionState::RECONNECT_PENDING) {
            handle_reconnect();
        }
    }

    impl_->monitor_running = false;
}

void WebSocketClient::check_connection_health() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    auto time_since_last_msg = now - impl_->last_message_time.load();

    // Check for message timeout
    if (impl_->last_message_time.load() > 0 &&
        time_since_last_msg > static_cast<int64_t>(impl_->config.message_timeout_ms)) {
        std::cout << "[WebSocket] Message timeout detected (" << time_since_last_msg
                  << "ms since last message)" << std::endl;
        set_state(ConnectionState::RECONNECT_PENDING);
    }
}

void WebSocketClient::handle_reconnect() {
    impl_->reconnect_attempts++;

    // Check max attempts
    if (impl_->config.max_reconnect_attempts > 0 &&
        impl_->reconnect_attempts > static_cast<int>(impl_->config.max_reconnect_attempts)) {
        std::cout << "[WebSocket] Max reconnection attempts reached" << std::endl;
        set_state(ConnectionState::FAILED);
        if (impl_->error_callback) {
            impl_->error_callback("Max reconnection attempts reached");
        }
        return;
    }

    // Notify reconnect attempt
    if (impl_->reconnect_callback) {
        impl_->reconnect_callback(impl_->reconnect_attempts,
                                   impl_->config.max_reconnect_attempts);
    }

    std::cout << "[WebSocket] Reconnect attempt " << impl_->reconnect_attempts
              << "/" << impl_->config.max_reconnect_attempts << std::endl;

    // Calculate backoff time
    size_t backoff = calculate_backoff_time();
    std::cout << "[WebSocket] Waiting " << backoff << "ms before reconnect" << std::endl;

    // Wait with backoff
    std::unique_lock<std::mutex> lock(impl_->state_mutex);
    impl_->state_cv.wait_for(lock,
        std::chrono::milliseconds(backoff),
        [this] { return !impl_->running; });

    if (!impl_->running) return;

    // Attempt reconnect
    set_state(ConnectionState::CONNECTING);

    try {
        // Recreate I/O context and websockets
        impl_->io_context = std::make_unique<boost::asio::io_context>();
        impl_->websockets = std::make_unique<binapi::ws::websockets>(
            *impl_->io_context,
            impl_->config.host,
            impl_->config.port,
            [this](const char* channel, const char* data, std::size_t size) {
                this->on_websocket_message(channel, data, size);
                return true;
            },
            nullptr,
            impl_->config.stat_interval_sec
        );

        // Restart I/O thread
        impl_->io_thread = std::thread([this]() { run_io_loop(); });

        set_state(ConnectionState::CONNECTED);
        resubscribe_all();

        impl_->reconnect_attempts = 0;  // Reset on success

    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] Reconnect failed: " << e.what() << std::endl;
        set_state(ConnectionState::RECONNECT_PENDING);
    }
}

size_t WebSocketClient::calculate_backoff_time() const {
    double base = static_cast<double>(impl_->config.reconnect_interval_ms);
    double multiplier = std::pow(impl_->config.backoff_multiplier, impl_->reconnect_attempts);
    size_t wait_time = static_cast<size_t>(base * multiplier);
    return std::min(wait_time, impl_->config.max_reconnect_interval_ms);
}

//=============================================================================
// State Management
//=============================================================================

void WebSocketClient::set_state(ConnectionState new_state) {
    ConnectionState old_state = impl_->state.exchange(new_state);
    if (old_state != new_state) {
        std::cout << "[WebSocket] State changed: "
                  << connection_state_to_string(old_state) << " -> "
                  << connection_state_to_string(new_state) << std::endl;

        if (impl_->state_callback) {
            impl_->state_callback(new_state);
        }
    }
}

void WebSocketClient::on_connection_established() {
    std::cout << "[WebSocket] Connection established" << std::endl;
    impl_->last_message_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    if (impl_->connect_callback) {
        impl_->connect_callback(true);
    }
}

void WebSocketClient::on_connection_lost() {
    std::cout << "[WebSocket] Connection lost" << std::endl;
    if (impl_->disconnect_callback) {
        impl_->disconnect_callback(false);
    }
}

//=============================================================================
// Message Handling
//=============================================================================

void WebSocketClient::on_websocket_message(const char* channel, const char* data, std::size_t size) {
    // Update last message time
    impl_->last_message_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // Forward to message callback
    if (impl_->message_callback) {
        std::string msg(data, size);
        impl_->message_callback(msg);
    }
}

//=============================================================================
// Subscription Management
//=============================================================================

void WebSocketClient::subscribe(const std::string& stream) {
    std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);

    // Check if already subscribed
    for (const auto& sub : impl_->subscriptions) {
        if (sub.stream == stream) return;
    }

    // Add to subscription list
    impl_->subscriptions.push_back({stream, nullptr});

    // Subscribe if connected
    if (is_connected()) {
        do_subscribe(stream);
    }
}

void WebSocketClient::subscribe(const std::vector<std::string>& streams) {
    for (const auto& stream : streams) {
        subscribe(stream);
    }
}

void WebSocketClient::unsubscribe(const std::string& stream) {
    std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);

    for (auto it = impl_->subscriptions.begin(); it != impl_->subscriptions.end(); ++it) {
        if (it->stream == stream) {
            if (it->handle && impl_->websockets) {
                impl_->websockets->async_unsubscribe(it->handle);
            }
            impl_->subscriptions.erase(it);
            break;
        }
    }
}

void WebSocketClient::unsubscribe(const std::vector<std::string>& streams) {
    for (const auto& stream : streams) {
        unsubscribe(stream);
    }
}

std::vector<std::string> WebSocketClient::get_subscriptions() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(impl_->subscriptions_mutex));

    std::vector<std::string> result;
    result.reserve(impl_->subscriptions.size());
    for (const auto& sub : impl_->subscriptions) {
        result.push_back(sub.stream);
    }
    return result;
}

void WebSocketClient::do_subscribe(const std::string& stream) {
    if (!impl_->websockets) return;

    // Parse stream name to determine type
    // Format: symbol@channel (e.g., "btcusdt@ticker", "btcusdt@kline_1m")
    size_t at_pos = stream.find('@');
    if (at_pos == std::string::npos) return;

    std::string symbol = stream.substr(0, at_pos);
    std::string channel = stream.substr(at_pos + 1);

    binapi::ws::websockets::handle handle = nullptr;
    auto& ws = *impl_->websockets;
    auto cb = [this](const std::string& msg) {
        if (impl_->message_callback) {
            impl_->message_callback(msg);
        }
    };

    // Subscribe based on channel type
    if (channel == "ticker") {
        handle = ws.market(symbol.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::market_ticker_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                // Convert to JSON string
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("e":"24hrTicker",)"
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("c":")" << msg.c << R"(",)"
                    << R"("o":")" << msg.o << R"(",)"
                    << R"("h":")" << msg.h << R"(",)"
                    << R"("l":")" << msg.l << R"(",)"
                    << R"("v":")" << msg.v << R"(",)"
                    << R"("q":")" << msg.q << R"(",)"
                    << R"("E":)" << msg.E
                    << "}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    } else if (channel == "bookTicker") {
        handle = ws.book(symbol.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::book_ticker_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("u":)" << msg.u << ","
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("b":")" << msg.b << R"(",)"
                    << R"("B":")" << msg.B << R"(",)"
                    << R"("a":")" << msg.a << R"(",)"
                    << R"("A":")" << msg.A << R"(",)"
                    << "}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    } else if (channel == "aggTrade") {
        handle = ws.agg_trade(symbol.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::agg_trade_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("e":"aggTrade",)"
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("p":")" << msg.p << R"(",)"
                    << R"("q":")" << msg.q << R"(",)"
                    << R"("T":)" << msg.T << ","
                    << R"("m":)" << (msg.m ? "true" : "false")
                    << "}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    } else if (channel == "trade") {
        handle = ws.trade(symbol.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::trade_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("e":"trade",)"
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("p":")" << msg.p << R"(",)"
                    << R"("q":")" << msg.q << R"(",)"
                    << R"("T":)" << msg.T
                    << "}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    } else if (channel.substr(0, 5) == "kline") {
        std::string interval = (channel.length() > 6) ? channel.substr(6) : "1m";
        handle = ws.klines(symbol.c_str(), interval.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::kline_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("e":"kline",)"
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("k":{)"
                    << R"("t":)" << msg.t << ","
                    << R"("T":)" << msg.T << ","
                    << R"("o":")" << msg.o << R"(",)"
                    << R"("c":")" << msg.c << R"(",)"
                    << R"("h":")" << msg.h << R"(",)"
                    << R"("l":")" << msg.l << R"(",)"
                    << R"("v":")" << msg.v << R"(",)"
                    << R"("q":")" << msg.q << R"(",)"
                    << R"("x":)" << (msg.x ? "true" : "false")
                    << "}}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    } else if (channel == "miniTicker") {
        handle = ws.mini_ticker(symbol.c_str(),
            [this, stream](const char* fl, int ec, std::string errmsg, binapi::ws::mini_ticker_t msg) {
                if (ec) {
                    if (impl_->error_callback) {
                        impl_->error_callback(stream + ": " + errmsg);
                    }
                    return false;
                }
                std::ostringstream oss;
                oss << R"({"stream":")" << stream << R"(",)"
                    << R"("e":"24hrMiniTicker",)"
                    << R"("s":")" << msg.s << R"(",)"
                    << R"("c":")" << msg.c << R"(",)"
                    << R"("o":")" << msg.o << R"(",)"
                    << R"("h":")" << msg.h << R"(",)"
                    << R"("l":")" << msg.l << R"(",)"
                    << R"("v":")" << msg.v << R"(",)"
                    << R"("q":")" << msg.q << R"(",)"
                    << R"("E":)" << msg.E
                    << "}";
                if (impl_->message_callback) {
                    impl_->message_callback(oss.str());
                }
                return true;
            });
    }

    // Store handle
    for (auto& sub : impl_->subscriptions) {
        if (sub.stream == stream) {
            sub.handle = handle;
            break;
        }
    }
}

void WebSocketClient::resubscribe_all() {
    std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);

    if (!impl_->subscriptions.empty()) {
        std::cout << "[WebSocket] Re-subscribing to " << impl_->subscriptions.size()
                  << " streams" << std::endl;

        // Clear old handles
        for (auto& sub : impl_->subscriptions) {
            sub.handle = nullptr;
        }

        // Re-subscribe each stream
        for (const auto& sub : impl_->subscriptions) {
            do_subscribe(sub.stream);
        }
    }
}

//=============================================================================
// Message Sending
//=============================================================================

bool WebSocketClient::send(const std::string& message) {
    // binapi websockets doesn't support direct send
    // Use subscribe/unsubscribe for stream management
    return false;
}

bool WebSocketClient::send_json(const std::string& method, const std::vector<std::string>& params) {
    // binapi websockets doesn't support direct JSON send
    // Use subscribe/unsubscribe for stream management
    return false;
}

//=============================================================================
// Binance WebSocket Helper
//=============================================================================

namespace BinanceStream {

std::string agg_trade(const std::string& symbol) {
    return symbol + "@aggTrade";
}

std::string trade(const std::string& symbol) {
    return symbol + "@trade";
}

std::string kline(const std::string& symbol, const std::string& interval) {
    return symbol + "@kline_" + interval;
}

std::string mini_ticker(const std::string& symbol) {
    return symbol + "@miniTicker";
}

std::string mini_ticker_all() {
    return "!miniTicker@arr";
}

std::string ticker(const std::string& symbol) {
    return symbol + "@ticker";
}

std::string ticker_all() {
    return "!ticker@arr";
}

std::string book_ticker(const std::string& symbol) {
    return symbol + "@bookTicker";
}

std::string depth(const std::string& symbol, int levels) {
    std::string level_str = std::to_string(levels);
    return symbol + "@depth" + level_str;
}

std::string depth_diff(const std::string& symbol) {
    return symbol + "@depth";
}

std::string combine(const std::vector<std::string>& streams) {
    if (streams.empty()) return "";
    std::string result = streams[0];
    for (size_t i = 1; i < streams.size(); ++i) {
        result += "/" + streams[i];
    }
    return result;
}

} // namespace BinanceStream

} // namespace bitquant
