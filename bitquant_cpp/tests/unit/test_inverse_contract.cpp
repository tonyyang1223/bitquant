/**
 * @file test_inverse_contract.cpp
 * @brief Test inverse contract PnL calculations
 */

#include "exchange/inverse_contract.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>

using namespace bitquant;

#define EXPECT_NEAR(a, b, eps) do { \
    if (std::abs((a) - (b)) > eps) { \
        std::cerr << "FAIL: " << (a) << " != " << (b) << " (eps=" << eps << ")" << std::endl; \
        return false; \
    } \
} while(0)

#define EXPECT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAIL: " << (a) << " != " << (b) << std::endl; \
        return false; \
    } \
} while(0)

bool test_linear_pnl_long() {
    std::cout << "[Test] Linear PnL - Long Position" << std::endl;

    // USDT-margined BTCUSDT
    // Entry: 50000, Exit: 55000, Volume: 1 BTC
    double pnl = calculate_pnl(false, 50000.0, 55000.0, 1.0, 1.0, Direction::LONG);

    // Expected: 1 * (55000 - 50000) = 5000 USDT
    EXPECT_NEAR(pnl, 5000.0, 0.01);

    std::cout << "  PASS: PnL = " << pnl << " USDT" << std::endl;
    return true;
}

bool test_linear_pnl_short() {
    std::cout << "[Test] Linear PnL - Short Position" << std::endl;

    // USDT-margined BTCUSDT
    // Entry: 50000, Exit: 45000, Volume: 1 BTC
    double pnl = calculate_pnl(false, 50000.0, 45000.0, 1.0, 1.0, Direction::SHORT);

    // Expected: 1 * (50000 - 45000) = 5000 USDT
    EXPECT_NEAR(pnl, 5000.0, 0.01);

    std::cout << "  PASS: PnL = " << pnl << " USDT" << std::endl;
    return true;
}

bool test_inverse_pnl_long() {
    std::cout << "[Test] Inverse PnL - Long Position" << std::endl;

    // Coin-margined BTCUSD_PERP
    // Entry: 50000 USD/BTC, Exit: 55000 USD/BTC
    // Volume: 1 contract (100 USD face value)
    double pnl = calculate_pnl(true, 50000.0, 55000.0, 1.0, 100.0, Direction::LONG);

    // Expected: 1 * 100 * (1/50000 - 1/55000) = 100 * (0.00002 - 0.00001818) = 0.001818 BTC
    // Profit when price rises (for inverse long)
    double expected = 100.0 * (1.0/50000.0 - 1.0/55000.0);
    EXPECT_NEAR(pnl, expected, 0.0001);

    std::cout << "  PASS: PnL = " << std::fixed << std::setprecision(6) << pnl << " BTC" << std::endl;
    return true;
}

bool test_inverse_pnl_short() {
    std::cout << "[Test] Inverse PnL - Short Position" << std::endl;

    // Coin-margined BTCUSD_PERP
    // Entry: 50000 USD/BTC, Exit: 45000 USD/BTC
    // Volume: 1 contract (100 USD face value)
    double pnl = calculate_pnl(true, 50000.0, 45000.0, 1.0, 100.0, Direction::SHORT);

    // Expected: -1 * 100 * (1/50000 - 1/45000) = -100 * (0.00002 - 0.00002222) = 0.000222 BTC
    // Profit when price falls (for inverse short)
    double expected = -100.0 * (1.0/50000.0 - 1.0/45000.0);
    EXPECT_NEAR(pnl, expected, 0.0001);

    std::cout << "  PASS: PnL = " << std::fixed << std::setprecision(6) << pnl << " BTC" << std::endl;
    return true;
}

bool test_contract_size() {
    std::cout << "[Test] Contract Size" << std::endl;

    // BTCUSD contracts: 100 USD
    EXPECT_NEAR(get_contract_size("BTCUSD_PERP"), 100.0, 0.01);

    // ETHUSD contracts: 10 USD
    EXPECT_NEAR(get_contract_size("ETHUSD_PERP"), 10.0, 0.01);

    // USDT-margined: 1 (no contract size concept)
    EXPECT_NEAR(get_contract_size("BTCUSDT"), 1.0, 0.01);

    std::cout << "  PASS: Contract sizes correct" << std::endl;
    return true;
}

bool test_inverse_detection() {
    std::cout << "[Test] Inverse Contract Detection" << std::endl;

    // BTCUSD_PERP is inverse
    EXPECT_EQ(is_inverse_contract("BTCUSD_PERP"), true);

    // BTCUSDT is NOT inverse (linear)
    EXPECT_EQ(is_inverse_contract("BTCUSDT"), false);

    // BTCUSD_240927 is inverse (quarterly)
    EXPECT_EQ(is_inverse_contract("BTCUSD_240927"), true);

    std::cout << "  PASS: Detection works correctly" << std::endl;
    return true;
}

bool test_settlement_currency() {
    std::cout << "[Test] Settlement Currency" << std::endl;

    // BTCUSD_PERP settles in BTC
    std::string btc_settlement = get_settlement_currency("BTCUSD_PERP", true);
    if (btc_settlement != "BTC") {
        std::cerr << "FAIL: Expected BTC, got " << btc_settlement << std::endl;
        return false;
    }

    // BTCUSDT settles in USDT
    std::string usdt_settlement = get_settlement_currency("BTCUSDT", false);
    if (usdt_settlement != "USDT") {
        std::cerr << "FAIL: Expected USDT, got " << usdt_settlement << std::endl;
        return false;
    }

    std::cout << "  PASS: Settlement currencies correct" << std::endl;
    return true;
}

bool test_position_value() {
    std::cout << "[Test] Position Value Calculation" << std::endl;

    // Linear: 1 BTC at 50000 USDT = 50000 USDT
    double linear_value = calculate_position_value(false, 50000.0, 1.0, 1.0);
    EXPECT_NEAR(linear_value, 50000.0, 0.01);

    // Inverse: 1 contract (100 USD) at 50000 USD/BTC = 100/50000 = 0.002 BTC
    double inverse_value = calculate_position_value(true, 50000.0, 1.0, 100.0);
    EXPECT_NEAR(inverse_value, 0.002, 0.0001);

    std::cout << "  PASS: Position values correct" << std::endl;
    return true;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Inverse Contract Unit Tests                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_linear_pnl_long,
        test_linear_pnl_short,
        test_inverse_pnl_long,
        test_inverse_pnl_short,
        test_inverse_detection,
        test_contract_size,
        test_settlement_currency,
        test_position_value
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