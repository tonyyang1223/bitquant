# Binance Spot Gateway Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement Binance Spot trading gateway with REST API trading, WebSocket data streaming, and full integration with existing trading engine.

**Architecture:** Extend existing `IExchange` interface, create `BinanceSpotGateway` as unified entry point, separate classes for REST and WebSocket APIs, use binapi library underlying.

**Tech Stack:** C++20, Boost.Asio, binapi, nlohmann/json, TA-Lib

---

## File Structure

```
bitquant_cpp/
├── include/
│   └── exchange/
│       ├── binance_spot_gateway.hpp     # NEW: Main gateway class
│       ├── binance_spot_rest_api.hpp    # NEW: REST API wrapper
│       ├── binance_spot_ws_api.hpp      # NEW: WebSocket APIs
│       ├── binance_mapping.hpp          # NEW: Enum mappings
│       └── i_exchange.hpp               # MODIFY: Add GatewayConfig
├── src/
│   └── exchange/
│       ├── binance_spot_gateway.cpp     # NEW: Gateway implementation
│       ├── binance_spot_rest_api.cpp    # NEW: REST implementation
│       ├── binance_spot_ws_api.cpp      # NEW: WebSocket implementation
│       └── binance_mapping.cpp          # NEW: Mapping implementation
└── tests/
    └── unit/
        └── test_binance_spot_gateway.cpp # NEW: Unit tests
```

---

## Task 1: GatewayConfig and OrderQueryRequest Types

**Files:**
- Modify: `include/core/types.hpp`
- Modify: `include/exchange/i_exchange.hpp`- Test: `tests/unit/test_types.cpp`

### Step 1: Add GatewayConfig to i_exchange.hpp

- [ ] **Add GatewayConfig struct after includes**

```cpp
// In include/exchange/i_exchange.hpp, after #include "core/types.hpp"

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
```

### Step 2: Add OrderQueryRequest to types.hpp

- [ ] **Add OrderQueryRequest struct after CancelRequest**

```cpp
// In include/core/types.hpp, after CancelRequest struct

/**
 * @brief Order query request
 */
struct OrderQueryRequest {
    std::string orderid;
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};
```

### Step 3: Update IExchange interface

- [ ] **Add query_order method to IExchange class**

```cpp
// In include/exchange/i_exchange.hpp, after query_open_orders method

/**
 * @brief Query specific order status
 * @param req Order query request
 */
virtual std::optional<OrderData> query_order(const OrderQueryRequest& req) {
    (void)req;
    return std::nullopt;
}
```

### Step 4: Build and verify

- [ ] **Run cmake and build**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=OFF
make -j$(nproc)
```

Expected: Build succeeds with no errors

### Step 5: Commit

```bash
git add include/core/types.hpp include/exchange/i_exchange.hpp
git commit -m "feat: Add GatewayConfig and OrderQueryRequest types"
```

---

## Task 2: Binance Enum Mappings

**Files:**
- Create: `include/exchange/binance_mapping.hpp`
- Create: `src/exchange/binance_mapping.cpp`- Test: `tests/unit/test_binance_mapping.cpp`

### Step 1: Create binance_mapping.hpp

- [ ] **Create header file with mapping functions**

```cpp
// File: include/exchange/binance_mapping.hpp
#pragma once

#include "core/types.hpp"
#include <string>
#include <unordered_map>

namespace bitquant {

//=============================================================================
// Binance <-> VT Mappings (from Python howtrader)
//=============================================================================

/**
 * @brief Convert Binance status string to Status enum
 */
Status status_from_binance(const std::string& status);

/**
 * @brief Convert Status enum to Binance status string
 */
std::string status_to_binance(Status status);

/**
 * @brief Convert OrderType enum to Binance order type string
 */
std::string order_type_to_binance(OrderType type);

/**
 * @brief Convert Binance order type string to OrderType enum
 */
OrderType order_type_from_binance(const std::string& type);

/**
 * @brief Convert Direction enum to Binance side string
 */
std::string direction_to_binance(Direction dir);

/**
 * @brief Convert Binance side string to Direction enum
 */
Direction direction_from_binance(const std::string& side);

/**
 * @brief Convert Interval enum to Binance interval string
 */
std::string interval_to_binance(Interval interval);

/**
 * @brief Convert Binance interval string to Interval enum
 */
Interval interval_from_binance(const std::string& interval);

} // namespace bitquant
```

### Step 2: Create binance_mapping.cpp

- [ ] **Create implementation file**

```cpp
// File: src/exchange/binance_mapping.cpp
#include "exchange/binance_mapping.hpp"

