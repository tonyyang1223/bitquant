---
name: Binance Spot API C++ Implementation
description: C++ implementation for Binance Spot trading with backtest/live unified strategy interface
type: project
---

# Binance Spot API C++ Implementation Design

## Overview

This document describes the C++ implementation for Binance Spot trading API, following the design patterns from howtrader Python project.

## Requirements Summary

| Requirement | Priority |
|-------------|----------|
| Binance Spot trading support | High |
| Backtest + Live unified interface | High |
| Multi-asset portfolio strategy | High |
| Limit + Market orders | Medium |
| Futures support (reserved) | Low |

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        用户策略层                                │
│  MultiAssetStrategy: 管理 BTCUSDT + ETHUSDT + ... 组合持仓      │
└─────────────────────────────────────────────────────────────────┘
                              ↓ next_bar(tick)
┌─────────────────────────────────────────────────────────────────┐
│                        交易引擎层                                │
│  TradingEngine: 事件驱动，策略调度，行情分发                      │
│  Broker: 订单管理，持仓跟踪，模拟撮合（回测模式）                  │
│  RiskManager: 风控检查（持仓限制、订单流控、自成交预防）           │
└─────────────────────────────────────────────────────────────────┘
                              ↓ send_order()
┌─────────────────────────────────────────────────────────────────┐
│                        交易所抽象层                              │
│  IExchange: 统一接口（connect, subscribe, send_order...）        │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────┴─────────────────────┐
        ↓                                           ↓
┌───────────────────┐                    ┌───────────────────┐
│ BinanceSpotGateway│                    │   (预留接口)       │
│  ├─ RestApi       │                    │ FuturesGateway    │
│  ├─ TradeWsApi    │                    │  (后续实现)        │
│  └─ DataWsApi    │                    └───────────────────┘
└───────────────────┘
        ↓
┌───────────────────┐
│  binapiLibrary    │
│  api.binance.com  │
└───────────────────┘
```

## Gateway Components

### GatewayConfig

Configuration structure for gateway connection.

```cpp
struct GatewayConfig {
    std::string name;
    std::string api_key;
    std::string api_secret;
    std::string proxy_host;
    int proxy_port = 0;
    bool testnet = false;
    size_t timeout_ms = 10000;
};
```

### BinanceSpotGateway

Main gateway class coordinating REST and WebSocket connections.

```cpp
class BinanceSpotGateway : public IExchange {
public:
    std::string name() const override { return "BinanceSpot"; }
    Exchange exchange() const override { return Exchange::BINANCE; }
    
    bool connect(const GatewayConfig& config) override;
    void close() override;
    
    void subscribe(const SubscribeRequest& req) override;
    std::string send_order(const OrderRequest& req) override;
    void cancel_order(const CancelRequest& req) override;
    void query_order(const OrderQueryRequest& req) override;
    void query_account() override;
    std::vector<BarData> query_history(const HistoryRequest& req) override;
    
    void on_tick(TickCallback cb) override;
    void on_order(OrderCallback cb) override;
    void on_trade(TradeCallback cb) override;
    void on_account(AccountCallback cb) override;
    void on_error(ErrorCallback cb) override;

private:
    std::unique_ptr<BinanceSpotRestApi> rest_api_;
    std::unique_ptr<BinanceSpotTradeWsApi> trade_ws_api_;
    std::unique_ptr<BinanceSpotDataWsApi> data_ws_api_;
    
    std::unordered_map<std::string, OrderData> orders_;
    std::unordered_map<std::string, ContractData> contracts_;
    std::string gateway_name_ = "BINANCE_SPOT";
    
    int64_t time_offset_ = 0;
    int order_count_ = 0;
    int64_t connect_time_ = 0;
    std::mutex order_mutex_;
};
```

### BinanceSpotRestApi

REST API wrapper using binapi.

```cpp
class BinanceSpotRestApi {
public:
    bool init(const std::string& host, const std::string& port,
              const std::string& api_key, const std::string& api_secret);
    
    // Market Data
    bool ping();
    int64_t get_server_time();
    std::vector<BarData> get_klines(const std::string& symbol, 
                                     const std::string& interval, int limit);
    double get_price(const std::string& symbol);
    std::vector<ContractData> get_exchange_info();
    
    // Trading
    std::string send_order(const OrderRequest& req);
    void cancel_order(const std::string& symbol, const std::string& order_id);
    OrderData query_order(const std::string& symbol, const std::string& order_id);
    std::vector<OrderData> query_open_orders(const std::string& symbol = "");
    
    // Account
    std::vector<AccountData> query_account();
    
