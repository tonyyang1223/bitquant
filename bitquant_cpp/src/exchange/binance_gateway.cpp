/**
 * @file binance_gateway.cpp
 * @brief Binance gateway implementation
 *
 * Implements IExchange interface using binapi library
 */

#include "exchange/binance_gateway.hpp"
#include "exchange/binance_api.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

namespace bitquant {

//=============================================================================
// Auto-registration
//=============================================================================

namespace {
    bool register_binance_gateway() {
        ExchangeFactory::register_exchange("binance", []() {
            return std::make_unique<BinanceGateway>();
        });
        return true;
    }
}

bool binance_gateway_registered = register_binance_gateway();

//=============================================================================
// Constructor/Destructor
//=============================================================================

BinanceGateway::BinanceGateway() = default;

BinanceGateway::BinanceGateway(const BinanceGatewayConfig& config)
    : config_(config) {}

BinanceGateway::~BinanceGateway() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool BinanceGateway::connect(const std::string& config) {
    // Parse JSON config if provided
    if (!config.empty()) {
        // Simple JSON parsing for key fields
        auto get_value = [&config](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":";
            size_t pos = config.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            while (pos < config.length() && (config[pos] == ' ' || config[pos] == '"')) pos++;
            size_t end = config.find_first_of(",}\"", pos);
            return config.substr(pos, end - pos);
        };

        config_.api_key = get_value("api_key");
        config_.api_secret = get_value("api_secret");
        config_.host = get_value("host").empty() ? config_.host : get_value("host");
        config_.testnet = get_value("testnet") == "true";
    }

    // Initialize REST API
    if (!init_rest_api()) {
        last_error_ = "Failed to initialize REST API";
        return false;
    }

    // Initialize WebSocket if enabled
    if (config_.use_websocket) {
        if (!init_websocket()) {
            // WebSocket is optional, continue without it
            std::cerr << "Warning: WebSocket initialization failed" << std::endl;
        }
    }

    connected_ = true;
    return true;
}

void BinanceGateway::close() {
    connected_ = false;

    // Stop WebSocket
    ws_running_ = false;
    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }

    // Clean up implementations
    rest_impl_.reset();
    ws_impl_.reset();
}

bool BinanceGateway::is_connected() const {
    return connected_;
}

//=============================================================================
// REST API Implementation
//=============================================================================

struct BinanceGateway::RestImpl {
    BinanceApiClient client;
    bool initialized = false;
};

bool BinanceGateway::init_rest_api() {
    rest_impl_ = std::make_unique<RestImpl>();

    BinanceConfig api_config;
    api_config.host = config_.host;
    api_config.port = config_.port;
    api_config.api_key = config_.api_key;
    api_config.api_secret = config_.api_secret;
    api_config.timeout_ms = config_.timeout_ms;
    api_config.testnet = config_.testnet;

    rest_impl_->client = BinanceApiClient(api_config);
    rest_impl_->initialized = rest_impl_->client.init();

    return rest_impl_->initialized;
}

//=============================================================================
// Market Data - Synchronous
//=============================================================================

std::optional<TickData> BinanceGateway::get_tick(const std::string& symbol) {
    // Check cache first
    auto it = last_ticks_.find(symbol);
    if (it != last_ticks_.end()) {
        return it->second;
    }

    // Fetch current price and create basic tick
    double price = get_price(symbol);
    if (price <= 0) {
        return std::nullopt;
    }

    TickData tick;
    tick.symbol = symbol;
    tick.exchange = Exchange::BINANCE;
    tick.last_price = price;
    tick.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    last_ticks_[symbol] = tick;
    return tick;
}

std::vector<BarData> BinanceGateway::get_bars(const HistoryRequest& req) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return {};
    }

    // Convert Interval to Binance KlineInterval
    KlineInterval interval = KlineInterval::MIN_1;
    switch (req.interval) {
        case Interval::MINUTE_1: interval = KlineInterval::MIN_1; break;
        case Interval::MINUTE_3: interval = KlineInterval::MIN_3; break;
        case Interval::MINUTE_5: interval = KlineInterval::MIN_5; break;
        case Interval::MINUTE_15: interval = KlineInterval::MIN_15; break;
        case Interval::MINUTE_30: interval = KlineInterval::MIN_30; break;
        case Interval::HOUR_1: interval = KlineInterval::HOUR_1; break;
        case Interval::HOUR_2: interval = KlineInterval::HOUR_2; break;
        case Interval::HOUR_4: interval = KlineInterval::HOUR_4; break;
        case Interval::HOUR_6: interval = KlineInterval::HOUR_6; break;
        case Interval::HOUR_12: interval = KlineInterval::HOUR_12; break;
        case Interval::DAILY: interval = KlineInterval::DAY_1; break;
        case Interval::WEEKLY: interval = KlineInterval::WEEK_1; break;
        default: break;
    }

    return rest_impl_->client.get_klines(req.symbol, interval, req.limit);
}

double BinanceGateway::get_price(const std::string& symbol) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return 0.0;
    }
    return rest_impl_->client.get_price(symbol);
}