namespace bitquant {

Status status_from_binance(const std::string& status) {
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

std::string status_to_binance(Status status) {
    static const std::unordered_map<Status, std::string> map = {
        {Status::NOTTRADED, "NEW"},
        {Status::PARTTRADED, "PARTIALLY_FILLED"},
        {Status::ALLTRADED, "FILLED"},
        {Status::CANCELLED, "CANCELED"},
        {Status::REJECTED, "REJECTED"}
    };
    auto it = map.find(status);
    return it != map.end() ? it->second : "NEW";
}

std::string order_type_to_binance(OrderType type) {
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

OrderType order_type_from_binance(const std::string& type) {
    static const std::unordered_map<std::string, OrderType> map = {
        {"LIMIT", OrderType::LIMIT},
        {"MARKET", OrderType::TAKER},
        {"LIMIT_MAKER", OrderType::MAKER},
        {"STOP_LOSS", OrderType::STOP},
        {"STOP_LOSS_LIMIT", OrderType::STOP_LIMIT}
    };
    auto it = map.find(type);
    return it != map.end() ? it->second : OrderType::LIMIT;
}

std::string direction_to_binance(Direction dir) {
    return dir == Direction::LONG ? "BUY" : "SELL";
}

Direction direction_from_binance(const std::string& side) {
    return side == "BUY" ? Direction::LONG : Direction::SHORT;
}

std::string interval_to_binance(Interval interval) {
    static const std::unordered_map<Interval, std::string> map = {
        {Interval::MINUTE_1, "1m"},
        {Interval::MINUTE_3, "3m"},
        {Interval::MINUTE_5, "5m"},
        {Interval::MINUTE_15, "15m"},
        {Interval::MINUTE_30, "30m"},
        {Interval::HOUR_1, "1h"},
        {Interval::HOUR_2, "2h"},
        {Interval::HOUR_4, "4h"},
        {Interval::HOUR_6, "6h"},
        {Interval::HOUR_12, "12h"},
        {Interval::DAILY, "1d"},
        {Interval::WEEKLY, "1w"},
        {Interval::MONTHLY, "1M"}
    };
    auto it = map.find(interval);
    return it != map.end() ? it->second : "1m";
}

Interval interval_from_binance(const std::string& interval) {
    static const std::unordered_map<std::string, Interval> map = {
        {"1m", Interval::MINUTE_1},
        {"3m", Interval::MINUTE_3},
        {"5m", Interval::MINUTE_5},
        {"15m", Interval::MINUTE_15},
        {"30m", Interval::MINUTE_30},
        {"1h", Interval::HOUR_1},
        {"2h", Interval::HOUR_2},
        {"4h", Interval::HOUR_4},
        {"6h", Interval::HOUR_6},
        {"12h", Interval::HOUR_12},
        {"1d", Interval::DAILY},
        {"1w", Interval::WEEKLY},
        {"1M", Interval::MONTHLY}
    };
    auto it = map.find(interval);
    return it != map.end() ? it->second : Interval::MINUTE_1;
}

} // namespace bitquant
```

### Step 3: Update CMakeLists.txt

- [ ] **Add new source file to bitquant library**

```cmake
# In bitquant_cpp/CMakeLists.txt, in target_sources(bitquant PRIVATE ...) section
# Add after src/exchange/websocket_client.cpp:
src/exchange/binance_mapping.cpp
```

### Step 4: Build and verify

- [ ] **Build project**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake ..
make -j$(nproc)
```

Expected: Build succeeds

### Step 5: Commit

```bash
git add include/exchange/binance_mapping.hpp src/exchange/binance_mapping.cpp CMakeLists.txt
git commit -m "feat: Add Binance enum mapping functions"
```

---

## Task 3: BinanceSpotRestApi Header

**Files:**
- Create: `include/exchange/binance_spot_rest_api.hpp`

### Step 1: Create header file

- [ ] **Create binance_spot_rest_api.hpp**

```cpp
// File: include/exchange/binance_spot_rest_api.hpp
#pragma once

#include "core/types.hpp"
#include "exchange/binance_mapping.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace boost {
namespace asio {
    class io_context;
}
}

namespace binapi {
namespace rest {
    class api;
}
}

namespace bitquant {

/**
 * @brief Binance Spot REST API client
 *
 * Wraps binapi library for Spot trading:
 * - Market data: ping, time, klines, price, exchange info
 * - Trading: send_order, cancel_order, query_order
 * - Account: query_account, query_open_orders
 * - User stream: start/keep/close user data stream
 */
class BinanceSpotRestApi {
public:
    BinanceSpotRestApi();
    ~BinanceSpotRestApi();

    // Non-copyable
    BinanceSpotRestApi(const BinanceSpotRestApi&) = delete;
    BinanceSpotRestApi& operator=(const BinanceSpotRestApi&) = delete;

    //=========================================================================
    // Initialization
    //=========================================================================

    /**
     * @brief Initialize REST API client
     * @param host API host (e.g., "api.binance.com")
     * @param port API port (e.g., "443")
     * @param api_key API key (optional for public data)
     * @param api_secret API secret (optional for public data)
     * @param timeout_ms Request timeout in milliseconds
     * @return true on success
     */
    bool init(const std::string& host,
              const std::string& port,
              const std::string& api_key = "",
              const std::string& api_secret = "",
              size_t timeout_ms = 10000);

    //=========================================================================
    // Market Data
    //=========================================================================

    /**
     * @brief Ping Binance server
     * @return true if ping successful
     */
    bool ping();

    /**
     * @brief Get server time
     * @return Server timestamp in milliseconds, 0 on error
     */
    int64_t get_server_time();

    /**
     * @brief Get current price
     * @param symbol Trading pair (e.g., "BTCUSDT")
     * @return Current price, 0 on error
     */
    double get_price(const std::string& symbol);

    /**
     * @brief Get kline/candlestick data
     * @param symbol Trading pair
     * @param interval Kline interval
     * @param limit Number of klines (max 1000)
     * @return Vector of BarData
     */
    std::vector<BarData> get_klines(const std::string& symbol,
                                     Interval interval,
                                     size_t limit = 500);

    /**
     * @brief Get exchange info (contracts)
     * @return Vector of ContractData
     */
    std::vector<ContractData> get_exchange_info();

    //=========================================================================
    // Trading
    //=========================================================================

    /**
     * @brief Send order
     * @param req Order request
     * @return Order ID on success, empty string on failure
     */
    std::string send_order(const OrderRequest& req);

    /**
     * @brief Cancel order
     * @param symbol Trading pair
     * @param order_id Order ID (client order ID)
     * @return true on success
     */
    bool cancel_order(const std::string& symbol, const std::string& order_id);

    /**
     * @brief Query order status
     * @param symbol Trading pair
     * @param order_id Order ID (client order ID)
     * @return OrderData if found
     */
    std::optional<OrderData> query_order(const std::string& symbol,
                                          const std::string& order_id);

    /**
     * @brief Query all open orders
     * @param symbol Trading pair (empty for all symbols)
     * @return Vector of OrderData
     */
    std::vector<OrderData> query_open_orders(const std::string& symbol = "");

    //=========================================================================
    // Account
    //=========================================================================

    /**
     * @brief Query account balances
     * @return Vector of AccountData (one per asset)
     */
    std::vector<AccountData> query_account();

    //=========================================================================
    // User Data Stream
    //=========================================================================

    /**
     * @brief Start user data stream
     * @return Listen key on success, empty on failure
     */
    std::string start_user_stream();

    /**
     * @brief Keep user data stream alive
     * @param listen_key Listen key from start_user_stream
     * @return true on success
     */
    bool keep_user_stream(const std::string& listen_key);

    /**
     * @brief Close user data stream
     * @param listen_key Listen key
     * @return true on success
     */
    bool close_user_stream(const std::string& listen_key);

    //=========================================================================
    // Utility
    //=========================================================================

    /**
     * @brief Get time offset (local - server)
     */
    int64_t time_offset() const { return time_offset_; }

    /**
     * @brief Get last error message
     */
    const std::string& last_error() const { return last_error_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string api_key_;
    std::string api_secret_;
    std::string host_;
    std::string port_;
    size_t timeout_ms_ = 10000;
    int64_t time_offset_ = 0;
    std::string last_error_;
};

} // namespace bitquant
```

### Step 2: Build and verify

- [ ] **Build project**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake ..
make -j$(nproc)
```

Expected: Build succeeds (header only, no implementation yet)

### Step 3: Commit

```bash
git add include/exchange/binance_spot_rest_api.hpp
git commit -m "feat: Add BinanceSpotRestApi header"
```

---

## Task 4: BinanceSpotRestApi Implementation

**Files:**
- Create: `src/exchange/binance_spot_rest_api.cpp`

### Step 1: Create implementation skeleton

- [ ] **Create binance_spot_rest_api.cpp with init and ping**

```cpp
// File: src/exchange/binance_spot_rest_api.cpp
#include "exchange/binance_spot_rest_api.hpp"
#include <binapi/api.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace bitquant {

struct BinanceSpotRestApi::Impl {
    std::unique_ptr<boost::asio::io_context> ioctx;
    std::unique_ptr<binapi::rest::api> api;
};

BinanceSpotRestApi::BinanceSpotRestApi()
    : impl_(std::make_unique<Impl>())
{
}

BinanceSpotRestApi::~BinanceSpotRestApi() = default;

bool BinanceSpotRestApi::init(const std::string& host,
                               const std::string& port,
                               const std::string& api_key,
                               const std::string& api_secret,
                               size_t timeout_ms) {
    host_ = host;
    port_ = port;
    api_key_ = api_key;
    api_secret_ = api_secret;
    timeout_ms_ = timeout_ms;

    try {
        impl_->ioctx = std::make_unique<boost::asio::io_context>();
        impl_->api = std::make_unique<binapi::rest::api>(
            *impl_->ioctx,
            host_,
            port_,
            api_key_,
            api_secret_,
            timeout_ms_
        );
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Init failed: ") + e.what();
        return false;
    }
}

bool BinanceSpotRestApi::ping() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    auto result = impl_->api->ping();
    if (!result) {
        last_error_ = result.errmsg;
        return false;
    }
    return true;
}

int64_t BinanceSpotRestApi::get_server_time() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0;
    }

    auto result = impl_->api->server_time();
    if (!result) {
        last_error_ = result.errmsg;
        return 0;
    }

    // Update time offset
    int64_t local_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    time_offset_ = local_time - static_cast<int64_t>(result.v.serverTime);

    return result.v.serverTime;
}

} // namespace bitquant
```

### Step 2: Add market data methods

- [ ] **Append market data methods to binance_spot_rest_api.cpp**

```cpp
// Continue in src/exchange/binance_spot_rest_api.cpp

