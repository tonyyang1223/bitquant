/**
 * @file binance_usdt_rest_api.cpp
 * @brief Binance USDT-M Futures REST API implementation
 *
 * Uses binapi for HTTP requests.
 * Reference: howtrader/gateway/binance/binance_usdt_gateway.py
 */

#include "exchange/binance_usdt_rest_api.hpp"
#include "exchange/binance_usdt_gateway.hpp"
#include "utils/datetime.hpp"
#include "utils/logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace bitquant {

//=============================================================================
// Constants
//=============================================================================

// REST API endpoints
constexpr const char* F_REST_HOST = "https://fapi.binance.com";

// Endpoint paths
constexpr const char* PATH_TIME = "/fapi/v1/time";
constexpr const char* PATH_EXCHANGE_INFO = "/fapi/v1/exchangeInfo";
constexpr const char* PATH_BALANCE = "/fapi/v2/balance";
constexpr const char* PATH_POSITION_RISK = "/fapi/v2/positionRisk";
constexpr const char* PATH_ORDER = "/fapi/v1/order";
constexpr const char* PATH_OPEN_ORDERS = "/fapi/v1/openOrders";
constexpr const char* PATH_LEVERAGE = "/fapi/v1/leverage";
constexpr const char* PATH_POSITION_SIDE = "/fapi/v1/positionSide/dual";
constexpr const char* PATH_LISTEN_KEY = "/fapi/v1/listenKey";
constexpr const char* PATH_PREMIUM_INDEX = "/fapi/v1/premiumIndex";
constexpr const char* PATH_KLINES = "/fapi/v1/klines";

//=============================================================================
// Construction
//=============================================================================

BinanceUsdtRestApi::BinanceUsdtRestApi(BinanceUsdtGateway* gateway)
    : gateway_(gateway) {
    if (gateway_) {
        gateway_name_ = gateway_->gateway_name();
    }
}

//=============================================================================
// Connection
//=============================================================================

void BinanceUsdtRestApi::connect(const std::string& api_key,
                                  const std::string& api_secret,
                                  const std::string& proxy_host,
                                  int proxy_port) {
    api_key_ = api_key;
    api_secret_ = api_secret;
    proxy_host_ = proxy_host;
    proxy_port_ = proxy_port;

    // Generate connect time for order ID
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%y%m%d%H%M%S");
    connect_time_ = std::stoll(oss.str()) * 1000000;

    write_log("REST API connected");
}

void BinanceUsdtRestApi::stop() {
    write_log("REST API stopped");
}

//=============================================================================
// Public Endpoints
//=============================================================================

int64_t BinanceUsdtRestApi::query_time() {
    // For now, return local time
    // TODO: Use binapi to make actual request
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    return ms;
}

std::vector<ContractData> BinanceUsdtRestApi::query_exchange_info() {
    std::vector<ContractData> contracts;

    // Return default contracts for testing
    // TODO: Use binapi to fetch actual exchange info

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
    btc.gateway_name = gateway_name_;
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
    eth.gateway_name = gateway_name_;
    contracts.push_back(eth);

    write_log("Query exchange info: " + std::to_string(contracts.size()) + " contracts");

    for (const auto& c : contracts) {
        if (contract_callback_) {
            contract_callback_(c);
        }
    }

    return contracts;
}

std::vector<FundingRateData> BinanceUsdtRestApi::query_funding_rate(const std::string& symbol) {
    std::vector<FundingRateData> rates;

    // TODO: Use binapi to fetch actual funding rates
    // For now, return empty

    write_log("Query funding rate for: " + (symbol.empty() ? "all" : symbol));

    return rates;
}

std::vector<BarData> BinanceUsdtRestApi::query_klines(const HistoryRequest& req) {
    std::vector<BarData> bars;

    // TODO: Use binapi to fetch actual klines
    // For now, return empty

    write_log("Query klines for: " + req.symbol);

    return bars;
}

//=============================================================================
// Private Endpoints
//=============================================================================

std::vector<AccountData> BinanceUsdtRestApi::query_account() {
    std::vector<AccountData> accounts;

    // TODO: Use binapi to fetch actual account data
    // For now, return a default account

    AccountData account;
    account.accountid = "USDT";
    account.gateway_name = gateway_name_;
    account.balance = 10000.0;
    account.frozen = 0.0;
    accounts.push_back(account);

    write_log("Query account: balance=" + std::to_string(account.balance));

    for (const auto& acc : accounts) {
        if (account_callback_) {
            account_callback_(acc);
        }
    }

    return accounts;
}

std::vector<PositionData> BinanceUsdtRestApi::query_positions(const std::string& symbol) {
    std::vector<PositionData> positions;

    // TODO: Use binapi to fetch actual positions
    // For now, return empty

    write_log("Query positions for: " + (symbol.empty() ? "all" : symbol));

    for (const auto& pos : positions) {
        if (position_callback_) {
            position_callback_(pos);
        }
    }

    return positions;
}

