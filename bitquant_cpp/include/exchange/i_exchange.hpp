/**
 * @file i_exchange.hpp
 * @brief Unified exchange interface for trading
 *
 * Based on howtrader's BaseGateway design:
 * - Market data: tick/bar subscription
 * - Order management: send/cancel/query orders
 * - Account management: position/balance query
 * - WebSocket support for real-time data
 *
 * All exchange adapters must implement this interface.
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace bitquant {

//=============================================================================
// Gateway Configuration
//=============================================================================

/**
 * @brief Gateway connection configuration
 */
struct GatewayConfig {
    std::string name;
    std::string api_key;
    std::string api_secret;
    std::string proxy_host;
    int proxy_port = 0;
    bool testnet = false;
    size_t timeout_ms = 10000;
};

//=============================================================================
// Callback Types
//=============================================================================

/// Tick data callback
using TickCallback = std::function<void(const TickData&)>;

/// Bar data callback
using BarCallback = std::function<void(const BarData&)>;

/// Order update callback
using OrderCallback = std::function<void(const OrderData&)>;

/// Trade update callback
using TradeCallback = std::function<void(const TradeData&)>;

/// Position update callback
using PositionCallback = std::function<void(const PositionData&)>;

/// Account update callback
using AccountCallback = std::function<void(const AccountData&)>;

/// Contract info callback
using ContractCallback = std::function<void(const ContractData&)>;

/// Error callback
using ErrorCallback = std::function<void(const std::string&)>;

//=============================================================================
// Exchange Capability Flags
//=============================================================================

/**
 * @brief Exchange capabilities
 */
struct ExchangeCapabilities {
    bool market_data = true;        // Market data support
    bool trading = true;            // Trading support
    bool stop_order = false;        // Server-side stop orders
    bool history_data = true;       // Historical data query
    bool websocket = true;          // WebSocket support
    bool margin = false;            // Margin trading
    bool futures = false;           // Futures trading
    bool options = false;           // Options trading
};

//=============================================================================
// IExchange Interface
//=============================================================================

/**
 * @brief Abstract exchange interface
 *
 * All exchange adapters must implement this interface.
 * Provides unified API for:
 * - Connection management
 * - Market data subscription
 * - Order management
 * - Account/position query
 *
 * Usage:
 * @code
 * auto exchange = ExchangeFactory::create("binance");
 * exchange->connect(config);
 * exchange->subscribe_tick("BTCUSDT", on_tick);
 * exchange->send_order(order_req);
 * @endcode
 */
class IExchange {
public:
    virtual ~IExchange() = default;

    //=========================================================================
    // Identification
    //=========================================================================

    /**
     * @brief Get exchange name
     */
    virtual std::string name() const = 0;

    /**
     * @brief Get exchange enum
     */
    virtual Exchange exchange() const = 0;

    /**
     * @brief Get gateway name (unique identifier)
     */
    virtual std::string gateway_name() const = 0;

    /**
     * @brief Get supported exchanges
     */
    virtual std::vector<Exchange> supported_exchanges() const = 0;

    /**
     * @brief Get capabilities
     */
    virtual ExchangeCapabilities capabilities() const {
        return ExchangeCapabilities{};
    }

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to exchange
     * @param config JSON configuration string
     * @return true on success
     */
    virtual bool connect(const std::string& config) = 0;

    /**
     * @brief Close connection
     */
    virtual void close() = 0;

    /**
     * @brief Check if connected
     */
    virtual bool is_connected() const = 0;

    //=========================================================================
    // Market Data - Synchronous
    //=========================================================================

    /**
     * @brief Get current tick data
     * @param symbol Trading pair
     * @return Current tick data
     */
    virtual std::optional<TickData> get_tick(const std::string& symbol) {
        (void)symbol;
        return std::nullopt;
    }

    /**
     * @brief Get historical bar data
     * @param req History request
     * @return Vector of bar data
     */
    virtual std::vector<BarData> get_bars(const HistoryRequest& req) {
        (void)req;
        return {};
    }

    /**
     * @brief Get current price
     * @param symbol Trading pair
     * @return Current price, 0 on error
     */
    virtual double get_price(const std::string& symbol) {
        (void)symbol;
        return 0.0;
    }

    /**
     * @brief Get contract info
     * @param symbol Trading pair
     * @return Contract data
     */
    virtual std::optional<ContractData> get_contract(const std::string& symbol) {
        (void)symbol;
        return std::nullopt;
    }

    //=========================================================================
    // Market Data - Asynchronous (WebSocket)
    //=========================================================================

    /**
     * @brief Subscribe tick data updates
     * @param req Subscribe request
     */
    virtual void subscribe_tick(const SubscribeRequest& req) {
        (void)req;
    }