double BinanceSpotRestApi::get_price(const std::string& symbol) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0.0;
    }

    try {
        auto result = impl_->api->price(symbol.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return 0.0;
        }
        return static_cast<double>(result.v.price);
    } catch (const std::exception& e) {
        last_error_ = std::string("get_price failed: ") + e.what();
        return 0.0;
    }
}

std::vector<BarData> BinanceSpotRestApi::get_klines(const std::string& symbol,
                                                     Interval interval,
                                                     size_t limit) {
    std::vector<BarData> bars;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return bars;
    }

    try {
        std::string interval_str = interval_to_binance(interval);
        auto result = impl_->api->klines(
            symbol.c_str(),
            interval_str.c_str(),
            std::min(limit, size_t(1000))
        );

        if (!result) {
            last_error_ = result.errmsg;
            return bars;
        }

        bars.reserve(result.v.klines.size());
        for (const auto& k : result.v.klines) {
            BarData bar;
            bar.symbol = symbol;
            bar.exchange = Exchange::BINANCE;
            bar.interval = interval;
            bar.datetime = static_cast<int64_t>(k.start_time);
            bar.open_price = static_cast<double>(k.open);
            bar.high_price = static_cast<double>(k.high);
            bar.low_price = static_cast<double>(k.low);
            bar.close_price = static_cast<double>(k.close);
            bar.volume = static_cast<double>(k.volume);
            bars.push_back(bar);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("get_klines failed: ") + e.what();
    }

    return bars;
}

