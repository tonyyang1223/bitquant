/**
 * @file types.hpp
 * @brief Core data types for BitQuant C++ framework
 *
 * Based on howtrader design patterns from https://github.com/51bitquant/howtrader
 * 微信：bitquant51
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace bitquant {

// Type aliases
using timestamp_t = std::chrono::milliseconds;
using order_id_t = uint64_t;
using trade_id_t = uint64_t;

//=============================================================================
// Enumerations (Based on howtrader/trader/constant.py)
//=============================================================================

/**
 * @brief Direction of order/trade/position
 */
enum class Direction : uint8_t {
    LONG,   // Long position / Buy
    SHORT,  // Short position / Sell
    NET     // Net position (for futures)
};

/**
 * @brief Offset of order/trade (开平仓标志)
 */
enum class Offset : uint8_t {
    NONE,         // No offset (spot trading)
    OPEN,         // Open position (开仓)
    CLOSE,        // Close position (平仓)
    CLOSETODAY,   // Close today (平今)
    CLOSEYESTERDAY // Close yesterday (平昨)
};

/**
 * @brief Order status
 */
enum class Status : uint8_t {
    SUBMITTING,      // Submitting to exchange
    NOTTRADED,       // Not traded yet (pending)
    PARTTRADED,      // Partially traded
    ALLTRADED,       // Fully traded
    CANCELLED,       // Cancelled
    REJECTED         // Rejected by exchange
};

/**
 * @brief Order type
 */
enum class OrderType : uint8_t {
    LIMIT,       // Limit order
    TAKER,       // Taker order (market)
    MAKER,       // Maker order (post-only)
    STOP,        // Stop market order
    STOP_LIMIT,  // Stop limit order
    FAK,         // Fill and Kill
    FOK          // Fill or Kill
};

/**
 * @brief Exchange enumeration
 */
enum class Exchange : uint8_t {
    // CryptoCurrency
    BINANCE,
    OKX,
    BYBIT,
    BITFINEX,
    // Local/Simulation
    LOCAL,
    // Chinese exchanges (for reference)
    SHFE,
    DCE,
    CZCE,
    CFFEX,
    INE
};

/**
 * @brief Interval for bar data
 */
enum class Interval : uint8_t {
    TICK,
    MINUTE_1,
    MINUTE_3,
    MINUTE_5,
    MINUTE_15,
    MINUTE_30,
    HOUR_1,
    HOUR_2,
    HOUR_4,
    HOUR_6,
    HOUR_12,
    DAILY,
    WEEKLY,
    MONTHLY
};

/**
 * @brief Product class
 */
enum class Product : uint8_t {
    SPOT,
    FUTURES,
    OPTION,
    INDEX,
    ETF
};

//=============================================================================
// Active Status Set
//=============================================================================

/**
 * @brief Check if status is active (pending execution)
 */
inline bool is_active_status(Status status) {
    return status == Status::SUBMITTING ||
           status == Status::NOTTRADED ||
           status == Status::PARTTRADED;
}

//=============================================================================
// Enum Conversion Functions
//=============================================================================

/**
 * @brief Convert Exchange enum to string
 */
std::string exchange_to_string(Exchange exchange);

/**
 * @brief Convert string to Exchange enum
 */
Exchange exchange_from_string(const std::string& str);

/**
 * @brief Convert Interval enum to string
 */
std::string interval_to_string(Interval interval);

/**
 * @brief Convert string to Interval enum
 */
Interval interval_from_string(const std::string& str);

/**
 * @brief Convert Direction enum to string
 */
std::string direction_to_string(Direction direction);

/**
 * @brief Convert string to Direction enum
 */
Direction direction_from_string(const std::string& str);

//=============================================================================
// Data Structures (Based on howtrader/trader/object.py)
//=============================================================================

/**
 * @brief K-line bar data structure
 */
struct BarData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    Interval interval = Interval::MINUTE_1;
    int64_t datetime = 0;    // Milliseconds since epoch

    double open_price = 0.0;
    double high_price = 0.0;
    double low_price = 0.0;
    double close_price = 0.0;
    double volume = 0.0;
    double turnover = 0.0;       // Turnover (成交额)
    double open_interest = 0.0;  // Open interest (持仓量)

    BarData() = default;

    BarData(int64_t dt, double o, double h, double l, double c, double v)
        : datetime(dt), open_price(o), high_price(h),
          low_price(l), close_price(c), volume(v) {}

    // VT symbol (symbol.exchange)
    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }

    // Typical price for calculations
    double typical_price() const {
        return (high_price + low_price + close_price) / 3.0;
    }

    // Range
    double range() const {
        return high_price - low_price;
    }

    // Body size
    double body() const {
        return std::abs(close_price - open_price);
    }

    // Is bullish
    bool is_bullish() const {
        return close_price > open_price;
    }
};