    // User Data Stream
    std::string start_user_stream();
    void keep_user_stream(const std::string& listen_key);
    void close_user_stream(const std::string& listen_key);

private:
    std::unique_ptr<boost::asio::io_context> ioctx_;
    std::unique_ptr<binapi::rest::api> api_;
    std::string api_key_;
    std::string api_secret_;
    int64_t time_offset_ = 0;
};
```

### BinanceSpotTradeWsApi

WebSocket client for user data stream (order updates).

```cpp
class BinanceSpotTradeWsApi : public WebsocketClient {
public:
    void connect(const std::string& url, const std::string& proxy_host, int proxy_port);
    
    void on_packet(const nlohmann::json& packet) override;
    void on_account(const nlohmann::json& packet);
    void on_order(const nlohmann::json& packet);

private:
    BinanceSpotGateway* gateway_;
};
```

### BinanceSpotDataWsApi

WebSocket client for market data stream.

```cpp
class BinanceSpotDataWsApi : public WebsocketClient {
public:
    void connect(const std::string& proxy_host, int proxy_port);
    void subscribe(const std::string& symbol);
    
    void on_packet(const nlohmann::json& packet) override;
    void on_ticker(const nlohmann::json& data, const std::string& symbol);
    void on_depth(const nlohmann::json& data, const std::string& symbol);

private:
    BinanceSpotGateway* gateway_;
    std::unordered_map<std::string, TickData> ticks_;
    int reqid_ = 0;
};
```

## Data Types Mapping

### OrderRequest (from Python howtrader)

| Python Field | C++ Field | Type |
|--------------|-----------|------|
| symbol | symbol | std::string |
| exchange | exchange | Exchange enum |
| direction | direction | Direction enum |
| type | type | OrderType enum |
| volume | volume | double |
| price | price | double |
| offset | offset | Offset enum |
| reference | reference | std::string |

### OrderData (from Python howtrader)

| Python Field | C++ Field | Type |
|--------------|-----------|------|
| symbol | symbol | std::string |
| exchange | exchange | Exchange enum |
| orderid | orderid | std::string |
| type | type | OrderType enum |
| direction | direction | Direction enum |
| offset | offset | Offset enum |
| price | price | double |
| volume | volume | double |
| traded | traded | double |
| traded_price | traded_price | double |
| status | status | Status enum |
| datetime | datetime | int64_t |
| update_time | update_time | int64_t |
| reference | reference | std::string |
| rejected_reason | rejected_reason | std::string |

### Enum Mappings

```cpp
// Order Status (from Python STATUS_BINANCE2VT)
inline Status status_from_binance(const std::string& status) {
    static const std::unordered_map<std::string, Status> map = {
        {"NEW", Status::NOTTRADED},
        {"PARTIALLY_FILLED", Status::PARTTRADED},
        {"FILLED", Status::ALLTRADED},
        {"CANCELED", Status::CANCELLED},
        {"REJECTED", Status::REJECTED},
        {"EXPIRED", Status::CANCELLED}
    };
    auto it = map.find(status);
    return it != map.end() ? it->second : Status::NOTTRADED;
}

// Order Type (from Python ORDERTYPE_VT2BINANCE)
inline std::string order_type_to_binance(OrderType type) {
    static const std::unordered_map<OrderType, std::string> map = {
        {OrderType::LIMIT, "LIMIT"},
        {OrderType::TAKER, "MARKET"},
        {OrderType::MAKER, "LIMIT_MAKER"},
        {OrderType::STOP, "STOP_LOSS"},
        {OrderType::STOP_LIMIT, "STOP_LOSS_LIMIT"}
    };
    auto it = map.find(type);
    return it != map.end() ? it->second : "LIMIT";
}

