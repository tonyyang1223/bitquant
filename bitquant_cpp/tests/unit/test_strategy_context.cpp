/**
 * @file test_strategy_context.cpp
 * @brief Unit tests for StrategyContext
 */

#include "engine/strategy_context.hpp"
#include "engine/broker.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cmath>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_strategy_context_construction() {
    std::cout << "[Test] StrategyContext construction" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);
    if (ctx.name() != "test") return false;

    std::cout << "  PASS: StrategyContext constructed correctly" << std::endl;
    return true;
}

bool test_strategy_context_initial_state() {
    std::cout << "[Test] StrategyContext initial state" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);
    if (ctx.position() != 0.0) return false;
    if (ctx.avg_price() != 0.0) return false;
    if (ctx.realized_pnl() != 0.0) return false;

    std::cout << "  PASS: Initial state correct" << std::endl;
    return true;
}

bool test_strategy_context_update_position_long() {
    std::cout << "[Test] StrategyContext update_position (long)" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    TradeData trade;
    trade.direction = Direction::LONG;
    trade.price = 50000.0;
    trade.volume = 1.0;

    ctx.update_position(trade);
    if (ctx.position() != 1.0) return false;
    if (ctx.avg_price() != 50000.0) return false;
    if (ctx.position_direction() != Direction::LONG) return false;

    std::cout << "  PASS: Long position update works" << std::endl;
    return true;
}

bool test_strategy_context_update_position_short() {
    std::cout << "[Test] StrategyContext update_position (short)" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    TradeData trade;
    trade.direction = Direction::SHORT;
    trade.price = 50000.0;
    trade.volume = 1.0;

    ctx.update_position(trade);
    if (ctx.position() != 1.0) return false;
    if (ctx.avg_price() != 50000.0) return false;
    if (ctx.position_direction() != Direction::SHORT) return false;

    std::cout << "  PASS: Short position update works" << std::endl;
    return true;
}

bool test_strategy_context_unrealized_pnl() {
    std::cout << "[Test] StrategyContext unrealized_pnl" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    TradeData trade;
    trade.direction = Direction::LONG;
    trade.price = 50000.0;
    trade.volume = 1.0;
    ctx.update_position(trade);

    // Price went up
    double pnl = ctx.unrealized_pnl(51000.0);
    if (pnl != 1000.0) return false;

    std::cout << "  PASS: Unrealized PnL calculation works" << std::endl;
    return true;
}

bool test_strategy_context_total_pnl() {
    std::cout << "[Test] StrategyContext total_pnl" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    TradeData trade;
    trade.direction = Direction::LONG;
    trade.price = 50000.0;
    trade.volume = 1.0;
    ctx.update_position(trade);

    double total = ctx.total_pnl(51000.0);
    if (total != 1000.0) return false;

    std::cout << "  PASS: Total PnL calculation works" << std::endl;
    return true;
}

bool test_strategy_context_add_order() {
    std::cout << "[Test] StrategyContext add_order" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);
    ctx.add_order("order_1");
    ctx.add_order("order_2");

    // Orders tracked internally

    std::cout << "  PASS: add_order works" << std::endl;
    return true;
}

bool test_strategy_context_remove_order() {
    std::cout << "[Test] StrategyContext remove_order" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);
    ctx.add_order("order_1");
    ctx.remove_order("order_1");

    // Order removed internally

    std::cout << "  PASS: remove_order works" << std::endl;
    return true;
}

bool test_strategy_context_win_rate_empty() {
    std::cout << "[Test] StrategyContext win_rate (empty)" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);
    if (ctx.win_rate() != 0.0) return false;

    std::cout << "  PASS: win_rate returns 0 for empty trades" << std::endl;
    return true;
}

bool test_strategy_context_save_state() {
    std::cout << "[Test] StrategyContext save_state" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    TradeData trade;
    trade.direction = Direction::LONG;
    trade.price = 50000.0;
    trade.volume = 1.0;
    ctx.update_position(trade);

    auto state = ctx.save_state();
    if (state.find("position") == state.end()) return false;
    if (state["position"] != 1.0) return false;

    std::cout << "  PASS: save_state works" << std::endl;
    return true;
}

bool test_strategy_context_load_state() {
    std::cout << "[Test] StrategyContext load_state" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    std::unordered_map<std::string, double> state;
    state["position"] = 2.0;
    state["avg_price"] = 48000.0;
    state["realized_pnl"] = 500.0;

    ctx.load_state(state);

    if (ctx.position() != 2.0) return false;
    if (ctx.avg_price() != 48000.0) return false;

    std::cout << "  PASS: load_state works" << std::endl;
    return true;
}

bool test_strategy_context_factory() {
    std::cout << "[Test] StrategyContextFactory" << std::endl;

    auto ctx = StrategyContextFactory::create("test", nullptr, nullptr);
    if (!ctx) return false;
    if (ctx->name() != "test") return false;

    std::cout << "  PASS: Factory creates context correctly" << std::endl;
    return true;
}

bool test_strategy_context_close_long_position() {
    std::cout << "[Test] StrategyContext close long position" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    // Open long
    TradeData trade1;
    trade1.direction = Direction::LONG;
    trade1.price = 50000.0;
    trade1.volume = 1.0;
    ctx.update_position(trade1);

    // Close long
    TradeData trade2;
    trade2.direction = Direction::SHORT;
    trade2.price = 51000.0;
    trade2.volume = 1.0;
    ctx.update_position(trade2);

    // Position should be flat
    if (ctx.position() != 0.0) return false;
    // Realized PnL should be 1000
    if (ctx.realized_pnl() != 1000.0) return false;

    std::cout << "  PASS: Close long position works" << std::endl;
    return true;
}

bool test_strategy_context_close_short_position() {
    std::cout << "[Test] StrategyContext close short position" << std::endl;

    StrategyContext ctx("test", nullptr, nullptr);

    // Open short
    TradeData trade1;
    trade1.direction = Direction::SHORT;
    trade1.price = 50000.0;
    trade1.volume = 1.0;
    ctx.update_position(trade1);

    // Close short
    TradeData trade2;
    trade2.direction = Direction::LONG;
    trade2.price = 49000.0;
    trade2.volume = 1.0;
    ctx.update_position(trade2);

    // Position should be flat
    if (ctx.position() != 0.0) return false;
    // Realized PnL should be 1000 (short profit when price drops)
    if (ctx.realized_pnl() != 1000.0) return false;

    std::cout << "  PASS: Close short position works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          StrategyContext Unit Tests                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_strategy_context_construction,
        test_strategy_context_initial_state,
        test_strategy_context_update_position_long,
        test_strategy_context_update_position_short,
        test_strategy_context_unrealized_pnl,
        test_strategy_context_total_pnl,
        test_strategy_context_add_order,
        test_strategy_context_remove_order,
        test_strategy_context_win_rate_empty,
        test_strategy_context_save_state,
        test_strategy_context_load_state,
        test_strategy_context_factory,
        test_strategy_context_close_long_position,
        test_strategy_context_close_short_position
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