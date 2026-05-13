/**
 * @file okx_gateway.cpp
 * @brief OKX Exchange Gateway implementation
 */

#include "exchange/okx_gateway.hpp"
#include "utils/datetime.hpp"
#include "utils/logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <iostream>

namespace bitquant {

// OKX API endpoints
constexpr const char* REST_HOST = "https://www.okx.com";
constexpr const char* WS_PUBLIC_HOST = "wss://ws.okx.com:8443/ws/v5/public";
constexpr const char* WS_PRIVATE_HOST = "wss://ws.okx.com:8443/ws/v5/private";

// API paths
constexpr const char* PATH_TIME = "/api/v5/public/time";
constexpr const char* PATH_INSTRUMENTS = "/api/v5/public/instruments";
constexpr const char* PATH_BALANCE = "/api/v5/account/balance";
constexpr const char* PATH_POSITIONS = "/api/v5/account/positions";
constexpr const char* PATH_ORDERS = "/api/v5/trade/orders";
constexpr const char* PATH_ORDER_PENDING = "/api/v5/trade/orders-pending";
constexpr const char* PATH_TICKERS = "/api/v5/market/tickers";
constexpr const char* PATH_CANDLES = "/api/v5/market/candles";

//=============================================================================
// Construction
//=============================================================================

OkxGateway::OkxGateway() = default;

OkxGateway::OkxGateway(const std::string& gateway_name)
    : gateway_name_(gateway_name) {}

OkxGateway::~OkxGateway() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool OkxGateway::connect(const std::string& config) {
    if (connected_) return true;

    // Parse config (simple format: key|secret|passphrase)
    // TODO: Use proper JSON parsing
    size_t pos1 = config.find('|');
    size_t pos2 = config.find('|', pos1 + 1);

    if (pos1 != std::string::npos && pos2 != std::string::npos) {
        api_key_ = config.substr(0, pos1);
        api_secret_ = config.substr(pos1 + 1, pos2 - pos1 - 1);
        passphrase_ = config.substr(pos2 + 1);
    }

    // Query instruments
    auto contracts = get_instruments();
    for (const auto& c : contracts) {
        contracts_[c.symbol] = c;
        if (contract_callback_) {
            contract_callback_(c);
        }
    }

    // Query account balance
    query_account();

    // Query positions
    query_positions();

    connected_ = true;
    std::cout << "[" << gateway_name_ << "] Connected" << std::endl;
    return true;
}

void OkxGateway::close() {
    if (!connected_) return;

    connected_ = false;
    ws_running_ = false;

    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }
}

bool OkxGateway::is_connected() const {
    return connected_;
}

//=============================================================================
// Market Data - Synchronous
//=============================================================================

