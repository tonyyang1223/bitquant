/**
 * @file binance_mapping.cpp
 * @brief Implementation of Binance enum mapping functions
 *
 * Based on howtrader design patterns from https://github.com/51bitquant/howtrader
 */

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
        {Status::SUBMITTING, "NEW"},      // Internal state, not yet submitted
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
        {OrderType::STOP_LIMIT, "STOP_LOSS_LIMIT"},
        {OrderType::FAK, "IOC"},          // Fill And Kill -> Immediate Or Cancel
        {OrderType::FOK, "FOK"}           // Fill Or Kill
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
        {"STOP_LOSS_LIMIT", OrderType::STOP_LIMIT},
        {"IOC", OrderType::FAK},          // Immediate Or Cancel -> Fill And Kill
        {"FOK", OrderType::FOK}           // Fill Or Kill
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
