/**
 * @file okx_gateway.hpp
 * @brief OKX Exchange Gateway
 *
 * Implements IExchange interface for OKX trading.
 * Based on howtrader's OkxGateway design.
 *
 * Features:
 * - Spot, Margin, and Futures trading
 * - REST API and WebSocket support
 * - Single currency margin mode
 * - Cross margin mode
 * - One-way position mode
 */

#pragma once

#include "exchange/i_exchange.hpp"
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
 * @brief OKX Gateway
 *
 * Implements IExchange interface for OKX exchange.
 * Supports Spot, Swap (perpetual), and Futures trading.
 */
class OkxGateway : public IExchange {
public:
    OkxGateway();
    explicit OkxGateway(const std::string& gateway_name);
    ~OkxGateway() override;

    //=========================================================================
    // IExchange Interface
    //=========================================================================

    std::string name() const override { return "OKX"; }
    Exchange exchange() const override { return Exchange::OKX; }
    std::string gateway_name() const override { return gateway_name_; }

    std::vector<Exchange> supported_exchanges() const override {
        return { Exchange::OKX };
    }

    ExchangeCapabilities capabilities() const override {
        return {
            .market_data = true,
            .trading = true,
            .stop_order = true,
            .history_data = true,
            .websocket = true,
            .margin = true,
            .futures = true,
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
    // Utility
    //=========================================================================

    int64_t get_server_time();
    bool ping();
    std::vector<ContractData> get_instruments(const std::string& inst_type = "SPOT");
    const std::string& last_error() const { return last_error_; }

    //=========================================================================
    // WebSocket Data Processing
    //=========================================================================

    void process_order_update(const OrderData& order);
    void process_account_update(const AccountData& account);
    void process_position_update(const PositionData& position);
    void process_tick_update(const TickData& tick);

private:
    std::string generate_order_id();
    std::string generate_signature(const std::string& timestamp,
                                    const std::string& method,
                                    const std::string& request_path,
                                    const std::string& body);

    //=========================================================================
    // Members
    //=========================================================================

    std::string gateway_name_ = "OKX";
    std::atomic<bool> connected_{false};
    std::string last_error_;

    // API credentials
    std::string api_key_;
    std::string api_secret_;
    std::string passphrase_;
    bool testnet_ = false;

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

    // Tick cache
    std::unordered_map<std::string, TickData> last_ticks_;
    std::mutex ticks_mutex_;

    // WebSocket thread
    std::thread ws_thread_;
    std::atomic<bool> ws_running_{false};
};

} // namespace bitquant
