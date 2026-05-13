/**
 * @file binance_usdt_gateway.cpp
 * @brief Binance USDT-M Futures Gateway implementation
 */

#include "exchange/binance_usdt_gateway.hpp"
#include "exchange/binance_usdt_rest_api.hpp"
#include "utils/datetime.hpp"
#include "utils/logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace bitquant {

// Binance USDT Futures API endpoints
constexpr const char* F_REST_HOST = "https://fapi.binance.com";
constexpr const char* F_WS_DATA_HOST = "wss://fstream.binance.com/stream";
constexpr const char* F_WS_TRADE_HOST = "wss://fstream.binance.com/ws/";

// Order status mapping
Status parse_binance_status(const std::string& status) {
    if (status == "NEW") return Status::NOTTRADED;
    if (status == "PARTIALLY_FILLED") return Status::PARTTRADED;
    if (status == "FILLED") return Status::ALLTRADED;
    if (status == "CANCELED") return Status::CANCELLED;
    if (status == "REJECTED") return Status::REJECTED;
    if (status == "EXPIRED") return Status::CANCELLED;
    return Status::NOTTRADED;
}

// Direction mapping
std::string direction_to_binance(Direction dir) {
    return (dir == Direction::LONG) ? "BUY" : "SELL";
}

Direction binance_to_direction(const std::string& side) {
    return (side == "BUY") ? Direction::LONG : Direction::SHORT;
}

//=============================================================================
// Construction
//=============================================================================

BinanceUsdtGateway::BinanceUsdtGateway() = default;

BinanceUsdtGateway::BinanceUsdtGateway(const std::string& gateway_name)
    : gateway_name_(gateway_name) {}

BinanceUsdtGateway::~BinanceUsdtGateway() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool BinanceUsdtGateway::connect(const std::string& config) {
    if (connected_) return true;

    // Initialize REST API
    init_rest_api();

    // Initialize WebSocket API
    init_ws_api();

    // Connect REST API
    if (rest_api_) {
        // TODO: Parse config for API key and secret
        rest_api_->connect("", "", "", 0);

        // Query exchange info
        auto contracts = rest_api_->query_exchange_info();
        for (const auto& c : contracts) {
            contracts_[c.symbol] = c;
            if (contract_callback_) {
                contract_callback_(c);
            }
        }

        // Query account
        rest_api_->query_account();

        // Query positions
        rest_api_->query_positions();

        // Start user stream
        std::string listen_key = rest_api_->start_user_stream();
        if (!listen_key.empty() && user_ws_api_) {
            user_ws_api_->connect(listen_key);
        }
    }

    // Connect Market Data WebSocket
    if (ws_api_) {
        ws_api_->connect();
    }

    connected_ = true;
    return true;
}

void BinanceUsdtGateway::close() {
    if (!connected_) return;

    connected_ = false;

    // Stop WebSocket APIs
    if (ws_api_) {
        ws_api_->stop();
    }
    if (user_ws_api_) {
        user_ws_api_->stop();
    }

    // Stop REST API
    if (rest_api_) {
        rest_api_->stop();
    }
}

bool BinanceUsdtGateway::is_connected() const {
    return connected_;
}

//=============================================================================
// Market Data - Synchronous
//=============================================================================