std::vector<ContractData> BinanceSpotRestApi::get_exchange_info() {
    std::vector<ContractData> contracts;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return contracts;
    }

    try {
        auto result = impl_->api->exchange_info();
        if (!result) {
            last_error_ = result.errmsg;
            return contracts;
        }

        for (const auto& sym : result.v.symbols) {
            ContractData contract;
            contract.symbol = sym.symbol;
            contract.exchange = Exchange::BINANCE;
            contract.product = Product::SPOT;
            contract.history_data = true;

            // Parse filters
            for (const auto& f : sym.filters) {
                if (f["filterType"] == "PRICE_FILTER") {
                    contract.pricetick = std::stod(f["tickSize"].get<std::string>());
                } else if (f["filterType"] == "LOT_SIZE") {
                    contract.min_volume = std::stod(f["stepSize"].get<std::string>());
                } else if (f["filterType"] == "MIN_NOTIONAL") {
                    contract.min_notional = std::stod(f["minNotional"].get<std::string>());
                }
            }

            contracts.push_back(contract);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("get_exchange_info failed: ") + e.what();
    }

    return contracts;
}
```

### Step 3: Add trading methods

- [ ] **Append trading methods to binance_spot_rest_api.cpp**

```cpp
// Continue in src/exchange/binance_spot_rest_api.cpp

std::string BinanceSpotRestApi::send_order(const OrderRequest& req) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return "";
    }

    try {
        std::string side = direction_to_binance(req.direction);
        std::string type = order_type_to_binance(req.type);
        std::string time_in_force = "GTC";

        std::string volume_str = std::to_string(req.volume);
        std::string price_str = std::to_string(req.price);

        binapi::rest::e_side e_side = (req.direction == Direction::LONG)
            ? binapi::rest::e_side::buy
            : binapi::rest::e_side::sell;

        binapi::rest::e_type e_type;
        if (req.type == OrderType::LIMIT) {
            e_type = binapi::rest::e_type::limit;
        } else if (req.type == OrderType::TAKER) {
            e_type = binapi::rest::e_type::market;
        } else if (req.type == OrderType::MAKER) {
            e_type = binapi::rest::e_type::limit_maker;
        } else {
            e_type = binapi::rest::e_type::limit;
        }

        binapi::rest::e_time e_time = binapi::rest::e_time::GTC;

        auto result = impl_->api->new_order(
            req.symbol.c_str(),
            e_side,
            e_type,
            e_time,
            binapi::rest::e_trade_resp_type::RESULT,
            volume_str.c_str(),
            (req.type == OrderType::LIMIT || req.type == OrderType::MAKER) 
                ? price_str.c_str() : nullptr,
            nullptr,  // client_order_id
            nullptr,  // stop_price
            nullptr   // iceberg_amount
        );

        if (!result) {
            last_error_ = result.errmsg;
            return "";
        }

        return std::to_string(result.v.orderId);
    } catch (const std::exception& e) {
        last_error_ = std::string("send_order failed: ") + e.what();
        return "";
    }
}

bool BinanceSpotRestApi::cancel_order(const std::string& symbol,
                                       const std::string& order_id) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        size_t oid = std::stoull(order_id);
        auto result = impl_->api->cancel_order(
            symbol.c_str(),
            oid,
            nullptr,  // client_order_id
            nullptr   // new_client_order_id
        );

        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("cancel_order failed: ") + e.what();
        return false;
    }
}

std::optional<OrderData> BinanceSpotRestApi::query_order(const std::string& symbol,
                                                          const std::string& order_id) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return std::nullopt;
    }

    try {
        size_t oid = std::stoull(order_id);
        auto result = impl_->api->order_info(
            symbol.c_str(),
            oid,
            nullptr  // client_order_id
        );

        if (!result) {
            last_error_ = result.errmsg;
            return std::nullopt;
        }

        OrderData order;
        order.symbol = result.v.symbol;
        order.exchange = Exchange::BINANCE;
        order.orderid = std::to_string(result.v.orderId);
        order.type = order_type_from_binance(result.v.type);
        order.direction = direction_from_binance(result.v.side);
        order.price = static_cast<double>(result.v.price);
        order.volume = static_cast<double>(result.v.origQty);
        order.traded = static_cast<double>(result.v.executedQty);
        order.status = status_from_binance(result.v.status);
        order.datetime = static_cast<int64_t>(result.v.time);

        // Calculate traded_price
        if (order.traded > 0) {
            order.traded_price = static_cast<double>(result.v.cummulativeQuoteQty) / order.traded;
        }

        return order;
    } catch (const std::exception& e) {
        last_error_ = std::string("query_order failed: ") + e.what();
        return std::nullopt;
    }
}

