/**
 * @file demo_main_engine.cpp
 * @brief MainEngine 功能演示
 *
 * 演示如何使用 MainEngine 进行：
 * - Gateway 管理
 * - 事件驱动架构
 * - OMS (订单管理系统)
 * - 风险管理
 */

#include "engine/main_engine.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "core/types.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

// 简单的测试策略回调
void on_tick_callback(const TickData& tick) {
    std::cout << "[Tick] " << tick.symbol
              << " Price: " << std::fixed << std::setprecision(2) << tick.last_price
              << " Vol: " << tick.volume << std::endl;
}

void on_order_callback(const OrderData& order) {
    std::cout << "[Order] " << order.orderid
              << " " << order.symbol
              << " " << direction_to_string(order.direction)
              << " Status: " << static_cast<int>(order.status) << std::endl;
}

void on_trade_callback(const TradeData& trade) {
    std::cout << "[Trade] " << trade.tradeid
              << " " << trade.symbol
              << " Price: " << trade.price
              << " Vol: " << trade.volume << std::endl;
}

int main() {
    std::cout << "=== MainEngine Demo ===" << std::endl << std::endl;

    // 1. 创建 MainEngine
    std::cout << "--- Step 1: Create MainEngine ---" << std::endl;

    MainEngineConfig config;
    config.log_to_console = true;
    config.enable_risk_manager = true;
    config.order_flow_limit = 10;
    config.order_size_limit = 1.0;

    MainEngine engine(config);
    engine.start();

    std::cout << "MainEngine started" << std::endl << std::endl;

    // 2. 注册事件回调
    std::cout << "--- Step 2: Register Event Handlers ---" << std::endl;

    engine.event_engine()->register_handler(EVENT_TICK, [](const Event& e) {
        try {
            const TickData& tick = std::any_cast<const TickData&>(e.data());
            on_tick_callback(tick);
        } catch (...) {}
    });

    engine.event_engine()->register_handler(EVENT_ORDER, [](const Event& e) {
        try {
            const OrderData& order = std::any_cast<const OrderData&>(e.data());
            on_order_callback(order);
        } catch (...) {}
    });

    engine.event_engine()->register_handler(EVENT_TRADE, [](const Event& e) {
        try {
            const TradeData& trade = std::any_cast<const TradeData&>(e.data());
            on_trade_callback(trade);
        } catch (...) {}
    });

    std::cout << "Event handlers registered" << std::endl << std::endl;

    // 3. 添加 Gateway
    std::cout << "--- Step 3: Add Gateway ---" << std::endl;

    auto gateway = std::make_unique<BinanceSpotGateway>();
    std::cout << "Gateway name: " << gateway->gateway_name() << std::endl;
    std::cout << "Supported exchanges: ";
    for (auto ex : gateway->supported_exchanges()) {
        std::cout << exchange_to_string(ex) << " ";
    }
    std::cout << std::endl;

    engine.add_gateway("BINANCE_SPOT", std::move(gateway));
    std::cout << "Gateway added" << std::endl << std::endl;

    // 4. 获取 Gateway 列表
    std::cout << "--- Step 4: Gateway List ---" << std::endl;

    auto gateway_names = engine.get_gateway_names();
    std::cout << "Available gateways: ";
    for (const auto& name : gateway_names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;

    auto exchanges = engine.get_all_exchanges();
    std::cout << "Supported exchanges: ";
    for (auto ex : exchanges) {
        std::cout << exchange_to_string(ex) << " ";
    }
    std::cout << std::endl << std::endl;

    // 5. 模拟事件处理
    std::cout << "--- Step 5: Simulate Events ---" << std::endl;

    // 模拟 Tick 事件
    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    tick.last_price = 42000.50;
    tick.volume = 100.0;
    tick.bid_price_1 = 42000.00;
    tick.ask_price_1 = 42001.00;

    engine.event_engine()->put(Event(EVENT_TICK, tick));

    // 模拟 Order 事件
    OrderData order;
    order.symbol = "BTCUSDT";
    order.exchange = Exchange::BINANCE;
    order.orderid = "ORDER_001";
    order.gateway_name = "BINANCE_SPOT";
    order.direction = Direction::LONG;
    order.price = 42000.00;
    order.volume = 0.1;
    order.status = Status::NOTTRADED;

    engine.event_engine()->put(Event(EVENT_ORDER, order));

    // 等待事件处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << std::endl;

    // 6. 查询 OMS 数据
    std::cout << "--- Step 6: Query OMS Data ---" << std::endl;

    auto ticks = engine.get_all_ticks();
    std::cout << "Ticks in OMS: " << ticks.size() << std::endl;

    auto orders = engine.get_all_orders();
    std::cout << "Orders in OMS: " << orders.size() << std::endl;

    auto active_orders = engine.get_all_active_orders();
    std::cout << "Active orders: " << active_orders.size() << std::endl;

    // 查询特定 tick
    auto stored_tick = engine.get_tick("BTCUSDT");
    if (stored_tick) {
        std::cout << "Stored tick price: " << stored_tick->last_price << std::endl;
    }

    std::cout << std::endl;

    // 7. 测试风险管理
    std::cout << "--- Step 7: Risk Management Test ---" << std::endl;

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.direction = Direction::LONG;
    req.volume = 2.0;  // 超过限制
    req.price = 42000.0;

    std::cout << "Order volume: " << req.volume << " (limit: 1.0)" << std::endl;
    std::cout << "Risk check result: " << (engine.risk_manager()->check_risk(req, "BINANCE_SPOT") ? "PASS" : "REJECT") << std::endl;

    req.volume = 0.5;  // 在限制内
    std::cout << "Order volume: " << req.volume << " (limit: 1.0)" << std::endl;
    std::cout << "Risk check result: " << (engine.risk_manager()->check_risk(req, "BINANCE_SPOT") ? "PASS" : "REJECT") << std::endl;

    std::cout << std::endl;

    // 8. 停止引擎
    std::cout << "--- Step 8: Stop Engine ---" << std::endl;

    engine.stop();
    std::cout << "MainEngine stopped" << std::endl;

    std::cout << std::endl << "MainEngine demo completed!" << std::endl;

    return 0;
}