/**
 * @brief Tick data structure (Level 1 market data)
 */
struct TickData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    int64_t datetime = 0;

    std::string name;
    double last_price = 0.0;
    double volume = 0.0;
    double turnover = 0.0;
    double open_interest = 0.0;

    double limit_up = 0.0;
    double limit_down = 0.0;

    double open_price = 0.0;
    double high_price = 0.0;
    double low_price = 0.0;
    double pre_close = 0.0;

    // Bid/Ask levels (Level 1)
    double bid_price_1 = 0.0;
    double bid_volume_1 = 0.0;
    double ask_price_1 = 0.0;
    double ask_volume_1 = 0.0;

    // Price smoother metadata (for outlier detection)
    bool is_anomaly = false;         // Flag for detected anomaly
    double smoothed_price = 0.0;     // Smoothed price from PriceSmoother

    // VT symbol
    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

/**
 * @brief Order structure
 */
struct OrderData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    std::string orderid;
    std::string gateway_name;

    OrderType type = OrderType::LIMIT;
    Direction direction = Direction::LONG;
    Offset offset = Offset::NONE;
    Status status = Status::SUBMITTING;

    double price = 0.0;
    double volume = 0.0;
    double traded = 0.0;
    double traded_price = 0.0;

    int64_t datetime = 0;
    int64_t update_time = 0;

    std::string reference;
    std::string rejected_reason;

    // VT order ID
    std::string vt_orderid() const {
        return gateway_name + "." + orderid;
    }

    // VT symbol
    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }

    // Is order active
    bool is_active() const {
        return is_active_status(status);
    }

    // Remaining volume
    double remaining_volume() const {
        return volume - traded;
    }
};

/**
 * @brief Trade/execution structure
 */
struct TradeData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    std::string orderid;
    std::string tradeid;
    std::string gateway_name;

    Direction direction = Direction::LONG;
    Offset offset = Offset::NONE;

    double price = 0.0;
    double volume = 0.0;
    int64_t datetime = 0;

    // VT IDs
    std::string vt_orderid() const {
        return gateway_name + "." + orderid;
    }

    std::string vt_tradeid() const {
        return gateway_name + "." + tradeid;
    }

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }

    // Trade value
    double value() const {
        return price * volume;
    }
};

/**
 * @brief Position structure
 */
struct PositionData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    Direction direction = Direction::NET;
    std::string gateway_name;

    double volume = 0.0;
    double frozen = 0.0;       // Frozen volume for pending orders
    double price = 0.0;        // Average price
    double liquidation_price = 0.0;
    int leverage = 1;
    double pnl = 0.0;          // Unrealized PnL
    double yd_volume = 0.0;    // Yesterday's volume (for T+1)

    std::string vt_positionid() const {
        return vt_symbol() + "." + std::to_string(static_cast<int>(direction));
    }

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

/**
 * @brief Account data structure
 */
struct AccountData {
    std::string accountid;
    std::string gateway_name;

    double balance = 0.0;
    double frozen = 0.0;

    double available() const {
        return balance - frozen;
    }

    std::string vt_accountid() const {
        return gateway_name + "." + accountid;
    }
};

/**
 * @brief Contract data (交易合约信息)
 */
struct ContractData {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    std::string name;
    std::string gateway_name;

    Product product = Product::SPOT;
    double size = 1.0;          // Contract size
    double pricetick = 0.0;     // Minimum price tick
    double min_notional = 1.0;  // Minimum order value
    double min_size = 1.0;      // Minimum order size
    double min_volume = 1.0;    // Minimum trading volume

    bool stop_supported = false;
    bool net_position = false;
    bool history_data = false;

    // Futures-specific fields
    bool inverse = false;           // 是否为反向合约（币本位）
    double contract_size = 1.0;     // 合约面值（反向合约使用）
    std::string settlement_currency; // 结算货币（如 "BTC", "USDT"）

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

/**
 * @brief Log data structure
 */
struct LogData {
    std::string msg;
    std::string gateway_name;
    int64_t datetime = 0;
    int level = 0;  // 0=INFO, 1=WARNING, 2=ERROR
};

//=============================================================================
// Request Structures
//=============================================================================

/**
 * @brief Order request
 */
struct OrderRequest {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    Direction direction = Direction::LONG;
    OrderType type = OrderType::LIMIT;
    Offset offset = Offset::NONE;

    double volume = 0.0;
    double price = 0.0;
    std::string reference;

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }

    // Create order data from request
    OrderData create_order_data(const std::string& orderid, const std::string& gateway_name) const {
        OrderData order;
        order.symbol = symbol;
        order.exchange = exchange;
        order.orderid = orderid;
        order.gateway_name = gateway_name;
        order.type = type;
        order.direction = direction;
        order.offset = offset;
        order.price = price;
        order.volume = volume;
        order.reference = reference;
        order.status = Status::SUBMITTING;
        return order;
    }
};

