/**
 * @file binance_usdt_rest_api.hpp
 * @brief Binance USDT-M Futures REST API
 *
 * Implements REST API for Binance USDT-M perpetual futures.
 * Uses /fapi/* endpoints (futures API).
 *
 * Reference: howtrader/gateway/binance/binance_usdt_gateway.py
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace bitquant {

// Forward declarations
class BinanceUsdtGateway;

//=============================================================================
// Order Type Mapping (Futures)
//=============================================================================

/**
 * @brief Convert OrderType to Binance futures order type and timeInForce
 * @return pair<orderType, timeInForce>
 */
inline std::pair<std::string, std::string> order_type_to_binance_futures(OrderType type) {
    switch (type) {
        case OrderType::LIMIT: return {"LIMIT", "GTC"};
        case OrderType::TAKER: return {"MARKET", "GTC"};
        case OrderType::FAK:   return {"LIMIT", "IOC"};
        case OrderType::FOK:   return {"LIMIT", "FOK"};
        case OrderType::MAKER: return {"LIMIT", "GTX"};
        default: return {"LIMIT", "GTC"};
    }
}

/**
 * @brief Convert Binance order type to OrderType
 */
inline OrderType binance_futures_to_order_type(const std::string& order_type, const std::string& time_in_force) {
    if (order_type == "MARKET") return OrderType::TAKER;
    if (time_in_force == "GTC") return OrderType::LIMIT;
    if (time_in_force == "IOC") return OrderType::FAK;
    if (time_in_force == "FOK") return OrderType::FOK;
    if (time_in_force == "GTX") return OrderType::MAKER;
    return OrderType::LIMIT;
}

//=============================================================================
// Funding Rate Data
//=============================================================================

/**
 * @brief Funding rate data structure
 */
struct FundingRateData {
    std::string symbol;
    Exchange exchange = Exchange::BINANCE;
    double funding_rate = 0.0;
    int64_t next_funding_time = 0;
    int64_t last_funding_time = 0;
    std::string gateway_name;
};

//=============================================================================
// BinanceUsdtRestApi
//=============================================================================

/**
 * @brief Binance USDT-M Futures REST API client
 *
 * Implements all REST endpoints for Binance USDT-M futures:
 * - Account: /fapi/v2/balance
 * - Position: /fapi/v2/positionRisk
 * - Order: /fapi/v1/order
 * - Contract: /fapi/v1/exchangeInfo
 * - Funding: /fapi/v1/premiumIndex
 * - Kline: /fapi/v1/klines
 */
class BinanceUsdtRestApi {
public:
    // Callback types
    using AccountCallback = std::function<void(const AccountData&)>;
    using PositionCallback = std::function<void(const PositionData&)>;
    using OrderCallback = std::function<void(const OrderData&)>;
    using ContractCallback = std::function<void(const ContractData&)>;
    using FundingRateCallback = std::function<void(const FundingRateData&)>;
    using LogCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    /**
     * @brief Constructor
     * @param gateway Parent gateway
     */
    explicit BinanceUsdtRestApi(BinanceUsdtGateway* gateway = nullptr);

    //=========================================================================
    // Connection
    //=========================================================================

    /**
     * @brief Initialize REST API connection
     * @param api_key API key
     * @param api_secret API secret
     * @param proxy_host Proxy host (optional)
     * @param proxy_port Proxy port (optional)
     */
    void connect(const std::string& api_key,
                 const std::string& api_secret,
                 const std::string& proxy_host = "",
                 int proxy_port = 0);

    /**
     * @brief Stop REST API
     */
    void stop();

    //=========================================================================
    // Public Endpoints (No Authentication)
    //=========================================================================

    /**
     * @brief Query server time
     * @return Server time in milliseconds, or 0 on error
     */
    int64_t query_time();

    /**
     * @brief Query exchange info (contracts)
     * @return List of contracts
     */
    std::vector<ContractData> query_exchange_info();

    /**
     * @brief Query funding rate
     * @param symbol Symbol (optional, empty for all)
     * @return List of funding rates
     */
    std::vector<FundingRateData> query_funding_rate(const std::string& symbol = "");

    /**
     * @brief Query kline/candlestick data
     * @param req History request
     * @return List of bars
     */
    std::vector<BarData> query_klines(const HistoryRequest& req);

    //=========================================================================
    // Private Endpoints (Authentication Required)
    //=========================================================================

    /**
     * @brief Query account balance
     * @return List of account data
     */
    std::vector<AccountData> query_account();

    /**
     * @brief Query positions
     * @param symbol Symbol (optional, empty for all)
     * @return List of positions
     */
    std::vector<PositionData> query_positions(const std::string& symbol = "");