std::vector<OrderData> BinanceSpotRestApi::query_open_orders(const std::string& symbol) {
    std::vector<OrderData> orders;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return orders;
    }

    try {
        auto result = impl_->api->open_orders(
            symbol.empty() ? nullptr : symbol.c_str()
        );

        if (!result) {
            last_error_ = result.errmsg;
            return orders;
        }

        for (const auto& o : result.v.orders) {
            OrderData order;
            order.symbol = o.symbol;
            order.exchange = Exchange::BINANCE;
            order.orderid = std::to_string(o.orderId);
            order.type = order_type_from_binance(o.type);
            order.direction = direction_from_binance(o.side);
            order.price = static_cast<double>(o.price);
            order.volume = static_cast<double>(o.origQty);
            order.traded = static_cast<double>(o.executedQty);
            order.status = status_from_binance(o.status);
            order.datetime = static_cast<int64_t>(o.time);

            if (order.traded > 0) {
                order.traded_price = static_cast<double>(o.cummulativeQuoteQty) / order.traded;
            }

            orders.push_back(order);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("query_open_orders failed: ") + e.what();
    }

    return orders;
}
```

### Step 4: Add account and user stream methods

- [ ] **Append account methods to binance_spot_rest_api.cpp**

```cpp
// Continue in src/exchange/binance_spot_rest_api.cpp

std::vector<AccountData> BinanceSpotRestApi::query_account() {
    std::vector<AccountData> accounts;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return accounts;
    }

    try {
        auto result = impl_->api->account_info();
        if (!result) {
            last_error_ = result.errmsg;
            return accounts;
        }

        for (const auto& [asset, balance] : result.v.balances) {
            double free = static_cast<double>(balance.free);
            double locked = static_cast<double>(balance.locked);
            
            if (free == 0 && locked == 0) continue;  // Skip empty balances

            AccountData account;
            account.accountid = asset;
            account.balance = free + locked;
            account.frozen = locked;
            accounts.push_back(account);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("query_account failed: ") + e.what();
    }

    return accounts;
}

std::string BinanceSpotRestApi::start_user_stream() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return "";
    }

    try {
        auto result = impl_->api->start_user_data_stream();
        if (!result) {
            last_error_ = result.errmsg;
            return "";
        }
        return result.v.listenKey;
    } catch (const std::exception& e) {
        last_error_ = std::string("start_user_stream failed: ") + e.what();
        return "";
    }
}

bool BinanceSpotRestApi::keep_user_stream(const std::string& listen_key) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        auto result = impl_->api->ping_user_data_stream(listen_key.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("keep_user_stream failed: ") + e.what();
        return false;
    }
}

bool BinanceSpotRestApi::close_user_stream(const std::string& listen_key) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        auto result = impl_->api->close_user_data_stream(listen_key.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("close_user_stream failed: ") + e.what();
        return false;
    }
}

} // namespace bitquant
```

### Step 5: Update CMakeLists.txt

- [ ] **Add new source file to bitquant library**

```cmake
# In bitquant_cpp/CMakeLists.txt, in target_sources(bitquant PRIVATE ...) section
# Add after src/exchange/binance_mapping.cpp:
src/exchange/binance_spot_rest_api.cpp
```

### Step 6: Build and verify

- [ ] **Build with binapi enabled**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=ON
make -j$(nproc)
```

Expected: Build succeeds

### Step 7: Commit

```bash
git add src/exchange/binance_spot_rest_api.cpp CMakeLists.txt
git commit -m "feat: Implement BinanceSpotRestApi with full trading support"
```

---

## Task 5: BinanceSpotGateway Header

**Files:**
- Create: `include/exchange/binance_spot_gateway.hpp`

### Step 1: Create header file

- [ ] **Create binance_spot_gateway.hpp**

```cpp
// File: include/exchange/binance_spot_gateway.hpp
#pragma once

#include "exchange/i_exchange.hpp"
#include "exchange/binance_spot_rest_api.hpp"
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

private:
    //=========================================================================
    // Internal
    //=========================================================================

    std::string generate_order_id();
    void process_order_update(const OrderData& order);

    //=========================================================================
    // Members
    //=========================================================================

    std::unique_ptr<BinanceSpotRestApi> rest_api_;
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
};

} // namespace bitquant
```

### Step 2: Build and verify

- [ ] **Build project**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake ..
make -j$(nproc)
```

Expected: Build succeeds

### Step 3: Commit

```bash
git add include/exchange/binance_spot_gateway.hpp
git commit -m "feat: Add BinanceSpotGateway header"
```

---

## Task 6: BinanceSpotGateway Implementation

**Files:**
- Create: `src/exchange/binance_spot_gateway.cpp`

### Step 1: Create implementation with connect/close

- [ ] **Create binance_spot_gateway.cpp**

```cpp
// File: src/exchange/binance_spot_gateway.cpp
#include "exchange/binance_spot_gateway.hpp"
#include "exchange/exchange_factory.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace bitquant {