std::vector<OrderData> BinanceUsdtRestApi::query_open_orders(const std::string& symbol) {
    std::vector<OrderData> orders;

    // TODO: Use binapi to fetch actual open orders

    write_log("Query open orders for: " + (symbol.empty() ? "all" : symbol));

    return orders;
}

std::optional<OrderData> BinanceUsdtRestApi::query_order(const std::string& symbol, const std::string& orderid) {
    // TODO: Use binapi to fetch actual order

    write_log("Query order: " + orderid + " for " + symbol);

    return std::nullopt;
}

//=============================================================================
// Trading
//=============================================================================

std::optional<OrderData> BinanceUsdtRestApi::send_order(const OrderRequest& req, const std::string& orderid) {
    // Create order data
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
    order.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // TODO: Use binapi to send actual order
    // For now, simulate successful submission

    write_log("Send order: " + orderid + " " + req.symbol + " " +
              (req.direction == Direction::LONG ? "BUY" : "SELL") + " " +
              std::to_string(req.volume) + "@" + std::to_string(req.price));

    // Simulate order accepted
    order.status = Status::NOTTRADED;

    if (order_callback_) {
        order_callback_(order);
    }

    return order;
}

std::optional<OrderData> BinanceUsdtRestApi::cancel_order(const std::string& symbol, const std::string& orderid) {
    // TODO: Use binapi to cancel actual order

    write_log("Cancel order: " + orderid + " for " + symbol);

    return std::nullopt;
}

bool BinanceUsdtRestApi::cancel_all_orders(const std::string& symbol) {
    // TODO: Use binapi to cancel all orders

    write_log("Cancel all orders for: " + symbol);

    return true;
}

//=============================================================================
// Futures-specific
//=============================================================================

bool BinanceUsdtRestApi::set_leverage(const std::string& symbol, int leverage) {
    // TODO: Use binapi to set leverage

    write_log("Set leverage: " + symbol + " -> " + std::to_string(leverage));

    return true;
}

bool BinanceUsdtRestApi::set_position_side(bool dual_side_position) {
    // TODO: Use binapi to set position side

    write_log("Set position side: " + std::string(dual_side_position ? "hedge" : "one-way"));

    return true;
}

bool BinanceUsdtRestApi::query_position_side() {
    // TODO: Use binapi to query position side

    return false;  // Default to one-way
}

//=============================================================================
// User Stream
//=============================================================================

std::string BinanceUsdtRestApi::start_user_stream() {
    // TODO: Use binapi to get listen key

    write_log("Start user stream");

    return "";
}

bool BinanceUsdtRestApi::keep_user_stream(const std::string& listen_key) {
    // TODO: Use binapi to keep alive

    return true;
}

bool BinanceUsdtRestApi::close_user_stream(const std::string& listen_key) {
    // TODO: Use binapi to close

    return true;
}

//=============================================================================
// Order ID Generation
//=============================================================================

std::string BinanceUsdtRestApi::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;

    std::ostringstream oss;
    oss << "x-cLbi5uMH" << (connect_time_ + order_count_);
    return oss.str();
}

//=============================================================================
// Time Offset
//=============================================================================

void BinanceUsdtRestApi::update_time_offset(int64_t server_time) {
    auto local_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    time_offset_ = local_time - server_time;
}

//=============================================================================
// Internal Methods
//=============================================================================

std::string BinanceUsdtRestApi::sign_request(const std::string& path,
                                              std::unordered_map<std::string, std::string>& params) {
    // Add timestamp
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Adjust for time offset
    if (time_offset_ > 0) {
        now -= std::abs(time_offset_);
    } else if (time_offset_ < 0) {
        now += std::abs(time_offset_);
    }

    params["timestamp"] = std::to_string(now);
    params["recvWindow"] = std::to_string(recv_window_);

    // Build query string
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) oss << "&";
        oss << key << "=" << value;
        first = false;
    }
    std::string query = oss.str();

    // Generate HMAC-SHA256 signature
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;

    HMAC(EVP_sha256(),
         api_secret_.c_str(), api_secret_.length(),
         reinterpret_cast<const unsigned char*>(query.c_str()), query.length(),
         digest, &digest_len);

    // Convert to hex string
    std::ostringstream hex_oss;
    for (unsigned int i = 0; i < digest_len; i++) {
        hex_oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    std::string signature = hex_oss.str();

    return path + "?" + query + "&signature=" + signature;
}

std::string BinanceUsdtRestApi::make_request(const std::string& method,
                                              const std::string& path,
                                              const std::unordered_map<std::string, std::string>& params,
                                              bool signed_req) {
    // TODO: Use binapi or libcurl to make actual HTTP request
    return "";
}

void BinanceUsdtRestApi::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    }
}

void BinanceUsdtRestApi::on_error(const std::string& msg) {
    write_log("ERROR: " + msg);
    if (error_callback_) {
        error_callback_(msg);
    }
}

} // namespace bitquant