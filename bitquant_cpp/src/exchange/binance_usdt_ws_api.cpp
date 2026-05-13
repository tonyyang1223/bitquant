/**
 * @file binance_usdt_ws_api.cpp
 * @brief Binance USDT-M Futures WebSocket API implementation
 *
 * Reference: howtrader/gateway/binance/binance_usdt_gateway.py
 */

#include "exchange/binance_usdt_ws_api.hpp"
#include "exchange/binance_usdt_gateway.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

namespace bitquant {

//=============================================================================
// Constants
//=============================================================================

// WebSocket endpoints
constexpr const char* WS_DATA_HOST = "fstream.binance.com";
constexpr const char* WS_DATA_PATH = "/stream";
constexpr const char* WS_USER_HOST = "fstream.binance.com";
constexpr const char* WS_USER_PATH = "/ws/";

//=============================================================================
// BinanceUsdtWsApi Implementation
//=============================================================================

BinanceUsdtWsApi::BinanceUsdtWsApi(BinanceUsdtGateway* gateway)
    : gateway_(gateway) {
    if (gateway_) {
        gateway_name_ = gateway_->gateway_name();
    }
}

BinanceUsdtWsApi::~BinanceUsdtWsApi() {
    stop();
}

void BinanceUsdtWsApi::connect(const BinanceUsdtWsConfig& config) {
    config_ = config;
    connected_ = true;
    running_ = true;

    write_log("Market WebSocket connected");

    if (connected_callback_) {
        connected_callback_();
    }
}

void BinanceUsdtWsApi::stop() {
    running_ = false;
    connected_ = false;

    write_log("Market WebSocket stopped");
}

void BinanceUsdtWsApi::subscribe_ticker(const std::string& symbol) {
    std::string stream = to_lower_symbol(symbol) + "@ticker";
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[symbol].push_back(stream);
    }
    write_log("Subscribe ticker: " + symbol);
}

void BinanceUsdtWsApi::subscribe_depth(const std::string& symbol, int levels) {
    std::string stream = to_lower_symbol(symbol) + "@depth" + std::to_string(levels);
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[symbol].push_back(stream);
    }
    write_log("Subscribe depth: " + symbol + " levels=" + std::to_string(levels));
}

void BinanceUsdtWsApi::subscribe_kline(const std::string& symbol, const std::string& interval) {
    std::string stream = to_lower_symbol(symbol) + "@kline_" + interval;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[symbol].push_back(stream);
    }
    write_log("Subscribe kline: " + symbol + " interval=" + interval);
}

void BinanceUsdtWsApi::subscribe_trade(const std::string& symbol) {
    std::string stream = to_lower_symbol(symbol) + "@trade";
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[symbol].push_back(stream);
    }
    write_log("Subscribe trade: " + symbol);
}

void BinanceUsdtWsApi::subscribe_agg_trade(const std::string& symbol) {
    std::string stream = to_lower_symbol(symbol) + "@aggTrade";
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[symbol].push_back(stream);
    }
    write_log("Subscribe aggTrade: " + symbol);
}

void BinanceUsdtWsApi::unsubscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.erase(symbol);
    write_log("Unsubscribe: " + symbol);
}

void BinanceUsdtWsApi::on_connected() {
    connected_ = true;
    write_log("WebSocket connected");
    resubscribe_all();

    if (connected_callback_) {
        connected_callback_();
    }
}

void BinanceUsdtWsApi::on_disconnected() {
    connected_ = false;
    write_log("WebSocket disconnected");
}

void BinanceUsdtWsApi::on_message(const std::string& message) {
    // Parse message and route to appropriate handler
    // Message format: {"stream":"btcusdt@ticker","data":{...}}

    // For now, just log the message
    // TODO: Implement actual JSON parsing with flatjson

    if (message.find("@ticker") != std::string::npos) {
        // process_ticker(symbol, data);
    } else if (message.find("@depth") != std::string::npos) {
        // process_depth(symbol, data);
    } else if (message.find("@kline") != std::string::npos) {
        // process_kline(symbol, data);
    } else if (message.find("@trade") != std::string::npos) {
        // process_trade(symbol, data);
    } else if (message.find("@aggTrade") != std::string::npos) {
        // process_agg_trade(symbol, data);
    }
}

void BinanceUsdtWsApi::process_stream_message(const std::string& stream, const std::string& data) {
    // Parse stream name to get symbol and channel
    size_t at_pos = stream.find('@');
    if (at_pos == std::string::npos) return;

    std::string symbol_lower = stream.substr(0, at_pos);
    std::string channel = stream.substr(at_pos + 1);

    // Convert symbol to uppercase
    std::string symbol;
    for (char c : symbol_lower) {
        symbol += std::toupper(c);
    }

    if (channel == "ticker") {
        process_ticker(symbol, data);
    } else if (channel.find("depth") == 0) {
        process_depth(symbol, data);
    } else if (channel.find("kline") == 0) {
        process_kline(symbol, data);
    } else if (channel == "trade") {
        process_trade(symbol, data);
    } else if (channel == "aggTrade") {
        process_agg_trade(symbol, data);
    }
}

void BinanceUsdtWsApi::process_ticker(const std::string& symbol, const std::string& data) {
    // Parse ticker data
    // {"e":"24hrTicker","E":123456789,"s":"BTCUSDT","p":"0.0015","P":"0.04",...}

    TickData tick;
    tick.symbol = symbol;
    tick.exchange = Exchange::BINANCE;

    // TODO: Parse actual JSON data
    // For now, create a placeholder tick

    {
        std::lock_guard<std::mutex> lock(ticks_mutex_);
        last_ticks_[symbol] = tick;
    }

    if (tick_callback_) {
        tick_callback_(tick);
    }
}