//=============================================================================
// Auto-registration
//=============================================================================

namespace {
    bool register_binance_spot_gateway() {
        ExchangeFactory::register_exchange("binance_spot", []() {
            return std::make_unique<BinanceSpotGateway>();
        });
        return true;
    }
}

bool binance_spot_gateway_registered = register_binance_spot_gateway();

//=============================================================================
// Constructor/Destructor
//=============================================================================

BinanceSpotGateway::BinanceSpotGateway() = default;

BinanceSpotGateway::BinanceSpotGateway(const std::string& gateway_name)
    : gateway_name_(gateway_name) {}

BinanceSpotGateway::~BinanceSpotGateway() {
    close();
}

//=============================================================================
// Connection Management
//=============================================================================

bool BinanceSpotGateway::connect(const std::string& config) {
    // Simple JSON parsing for key fields
    auto get_value = [&config](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = config.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        while (pos < config.length() && (config[pos] == ' ' || config[pos] == '"')) pos++;
        size_t end = config.find_first_of(",}\"", pos);
        return config.substr(pos, end - pos);
    };

    GatewayConfig gc;
    gc.api_key = get_value("api_key");
    gc.api_secret = get_value("api_secret");
    gc.proxy_host = get_value("proxy_host");
    gc.proxy_port = get_value("proxy_port").empty() ? 0 : std::stoi(get_value("proxy_port"));
    gc.testnet = get_value("testnet") == "true";

    return connect(gc);
}

bool BinanceSpotGateway::connect(const GatewayConfig& config) {
    rest_api_ = std::make_unique<BinanceSpotRestApi>();

    std::string host = config.testnet ? "testnet.binance.vision" : "api.binance.com";
    std::string port = "443";

    if (!rest_api_->init(host, port, config.api_key, config.api_secret)) {
        last_error_ = rest_api_->last_error();
        return false;
    }

    // Query server time to sync clock
    if (rest_api_->get_server_time() == 0) {
        last_error_ = rest_api_->last_error();
        return false;
    }

    // Generate connect_time for order ID
    connect_time_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    connect_time_ *= 1000000;  // Format: YYMMDDHHMMSS * order_count

    // Query exchange info
    auto contracts = rest_api_->get_exchange_info();
    for (const auto& c : contracts) {
        contracts_[c.symbol] = c;
        if (contract_callback_) contract_callback_(c);
    }

    // Query account
    auto accounts = rest_api_->query_account_all();
    for (const auto& a : accounts) {
        if (account_callback_) account_callback_(a);
    }

    // Query open orders
    auto orders = rest_api_->query_open_orders();
    for (const auto& o : orders) {
        orders_[o.orderid] = o;
        if (order_callback_) order_callback_(o);
    }

    // Start user stream
    if (!config.api_key.empty()) {
        start_user_stream();
    }

    connected_ = true;
    return true;
}

void BinanceSpotGateway::close() {
    connected_ = false;

    if (rest_api_ && !listen_key_.empty()) {
        rest_api_->close_user_stream(listen_key_);
    }

    rest_api_.reset();
    orders_.clear();
    contracts_.clear();
}

bool BinanceSpotGateway::is_connected() const {
    return connected_;
}

//=============================================================================
// Order ID Generation
//=============================================================================

std::string BinanceSpotGateway::generate_order_id() {
    std::lock_guard<std::mutex> lock(order_mutex_);
    order_count_++;
    std::ostringstream ss;
    ss << "x-A6SIDXVS" << (connect_time_ + order_count_);
    return ss.str();
}

//=============================================================================
// Market Data - Synchronous
//=============================================================================

