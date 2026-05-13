/**
 * @file test_offset_converter_standalone.cpp
 * @brief Standalone test for OffsetConverter (no GTest dependency)
 */

#include "exchange/offset_converter.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace bitquant;

// Helper to convert Offset to string
std::string offset_to_string(Offset offset) {
    switch (offset) {
        case Offset::NONE: return "NONE";
        case Offset::OPEN: return "OPEN";
        case Offset::CLOSE: return "CLOSE";
        case Offset::CLOSETODAY: return "CLOSETODAY";
        case Offset::CLOSEYESTERDAY: return "CLOSEYESTERDAY";
        default: return "UNKNOWN";
    }
}

#define EXPECT_EQ(a, b) do { if ((a) != (b)) { std::cerr << "FAIL: " << (a) << " != " << (b) << std::endl; return false; } } while(0)
#define EXPECT_DOUBLE_EQ(a, b) do { if (std::abs((a) - (b)) > 1e-9) { std::cerr << "FAIL: " << (a) << " != " << (b) << std::endl; return false; } } while(0)
#define EXPECT_TRUE(x) do { if (!(x)) { std::cerr << "FAIL: " << #x << " is false" << std::endl; return false; } } while(0)
#define ASSERT_EQ(a, b) EXPECT_EQ(a, b)
#define EXPECT_OFFSET_EQ(a, b) do { if ((a) != (b)) { std::cerr << "FAIL: " << offset_to_string(a) << " != " << offset_to_string(b) << std::endl; return false; } } while(0)

//=============================================================================
// Test Functions
//=============================================================================

bool test_position_holding_update_long() {
    std::cout << "[Test] PositionHolding_UpdatePosition_Long" << std::endl;

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

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_position_holding_update_short() {
    std::cout << "[Test] PositionHolding_UpdatePosition_Short" << std::endl;

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

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_shfe_convert_close_today_only() {
    std::cout << "[Test] SHFE_Convert_CloseTodayOnly" << std::endl;

    OffsetConverter converter;

    // Register contract
    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;
    contract.net_position = false;
    converter.register_contract(contract);

    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter.update_position(pos);

    // Close 5 (all from today)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter.convert_order_request(req);

    ASSERT_EQ(result.size(), 1);
    EXPECT_OFFSET_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 5.0);

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_shfe_convert_close_today_and_yesterday() {
    std::cout << "[Test] SHFE_Convert_CloseTodayAndYesterday" << std::endl;

    OffsetConverter converter;

    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;
    contract.net_position = false;
    converter.register_contract(contract);

    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter.update_position(pos);

    // Close 9 (7 from today, 2 from yesterday)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 9.0;
    req.price = 3500.0;

    auto result = converter.convert_order_request(req);

    ASSERT_EQ(result.size(), 2);
    EXPECT_OFFSET_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 7.0);
    EXPECT_OFFSET_EQ(result[1].offset, Offset::CLOSEYESTERDAY);
    EXPECT_DOUBLE_EQ(result[1].volume, 2.0);

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_shfe_convert_exceed_available() {
    std::cout << "[Test] SHFE_Convert_ExceedAvailable" << std::endl;

    OffsetConverter converter;

    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;
    contract.net_position = false;
    converter.register_contract(contract);

    // Setup position: short 10
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter.update_position(pos);

    // Try to close 15 (exceeds available)
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 15.0;
    req.price = 3500.0;

    auto result = converter.convert_order_request(req);

    EXPECT_EQ(result.size(), 0);  // Empty - not enough position

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_lock_mode_with_today_position() {
    std::cout << "[Test] LockMode_WithTodayPosition" << std::endl;

    OffsetConverter converter;

    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;
    contract.net_position = false;
    converter.register_contract(contract);

    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter.update_position(pos);

    // Lock mode - should use OPEN
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 5.0;
    req.price = 3500.0;

    auto result = converter.convert_order_request(req, true, false);  // lock=true

    ASSERT_EQ(result.size(), 1);
    EXPECT_OFFSET_EQ(result[0].offset, Offset::OPEN);  // Lock position
    EXPECT_DOUBLE_EQ(result[0].volume, 5.0);

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_net_mode_close_then_open() {
    std::cout << "[Test] NetMode_CloseThenOpen" << std::endl;

    OffsetConverter converter;

    ContractData contract;
    contract.symbol = "rb2310";
    contract.exchange = Exchange::SHFE;
    contract.net_position = false;
    converter.register_contract(contract);

    // Setup position: short 10, td=7, yd=3
    PositionData pos;
    pos.symbol = "rb2310";
    pos.exchange = Exchange::SHFE;
    pos.direction = Direction::SHORT;
    pos.volume = 10.0;
    pos.yd_volume = 3.0;
    converter.update_position(pos);

    // Net mode - close existing, then open new
    OrderRequest req;
    req.symbol = "rb2310";
    req.exchange = Exchange::SHFE;
    req.direction = Direction::LONG;
    req.offset = Offset::CLOSE;
    req.volume = 15.0;
    req.price = 3500.0;

    auto result = converter.convert_order_request(req, false, true);  // net=true

    ASSERT_EQ(result.size(), 3);
    // Close today (7)
    EXPECT_OFFSET_EQ(result[0].offset, Offset::CLOSETODAY);
    EXPECT_DOUBLE_EQ(result[0].volume, 7.0);
    // Close yesterday (3)
    EXPECT_OFFSET_EQ(result[1].offset, Offset::CLOSEYESTERDAY);
    EXPECT_DOUBLE_EQ(result[1].volume, 3.0);
    // Open new (5)
    EXPECT_OFFSET_EQ(result[2].offset, Offset::OPEN);
    EXPECT_DOUBLE_EQ(result[2].volume, 5.0);

    std::cout << "  PASS" << std::endl;
    return true;
}

bool test_frozen_volume_close_order() {
    std::cout << "[Test] FrozenVolume_CloseOrder" << std::endl;

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

    std::cout << "  PASS" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          OffsetConverter Unit Tests                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_position_holding_update_long,
        test_position_holding_update_short,
        test_shfe_convert_close_today_only,
        test_shfe_convert_close_today_and_yesterday,
        test_shfe_convert_exceed_available,
        test_lock_mode_with_today_position,
        test_net_mode_close_then_open,
        test_frozen_volume_close_order
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}