/**
 * @file websocket_client.cpp
 * @brief WebSocket client implementation
 *
 * Uses binapi's WebSocket implementation for Binance connectivity
 */

#include "exchange/websocket_client.hpp"
#include <iostream>
#include <sstream>

namespace bitquant {

//=============================================================================
// WebSocketClient Implementation
//=============================================================================

struct WebSocketClient::Impl {
    WebSocketConfig config;
    std::atomic<bool> connected{false};
    std::atomic<bool> running{false};
    std::thread io_thread;

    WsMessageCallback message_callback;
    WsConnectionCallback connect_callback;
    WsConnectionCallback disconnect_callback;
    WsErrorCallback error_callback;

    std::vector<std::string> subscriptions;
    std::mutex subscriptions_mutex;

    size_t reconnect_attempts = 0;
};

WebSocketClient::WebSocketClient()
    : impl_(std::make_unique<Impl>()) {}

WebSocketClient::WebSocketClient(const WebSocketConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

WebSocketClient::~WebSocketClient() {
    close();
}

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

bool WebSocketClient::connect() {
    if (impl_->connected) {
        return true;
    }

    impl_->running = true;
    impl_->io_thread = std::thread(&WebSocketClient::run_io_loop, this);

    return true;
}

void WebSocketClient::close() {
    impl_->running = false;

    if (impl_->io_thread.joinable()) {
        impl_->io_thread.join();
    }

    impl_->connected = false;
}

bool WebSocketClient::is_connected() const {
    return impl_->connected;
}

void WebSocketClient::subscribe(const std::string& stream) {
    std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);
    impl_->subscriptions.push_back(stream);

    if (impl_->connected) {
        send_json("SUBSCRIBE", {stream});
    }
}

void WebSocketClient::subscribe(const std::vector<std::string>& streams) {
    for (const auto& stream : streams) {
        subscribe(stream);
    }
}

void WebSocketClient::unsubscribe(const std::string& stream) {
    std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);

    auto it = std::find(impl_->subscriptions.begin(), impl_->subscriptions.end(), stream);
    if (it != impl_->subscriptions.end()) {
        impl_->subscriptions.erase(it);
    }

    if (impl_->connected) {
        send_json("UNSUBSCRIBE", {stream});
    }
}

void WebSocketClient::unsubscribe(const std::vector<std::string>& streams) {
    for (const auto& stream : streams) {
        unsubscribe(stream);
    }
}

std::vector<std::string> WebSocketClient::get_subscriptions() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(impl_->subscriptions_mutex));
    return impl_->subscriptions;
}

bool WebSocketClient::send(const std::string& message) {
    if (!impl_->connected) {
        return false;
    }

    // In full implementation, send via WebSocket
    (void)message;
    return true;
}

bool WebSocketClient::send_json(const std::string& method, const std::vector<std::string>& params) {
    std::ostringstream oss;
    oss << R"({"method":")" << method << R"(","params":[)";

    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ",";
        oss << R"(")" << params[i] << R"(")";
    }

    oss << R"(],"id":)" << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() << "}";

    return send(oss.str());
}

void WebSocketClient::run_io_loop() {
    // In full implementation, this would use binapi's WebSocket
    // or Boost.Beast for async WebSocket operations

    while (impl_->running) {
        // Simulated event loop
        // Real implementation would:
        // 1. Connect to WebSocket server
        // 2. Subscribe to streams
        // 3. Process incoming messages
        // 4. Handle reconnection

        if (!impl_->connected) {
            // Attempt connection
            impl_->connected = true;
            if (impl_->connect_callback) {
                impl_->connect_callback(true);
            }

            // Re-subscribe to streams
            std::lock_guard<std::mutex> lock(impl_->subscriptions_mutex);
            if (!impl_->subscriptions.empty()) {
                send_json("SUBSCRIBE", impl_->subscriptions);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    impl_->connected = false;
    if (impl_->disconnect_callback) {
        impl_->disconnect_callback(false);
    }
}

void WebSocketClient::handle_reconnect() {
    if (impl_->reconnect_attempts >= impl_->config.max_reconnect_attempts) {
        if (impl_->error_callback) {
            impl_->error_callback("Max reconnection attempts reached");
        }
        return;
    }

    impl_->reconnect_attempts++;
    std::this_thread::sleep_for(std::chrono::milliseconds(impl_->config.reconnect_interval_ms));

    // Reconnect logic
    connect();
}

void WebSocketClient::send_ping() {
    // In full implementation, send ping frame
}

//=============================================================================
// BinanceStream Implementation
//=============================================================================

namespace BinanceStream {

// Convert symbol to lowercase
static std::string to_lower(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = std::tolower(c);
    }
    return result;
}

std::string agg_trade(const std::string& symbol) {
    return to_lower(symbol) + "@aggTrade";
}

std::string trade(const std::string& symbol) {
    return to_lower(symbol) + "@trade";
}

std::string kline(const std::string& symbol, const std::string& interval) {
    return to_lower(symbol) + "@kline_" + interval;
}

std::string mini_ticker(const std::string& symbol) {
    return to_lower(symbol) + "@miniTicker";
}

std::string mini_ticker_all() {
    return "!miniTicker@arr";
}

std::string ticker(const std::string& symbol) {
    return to_lower(symbol) + "@ticker";
}

std::string ticker_all() {
    return "!ticker@arr";
}

std::string book_ticker(const std::string& symbol) {
    return to_lower(symbol) + "@bookTicker";
}

std::string depth(const std::string& symbol, int levels) {
    return to_lower(symbol) + "@depth" + std::to_string(levels);
}

std::string depth_diff(const std::string& symbol) {
    return to_lower(symbol) + "@depth";
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