void BinanceUsdtWsApi::process_depth(const std::string& symbol, const std::string& data) {
    // Parse depth data
    // {"e":"depthUpdate","E":123456789,"s":"BTCUSDT","b":[["0.0024","10"]],...}

    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;

    // TODO: Parse actual JSON data

    if (depth_callback_) {
        depth_callback_(symbol, bids, asks);
    }
}

void BinanceUsdtWsApi::process_kline(const std::string& symbol, const std::string& data) {
    // Parse kline data
    // {"e":"kline","E":123456789,"s":"BTCUSDT","k":{"t":123456789,"o":"0.0015",...}}

    BarData bar;
    bar.symbol = symbol;
    bar.exchange = Exchange::BINANCE;

    // TODO: Parse actual JSON data

    if (bar_callback_) {
        bar_callback_(bar);
    }
}

void BinanceUsdtWsApi::process_trade(const std::string& symbol, const std::string& data) {
    // Parse trade data
    // {"e":"trade","E":123456789,"s":"BTCUSDT","t":12345,"p":"0.0015","q":100,...}

    double price = 0.0;
    double quantity = 0.0;
    bool is_buyer_maker = false;

    // TODO: Parse actual JSON data

    if (trade_callback_) {
        trade_callback_(symbol, price, quantity, is_buyer_maker);
    }
}

void BinanceUsdtWsApi::process_agg_trade(const std::string& symbol, const std::string& data) {
    // Parse aggregated trade data
    // {"e":"aggTrade","E":123456789,"s":"BTCUSDT","a":12345,"p":"0.0015","q":100,...}

    double price = 0.0;
    double quantity = 0.0;
    bool is_buyer_maker = false;

    // TODO: Parse actual JSON data

    if (trade_callback_) {
        trade_callback_(symbol, price, quantity, is_buyer_maker);
    }
}

void BinanceUsdtWsApi::resubscribe_all() {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    for (const auto& [symbol, streams] : subscriptions_) {
        for (const auto& stream : streams) {
            write_log("Resubscribe: " + stream);
        }
    }
}

void BinanceUsdtWsApi::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    } else {
        std::cout << "[" << gateway_name_ << " WS] " << msg << std::endl;
    }
}

std::string BinanceUsdtWsApi::to_lower_symbol(const std::string& symbol) {
    std::string result;
    for (char c : symbol) {
        result += std::tolower(c);
    }
    return result;
}

//=============================================================================
// BinanceUsdtUserWsApi Implementation
//=============================================================================

BinanceUsdtUserWsApi::BinanceUsdtUserWsApi(BinanceUsdtGateway* gateway)
    : gateway_(gateway) {
    if (gateway_) {
        gateway_name_ = gateway_->gateway_name();
    }
}

BinanceUsdtUserWsApi::~BinanceUsdtUserWsApi() {
    stop();
}

void BinanceUsdtUserWsApi::connect(const std::string& listen_key, const BinanceUsdtWsConfig& config) {
    listen_key_ = listen_key;
    config_ = config;
    connected_ = true;
    running_ = true;

    write_log("User WebSocket connected with listen key");
}

void BinanceUsdtUserWsApi::stop() {
    running_ = false;
    connected_ = false;

    write_log("User WebSocket stopped");
}

void BinanceUsdtUserWsApi::on_connected() {
    connected_ = true;
    write_log("User WebSocket connected");
}

void BinanceUsdtUserWsApi::on_disconnected() {
    connected_ = false;
    write_log("User WebSocket disconnected");
}

void BinanceUsdtUserWsApi::on_message(const std::string& message) {
    // Parse message type
    // {"e":"ACCOUNT_UPDATE","E":123456789,"T":123456789,"a":{"m":"ORDER","B":[...]}}

    if (message.find("ACCOUNT_UPDATE") != std::string::npos) {
        process_account_update(message);
    } else if (message.find("ORDER_TRADE_UPDATE") != std::string::npos) {
        process_order_update(message);
    }
}

void BinanceUsdtUserWsApi::process_account_update(const std::string& data) {
    // Parse account update
    // {"e":"ACCOUNT_UPDATE","E":123456789,"T":123456789,"a":{"m":"ORDER","B":[...],"P":[...]}}

    // Account balance updates
    AccountData account;
    account.gateway_name = gateway_name_;

    // TODO: Parse actual JSON data

    if (account_callback_) {
        account_callback_(account);
    }

    // Position updates
    PositionData position;
    position.gateway_name = gateway_name_;

    // TODO: Parse actual JSON data

    if (position_callback_) {
        position_callback_(position);
    }
}

void BinanceUsdtUserWsApi::process_order_update(const std::string& data) {
    // Parse order update
    // {"e":"ORDER_TRADE_UPDATE","E":123456789,"T":123456789,"o":{"s":"BTCUSDT","c":"my_order_id",...}}

    OrderData order;
    order.gateway_name = gateway_name_;

    // TODO: Parse actual JSON data
    // Fields: s=symbol, c=clientOrderId, S=side, o=orderType, f=timeInForce
    // q=quantity, p=price, X=status, z=cumulativeFilledQty, L=lastFilledPrice

    if (order_callback_) {
        order_callback_(order);
    }
}

void BinanceUsdtUserWsApi::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    } else {
        std::cout << "[" << gateway_name_ << " UserWS] " << msg << std::endl;
    }
}

} // namespace bitquant