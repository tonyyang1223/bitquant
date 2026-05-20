/**
 * @file binance_spot_gateway.hpp
 * @brief Binance Spot Gateway
 *
 * Implements IExchange interface for Binance Spot trading.
 * Coordinates REST API and WebSocket connections.
 *
 * Following Python howtrader design:
 * - REST API for trading and queries
 * - Trade WebSocket for user data (order updates)
 * - Data WebSocket for market data (tick/bar)
 */

#pragma once

#include "exchange/i_exchange.hpp"
#include "exchange/binance_spot_rest_api.hpp"
#include "exchange/binance_spot_ws_api.hpp"
#include "core/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace bitquant {

/**
 * @brief Binance Spot Gateway
 *
 * Implements IExchange interface for Binance Spot trading.
 * Coordinates REST API and WebSocket connections.
 *
 * Following Python howtrader design:
 * - REST API for trading and queries
 * - Trade WebSocket for user data (order updates)
 * - Data WebSocket for market data (tick/bar)
 */
class BinanceSpotGateway : public IExchange {
public:
    BinanceSpotGateway();
    explicit BinanceSpotGateway(const std::string& gateway_name);
    ~BinanceSpotGateway() override;

    //=========================================================================
    // IExchange Interface
    //=========================================================================

    std::string name() const override { return "BinanceSpot"; }
    Exchange exchange() const override { return Exchange::BINANCE; }
    std::string gateway_name() const override { return gateway_name_; }

    std::vector<Exchange> supported_exchanges() const override {
        return { Exchange::BINANCE };
    }

    ExchangeCapabilities capabilities() const override {
        return {
            .market_data = true,
            .trading = true,
            .stop_order = false,
            .history_data = true,
            .websocket = true,
            .margin = false,
            .futures = false,
            .options = false
        };
    }

    //=========================================================================
    // Connection Management
    //=========================================================================

    bool connect(const std::string& config) override;
    bool connect(const GatewayConfig& config);
    void close() override;
    bool is_connected() const override;

    //=========================================================================
    // Market Data - Synchronous
    //=========================================================================

    std::optional<TickData> get_tick(const std::string& symbol) override;
    std::vector<BarData> get_bars(const HistoryRequest& req) override;
    double get_price(const std::string& symbol) override;
    std::optional<ContractData> get_contract(const std::string& symbol) override;

    //=========================================================================
    // Market Data - Asynchronous (WebSocket)
    //=========================================================================

    void subscribe_tick(const SubscribeRequest& req) override;
    void subscribe_bar(const std::string& symbol, Interval interval) override;
    void unsubscribe_tick(const std::string& symbol) override;

    //=========================================================================
    // Order Management
    //=========================================================================

    std::string send_order(const OrderRequest& req) override;
    bool cancel_order(const CancelRequest& req) override;
    void cancel_all_orders(const std::string& symbol = "") override;
    std::optional<OrderData> query_order(const OrderQueryRequest& req) override;

    //=========================================================================
    // Account Management
    //=========================================================================

    std::vector<PositionData> query_positions(const std::string& symbol = "") override;
    std::optional<AccountData> query_account() override;
    std::vector<OrderData> query_open_orders(const std::string& symbol = "") override;

    //=========================================================================
    // Utility
    //=========================================================================

    /**
     * @brief Get server time
     */
    int64_t get_server_time();

    /**
     * @brief Check connectivity (ping)
     */
    bool ping();

    /**
     * @brief Get exchange info (contracts)
     */
    std::vector<ContractData> get_exchange_info();

    /**
     * @brief Query account balances
     */
    std::vector<AccountData> query_account_all();

    /**
     * @brief Start user data stream
     */
    bool start_user_stream();

    /**
     * @brief Keep user data stream alive
     */
    void keep_user_stream();

    /**
     * @brief Get last error message
     */
    const std::string& last_error() const { return last_error_; }

    //=========================================================================
    // WebSocket Data Processing (called by BinanceSpotWsApi)
    //=========================================================================

    /**
     * @brief Process order update from WebSocket
     * Called by BinanceSpotWsApi when order update received
     */
    void process_order_update(const OrderData& order);

    /**
     * @brief Process account update from WebSocket
     */
    void process_account_update(const AccountData& account);

private:
    //=========================================================================
    // Internal
    //=========================================================================

    std::string generate_order_id();

    //=========================================================================
    // Members
    //=========================================================================

    std::unique_ptr<BinanceSpotRestApi> rest_api_;
    std::unique_ptr<BinanceSpotWsApi> ws_api_;
    std::string gateway_name_ = "BINANCE_SPOT";
    std::atomic<bool> connected_{false};
    std::string last_error_;

    // Order tracking (for Trade generation)
    std::unordered_map<std::string, OrderData> orders_;
    std::mutex orders_mutex_;

    // Contract cache
    std::unordered_map<std::string, ContractData> contracts_;

    // Order ID generation
    int64_t connect_time_ = 0;
    int order_count_ = 0;
    std::mutex order_mutex_;

    // User stream
    std::string listen_key_;
    std::atomic<int> keep_alive_count_{0};

    // Tick cache
    std::unordered_map<std::string, TickData> last_ticks_;
    std::mutex ticks_mutex_;

    // Testnet mode
    bool testnet_ = false;
};

} // namespace bitquant