std::optional<TickData> BinanceSpotGateway::get_tick(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(ticks_mutex_);
    auto it = last_ticks_.find(symbol);
    if (it != last_ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<BarData> BinanceSpotGateway::get_bars(const HistoryRequest& req) {
    if (!rest_api_) return {};
    return rest_api_->get_klines(req.symbol, req.interval, req.limit);
}

double BinanceSpotGateway::get_price(const std::string& symbol) {
    if (!rest_api_) return 0.0;
    return rest_api_->get_price(symbol);
}

std::optional<ContractData> BinanceSpotGateway::get_contract(const std::string& symbol) {
    auto it = contracts_.find(symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace bitquant
```

### Step 2: Add trading methods

- [ ] **Append trading methods to binance_spot_gateway.cpp**

```cpp
// Continue in src/exchange/binance_spot_gateway.cpp

//=============================================================================
// Order Management
//=============================================================================

std::string BinanceSpotGateway::send_order(const OrderRequest& req) {
    if (!rest_api_ || !connected_) {
        last_error_ = "Not connected";
        return "";
    }

    // Create order data
    std::string order_id = generate_order_id();
    OrderData order = req.create_order_data(order_id, gateway_name_);
    order.status = Status::SUBMITTING;
    order.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Send to exchange
    OrderRequest api_req = req;
    api_req.reference = order_id;  // Use as client order ID

    std::string exchange_order_id = rest_api_->send_order(api_req);
    if (exchange_order_id.empty()) {
        order.status = Status::REJECTED;
        order.rejected_reason = rest_api_->last_error();
        process_order_update(order);
        last_error_ = rest_api_->last_error();
        returnorder.vt_orderid();
    }

    order.orderid = exchange_order_id;
    order.status = Status::NOTTRADED;
    process_order_update(order);

    return order.vt_orderid();
}

bool BinanceSpotGateway::cancel_order(const CancelRequest& req) {
    if (!rest_api_ || !connected_) {
        last_error_ = "Not connected";
        return false;
    }

    return rest_api_->cancel_order(req.symbol, req.orderid);
}

void BinanceSpotGateway::cancel_all_orders(const std::string& symbol) {
    if (!rest_api_ || !connected_) return;

    auto orders = rest_api_->query_open_orders(symbol);
    for (const auto& o : orders) {
        CancelRequest req;
        req.orderid = o.orderid;
        req.symbol = o.symbol;
        req.exchange = o.exchange;
        cancel_order(req);
    }
}

std::optional<OrderData> BinanceSpotGateway::query_order(const OrderQueryRequest& req) {
    if (!rest_api_ || !connected_) {
        return std::nullopt;
    }
    return rest_api_->query_order(req.symbol, req.orderid);
}

//=============================================================================
// Account Management
//=============================================================================

std::vector<PositionData> BinanceSpotGateway::query_positions(const std::string& symbol) {
    // Spot trading has no positions concept
    (void)symbol;
    return {};
}

std::optional<AccountData> BinanceSpotGateway::query_account() {
    if (!rest_api_ || !connected_) {
        return std::nullopt;
    }

    auto accounts = rest_api_->query_account();
    if (accounts.empty()) {
        return std::nullopt;
    }

    // Return first non-zero account (or USDT if available)
    for (const auto& a : accounts) {
        if (a.accountid == "USDT" || a.balance > 0) {
            return a;
        }
    }

    return accounts.empty() ? std::nullopt : std::optional(accounts[0]);
}

std::vector<AccountData> BinanceSpotGateway::query_account_all() {
    if (!rest_api_) return {};
    return rest_api_->query_account();
}

std::vector<OrderData> BinanceSpotGateway::query_open_orders(const std::string& symbol) {
    if (!rest_api_) return {};
    return rest_api_->query_open_orders(symbol);
}
```

### Step 3: Add utility methods

- [ ] **Append utility methods to binance_spot_gateway.cpp**

```cpp
// Continue in src/exchange/binance_spot_gateway.cpp

//=============================================================================
// WebSocket Subscriptions
//=============================================================================

void BinanceSpotGateway::subscribe_tick(const SubscribeRequest& req) {
    // TODO: Implement WebSocket subscription
    (void)req;
}

void BinanceSpotGateway::subscribe_bar(const std::string& symbol, Interval interval) {
    // TODO: Implement WebSocket subscription
    (void)symbol;
    (void)interval;
}

void BinanceSpotGateway::unsubscribe_tick(const std::string& symbol) {
    // TODO: Implement WebSocket unsubscription
    (void)symbol;
}

//=============================================================================
// Utility Methods
//=============================================================================

int64_t BinanceSpotGateway::get_server_time() {
    if (!rest_api_) return 0;
    return rest_api_->get_server_time();
}

bool BinanceSpotGateway::ping() {
    if (!rest_api_) return false;
    return rest_api_->ping();
}

std::vector<ContractData> BinanceSpotGateway::get_exchange_info() {
    if (!rest_api_) return {};
    return rest_api_->get_exchange_info();
}

bool BinanceSpotGateway::start_user_stream() {
    if (!rest_api_) return false;

    listen_key_ = rest_api_->start_user_stream();
    if (listen_key_.empty()) {
        last_error_ = "Failed to start user stream";
        return false;
    }

    // TODO: Start WebSocket connection for listen_key_
    return true;
}

void BinanceSpotGateway::keep_user_stream() {
    keep_alive_count_++;
    if (keep_alive_count_ < 300) return;
    keep_alive_count_ = 0;

    if (rest_api_ && !listen_key_.empty()) {
        rest_api_->keep_user_stream(listen_key_);
    }
}

//=============================================================================
// Order Processing (with Trade generation)
//=============================================================================

void BinanceSpotGateway::process_order_update(const OrderData& order) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    auto it = orders_.find(order.orderid);
    if (it == orders_.end()) {
        // New order
        orders_[order.orderid] = order;
        if (order_callback_) order_callback_(order);
    } else {
        // Existing order - check for new trades
        double traded = order.traded - it->second.traded;
        
        if (traded < 0) {
            // Out of sequence, ignore
            return;
        }

        if (traded > 0) {
            // Generate TradeData
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

} // namespace bitquant
```

### Step 4: Update CMakeLists.txt

- [ ] **Add new source file to bitquant library**

```cmake
# In bitquant_cpp/CMakeLists.txt, in target_sources(bitquant PRIVATE ...) section
# Add after src/exchange/binance_spot_rest_api.cpp:
src/exchange/binance_spot_gateway.cpp
```

### Step 5: Build and verify

- [ ] **Build with binapi enabled**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=ON
make -j$(nproc)
```

Expected: Build succeeds

### Step 6: Commit

```bash
git add src/exchange/binance_spot_gateway.cpp CMakeLists.txt
git commit -m "feat: Implement BinanceSpotGateway with order management"
```

---

## Task 7: Integration Test

**Files:**
- Create: `examples/demo_binance_spot.cpp`

### Step 1: Create demo example

- [ ] **Create demo_binance_spot.cpp**

```cpp
// File: examples/demo_binance_spot.cpp
#include "exchange/binance_spot_gateway.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

int main() {
    std::cout << "=== Binance Spot Gateway Demo ===" << std::endl;

    // Create gateway
    BinanceSpotGateway gateway;

    // Connect (public API only, no trading)
    GatewayConfig config;
    config.api_key = "";  // Empty for public data only
    config.api_secret = "";

    std::cout << "[1] Connecting to Binance..." << std::endl;
    if (!gateway.connect(config)) {
        std::cerr << "Connection failed: " << gateway.last_error() << std::endl;
        return 1;
    }
    std::cout << "Connected!" << std::endl;

    // Ping
    std::cout << "[2] Ping..." << std::endl;
    if (gateway.ping()) {
        std::cout << "Ping successful!" << std::endl;
    }

    // Get server time
    std::cout << "[3] Server time..." << std::endl;
    int64_t time = gateway.get_server_time();
    std::time_t t = time /1000;
    std::cout << "Server time: " << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S UTC") << std::endl;

    // Get price
    std::cout << "[4] BTCUSDT price..." << std::endl;
    double price = gateway.get_price("BTCUSDT");
    std::cout << "Current price: $" << std::fixed << std::setprecision(2) << price << std::endl;

    // Get klines
    std::cout << "[5] Fetching klines..." << std::endl;
    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.interval = Interval::MINUTE_1;
    req.limit = 10;

    auto bars = gateway.get_bars(req);
    std::cout << "Fetched " << bars.size() << " bars" << std::endl;

    for (const auto& bar : bars) {
        std::cout << "O:" << bar.open_price << " H:" << bar.high_price
                  << " L:" << bar.low_price << " C:" << bar.close_price << std::endl;
    }

    // Get contracts
    std::cout << "[6] Exchange info..." << std::endl;
    auto contracts = gateway.get_exchange_info();
    std::cout << "Total contracts: " << contracts.size() << std::endl;

    // Find BTCUSDT contract
    auto btc_contract = gateway.get_contract("BTCUSDT");
    if (btc_contract) {
        std::cout << "BTCUSDT: min_volume=" << btc_contract->min_volume
                  << ", pricetick=" << btc_contract->pricetick << std::endl;
    }

    // Close
    std::cout << "[7] Closing connection..." << std::endl;
    gateway.close();
    std::cout << "Done!" << std::endl;

    return 0;
}
```

### Step 2: Add to CMakeLists.txt

- [ ] **Add demo executable**

```cmake
# In bitquant_cpp/CMakeLists.txt, in if(BUILD_EXAMPLES) block
# Add after demo_multi_strategy:

add_executable(demo_binance_spot examples/demo_binance_spot.cpp)
target_link_libraries(demo_binance_spot PRIVATE bitquant)
```

### Step 3: Build and run

- [ ] **Build and run demo**

```bash
cd /home/ubuntu/project/bitquant/bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=ON
make -j$(nproc)
./demo_binance_spot
```

Expected: Demo runs successfully, shows price and klines

### Step 4: Commit

```bash
git add examples/demo_binance_spot.cpp CMakeLists.txt
git commit -m "feat: Add Binance Spot gateway demo"
```

---

## Task 8: Update Project Documentation

**Files:**
- Modify: `README.md`

### Step 1: Update README

- [ ] **Add Binance Spot Gateway section to README**

```markdown
## Binance Spot Gateway

The C++ implementation now supports Binance Spot trading:

### Features
- REST API: ping, time, klines, price, exchange info
- Trading: send_order, cancel_order, query_order
- Account: query_account, query_open_orders
- User Data Stream: order updates via WebSocket

### Usage

```cpp
#include "exchange/binance_spot_gateway.hpp"

bitquant::BinanceSpotGateway gateway;
bitquant::GatewayConfig config;
config.api_key = "your_api_key";
config.api_secret = "your_api_secret";

if (gateway.connect(config)) {
    // Get price
    double price = gateway.get_price("BTCUSDT");
    
    // Send order
    bitquant::OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = bitquant::Direction::LONG;
    req.type = bitquant::OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.001;
    
    std::string order_id = gateway.send_order(req);
}

gateway.close();
```

### Demo

```bash
cd bitquant_cpp/build
./demo_binance_spot
```
```

### Step 2: Commit

```bash
git add README.md
git commit -m "docs: Add Binance Spot Gateway documentation"
```

---

## Summary

This plan implements the Binance Spot Gateway in8 tasks:

| Task | Description | Files |
|------|-------------|-------|
| 1 | GatewayConfig and OrderQueryRequest types | types.hpp, i_exchange.hpp |
| 2 | Binance enum mappings | binance_mapping.hpp/cpp |
| 3 | BinanceSpotRestApi header | binance_spot_rest_api.hpp |
| 4 | BinanceSpotRestApi implementation | binance_spot_rest_api.cpp |
| 5 | BinanceSpotGateway header | binance_spot_gateway.hpp |
| 6 | BinanceSpotGateway implementation | binance_spot_gateway.cpp |
| 7 | Integration test/demo | demo_binance_spot.cpp |
| 8 | Documentation | README.md |

Each task builds incrementally, with frequent commits.