    /**
     * @brief Subscribe bar data updates
     * @param symbol Trading pair
     * @param interval Bar interval
     */
    virtual void subscribe_bar(const std::string& symbol, Interval interval) {
        (void)symbol;
        (void)interval;
    }

    /**
     * @brief Unsubscribe tick data
     */
    virtual void unsubscribe_tick(const std::string& symbol) {
        (void)symbol;
    }

    //=========================================================================
    // Order Management
    //=========================================================================

    /**
     * @brief Send order
     * @param req Order request
     * @return Order ID, empty on failure
     */
    virtual std::string send_order(const OrderRequest& req) = 0;

    /**
     * @brief Cancel order
     * @param req Cancel request
     * @return true on success
     */
    virtual bool cancel_order(const CancelRequest& req) = 0;

    /**
     * @brief Cancel all orders for symbol
     */
    virtual void cancel_all_orders(const std::string& symbol = "") {
        (void)symbol;
    }

    /**
     * @brief Query order status
     * @param orderid Order ID
     * @return Order data
     */
    virtual std::optional<OrderData> query_order(const std::string& orderid) {
        (void)orderid;
        return std::nullopt;
    }

    /**
     * @brief Query specific order status
     * @param req Order query request
     */
    virtual std::optional<OrderData> query_order(const OrderQueryRequest& req) {
        (void)req;
        return std::nullopt;
    }

    //=========================================================================
    // Account Management
    //=========================================================================

    /**
     * @brief Query position
     * @param symbol Trading pair (empty for all)
     * @return Position data
     */
    virtual std::vector<PositionData> query_positions(const std::string& symbol = "") {
        (void)symbol;
        return {};
    }

    /**
     * @brief Query account balance
     * @return Account data
     */
    virtual std::optional<AccountData> query_account() {
        return std::nullopt;
    }

    /**
     * @brief Query all open orders
     */
    virtual std::vector<OrderData> query_open_orders(const std::string& symbol = "") {
        (void)symbol;
        return {};
    }

    //=========================================================================
    // Callback Registration
    //=========================================================================

    /**
     * @brief Register tick callback
     */
    virtual void on_tick(TickCallback callback) {
        tick_callback_ = std::move(callback);
    }

    /**
     * @brief Register bar callback
     */
    virtual void on_bar(BarCallback callback) {
        bar_callback_ = std::move(callback);
    }

    /**
     * @brief Register order callback
     */
    virtual void on_order(OrderCallback callback) {
        order_callback_ = std::move(callback);
    }

    /**
     * @brief Register trade callback
     */
    virtual void on_trade(TradeCallback callback) {
        trade_callback_ = std::move(callback);
    }

    /**
     * @brief Register position callback
     */
    virtual void on_position(PositionCallback callback) {
        position_callback_ = std::move(callback);
    }

    /**
     * @brief Register account callback
     */
    virtual void on_account(AccountCallback callback) {
        account_callback_ = std::move(callback);
    }

    /**
     * @brief Register error callback
     */
    virtual void on_error(ErrorCallback callback) {
        error_callback_ = std::move(callback);
    }

protected:
    //=========================================================================
    // Protected callback storage
    //=========================================================================

    TickCallback tick_callback_;
    BarCallback bar_callback_;
    OrderCallback order_callback_;
    TradeCallback trade_callback_;
    PositionCallback position_callback_;
    AccountCallback account_callback_;
    ErrorCallback error_callback_;
};

//=============================================================================
// Exchange Factory
//=============================================================================

/**
 * @brief Factory for creating exchange instances
 *
 * Usage:
 * @code
 * auto binance = ExchangeFactory::create("binance");
 * auto okx = ExchangeFactory::create("okx");
 * @endcode
 */
class ExchangeFactory {
public:
    using Creator = std::function<std::unique_ptr<IExchange>()>;

    /**
     * @brief Register exchange creator
     * @param name Exchange name
     * @param creator Factory function
     */
    static void register_exchange(const std::string& name, Creator creator);

    /**
     * @brief Create exchange instance
     * @param name Exchange name
     * @return Exchange instance, nullptr if not found
     */
    static std::unique_ptr<IExchange> create(const std::string& name);

    /**
     * @brief Get list of registered exchanges
     */
    static std::vector<std::string> list_exchanges();

    /**
     * @brief Check if exchange is registered
     */
    static bool has_exchange(const std::string& name);
};

//=============================================================================
// Auto-registration helper
//=============================================================================

/**
 * @brief Helper for automatic exchange registration
 */
template<typename ExchangeClass>
class ExchangeRegistrar {
public:
    explicit ExchangeRegistrar(const std::string& name) {
        ExchangeFactory::register_exchange(name, []() {
            return std::make_unique<ExchangeClass>();
        });
    }
};

// Macro for easy registration
#define REGISTER_EXCHANGE(Name, Class) \
    static bitquant::ExchangeRegistrar<Class> _registrar_##Class(Name)

} // namespace bitquant