std::optional<TickData> BinanceUsdtGateway::get_tick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(ticks_mutex_);
    auto it = last_ticks_.find(symbol);
    if (it != last_ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<BarData> BinanceUsdtGateway::get_bars(const HistoryRequest& req) {
    // TODO: Implement futures kline query
    return {};
}

double BinanceUsdtGateway::get_price(const std::string& symbol) {
    auto tick = get_tick(symbol);
    return tick ? tick->last_price : 0.0;
}

std::optional<ContractData> BinanceUsdtGateway::get_contract(const std::string& symbol) {
    auto it = contracts_.find(symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Market Data - Asynchronous
//=============================================================================

void BinanceUsdtGateway::subscribe_tick(const SubscribeRequest& req) {
    if (ws_api_ && connected_) {
        ws_api_->subscribe_ticker(req.symbol);
        ws_api_->subscribe_depth(req.symbol, 5);
    }
}

void BinanceUsdtGateway::subscribe_bar(const std::string& symbol, Interval interval) {
    if (ws_api_ && connected_) {
        // Convert interval to Binance format
        std::string binance_interval;
        switch (interval) {
            case Interval::MINUTE_1: binance_interval = "1m"; break;
            case Interval::MINUTE_3: binance_interval = "3m"; break;
            case Interval::MINUTE_5: binance_interval = "5m"; break;
            case Interval::MINUTE_15: binance_interval = "15m"; break;
            case Interval::MINUTE_30: binance_interval = "30m"; break;
            case Interval::HOUR_1: binance_interval = "1h"; break;
            case Interval::HOUR_2: binance_interval = "2h"; break;
            case Interval::HOUR_4: binance_interval = "4h"; break;
            case Interval::HOUR_6: binance_interval = "6h"; break;
            case Interval::HOUR_12: binance_interval = "12h"; break;
            case Interval::DAILY: binance_interval = "1d"; break;
            case Interval::WEEKLY: binance_interval = "1w"; break;
            default: binance_interval = "1m"; break;
        }
        ws_api_->subscribe_kline(symbol, binance_interval);
    }
}

void BinanceUsdtGateway::unsubscribe_tick(const std::string& symbol) {
    if (ws_api_ && connected_) {
        ws_api_->unsubscribe(symbol);
    }
}

//=============================================================================
// Order Management
//=============================================================================

std::string BinanceUsdtGateway::send_order(const OrderRequest& req) {
    if (!connected_ || !rest_api_) {
        return "";
    }

    // Generate order ID
    std::string orderid = rest_api_->generate_order_id();

    // Send order via REST API
    auto order = rest_api_->send_order(req, orderid);
    if (order) {
        return order->vt_orderid();
    }

    return "";
}

bool BinanceUsdtGateway::cancel_order(const CancelRequest& req) {
    if (!connected_ || !rest_api_) {
        return false;
    }

    auto order = rest_api_->cancel_order(req.symbol, req.orderid);
    return order.has_value();
}

void BinanceUsdtGateway::cancel_all_orders(const std::string& symbol) {
    if (!connected_ || !rest_api_) {
        return;
    }

    // TODO: Implement cancel all via REST API
}

std::optional<OrderData> BinanceUsdtGateway::query_order(const OrderQueryRequest& req) {
    if (!connected_ || !rest_api_) {
        return std::nullopt;
    }

    // TODO: Query order via REST API
    return std::nullopt;
}

//=============================================================================
// Account Management
//=============================================================================

std::vector<PositionData> BinanceUsdtGateway::query_positions(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(positions_mutex_);

    if (symbol.empty()) {
        std::vector<PositionData> result;
        for (const auto& [_, pos] : positions_) {
            result.push_back(pos);
        }
        return result;
    }

    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return { it->second };
    }
    return {};
}

std::optional<AccountData> BinanceUsdtGateway::query_account() {
    if (!connected_ || !rest_api_) {
        return std::nullopt;
    }

    auto accounts = rest_api_->query_account();
    if (!accounts.empty()) {
        return accounts[0];
    }
    return std::nullopt;
}

std::vector<OrderData> BinanceUsdtGateway::query_open_orders(const std::string& symbol) {
    if (!connected_ || !rest_api_) {
        return {};
    }

    // TODO: Query open orders via REST API
    return {};
}

//=============================================================================
// Futures-specific Methods
//=============================================================================

bool BinanceUsdtGateway::set_leverage(const std::string& symbol, int leverage) {
    if (!connected_ || !rest_api_) {
        return false;
    }

    // TODO: Set leverage via REST API
    return true;
}

std::vector<BinanceUsdtGateway::FundingRate> BinanceUsdtGateway::query_funding_rate() {
    if (!connected_ || !rest_api_) {
        return {};
    }

    // TODO: Query funding rate via REST API
    return {};
}

std::optional<PositionData> BinanceUsdtGateway::get_position(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

//=============================================================================
// Utility
//=============================================================================

int64_t BinanceUsdtGateway::get_server_time() {
    // TODO: Query server time via REST API
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool BinanceUsdtGateway::ping() {
    // TODO: Ping via REST API
    return connected_;
}

std::vector<ContractData> BinanceUsdtGateway::get_exchange_info() {
    // TODO: Query exchange info via REST API
    // Return some default contracts for testing
    std::vector<ContractData> contracts;

    ContractData btc;
    btc.symbol = "BTCUSDT";
    btc.exchange = Exchange::BINANCE;
    btc.name = "BTC/USDT";
    btc.product = Product::FUTURES;
    btc.pricetick = 0.1;
    btc.size = 1.0;
    btc.min_volume = 0.001;
    btc.min_notional = 5.0;
    btc.net_position = true;
    btc.history_data = true;
    contracts.push_back(btc);

    ContractData eth;
    eth.symbol = "ETHUSDT";
    eth.exchange = Exchange::BINANCE;
    eth.name = "ETH/USDT";
    eth.product = Product::FUTURES;
    eth.pricetick = 0.01;
    eth.size = 1.0;
    eth.min_volume = 0.01;
    eth.min_notional = 5.0;
    eth.net_position = true;
    eth.history_data = true;
    contracts.push_back(eth);

    return contracts;
}

//=============================================================================
// WebSocket Data Processing
//=============================================================================

void BinanceUsdtGateway::process_order_update(const OrderData& order) {
    // Update stored order
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = orders_.find(order.orderid);
        if (it != orders_.end()) {
            // Calculate traded volume
            double traded = order.traded - it->second.traded;

            // Generate trade if volume increased
            if (traded > 0) {
                TradeData trade;
                trade.symbol = order.symbol;
                trade.exchange = order.exchange;
                trade.orderid = order.orderid;
                trade.tradeid = order.orderid + "_" + std::to_string(static_cast<int>(order.traded * 1000));
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

    // Notify via callback
    if (order_callback_) {
        order_callback_(order);
    }
}

void BinanceUsdtGateway::process_account_update(const AccountData& account) {
    if (account_callback_) {
        account_callback_(account);
    }
}

void BinanceUsdtGateway::process_position_update(const PositionData& position) {
    {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        positions_[position.symbol] = position;
    }

    if (position_callback_) {
        position_callback_(position);
    }
}

//=============================================================================
// Internal Methods
//=============================================================================

void BinanceUsdtGateway::init_rest_api() {
    // Create REST API instance
    rest_api_ = std::make_unique<BinanceUsdtRestApi>(this);

    // Set callbacks
    rest_api_->set_account_callback([this](const AccountData& acc) {
        process_account_update(acc);
    });

    rest_api_->set_position_callback([this](const PositionData& pos) {
        process_position_update(pos);
    });

    rest_api_->set_order_callback([this](const OrderData& order) {
        process_order_update(order);
    });

    rest_api_->set_contract_callback([this](const ContractData& contract) {
        contracts_[contract.symbol] = contract;
        if (contract_callback_) {
            contract_callback_(contract);
        }
    });

    rest_api_->set_log_callback([this](const std::string& msg) {
        // Log to console for now
        std::cout << "[" << gateway_name_ << "] " << msg << std::endl;
    });
}

void BinanceUsdtGateway::init_ws_api() {
    // Create Market Data WebSocket API
    ws_api_ = std::make_unique<BinanceUsdtWsApi>(this);

    ws_api_->set_tick_callback([this](const TickData& tick) {
        {
            std::lock_guard<std::mutex> lock(ticks_mutex_);
            last_ticks_[tick.symbol] = tick;
        }
        if (tick_callback_) {
            tick_callback_(tick);
        }
    });

    ws_api_->set_bar_callback([this](const BarData& bar) {
        if (bar_callback_) {
            bar_callback_(bar);
        }
    });

    ws_api_->set_log_callback([this](const std::string& msg) {
        std::cout << "[" << gateway_name_ << " WS] " << msg << std::endl;
    });

    // Create User Data WebSocket API
    user_ws_api_ = std::make_unique<BinanceUsdtUserWsApi>(this);

    user_ws_api_->set_account_callback([this](const AccountData& acc) {
        process_account_update(acc);
    });

    user_ws_api_->set_position_callback([this](const PositionData& pos) {
        process_position_update(pos);
    });

    user_ws_api_->set_order_callback([this](const OrderData& order) {
        process_order_update(order);
    });

    user_ws_api_->set_log_callback([this](const std::string& msg) {
        std::cout << "[" << gateway_name_ << " UserWS] " << msg << std::endl;
    });
}

std::string BinanceUsdtGateway::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();

    std::ostringstream oss;
    oss << "x-cLbi5uMH" << timestamp << order_count_;
    return oss.str();
}

// Register gateway
namespace {
    static bool registered = []() {
        ExchangeFactory::register_exchange("binance_usdt", []() {
            return std::make_unique<BinanceUsdtGateway>();
        });
        return true;
    }();
}

} // namespace bitquant
