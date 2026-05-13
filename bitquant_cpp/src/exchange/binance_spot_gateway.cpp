/**
 * @file binance_spot_gateway.cpp
 * @brief Binance Spot Gateway Implementation
 *
 * Implements IExchange interface for Binance Spot trading.
 * Coordinates REST API and WebSocket connections.
 */

#include "exchange/binance_spot_gateway.hpp"
#include "exchange/i_exchange.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace bitquant {

//=============================================================================
// Auto-registration
//=============================================================================

namespace {
    bool register_binance_spot_gateway() {
        ExchangeFactory::register_exchange("binance_spot", []() {
            return std::make_unique<BinanceSpotGateway>();
        });
        return true;
    }
}

static bool binance_spot_gateway_registered = register_binance_spot_gateway();

//=============================================================================
// Constructor/Destructor
//=============================================================================

BinanceSpotGateway::BinanceSpotGateway() = default;

BinanceSpotGateway::BinanceSpotGateway(const std::string& gateway_name)
    : gateway_name_(gateway_name) {}

BinanceSpotGateway::~BinanceSpotGateway() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool BinanceSpotGateway::connect(const std::string& config) {
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

    GatewayConfig gc;
    gc.api_key = get_value("api_key");
    gc.api_secret = get_value("api_secret");
    gc.proxy_host = get_value("proxy_host");
    gc.proxy_port = get_value("proxy_port").empty() ? 0 : std::stoi(get_value("proxy_port"));
    gc.testnet = get_value("testnet") == "true";

    return connect(gc);
}

bool BinanceSpotGateway::connect(const GatewayConfig& config) {
    rest_api_ = std::make_unique<BinanceSpotRestApi>();

    std::string host = config.testnet ? "testnet.binance.vision" : "api.binance.com";
    std::string port = "443";

    if (!rest_api_->init(host, port, config.api_key, config.api_secret)) {
        last_error_ = rest_api_->last_error();
        return false;
    }

    // Query server time to sync clock
    if (rest_api_->get_server_time() == 0) {
        last_error_ = rest_api_->last_error();
        return false;
    }

    // Generate connect_time for order ID
    connect_time_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    connect_time_ *= 1000000;  // Format: YYMMDDHHMMSS * order_count

    // Query exchange info (public API, no auth required)
    auto contracts = rest_api_->get_exchange_info();
    for (const auto& c : contracts) {
        contracts_[c.symbol] = c;
        if (contract_callback_) contract_callback_(c);
    }

    // Query account and orders only if API key is provided
    if (!config.api_key.empty()) {
        // Query account
        auto accounts = rest_api_->query_account_all();
        for (const auto& a : accounts) {
            if (account_callback_) account_callback_(a);
        }

        // Query open orders
        auto orders = rest_api_->query_open_orders();
        for (const auto& o : orders) {
            orders_[o.orderid] = o;
            if (order_callback_) order_callback_(o);
        }

        // Start user stream
        start_user_stream();
    }

    connected_ = true;
    return true;
}

void BinanceSpotGateway::close() {
    connected_ = false;

    if (rest_api_ && !listen_key_.empty()) {
        rest_api_->close_user_stream(listen_key_);
    }

    rest_api_.reset();
    orders_.clear();
    contracts_.clear();
}

bool BinanceSpotGateway::is_connected() const {
    return connected_;
}

//=============================================================================
// Order ID Generation
//=============================================================================

std::string BinanceSpotGateway::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;
    std::ostringstream ss;
    ss << "x-A6SIDXVS" << (connect_time_ + order_count_);
    return ss.str();
}

//=============================================================================
// Market Data - Synchronous
//=============================================================================