std::optional<ContractData> BinanceGateway::get_contract(const std::string& symbol) {
    auto it = contracts_.find(symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Market Data - Asynchronous (WebSocket)
//=============================================================================

struct BinanceGateway::WsImpl {
    // WebSocket implementation placeholder
    // In full implementation, this would use binapi websocket
    std::queue<std::string> message_queue;
    std::mutex mutex;
};

bool BinanceGateway::init_websocket() {
    ws_impl_ = std::make_unique<WsImpl>();
    ws_running_ = true;

    // Start WebSocket thread
    ws_thread_ = std::thread(&BinanceGateway::ws_thread_func, this);

    return true;
}

void BinanceGateway::ws_thread_func() {
    while (ws_running_) {
        // WebSocket event loop
        // In full implementation, this would process WebSocket messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void BinanceGateway::subscribe_tick(const SubscribeRequest& req) {
    tick_subscriptions_[req.symbol] = true;

    // In full implementation, send subscribe message to WebSocket
    // ws_impl_->send_subscribe(req.symbol + "@aggTrade");
}

void BinanceGateway::subscribe_bar(const std::string& symbol, Interval interval) {
    bar_subscriptions_[symbol] = interval;

    // In full implementation, send subscribe message to WebSocket
    // std::string stream = symbol + "@kline_" + interval_to_string(interval);
    // ws_impl_->send_subscribe(stream);
}

void BinanceGateway::unsubscribe_tick(const std::string& symbol) {
    tick_subscriptions_.erase(symbol);
}

void BinanceGateway::process_ws_message(const std::string& message) {
    // Parse WebSocket message and invoke callbacks
    (void)message;

    // Example: parse tick update
    if (tick_callback_) {
        // TickData tick = parse_tick(message);
        // tick_callback_(tick);
    }

    // Example: parse bar update
    if (bar_callback_) {
        // BarData bar = parse_bar(message);
        // bar_callback_(bar);
    }
}

//=============================================================================
// Order Management
//=============================================================================

std::string BinanceGateway::send_order(const OrderRequest& req) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        last_error_ = "Not connected";
        return "";
    }

    // In full implementation, use binapi to send order
    // For now, return placeholder
    (void)req;

    // binapi::rest::send_order(...)
    last_error_ = "Trading not implemented yet";
    return "";
}

bool BinanceGateway::cancel_order(const CancelRequest& req) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return false;
    }

    // In full implementation, use binapi to cancel order
    (void)req;

    last_error_ = "Trading not implemented yet";
    return false;
}

void BinanceGateway::cancel_all_orders(const std::string& symbol) {
    (void)symbol;
    // In full implementation, cancel all orders
}

std::optional<OrderData> BinanceGateway::query_order(const std::string& orderid) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return std::nullopt;
    }

    // In full implementation, query order status
    (void)orderid;

    return std::nullopt;
}

//=============================================================================
// Account Management
//=============================================================================

std::vector<PositionData> BinanceGateway::query_positions(const std::string& symbol) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return {};
    }

    // In full implementation, query positions via API
    (void)symbol;

    return {};
}

std::optional<AccountData> BinanceGateway::query_account() {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return std::nullopt;
    }

    // In full implementation, query account info via API
    // Requires API key/secret

    return std::nullopt;
}

std::vector<OrderData> BinanceGateway::query_open_orders(const std::string& symbol) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return {};
    }

    // In full implementation, query open orders via API
    (void)symbol;

    return {};
}

//=============================================================================
// Utility Methods
//=============================================================================

uint64_t BinanceGateway::get_server_time() {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return 0;
    }
    return rest_impl_->client.get_server_time();
}

bool BinanceGateway::ping() {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return false;
    }
    return rest_impl_->client.ping();
}

std::vector<ContractData> BinanceGateway::get_exchange_info() {
    // In full implementation, fetch exchange info from API
    return {};
}

size_t BinanceGateway::fetch_historical_data(
    const std::string& symbol,
    Interval interval,
    size_t total_bars,
    const std::string& output_dir
) {
    if (!rest_impl_ || !rest_impl_->initialized) {
        return 0;
    }

    // Convert Interval to KlineInterval
    KlineInterval ki = KlineInterval::MIN_1;
    switch (interval) {
        case Interval::MINUTE_1: ki = KlineInterval::MIN_1; break;
        case Interval::MINUTE_5: ki = KlineInterval::MIN_5; break;
        case Interval::MINUTE_15: ki = KlineInterval::MIN_15; break;
        case Interval::HOUR_1: ki = KlineInterval::HOUR_1; break;
        case Interval::HOUR_4: ki = KlineInterval::HOUR_4; break;
        case Interval::DAILY: ki = KlineInterval::DAY_1; break;
        default: break;
    }

    return rest_impl_->client.fetch_historical_data(symbol, ki, total_bars, output_dir);
}

std::string BinanceGateway::interval_to_string(Interval interval) {
    switch (interval) {
        case Interval::MINUTE_1: return "1m";
        case Interval::MINUTE_3: return "3m";
        case Interval::MINUTE_5: return "5m";
        case Interval::MINUTE_15: return "15m";
        case Interval::MINUTE_30: return "30m";
        case Interval::HOUR_1: return "1h";
        case Interval::HOUR_2: return "2h";
        case Interval::HOUR_4: return "4h";
        case Interval::HOUR_6: return "6h";
        case Interval::HOUR_12: return "12h";
        case Interval::DAILY: return "1d";
        case Interval::WEEKLY: return "1w";
        case Interval::MONTHLY: return "1M";
        default: return "1m";
    }
}

} // namespace bitquant
