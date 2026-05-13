/**
 * @file demo_paper_trading_full.cpp
 * @brief Complete paper trading demonstration with StrategyManager integration
 *
 * Demonstrates:
 * - Real-time market data from Binance WebSocket
 * - StrategyManager for strategy coordination
 * - PaperBroker for simulated order execution
 * - RiskManager for risk control
 * - Full integration of all components
 */

#include "engine/paper_broker.hpp"
#include "engine/strategy_manager.hpp"
#include "engine/risk_manager.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "strategy/builtin_strategies.hpp"
#include "data/bar_generator.hpp"
#include <iostream>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

using namespace bitquant;

//=============================================================================
// Global flag for graceful shutdown
//=============================================================================

std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
    std::cout << "\n[Main] Shutdown signal received..." << std::endl;
}

//=============================================================================
// Paper Trading Demo
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     BitQuant Paper Trading - Full Integration Demo         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Step 1: Create PaperBroker
        std::cout << "[1] Creating PaperBroker...\n";
        PaperBrokerConfig broker_config;
        broker_config.initial_capital = 100'000.0;
        broker_config.commission_rate = 0.001;
        broker_config.slippage_rate = 0.0005;

        PaperBroker broker(broker_config);
        std::cout << "    Initial capital: $100,000\n";
        std::cout << "    Commission: 0.1%\n";
        std::cout << "    Slippage: 0.05%\n";

        // Track order and trade updates
        broker.on_order([](const OrderData& order) {
            std::cout << "[Order] " << order.orderid
                      << " status: " << static_cast<int>(order.status)
                      << " filled: " << order.traded << "/" << order.volume
                      << " @ " << order.price
                      << std::endl;
        });

        broker.on_trade([](const TradeData& trade) {
            std::cout << "[Trade] " << trade.tradeid
                      << " " << (trade.direction == Direction::LONG ? "BUY" : "SELL")
                      << " " << trade.volume << " @ " << trade.price
                      << std::endl;
        });

        // Step 2: Create RiskManager
        std::cout << "\n[2] Creating RiskManager...\n";
        RiskConfig risk_config;
        risk_config.active = true;
        risk_config.order_flow_limit = 100;
        risk_config.order_size_limit = 10.0;
        risk_config.active_order_limit = 20;
        risk_config.prevent_self_trade = true;

        RiskManager risk_manager(risk_config);
        risk_manager.set_log_callback([](const std::string& msg) {
            std::cout << "[Risk] " << msg << std::endl;
        });
        std::cout << "    Order flow limit: " << risk_config.order_flow_limit << "/s\n";
        std::cout << "    Order size limit: " << risk_config.order_size_limit << "\n";
        std::cout << "    Active order limit: " << risk_config.active_order_limit << "\n";

        // Step 3: Create StrategyManager
        std::cout << "\n[3] Creating StrategyManager...\n";
        StrategyManager strategy_manager;
        strategy_manager.set_paper_broker(&broker);
        strategy_manager.set_risk_manager(&risk_manager);
        strategy_manager.set_log_callback([](const std::string& msg) {
            std::cout << "[StrategyManager] " << msg << std::endl;
        });

        // Step 4: Add strategy
        std::cout << "\n[4] Adding DoubleMaStrategy...\n";
        auto strategy = std::make_unique<DoubleMaStrategy>();
        std::unordered_map<std::string, double> params;
        params["fast_window"] = 10;
        params["slow_window"] = 20;

        strategy_manager.add_strategy("sma_btc", std::move(strategy), "BTCUSDT.BINANCE", params);
        strategy_manager.init_strategy("sma_btc");
        strategy_manager.start_strategy("sma_btc");
        std::cout << "    Fast MA: 10\n";
        std::cout << "    Slow MA: 20\n";

        // Step 5: Connect to Binance
        std::cout << "\n[5] Connecting to Binance for real-time data...\n";
        BinanceSpotGateway gateway;
        GatewayConfig gw_config;
        gw_config.api_key = "";
        gw_config.api_secret = "";

        if (!gateway.connect(gw_config)) {
            std::cerr << "Failed to connect to Binance" << std::endl;
            return 1;
        }
        std::cout << "    Connected!\n";

        // Step 6: Setup data flow
        std::cout << "\n[6] Setting up data flow...\n";

        // Bar generator for 1-minute bars
        BarGenerator bar_gen(Interval::MINUTE_1, [](const BarData&) {}, 0);

        // Track statistics
        int tick_count = 0;
        int bar_count = 0;
        double last_price = 0.0;
        auto start_time = std::chrono::steady_clock::now();

        // Tick callback
        gateway.on_tick([&](const TickData& tick) {
            tick_count++;
            last_price = tick.last_price;

            // Update broker with tick
            broker.on_tick(tick);

            // Update risk manager
            risk_manager.on_timer();

            // Update bar generator
            bar_gen.on_tick(tick);
        });

        // Bar callback
        gateway.on_bar([&](const BarData& bar) {
            bar_count++;

            // Update broker with bar
            broker.on_bar(bar);

            // Dispatch to strategy manager
            strategy_manager.on_bar(bar);
        });

        // Subscribe to market data
        gateway.subscribe_tick(SubscribeRequest{.symbol = "BTCUSDT"});
        gateway.subscribe_bar("BTCUSDT", Interval::MINUTE_1);

        std::cout << "    Subscribed to BTCUSDT tick and 1m bar\n";

        // Step 7: Run paper trading
        std::cout << "\n[7] Paper trading running...\n";
        std::cout << "    Press Ctrl+C to stop\n\n";

        // Display loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

            double equity = broker.get_equity();
            double position = broker.get_position("BTCUSDT");
            int trades = broker.get_trade_count();

            std::cout << "\r[Live] " << std::setw(4) << elapsed << "s | "
                      << "Ticks: " << std::setw(6) << tick_count << " | "
                      << "Bars: " << std::setw(4) << bar_count << " | "
                      << "BTCUSDT: $" << std::fixed << std::setprecision(2) << last_price << " | "
                      << "Position: " << std::setprecision(4) << position << " | "
                      << "Equity: $" << std::setprecision(2) << equity << " | "
                      << "Trades: " << trades
                      << std::flush;
        }

        // Step 8: Final report
        std::cout << "\n\n[8] Generating final report...\n";

        // Stop strategy
        strategy_manager.stop_strategy("sma_btc");

        // Close gateway
        gateway.close();

        // Final statistics
        auto end_time = std::chrono::steady_clock::now();
        auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                   Final Results                             ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Initial Capital:     $   " << std::setw(10) << std::fixed << std::setprecision(2)
                  << broker_config.initial_capital << "                        ║\n";
        std::cout << "║ Final Equity:       $   " << std::setw(10) << broker.get_equity() << "                        ║\n";
        std::cout << "║ Cash Balance:       $   " << std::setw(10) << broker.get_balance() << "                        ║\n";
        std::cout << "║ Position:                 " << std::setw(8) << std::setprecision(4)
                  << broker.get_position("BTCUSDT") << " BTC                   ║\n";
        std::cout << "║ Total Trades:               " << std::setw(6) << broker.get_trade_count() << "                        ║\n";
        std::cout << "║ Total Commission:   $   " << std::setw(10) << std::setprecision(2)
                  << broker.get_total_commission() << "                        ║\n";
        std::cout << "║ Duration:               " << std::setw(5) << total_elapsed << " seconds               ║\n";
        std::cout << "║ Ticks Received:          " << std::setw(6) << tick_count << "                        ║\n";
        std::cout << "║ Bars Generated:          " << std::setw(6) << bar_count << "                        ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";

        // Strategy stats
        int strategy_trades, strategy_wins;
        double strategy_pnl;
        strategy_manager.get_strategy_stats("sma_btc", strategy_trades, strategy_wins, strategy_pnl);

        std::cout << "\nStrategy Statistics:\n";
        std::cout << "  Total Trades: " << strategy_trades << "\n";
        std::cout << "  Winning Trades: " << strategy_wins << "\n";
        std::cout << "  Total PnL: $" << std::fixed << std::setprecision(2) << strategy_pnl << "\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
