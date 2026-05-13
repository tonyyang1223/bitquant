/**
 * @file binance_usdt_gateway.hpp
 * @brief Binance USDT-M Futures Gateway
 *
 * Implements IExchange interface for Binance USDT-M perpetual futures trading.
 * Based on howtrader's BinanceUsdtGateway design.
 *
 * Features:
 * - USDT-margined perpetual futures
 * - One-way position mode
 * - REST API for trading and queries
 * - WebSocket for real-time data
 */

#pragma once

#include "exchange/i_exchange.hpp"
#include "exchange/binance_usdt_rest_api.hpp"
#include "exchange/binance_usdt_ws_api.hpp"
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
 * @brief Binance USDT-M Futures Gateway
 *
 * Implements IExchange interface for Binance USDT-M perpetual futures.
 * Key differences from Spot:
 * - Uses /fapi/* endpoints (futures API)
 * - Supports leverage and margin
 * - Position-based trading
 * - Funding rate
 */
class BinanceUsdtGateway : public IExchange {
public:
    BinanceUsdtGateway();
    explicit BinanceUsdtGateway(const std::string& gateway_name);
    ~BinanceUsdtGateway() override;

    //=========================================================================
    // IExchange Interface
    //=========================================================================

    std::string name() const override { return "BinanceUsdt"; }
    Exchange exchange() const override { return Exchange::BINANCE; }
    std::string gateway_name() const override { return gateway_name_; }

    std::vector<Exchange> supported_exchanges() const override {
        return { Exchange::BINANCE };
    }

    ExchangeCapabilities capabilities() const override {
        return {
            .market_data = true,
            .trading = true,
            .stop_order = true,      // Futures support stop orders
            .history_data = true,
            .websocket = true,
            .margin = true,          // Margin trading
            .futures = true,         // Futures trading
            .options = false
        };
    }

    //=========================================================================
    // Connection Management
    //=========================================================================

    bool connect(const std::string& config) override;
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
    // Futures-specific Methods
    //=========================================================================

    /**
     * @brief Set leverage for symbol
     */
    bool set_leverage(const std::string& symbol, int leverage);

    /**
     * @brief Query funding rate
     */
    struct FundingRate {
        std::string symbol;
        double funding_rate = 0.0;
        int64_t next_funding_time = 0;
    };
    std::vector<FundingRate> query_funding_rate();

    /**
     * @brief Get position for symbol
     */
    std::optional<PositionData> get_position(const std::string& symbol);

    //=========================================================================
    // Utility
    //=========================================================================

    int64_t get_server_time();
    bool ping();
    std::vector<ContractData> get_exchange_info();
    const std::string& last_error() const { return last_error_; }

    //=========================================================================
    // WebSocket Data Processing
    //=========================================================================

    void process_order_update(const OrderData& order);
    void process_account_update(const AccountData& account);
    void process_position_update(const PositionData& position);

private:
    std::string generate_order_id();
    void init_rest_api();
    void init_ws_api();

    //=========================================================================
    // Members
    //=========================================================================

    // REST and WebSocket APIs
    std::unique_ptr<BinanceUsdtRestApi> rest_api_;
    std::unique_ptr<BinanceUsdtWsApi> ws_api_;
    std::unique_ptr<BinanceUsdtUserWsApi> user_ws_api_;
    std::string gateway_name_ = "BINANCE_USDT";
    std::atomic<bool> connected_{false};
    std::string last_error_;

    // Order tracking
    std::unordered_map<std::string, OrderData> orders_;
    std::mutex orders_mutex_;

    // Position tracking
    std::unordered_map<std::string, PositionData> positions_;
    std::mutex positions_mutex_;

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
};

} // namespace bitquant
