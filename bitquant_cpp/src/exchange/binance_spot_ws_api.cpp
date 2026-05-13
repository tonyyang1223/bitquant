/**
 * @file binance_spot_ws_api.cpp
 * @brief Binance Spot WebSocket API implementation
 *
 * Based on howtrader's BinanceSpotWebsocketApi implementation
 */

#include "exchange/binance_spot_ws_api.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "exchange/binance_mapping.hpp"
#include <binapi/flatjson.hpp>
#include <iostream>
#include <sstream>

namespace bitquant {

//=============================================================================
// Constructor/Destructor
//=============================================================================

BinanceSpotWsApi::BinanceSpotWsApi(BinanceSpotGateway* gateway)
    : gateway_(gateway)
{
}

BinanceSpotWsApi::~BinanceSpotWsApi() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool BinanceSpotWsApi::connect_market_stream(const std::string& proxy_host, int proxy_port) {
    WebSocketConfig config;
    config.host = "stream.binance.com";
    config.port = "443";
    config.use_ssl = true;

    // Reconnection settings
    config.reconnect_interval_ms = 1000;
    config.max_reconnect_interval_ms = 30000;
    config.max_reconnect_attempts = 10;
    config.backoff_multiplier = 1.5;

    // Heartbeat settings
    config.ping_interval_ms = 30000;
    config.pong_timeout_ms = 10000;

    // Note: Proxy support would need to be added to WebSocketClient
    (void)proxy_host;
    (void)proxy_port;

    ws_client_ = std::make_unique<WebSocketClient>(config);

    // Message callback
    ws_client_->on_message([this](const std::string& msg) {
        this->on_message(msg);
    });

    // Connection callback
    ws_client_->on_connect([this](bool connected) {
        if (connected) {
            std::cout << "[WebSocket] Market stream connected" << std::endl;
        }
    });

    // Disconnect callback
    ws_client_->on_disconnect([this](bool) {
        std::cout << "[WebSocket] Market stream disconnected, will reconnect..." << std::endl;
    });

    // State change callback
    ws_client_->on_state_change([this](ConnectionState state) {
        std::cout << "[WebSocket] Market stream state: "
                  << connection_state_to_string(state) << std::endl;

        // Notify gateway of state changes
        if (gateway_ && state == ConnectionState::FAILED) {
            // Connection failed permanently
        }
    });

    // Reconnect attempt callback
    ws_client_->on_reconnect_attempt([](int attempt, int max_attempts) {
        std::cout << "[WebSocket] Reconnect attempt " << attempt;
        if (max_attempts > 0) {
            std::cout << "/" << max_attempts;
        }
        std::cout << std::endl;
    });

    // Error callback
    ws_client_->on_error([](const std::string& error) {
        std::cerr << "[WebSocket] Error: " << error << std::endl;
    });

    return ws_client_->connect();
}

bool BinanceSpotWsApi::connect_user_stream(const std::string& listen_key) {
    WebSocketConfig config;
    config.host = "stream.binance.com";
    config.port = "443";
    config.use_ssl = true;

    // Reconnection settings for user stream
    config.reconnect_interval_ms = 1000;
    config.max_reconnect_interval_ms = 30000;
    config.max_reconnect_attempts = 10;
    config.backoff_multiplier = 1.5;

    user_ws_client_ = std::make_unique<WebSocketClient>(config);

    user_ws_client_->on_message([this](const std::string& msg) {
        this->on_message(msg);
    });

    user_ws_client_->on_connect([](bool connected) {
        if (connected) {
            std::cout << "[WebSocket] User stream connected" << std::endl;
        }
    });

    user_ws_client_->on_disconnect([this](bool) {
        std::cout << "[WebSocket] User stream disconnected, will reconnect..." << std::endl;
    });

    user_ws_client_->on_state_change([](ConnectionState state) {
        std::cout << "[WebSocket] User stream state: "
                  << connection_state_to_string(state) << std::endl;
    });

    user_ws_client_->on_reconnect_attempt([](int attempt, int max_attempts) {
        std::cout << "[WebSocket] User stream reconnect attempt " << attempt;
        if (max_attempts > 0) {
            std::cout << "/" << max_attempts;
        }
        std::cout << std::endl;
    });

    user_ws_client_->on_error([](const std::string& error) {
        std::cerr << "[WebSocket] User stream error: " << error << std::endl;
    });

    // Connect to listen key endpoint
    // URL: wss://stream.binance.com/ws/<listen_key>
    std::string path = "/ws/" + listen_key;

    if (!user_ws_client_->connect()) {
        return false;
    }

    return true;
}

void BinanceSpotWsApi::close() {
    if (ws_client_) {
        ws_client_->close();
        ws_client_.reset();
    }
    if (user_ws_client_) {
        user_ws_client_->close();
        user_ws_client_.reset();
    }
}

bool BinanceSpotWsApi::is_connected() const {
    return (ws_client_ && ws_client_->is_connected()) ||
           (user_ws_client_ && user_ws_client_->is_connected());
}

//=============================================================================
// Market Data Subscriptions
//=============================================================================

void BinanceSpotWsApi::subscribe_ticker(const std::string& symbol) {
    if (!ws_client_ || !ws_client_->is_connected()) {
        return;
    }

    // Convert to lowercase for Binance API
    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    std::string stream = sym + "@ticker";
    ws_client_->subscribe(stream);

    subscribed_symbols_.push_back(symbol);
}

void BinanceSpotWsApi::subscribe_depth(const std::string& symbol) {
    if (!ws_client_ || !ws_client_->is_connected()) {
        return;
    }

    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    // Use partial book depth stream (5 levels)
    std::string stream = sym + "@depth5@100ms";
    ws_client_->subscribe(stream);
}

void BinanceSpotWsApi::subscribe_kline(const std::string& symbol, const std::string& interval) {
    if (!ws_client_ || !ws_client_->is_connected()) {
        return;
    }

    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    std::string stream = sym + "@kline_" + interval;
    ws_client_->subscribe(stream);
}

void BinanceSpotWsApi::unsubscribe_ticker(const std::string& symbol) {
    if (!ws_client_ || !ws_client_->is_connected()) {
        return;
    }

    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    std::string stream = sym + "@ticker";
    ws_client_->unsubscribe(stream);

    subscribed_symbols_.erase(
        std::remove(subscribed_symbols_.begin(), subscribed_symbols_.end(), symbol),
        subscribed_symbols_.end()
    );
}

//=============================================================================
// Callback Registration
//=============================================================================

void BinanceSpotWsApi::on_tick(WsTickCallback callback) {
    tick_callback_ = std::move(callback);
}

void BinanceSpotWsApi::on_bar(WsBarCallback callback) {
    bar_callback_ = std::move(callback);
}

void BinanceSpotWsApi::on_order(WsOrderCallback callback) {
    order_callback_ = std::move(callback);
}

//=============================================================================
// Data Access
//=============================================================================

std::optional<TickData> BinanceSpotWsApi::get_tick(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(ticks_mutex_);
    auto it = ticks_.find(symbol);
    if (it != ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Message Processing
//=============================================================================

void BinanceSpotWsApi::on_message(const std::string& msg) {
    try {
        flatjson::fjson data(msg.c_str(), msg.size());
        process_message(data);
    } catch (const std::exception& e) {
        std::cerr << "[WebSocket] Parse error: " << e.what() << std::endl;
    }
}

void BinanceSpotWsApi::process_message(const flatjson::fjson& data) {
    if (!data.contains("e")) {
        return;
    }

    std::string event_type = data["e"].to_string();

    if (event_type == "24hrTicker") {
        process_ticker(data);
    } else if (event_type == "depthUpdate" || event_type == "depth") {
        process_depth(data);
    } else if (event_type == "kline") {
        process_kline(data);
    } else if (event_type == "executionReport") {
        process_order_update(data);
    } else if (event_type == "outboundAccountPosition") {
        process_account_update(data);
    }
}

void BinanceSpotWsApi::process_ticker(const flatjson::fjson& data) {
    TickData tick;
    tick.symbol = data["s"].to_string();
    tick.exchange = Exchange::BINANCE;
    tick.datetime = data["E"].to_int64();
    tick.last_price = data["c"].to_double();
    tick.volume = data["v"].to_double();
    tick.open_interest = 0.0; // Spot has no open interest

    // Binance ticker provides these
    tick.high_price = data["h"].to_double();
    tick.low_price = data["l"].to_double();
    tick.open_price = data["o"].to_double();

    // Bid/Ask - use last price as approximation (full depth needed for real bid/ask)
    tick.bid_price_1 = tick.last_price;
    tick.bid_volume_1 = 0.0;
    tick.ask_price_1 = tick.last_price;
    tick.ask_volume_1 = 0.0;

    // Cache tick
    {
        std::lock_guard<std::mutex> lock(ticks_mutex_);
        ticks_[tick.symbol] = tick;
    }

    // Callback (set by Gateway)
    if (tick_callback_) {
        tick_callback_(tick);
    }
}

void BinanceSpotWsApi::process_depth(const flatjson::fjson& data) {
    // Depth update processing
    // Could be used for order book visualization
    // For now, just update the tick cache with best bid/ask
}

void BinanceSpotWsApi::process_kline(const flatjson::fjson& data) {
    auto k = data["k"];

    BarData bar;
    bar.symbol = k["s"].to_string();
    bar.exchange = Exchange::BINANCE;
    bar.interval = interval_from_binance(k["i"].to_string());
    bar.datetime = k["t"].to_int64();
    bar.open_price = k["o"].to_double();
    bar.high_price = k["h"].to_double();
    bar.low_price = k["l"].to_double();
    bar.close_price = k["c"].to_double();
    bar.volume = k["v"].to_double();

    // Callback (set by Gateway)
    if (bar_callback_) {
        bar_callback_(bar);
    }
}

void BinanceSpotWsApi::process_order_update(const flatjson::fjson& data) {
    // Convert WebSocket order update to OrderData
    OrderData order;
    order.symbol = data["s"].to_string();
    order.exchange = Exchange::BINANCE;
    order.orderid = std::to_string(data["i"].to_int64());
    order.type = order_type_from_binance(data["o"].to_string());
    order.direction = direction_from_binance(data["S"].to_string());
    order.price = data["p"].to_double();
    order.volume = data["q"].to_double();
    order.traded = data["z"].to_double();
    order.status = status_from_binance(data["X"].to_string());
    order.datetime = data["T"].to_int64();

    // Calculate average traded price
    if (order.traded > 0) {
        double cum_quote = data["Z"].to_double();
        order.traded_price = cum_quote / order.traded;
    }

    // Callback (for raw JSON, if needed)
    if (order_callback_) {
        order_callback_(data);
    }

    // Process through gateway
    if (gateway_) {
        gateway_->process_order_update(order);
    }
}

void BinanceSpotWsApi::process_account_update(const flatjson::fjson& data) {
    // Account balance update from user stream
    // B: balances array
    auto balances = data["B"];
    for (size_t i = 0; i < balances.size(); ++i) {
        auto balance = balances[i];
        AccountData account;
        account.accountid = balance["a"].to_string();
        account.balance = balance["f"].to_double() + balance["l"].to_double();
        account.frozen = balance["l"].to_double();

        if (gateway_) {
            gateway_->process_account_update(account);
        }
    }
}

} // namespace bitquant
