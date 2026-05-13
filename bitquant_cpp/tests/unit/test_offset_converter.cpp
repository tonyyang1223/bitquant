/**
 * @file test_offset_converter.cpp
 * @brief Unit tests for OffsetConverter
 */

#include <gtest/gtest.h>
#include "exchange/offset_converter.hpp"
#include "core/types.hpp"

using namespace bitquant;

//=============================================================================
// Test Fixture
//=============================================================================

class OffsetConverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create converter
        converter = std::make_unique<OffsetConverter>();

        // Register test contract for SHFE
        ContractData shfe_contract;
        shfe_contract.symbol = "rb2310";
        shfe_contract.exchange = Exchange::SHFE;
        shfe_contract.net_position = false;
        converter->register_contract(shfe_contract);

        // Register test contract for non-SHFE
        ContractData binance_contract;
        binance_contract.symbol = "BTCUSDT";
        binance_contract.exchange = Exchange::BINANCE;
        binance_contract.net_position = false;
        converter->register_contract(binance_contract);
    }

    std::unique_ptr<OffsetConverter> converter;
};

//=============================================================================
// PositionHolding Tests
//=============================================================================

TEST_F(OffsetConverterTest, PositionHolding_UpdatePosition_Long) {
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;

    PositionHolding holding(contract);

    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::LONG;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;

    holding.update_position(pos);

    EXPECT_DOUBLE_EQ(holding.long_pos(), 10.0);
    EXPECT_DOUBLE_EQ(holding.long_yd(), 3.0);
    EXPECT_DOUBLE_EQ(holding.long_td(), 7.0);
    EXPECT_DOUBLE_EQ(holding.short_pos(), 0.0);
}

TEST_F(OffsetConverterTest, PositionHolding_UpdatePosition_Short) {
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;

    PositionHolding holding(contract);

    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 15.0;
    pos.yd_volume = 5.0;

    holding.update_position(pos);

    EXPECT_DOUBLE_EQ(holding.short_pos(), 15.0);
    EXPECT_DOUBLE_EQ(holding.short_yd(), 5.0);
    EXPECT_DOUBLE_EQ(holding.short_td(), 10.0);
    EXPECT_DOUBLE_EQ(holding.long_pos(), 0.0);
}

TEST_F(OffsetConverterTest, PositionHolding_UpdateTrade_OpenLong) {
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;

    PositionHolding holding(contract);

    TradeData trade;
    trade.symbol = "rb2310";
    trade.exchange = Exchange::SHFE;
    trade.direction = Direction::LONG;
    trade.offset = Offset::OPEN;
    trade.volume = 5.0;

    holding.update_trade(trade);

    EXPECT_DOUBLE_EQ(holding.long_td(), 5.0);
    EXPECT_DOUBLE_EQ(holding.long_pos(), 5.0);
}

TEST_F(OffsetConverterTest, PositionHolding_UpdateTrade_CloseToday) {
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;

    PositionHolding holding(contract);

    // Setup initial position
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    holding.update_position(pos);

    // Close today
    TradeData trade;
    trade.symbol = "rb2310";
    trade.exchange = Exchange::SHFE;
    trade.direction = Direction::LONG;
    trade.offset = Offset::CLOSETODAY;
    trade.volume = 2.0;

    holding.update_trade(trade);

    EXPECT_DOUBLE_EQ(holding.short_td(), 5.0);  // 7 - 2 = 5
    EXPECT_DOUBLE_EQ(holding.short_yd(), 3.0);
    EXPECT_DOUBLE_EQ(holding.short_pos(), 8.0);
}

//=============================================================================
// SHFE Conversion Tests
//=============================================================================

TEST_F(OffsetConverterTest, SHFE_Convert_OpenPosition) {
    // Setup position
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Open position - no conversion
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::OPEN;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].offset, Offset::OPEN);
    EXPECT_DOUBLE_EQ(result[0].volume, 5.0);
}

TEST_F(OffsetConverterTest, SHFE_Convert_CloseTodayOnly) {
    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Close 5 (all from today)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 5.0);
}

TEST_F(OffsetConverterTest, SHFE_Convert_CloseTodayAndYesterday) {
    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Close 9 (7 from today, 2 from yesterday)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 9.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 7.0);
    EXPECT_EQ(result[1].offset, Offset::CLOSEYESTERDAY);
    EXPECT_DOUBLE_EQ(result[1].volume, 2.0);
}

TEST_F(OffsetConverterTest, SHFE_Convert_ExceedAvailable) {
    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Try to close 15 (exceeds available)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 15.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req);

    EXPECT_EQ(result.size(), 0);  // Empty - not enough position
}

//=============================================================================
// Lock Mode Tests
//=============================================================================

TEST_F(OffsetConverterTest, LockMode_WithTodayPosition) {
    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Lock mode - should use OPEN
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req, true, false);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].offset, Offset::OPEN);  // Lock position
    EXPECT_DOUBLE_EQ(result[0].volume, 5.0);
}

TEST_F(OffsetConverterTest, LockMode_NoTodayPosition) {
    // Setup position: short 3, td=0, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 3.0;
    pos.yd_volume = 3.0;  // All yesterday
    converter->update_position(pos);

    // Lock mode - should close yd first, then open
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req, true, false);

    ASSERT_EQ(result.size(), 2);
    // First: close yesterday (3)
    EXPECT_EQ(result[0].offset, Offset::CLOSEYESTERDAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 3.0);
    // Second: open new (2)
    EXPECT_EQ(result[1].offset, Offset::OPEN);
    EXPECT_DOUBLE_EQ(result[1].volume, 2.0);
}

//=============================================================================
// Net Mode Tests
//=============================================================================

TEST_F(OffsetConverterTest, NetMode_CloseThenOpen) {
    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter->update_position(pos);

    // Net mode - close existing, then open new
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 15.0;
    req.price = 3500.0;

    auto result = converter->convert_order_request(req, false, true);

    ASSERT_EQ(result.size(), 3);
    // Close today (7)
    EXPECT_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 7.0);
    // Close yesterday (3)
    EXPECT_EQ(result[1].offset, Offset::CLOSEYESTERDAY);
    EXPECT_DOUBLE_EQ(result[1].volume, 3.0);
    // Open new (5)
    EXPECT_EQ(result[2].offset, Offset::OPEN);
    EXPECT_DOUBLE_EQ(result[2].volume, 5.0);
}

//=============================================================================
// Non-SHFE Exchange Tests
//=============================================================================

TEST_F(OffsetConverterTest, NonSHFE_NoConversion) {
    // For non-SHFE/INE exchanges, no today/yesterday split
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 50000.0;

    auto result = converter->convert_order_request(req);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].offset, Offset::CLOSE);  // No conversion
}

//=============================================================================
// Frozen Volume Tests
//=============================================================================

TEST_F(OffsetConverterTest, FrozenVolume_CloseOrder) {
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;

    PositionHolding holding(contract);

    // Setup position
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::LONG;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    holding.update_position(pos);

    // Place close order
    OrderData order;
    order.symbol = "rb2310";
    order.exchange = Exchange::SHFE;
    order.orderid = "12345";
    order.gateway_name = "SHFE";
    order.direction = Direction::SHORT;
    order.offset = Offset::CLOSETODAY;
    order.volume = 5.0;
    order.traded = 0.0;
    order.status = Status::NOTTRADED;
    holding.update_order(order);

    // Check frozen
    EXPECT_DOUBLE_EQ(holding.long_td_frozen(), 5.0);
    EXPECT_DOUBLE_EQ(holding.long_pos_frozen(), 5.0);
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}