/**
 * @brief Cancel request
 */
struct CancelRequest {
    std::string orderid;
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

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

/**
 * @brief Subscribe request
 */
struct SubscribeRequest {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

/**
 * @brief History data request
 */
struct HistoryRequest {
    std::string symbol;
    Exchange exchange = Exchange::LOCAL;
    Interval interval = Interval::MINUTE_1;

    int64_t start = 0;
    int64_t end = 0;
    int limit = 1000;

    std::string vt_symbol() const {
        return symbol + "." + std::to_string(static_cast<int>(exchange));
    }
};

//=============================================================================
// Stop Order (for backtesting)
//=============================================================================

/**
 * @brief Stop order status
 */
enum class StopOrderStatus : uint8_t {
    WAITING,
    CANCELLED,
    TRIGGERED
};

/**
 * @brief Stop order structure (for backtesting)
 */
struct StopOrder {
    std::string vt_symbol;
    Direction direction = Direction::LONG;
    Offset offset = Offset::NONE;
    double price = 0.0;
    double volume = 0.0;
    int64_t datetime = 0;

    std::string stop_orderid;
    std::string strategy_name;
    StopOrderStatus status = StopOrderStatus::WAITING;

    std::vector<std::string> vt_orderids;  // Generated limit orders
};

//=============================================================================
// Performance Metrics
//=============================================================================

/**
 * @brief Performance metrics structure
 */
struct PerformanceMetrics {
    // Basic metrics
    int total_trades = 0;
    int winning_trades = 0;
    int losing_trades = 0;
    double win_rate = 0.0;

    // Returns
    double initial_capital = 0.0;
    double final_capital = 0.0;
    double total_return = 0.0;
    double annualized_return = 0.0;

    // Risk metrics
    double max_drawdown = 0.0;
    double max_drawdown_percent = 0.0;
    int max_drawdown_duration = 0;
    double sharpe_ratio = 0.0;
    double sortino_ratio = 0.0;
    double calmar_ratio = 0.0;

    // Trade statistics
    double avg_win = 0.0;
    double avg_loss = 0.0;
    double profit_factor = 0.0;
    double avg_trade = 0.0;
    int max_consecutive_wins = 0;
    int max_consecutive_losses = 0;

    // Daily statistics
    int total_days = 0;
    int profit_days = 0;
    int loss_days = 0;

    // Costs
    double total_commission = 0.0;
    double total_slippage = 0.0;
    double total_turnover = 0.0;

    std::string to_string() const;
};

//=============================================================================
// Legacy Compatibility Types (for existing code)
//=============================================================================

// Legacy Order struct for backward compatibility
struct Order {
    order_id_t order_id = 0;
    Direction side = Direction::LONG;  // Changed from OrderSide
    OrderType type = OrderType::LIMIT;
    Status status = Status::SUBMITTING;
    double price = 0.0;
    double volume = 0.0;
    double filled_volume = 0.0;
    int64_t create_time = 0;
    int64_t update_time = 0;
    double stop_price = 0.0;

    Order() = default;

    double remaining_volume() const {
        return volume - filled_volume;
    }

    bool is_fully_filled() const {
        return filled_volume >= volume;
    }

    // Check if order is active
    bool is_active() const {
        return is_active_status(status);
    }
};

// Legacy Trade struct for backward compatibility
struct Trade {
    trade_id_t trade_id = 0;
    order_id_t order_id = 0;
    Direction side = Direction::LONG;
    double price = 0.0;
    double volume = 0.0;
    double commission = 0.0;
    int64_t timestamp = 0;

    Trade() = default;

    double value() const {
        return price * volume;
    }
};

// Legacy Position struct for backward compatibility
struct Position {
    Direction side = Direction::NET;
    double volume = 0.0;
    double avg_price = 0.0;
    double unrealized_pnl = 0.0;
    double realized_pnl = 0.0;

    Position() = default;

    bool is_flat() const {
        return volume == 0.0;
    }

    bool is_long() const {
        return side == Direction::LONG && volume > 0.0;
    }

    bool is_short() const {
        return side == Direction::SHORT && volume > 0.0;
    }

    double market_value(double current_price) const {
        return current_price * volume;
    }

    void update_unrealized_pnl(double current_price) {
        if (is_flat()) {
            unrealized_pnl = 0.0;
            return;
        }

        if (side == Direction::LONG) {
            unrealized_pnl = (current_price - avg_price) * volume;
        } else {
            unrealized_pnl = (avg_price - current_price) * volume;
        }
    }
};

} // namespace bitquant
