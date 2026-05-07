/**
 * @file test_binance_mapping.cpp
 * @brief Unit tests for Binance enum mapping functions
 */

#include <gtest/gtest.h>
#include "exchange/binance_mapping.hpp"

using namespace bitquant;

// Test status mappings
TEST(BinanceMappingTest, StatusFromBinance) {
    EXPECT_EQ(Status::NOTTRADED, status_from_binance("NEW"));
    EXPECT_EQ(Status::PARTTRADED, status_from_binance("PARTIALLY_FILLED"));
    EXPECT_EQ(Status::ALLTRADED, status_from_binance("FILLED"));
    EXPECT_EQ(Status::CANCELLED, status_from_binance("CANCELED"));
    EXPECT_EQ(Status::REJECTED, status_from_binance("REJECTED"));
    EXPECT_EQ(Status::CANCELLED, status_from_binance("EXPIRED"));
}

TEST(BinanceMappingTest, StatusToBinance) {
    EXPECT_EQ("NEW", status_to_binance(Status::NOTTRADED));
    EXPECT_EQ("PARTIALLY_FILLED", status_to_binance(Status::PARTTRADED));
    EXPECT_EQ("FILLED", status_to_binance(Status::ALLTRADED));
    EXPECT_EQ("CANCELED", status_to_binance(Status::CANCELLED));
    EXPECT_EQ("REJECTED", status_to_binance(Status::REJECTED));
}

// Test order type mappings
TEST(BinanceMappingTest, OrderTypeToBinance) {
    EXPECT_EQ("LIMIT", order_type_to_binance(OrderType::LIMIT));
    EXPECT_EQ("MARKET", order_type_to_binance(OrderType::TAKER));
    EXPECT_EQ("LIMIT_MAKER", order_type_to_binance(OrderType::MAKER));
    EXPECT_EQ("STOP_LOSS", order_type_to_binance(OrderType::STOP));
    EXPECT_EQ("STOP_LOSS_LIMIT", order_type_to_binance(OrderType::STOP_LIMIT));
}

TEST(BinanceMappingTest, OrderTypeFromBinance) {
    EXPECT_EQ(OrderType::LIMIT, order_type_from_binance("LIMIT"));
    EXPECT_EQ(OrderType::TAKER, order_type_from_binance("MARKET"));
    EXPECT_EQ(OrderType::MAKER, order_type_from_binance("LIMIT_MAKER"));
    EXPECT_EQ(OrderType::STOP, order_type_from_binance("STOP_LOSS"));
    EXPECT_EQ(OrderType::STOP_LIMIT, order_type_from_binance("STOP_LOSS_LIMIT"));
}

// Test direction mappings
TEST(BinanceMappingTest, DirectionToBinance) {
    EXPECT_EQ("BUY", direction_to_binance(Direction::LONG));
    EXPECT_EQ("SELL", direction_to_binance(Direction::SHORT));
}

TEST(BinanceMappingTest, DirectionFromBinance) {
    EXPECT_EQ(Direction::LONG, direction_from_binance("BUY"));
    EXPECT_EQ(Direction::SHORT, direction_from_binance("SELL"));
}

// Test interval mappings
TEST(BinanceMappingTest, IntervalToBinance) {
    EXPECT_EQ("1m", interval_to_binance(Interval::MINUTE_1));
    EXPECT_EQ("3m", interval_to_binance(Interval::MINUTE_3));
    EXPECT_EQ("5m", interval_to_binance(Interval::MINUTE_5));
    EXPECT_EQ("15m", interval_to_binance(Interval::MINUTE_15));
    EXPECT_EQ("30m", interval_to_binance(Interval::MINUTE_30));
    EXPECT_EQ("1h", interval_to_binance(Interval::HOUR_1));
    EXPECT_EQ("2h", interval_to_binance(Interval::HOUR_2));
    EXPECT_EQ("4h", interval_to_binance(Interval::HOUR_4));
    EXPECT_EQ("6h", interval_to_binance(Interval::HOUR_6));
    EXPECT_EQ("12h", interval_to_binance(Interval::HOUR_12));
    EXPECT_EQ("1d", interval_to_binance(Interval::DAILY));
    EXPECT_EQ("1w", interval_to_binance(Interval::WEEKLY));
    EXPECT_EQ("1M", interval_to_binance(Interval::MONTHLY));
}

TEST(BinanceMappingTest, IntervalFromBinance) {
    EXPECT_EQ(Interval::MINUTE_1, interval_from_binance("1m"));
    EXPECT_EQ(Interval::MINUTE_3, interval_from_binance("3m"));
    EXPECT_EQ(Interval::MINUTE_5, interval_from_binance("5m"));
    EXPECT_EQ(Interval::MINUTE_15, interval_from_binance("15m"));
    EXPECT_EQ(Interval::MINUTE_30, interval_from_binance("30m"));
    EXPECT_EQ(Interval::HOUR_1, interval_from_binance("1h"));
    EXPECT_EQ(Interval::HOUR_2, interval_from_binance("2h"));
    EXPECT_EQ(Interval::HOUR_4, interval_from_binance("4h"));
    EXPECT_EQ(Interval::HOUR_6, interval_from_binance("6h"));
    EXPECT_EQ(Interval::HOUR_12, interval_from_binance("12h"));
    EXPECT_EQ(Interval::DAILY, interval_from_binance("1d"));
    EXPECT_EQ(Interval::WEEKLY, interval_from_binance("1w"));
    EXPECT_EQ(Interval::MONTHLY, interval_from_binance("1M"));
}