std::optional<TickData> BinanceSpotGateway::get_tick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(ticks_mutex_);
    auto it = last_ticks_.find(symbol);
    if (it != last_ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<BarData> BinanceSpotGateway::get_bars(const HistoryRequest& req) {
    if (!rest_api_) return {};
    return rest_api_->get_klines(req.symbol, req.interval, req.limit);
}

double BinanceSpotGateway::get_price(const std::string& symbol) {
    if (!rest_api_) return 0.0;
    return rest_api_->get_price(symbol);
}

std::optional<ContractData> BinanceSpotGateway::get_contract(const std::string& symbol) {
    auto it = contracts_.find(symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Order Management
//=============================================================================

std::string BinanceSpotGateway::send_order(const OrderRequest& req) {
    if (!rest_api_ || !connected_) {
        last_error_ = "Not connected";
        return "";
    }

    // Create order data
    std::string order_id = generate_order_id();
    OrderData order = req.create_order_data(order_id, gateway_name_);
    order.status = Status::SUBMITTING;
    order.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Send to exchange
    OrderRequest api_req = req;
    api_req.reference = order_id;  // Use as client order ID

    std::string exchange_order_id = rest_api_->send_order(api_req);
    if (exchange_order_id.empty()) {
        order.status = Status::REJECTED;
        order.rejected_reason = rest_api_->last_error();
        process_order_update(order);
        last_error_ = rest_api_->last_error();
        return order.vt_orderid();
    }

    order.orderid = exchange_order_id;
    order.status = Status::NOTTRADED;
    process_order_update(order);

    return order.vt_orderid();
}

bool BinanceSpotGateway::cancel_order(const CancelRequest& req) {
    if (!rest_api_ || !connected_) {
        last_error_ = "Not connected";
        return false;
    }

    return rest_api_->cancel_order(req.symbol, req.orderid);
}

void BinanceSpotGateway::cancel_all_orders(const std::string& symbol) {
    if (!rest_api_ || !connected_) return;

    auto orders = rest_api_->query_open_orders(symbol);
    for (const auto& o : orders) {
        CancelRequest req;
        req.orderid = o.orderid;
        req.symbol = o.symbol;
        req.exchange = o.exchange;
        cancel_order(req);
    }
}

std::optional<OrderData> BinanceSpotGateway::query_order(const OrderQueryRequest& req) {
    if (!rest_api_ || !connected_) {
        return std::nullopt;
    }
    return rest_api_->query_order(req.symbol, req.orderid);
}

//=============================================================================
// Account Management
//=============================================================================

std::vector<PositionData> BinanceSpotGateway::query_positions(const std::string& symbol) {
    // Spot trading has no positions concept
    (void)symbol;
    return {};
}

std::optional<AccountData> BinanceSpotGateway::query_account() {
    if (!rest_api_ || !connected_) {
        return std::nullopt;
    }

    return rest_api_->query_account();
}

std::vector<AccountData> BinanceSpotGateway::query_account_all() {
    if (!rest_api_) return {};
    return rest_api_->query_account_all();
}

std::vector<OrderData> BinanceSpotGateway::query_open_orders(const std::string& symbol) {
    if (!rest_api_) return {};
    return rest_api_->query_open_orders(symbol);
}

//=============================================================================
// WebSocket Subscriptions
//=============================================================================

void BinanceSpotGateway::subscribe_tick(const SubscribeRequest& req) {
    if (!ws_api_) {
        ws_api_ = std::make_unique<BinanceSpotWsApi>(this);
        ws_api_->connect_market_stream();
        ws_api_->on_tick([this](const TickData& tick) {
            std::lock_guard<std::mutex> lock(ticks_mutex_);
            last_ticks_[tick.symbol] = tick;
            if (tick_callback_) tick_callback_(tick);
        });
    }
    ws_api_->subscribe_ticker(req.symbol);
}

void BinanceSpotGateway::subscribe_bar(const std::string& symbol, Interval interval) {
    if (!ws_api_) {
        ws_api_ = std::make_unique<BinanceSpotWsApi>(this);
        ws_api_->connect_market_stream();
        ws_api_->on_bar([this](const BarData& bar) {
            if (bar_callback_) bar_callback_(bar);
        });
    }
    std::string interval_str = interval_to_binance(interval);
    ws_api_->subscribe_kline(symbol, interval_str);
}

void BinanceSpotGateway::unsubscribe_tick(const std::string& symbol) {
    if (ws_api_) {
        ws_api_->unsubscribe_ticker(symbol);
    }
}

//=============================================================================
// Utility Methods
//=============================================================================

int64_t BinanceSpotGateway::get_server_time() {
    if (!rest_api_) return 0;
    return rest_api_->get_server_time();
}

bool BinanceSpotGateway::ping() {
    if (!rest_api_) return false;
    return rest_api_->ping();
}

std::vector<ContractData> BinanceSpotGateway::get_exchange_info() {
    if (!rest_api_) return {};
    return rest_api_->get_exchange_info();
}

bool BinanceSpotGateway::start_user_stream() {
    if (!rest_api_) return false;

    listen_key_ = rest_api_->start_user_stream();
    if (listen_key_.empty()) {
        last_error_ = "Failed to start user stream";
        return false;
    }

    // Start WebSocket connection for user data stream
    if (!ws_api_) {
        ws_api_ = std::make_unique<BinanceSpotWsApi>(this);
    }
    ws_api_->connect_user_stream(listen_key_);
    // Order updates are processed in BinanceSpotWsApi::process_order_update
    // which calls process_order_update on the gateway directly
    return true;
}

void BinanceSpotGateway::keep_user_stream() {
    keep_alive_count_++;
    if (keep_alive_count_ < 300) return;
    keep_alive_count_ = 0;

    if (rest_api_ && !listen_key_.empty()) {
        rest_api_->keep_user_stream(listen_key_);
    }
}

//=============================================================================
// Order Processing (with Trade generation)
//=============================================================================

void BinanceSpotGateway::process_order_update(const OrderData& order) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    auto it = orders_.find(order.orderid);
    if (it == orders_.end()) {
        // New order
        orders_[order.orderid] = order;
        if (order_callback_) order_callback_(order);
    } else {
        // Existing order - check for new trades
        double traded = order.traded - it->second.traded;

        if (traded < 0) {
            // Out of sequence, ignore
            return;
        }

        if (traded > 0) {
            // Generate TradeData
            TradeData trade;
            trade.symbol = order.symbol;
            trade.exchange = order.exchange;
            trade.orderid = order.orderid;
            trade.direction = order.direction;
            trade.price = order.traded_price;
            trade.volume = traded;
            trade.datetime = order.update_time;
            trade.gateway_name = gateway_name_;

            if (trade_callback_) trade_callback_(trade);
        }

        // Skip if no change
        if (traded == 0 && order.status == it->second.status) {
            return;
        }

        orders_[order.orderid] = order;
        if (order_callback_) order_callback_(order);
    }
}

//=============================================================================
// Account Processing
//=============================================================================

void BinanceSpotGateway::process_account_update(const AccountData& account) {
    if (account_callback_) {
        account_callback_(account);
    }
}

} // namespace bitquant