// Direction (from Python DIRECTION_VT2BINANCE)
inline std::string direction_to_binance(Direction dir) {
    return dir == Direction::LONG ? "BUY" : "SELL";
}
```

## Key Implementation Details

### on_order Trade Generation

Following Python howtrader logic exactly:

```cpp
void BinanceSpotGateway::on_order(const OrderData& order) {
    auto it = orders_.find(order.orderid);
    if (it == orders_.end()) {
        orders_[order.orderid] = order;
        if (order_callback_) order_callback_(order);
    } else {
        double traded = order.traded - it->second.traded;
        if (traded < 0) return;  // Filter out-of-sequence messages
        
        if (traded > 0) {
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
```

### Order ID Generation

Following Python howtrader format:

```cpp
std::string BinanceSpotGateway::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;
    std::ostringstream ss;
    ss << "x-A6SIDXVS" << (connect_time_ + order_count_);
    return ss.str();
}
```

### Time Offset Handling

```cpp
void BinanceSpotRestApi::on_query_time(const nlohmann::json& data) {
    int64_t local_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    int64_t server_time = data["serverTime"].get<int64_t>();
    time_offset_ = local_time - server_time;
}

int64_t BinanceSpotRestApi::get_timestamp_for_sign() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    if (time_offset_ > 0) {
        now -= std::abs(time_offset_);
    } else if (time_offset_ < 0) {
        now += std::abs(time_offset_);
    }
    return now;
}
```

### Listen Key Renewal

```cpp
void BinanceSpotGateway::keep_user_stream() {
    keep_alive_count_++;
    if (keep_alive_count_ < 300) return;
    keep_alive_count_ = 0;
    rest_api_->keep_user_stream(listen_key_);
}
```

## Multi-Asset Strategy

```cpp
class MultiAssetStrategy : public BaseStrategy {
public:
    std::vector<std::string> symbols_ = {"btcusdt", "ethusdt"};
    std::unordered_map<std::string, ArrayManager> ams_;
    std::unordered_map<std::string, double> positions_;
    
    void on_init() override {
        for (const auto& symbol : symbols_) {
            subscribe(symbol);
            ams_[symbol] = ArrayManager(100);
            positions_[symbol] = 0.0;
        }
    }
    
    void on_bar(const BarData& bar) override {
        auto it = ams_.find(bar.symbol);
        if (it != ams_.end() && it->second.update_bar(bar)) {
            generate_signal(bar.symbol);
        }
    }
    
    void on_order(const OrderData& order) override {
        if (order.status == Status::ALLTRADED) {
            auto& pos = positions_[order.symbol];
            if (order.direction == Direction::LONG) {
                pos += order.traded;
            } else {
                pos -= order.traded;
            }
        }
    }
};
```

## Run Mode Switching

```cpp
enum class RunMode {
    BACKTEST,
    LIVE
};

class TradingEngine {
public:
    void set_mode(RunMode mode) { mode_ = mode; }
    
    void send_order(const OrderRequest& req) {
        if (!risk_manager_.check_order(req)) return;
        
        if (mode_ == RunMode::BACKTEST) {
            broker_.simulate_order(req);
        } else {
            gateway_->send_order(req);
        }
    }

private:
    RunMode mode_ = RunMode::BACKTEST;
    Broker broker_;
    std::unique_ptr<IExchange> gateway_;
};
```

## Error Handling

### WebSocket Reconnection

```cpp
void WebSocketClient::on_disconnect() {
    if (!running_) return;
    
    int delay = std::min(reconnect_delay_, max_reconnect_delay_);
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    reconnect_delay_ = std::min(reconnect_delay_ * 2, max_reconnect_delay_);
    
    if (reconnect()) {
        reconnect_delay_ = initial_reconnect_delay_;
        resubscribe_all();
    } else {
        on_disconnect();
    }
}
```

### API Error Handling

```cpp
void on_api_error(int code, const std::string& msg) {
    if (code == -1021) {
        // INVALID_TIMESTAMP
        query_server_time();
    } else if (code == -2010 || code == -2011) {
        // Order rejected
        order.status = Status::REJECTED;
        order.rejected_reason = msg;
        on_order(order);
    }
}
```

## Configuration

```json
{
  "gateway": {
    "name": "BinanceSpotGateway",
    "api_key": "",
    "api_secret": "",
    "proxy_host": "",
    "proxy_port": 0
  },
  "risk": {
    "max_order_volume": 100.0,
    "max_position_ratio": 0.3,
    "max_daily_orders": 1000,
    "max_order_rate": 10
  }
}
```

## Implementation Phases

### Phase 1: Core Gateway (Week 1)
- [ ] BinanceSpotRestApi with binapi
- [ ] send_order, cancel_order, query_order
- [ ] query_account, query_history

### Phase 2: WebSocket (Week 2)
- [ ] BinanceSpotTradeWsApi (user data stream)
- [ ] BinanceSpotDataWsApi (market data)
- [ ] Reconnection logic

### Phase 3: Integration (Week 3)
- [ ] TradingEngine integration
- [ ] RiskManager integration
- [ ] Multi-asset strategy support

### Phase 4: Testing (Week 4)
- [ ] Unit tests
- [ ] Integration tests
- [ ] Live testing on testnet

## References

- Python howtrader: `/home/ubuntu/project/bitquant/howtrader/howtrader/gateway/binance/binance_spot_gateway.py`
- Python object types: `/home/ubuntu/project/bitquant/howtrader/howtrader/trader/object.py`
- Python constants: `/home/ubuntu/project/bitquant/howtrader/howtrader/trader/constant.py`
- binapi library: `/home/ubuntu/project/bitquant/binapi/`