    /**
     * @brief Query open orders
     * @param symbol Symbol (optional, empty for all)
     * @return List of orders
     */
    std::vector<OrderData> query_open_orders(const std::string& symbol = "");

    /**
     * @brief Query specific order
     * @param symbol Symbol
     * @param orderid Order ID (client order ID)
     * @return Order data, or nullopt if not found
     */
    std::optional<OrderData> query_order(const std::string& symbol, const std::string& orderid);

    //=========================================================================
    // Trading
    //=========================================================================

    /**
     * @brief Send order
     * @param req Order request
     * @param orderid Client order ID
     * @return Order data if successful, nullopt otherwise
     */
    std::optional<OrderData> send_order(const OrderRequest& req, const std::string& orderid);

    /**
     * @brief Cancel order
     * @param symbol Symbol
     * @param orderid Client order ID
     * @return Order data if successful, nullopt otherwise
     */
    std::optional<OrderData> cancel_order(const std::string& symbol, const std::string& orderid);

    /**
     * @brief Cancel all open orders
     * @param symbol Symbol
     * @return true if successful
     */
    bool cancel_all_orders(const std::string& symbol);

    //=========================================================================
    // Futures-specific
    //=========================================================================

    /**
     * @brief Set leverage for symbol
     * @param symbol Symbol
     * @param leverage Leverage (1-125)
     * @return true if successful
     */
    bool set_leverage(const std::string& symbol, int leverage);

    /**
     * @brief Set position mode (one-way or hedge)
     * @param dual_side_position true for hedge mode, false for one-way
     * @return true if successful
     */
    bool set_position_side(bool dual_side_position);

    /**
     * @brief Query position mode
     * @return true if hedge mode, false if one-way
     */
    bool query_position_side();

    //=========================================================================
    // User Stream
    //=========================================================================

    /**
     * @brief Start user stream (get listen key)
     * @return Listen key, or empty string on error
     */
    std::string start_user_stream();

    /**
     * @brief Keep user stream alive
     * @param listen_key Listen key
     * @return true if successful
     */
    bool keep_user_stream(const std::string& listen_key);

    /**
     * @brief Close user stream
     * @param listen_key Listen key
     * @return true if successful
     */
    bool close_user_stream(const std::string& listen_key);

    //=========================================================================
    // Order ID Generation
    //=========================================================================

    /**
     * @brief Generate unique order ID
     * @return Client order ID
     */
    std::string generate_order_id();

    //=========================================================================
    // Callbacks
    //=========================================================================

    void set_account_callback(AccountCallback cb) { account_callback_ = std::move(cb); }
    void set_position_callback(PositionCallback cb) { position_callback_ = std::move(cb); }
    void set_order_callback(OrderCallback cb) { order_callback_ = std::move(cb); }
    void set_contract_callback(ContractCallback cb) { contract_callback_ = std::move(cb); }
    void set_funding_rate_callback(FundingRateCallback cb) { funding_rate_callback_ = std::move(cb); }
    void set_log_callback(LogCallback cb) { log_callback_ = std::move(cb); }
    void set_error_callback(ErrorCallback cb) { error_callback_ = std::move(cb); }

    //=========================================================================
    // Time Offset
    //=========================================================================

    /**
     * @brief Get time offset (local - server)
     */
    int64_t time_offset() const { return time_offset_; }

    /**
     * @brief Update time offset from server time
     */
    void update_time_offset(int64_t server_time);

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    std::string sign_request(const std::string& path, std::unordered_map<std::string, std::string>& params);
    std::string make_request(const std::string& method, const std::string& path,
                            const std::unordered_map<std::string, std::string>& params = {},
                            bool signed_req = false);
    void write_log(const std::string& msg);
    void on_error(const std::string& msg);

    //=========================================================================
    // Members
    //=========================================================================

    BinanceUsdtGateway* gateway_;
    std::string gateway_name_ = "BINANCE_USDT";

    // API credentials
    std::string api_key_;
    std::string api_secret_;
    std::string proxy_host_;
    int proxy_port_ = 0;

    // Time offset for timestamp
    int64_t time_offset_ = 0;
    int recv_window_ = 5000;

    // Order ID generation
    int64_t connect_time_ = 0;
    int order_count_ = 0;
    std::mutex order_mutex_;

    // User stream
    std::string listen_key_;
    int keep_alive_count_ = 0;

    // Callbacks
    AccountCallback account_callback_;
    PositionCallback position_callback_;
    OrderCallback order_callback_;
    ContractCallback contract_callback_;
    FundingRateCallback funding_rate_callback_;
    LogCallback log_callback_;
    ErrorCallback error_callback_;
};

} // namespace bitquant