std::optional<TickData> OkxGateway::get_tick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(ticks_mutex_);
    auto it = last_ticks_.find(symbol);
    if (it != last_ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<BarData> OkxGateway::get_bars(const HistoryRequest& req) {
    // TODO: Implement OKX kline query
    return {};
}

double OkxGateway::get_price(const std::string& symbol) {
    auto tick = get_tick(symbol);
    return tick ? tick->last_price : 0.0;
}

std::optional<ContractData> OkxGateway::get_contract(const std::string& symbol) {
    auto it = contracts_.find(symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Market Data - Asynchronous
//=============================================================================

void OkxGateway::subscribe_tick(const SubscribeRequest& req) {
    if (!connected_) return;

    // TODO: Subscribe via WebSocket
    // Send: {"op":"subscribe","args":[{"channel":"tickers","instId":"BTC-USDT"}]}

    std::cout << "[" << gateway_name_ << "] Subscribe tick: " << req.symbol << std::endl;
}

void OkxGateway::subscribe_bar(const std::string& symbol, Interval interval) {
    if (!connected_) return;

    // Convert interval to OKX format
    std::string bar;
    switch (interval) {
        case Interval::MINUTE_1: bar = "1m"; break;
        case Interval::MINUTE_3: bar = "3m"; break;
        case Interval::MINUTE_5: bar = "5m"; break;
        case Interval::MINUTE_15: bar = "15m"; break;
        case Interval::MINUTE_30: bar = "30m"; break;
        case Interval::HOUR_1: bar = "1H"; break;
        case Interval::HOUR_2: bar = "2H"; break;
        case Interval::HOUR_4: bar = "4H"; break;
        case Interval::HOUR_6: bar = "6H"; break;
        case Interval::HOUR_12: bar = "12H"; break;
        case Interval::DAILY: bar = "1D"; break;
        case Interval::WEEKLY: bar = "1W"; break;
        default: bar = "1m"; break;
    }

    // TODO: Subscribe via WebSocket
    // Send: {"op":"subscribe","args":[{"channel":"candle1m","instId":"BTC-USDT"}]}

    std::cout << "[" << gateway_name_ << "] Subscribe bar: " << symbol << " " << bar << std::endl;
}

void OkxGateway::unsubscribe_tick(const std::string& symbol) {
    if (!connected_) return;

    // TODO: Unsubscribe via WebSocket
    // Send: {"op":"unsubscribe","args":[{"channel":"tickers","instId":"BTC-USDT"}]}

    std::cout << "[" << gateway_name_ << "] Unsubscribe tick: " << symbol << std::endl;
}

//=============================================================================
// Order Management
//=============================================================================

std::string OkxGateway::send_order(const OrderRequest& req) {
    if (!connected_) return "";

    std::string orderid = generate_order_id();

    OrderData order;
    order.symbol = req.symbol;
    order.exchange = req.exchange;
    order.orderid = orderid;
    order.gateway_name = gateway_name_;
    order.type = req.type;
    order.direction = req.direction;
    order.offset = req.offset;
    order.price = req.price;
    order.volume = req.volume;
    order.status = Status::SUBMITTING;

    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        orders_[orderid] = order;
    }

    if (order_callback_) {
        order_callback_(order);
    }

    // TODO: Send order via REST API
    // POST /api/v5/trade/order
    // Body: {"instId":"BTC-USDT","tdMode":"cash","side":"buy","ordType":"limit","px":"50000","sz":"0.01"}

    std::cout << "[" << gateway_name_ << "] Send order: " << orderid
              << " " << req.symbol
              << " " << (req.direction == Direction::LONG ? "BUY" : "SELL")
              << " " << req.volume << "@" << req.price << std::endl;

    // Simulate order accepted
    order.status = Status::NOTTRADED;
    if (order_callback_) {
        order_callback_(order);
    }

    return orderid;
}

bool OkxGateway::cancel_order(const CancelRequest& req) {
    if (!connected_) return false;

    // TODO: Cancel order via REST API
    // POST /api/v5/trade/cancel-order
    // Body: {"instId":"BTC-USDT","ordId":"123456"}

    std::cout << "[" << gateway_name_ << "] Cancel order: " << req.orderid << std::endl;

    return true;
}

void OkxGateway::cancel_all_orders(const std::string& symbol) {
    if (!connected_) return;

    // TODO: Cancel all orders via REST API
    // POST /api/v5/trade/cancel-all-orders

    std::cout << "[" << gateway_name_ << "] Cancel all orders for: " << symbol << std::endl;
}

std::optional<OrderData> OkxGateway::query_order(const OrderQueryRequest& req) {
    if (!connected_) return std::nullopt;

    // TODO: Query order via REST API
    // GET /api/v5/trade/order?instId={symbol}&ordId={orderid}

    return std::nullopt;
}

//=============================================================================
// Account Management
//=============================================================================

std::vector<PositionData> OkxGateway::query_positions(const std::string& symbol) {
    // TODO: Make actual REST API call
    // GET /api/v5/account/positions?instId={symbol}

    std::vector<PositionData> result;

    // For now, return cached positions
    std::lock_guard<std::mutex> lock(positions_mutex_);
    if (symbol.empty()) {
        for (const auto& [_, pos] : positions_) {
            result.push_back(pos);
        }
    } else {
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            result.push_back(it->second);
        }
    }

    return result;
}

std::optional<AccountData> OkxGateway::query_account() {
    // TODO: Make actual REST API call
    // GET /api/v5/account/balance

    // For now, return a default account
    AccountData account;
    account.accountid = "USDT";
    account.gateway_name = gateway_name_;
    account.balance = 10000.0;
    account.frozen = 0.0;

    if (account_callback_) {
        account_callback_(account);
    }

    return account;
}

std::vector<OrderData> OkxGateway::query_open_orders(const std::string& symbol) {
    // TODO: Make actual REST API call
    // GET /api/v5/trade/orders-pending?instId={symbol}

    return {};
}

//=============================================================================
// Utility
//=============================================================================

int64_t OkxGateway::get_server_time() {
    // TODO: Query server time via REST API
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool OkxGateway::ping() {
    return connected_;
}

std::vector<ContractData> OkxGateway::get_instruments(const std::string& inst_type) {
    // TODO: Query instruments via REST API
    // inst_type: SPOT, SWAP, FUTURES

    std::vector<ContractData> contracts;

    // Default contracts for testing
    if (inst_type == "SPOT" || inst_type.empty()) {
        ContractData btc;
        btc.symbol = "BTC-USDT";
        btc.exchange = Exchange::OKX;
        btc.name = "BTC/USDT";
        btc.product = Product::SPOT;
        btc.pricetick = 0.1;
        btc.size = 1.0;
        btc.min_volume = 0.0001;
        btc.min_notional = 10.0;
        btc.history_data = true;
        contracts.push_back(btc);

        ContractData eth;
        eth.symbol = "ETH-USDT";
        eth.exchange = Exchange::OKX;
        eth.name = "ETH/USDT";
        eth.product = Product::SPOT;
        eth.pricetick = 0.01;
        eth.size = 1.0;
        eth.min_volume = 0.001;
        eth.min_notional = 10.0;
        eth.history_data = true;
        contracts.push_back(eth);
    }

    if (inst_type == "SWAP" || inst_type.empty()) {
        ContractData btc_swap;
        btc_swap.symbol = "BTC-USDT-SWAP";
        btc_swap.exchange = Exchange::OKX;
        btc_swap.name = "BTC/USDT SWAP";
        btc_swap.product = Product::FUTURES;
        btc_swap.pricetick = 0.1;
        btc_swap.size = 1.0;
        btc_swap.min_volume = 1.0;
        btc_swap.min_notional = 10.0;
        btc_swap.net_position = true;
        btc_swap.history_data = true;
        contracts.push_back(btc_swap);
    }

    return contracts;
}

//=============================================================================
// WebSocket Data Processing
//=============================================================================

void OkxGateway::process_order_update(const OrderData& order) {
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = orders_.find(order.orderid);
        if (it != orders_.end()) {
            double traded = order.traded - it->second.traded;

            if (traded > 0) {
                TradeData trade;
                trade.symbol = order.symbol;
                trade.exchange = order.exchange;
                trade.orderid = order.orderid;
                trade.tradeid = order.orderid + "_T";
                trade.direction = order.direction;
                trade.price = order.traded_price > 0 ? order.traded_price : order.price;
                trade.volume = traded;
                trade.datetime = order.update_time;
                trade.gateway_name = gateway_name_;

                if (trade_callback_) {
                    trade_callback_(trade);
                }
            }

            it->second = order;
        }
    }

    if (order_callback_) {
        order_callback_(order);
    }
}

void OkxGateway::process_account_update(const AccountData& account) {
    if (account_callback_) {
        account_callback_(account);
    }
}

void OkxGateway::process_position_update(const PositionData& position) {
    {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        positions_[position.symbol] = position;
    }

    if (position_callback_) {
        position_callback_(position);
    }
}

void OkxGateway::process_tick_update(const TickData& tick) {
    {
        std::lock_guard<std::mutex> lock(ticks_mutex_);
        last_ticks_[tick.symbol] = tick;
    }

    if (tick_callback_) {
        tick_callback_(tick);
    }
}

//=============================================================================
// Internal Methods
//=============================================================================

std::string OkxGateway::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();

    std::ostringstream oss;
    oss << timestamp << order_count_;
    return oss.str();
}

std::string OkxGateway::generate_signature(const std::string& timestamp,
                                           const std::string& method,
                                           const std::string& request_path,
                                           const std::string& body) {
    // Create message to sign: timestamp + method + request_path + body
    std::string message = timestamp + method + request_path + body;

    // HMAC SHA256
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;

    HMAC(EVP_sha256(), api_secret_.c_str(), api_secret_.length(),
         reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
         digest, &digest_len);

    // Base64 encode
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_write(bio, digest, digest_len);
    BIO_flush(bio);

    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);

    std::string signature(buffer_ptr->data, buffer_ptr->length);

    // Remove newline if present
    if (!signature.empty() && signature.back() == '\n') {
        signature.pop_back();
    }

    BIO_free_all(bio);

    return signature;
}

// Register gateway
namespace {
    static bool registered = []() {
        ExchangeFactory::register_exchange("okx", []() {
            return std::make_unique<OkxGateway>();
        });
        return true;
    }();
}

} // namespace bitquant
