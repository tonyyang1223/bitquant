/**
 * @file demo_grid_martin_paper.cpp
 * @brief Grid Martin Strategy paper trading demo with real-time data
 *
 * Demonstrates:
 * - Real-time market data from Binance WebSocket
 * - GridMartinStrategy with paper trading
 * - PaperBroker for simulated order execution
 * - Full logging for debugging
 * - Support for Binance Testnet (simulated trading)
 *
 * Usage:
 *   Production: ./demo_grid_martin_paper
 *   Testnet:    ./demo_grid_martin_paper --testnet
 */

#include "engine/paper_broker.hpp"
#include "engine/strategy_manager.hpp"
#include "engine/risk_manager.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "strategy/grid_martin_strategy.hpp"
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
// Configuration
//=============================================================================

struct DemoConfig {
    bool use_testnet = false;
    std::string api_key;
    std::string api_secret;
    std::string symbol = "BTCUSDT";
    double base_price = 85000.0;
    int grid_count = 10;
    double grid_spacing = 0.001;  // 0.1% for frequent trading
    double amount_per_grid = 100.0;
    double initial_capital = 10000.0;
};

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n\n"
              << "Options:\n"
              << "  --testnet             Use Binance Testnet\n"
              << "  --api-key KEY         API Key (or set BINANCE_API_KEY env var)\n"
              << "  --api-secret SECRET   API Secret (or set BINANCE_API_SECRET env var)\n"
              << "  --symbol SYMBOL       Trading symbol (default: BTCUSDT)\n"
              << "  --base-price PRICE    Base price for grid (default: current price)\n"
              << "  --grid-count N        Number of grids (default: 10)\n"
              << "  --grid-spacing PCT    Grid spacing as decimal (default: 0.001 = 0.1%)\n"
              << "  --amount-per-grid A   Amount per grid in USD (default: 100)\n"
              << "  --help                Show this help\n\n"
              << "Examples:\n"
              << "  " << program << " --testnet\n"
              << "  " << program << " --symbol ETHUSDT --grid-spacing 0.002\n";
}

DemoConfig parse_args(int argc, char* argv[]) {
    DemoConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--testnet") {
            config.use_testnet = true;
        } else if (arg == "--api-key" && i + 1 < argc) {
            config.api_key = argv[++i];
        } else if (arg == "--api-secret" && i + 1 < argc) {
            config.api_secret = argv[++i];
        } else if (arg == "--symbol" && i + 1 < argc) {
            config.symbol = argv[++i];
        } else if (arg == "--base-price" && i + 1 < argc) {
            config.base_price = std::stod(argv[++i]);
        } else if (arg == "--grid-count" && i + 1 < argc) {
            config.grid_count = std::stoi(argv[++i]);
        } else if (arg == "--grid-spacing" && i + 1 < argc) {
            config.grid_spacing = std::stod(argv[++i]);
        } else if (arg == "--amount-per-grid" && i + 1 < argc) {
            config.amount_per_grid = std::stod(argv[++i]);
        } else if (arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        }
    }

    // Check environment variables
    if (config.api_key.empty()) {
        const char* env_key = std::getenv("BINANCE_API_KEY");
        if (env_key) config.api_key = env_key;
    }
    if (config.api_secret.empty()) {
        const char* env_secret = std::getenv("BINANCE_API_SECRET");
        if (env_secret) config.api_secret = env_secret;
    }

    return config;
}

//=============================================================================
// Paper Trading Demo
//=============================================================================

int main(int argc, char* argv[]) {
    DemoConfig config = parse_args(argc, argv);

    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Grid Martin Strategy - Paper Trading Demo              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Display configuration
    std::cout << "[Config] Environment: " << (true ? "TESTNET" : "PRODUCTION") << "\n";
    std::cout << "[Config] Symbol: " << config.symbol << "\n";
    std::cout << "[Config] Grid Count: " << config.grid_count << "\n";
    std::cout << "[Config] Grid Spacing: " << (config.grid_spacing * 100) << "%\n";
    std::cout << "[Config] Amount per Grid: $" << config.amount_per_grid << "\n";
    std::cout << "[Config] Initial Capital: $" << config.initial_capital << "\n\n";

    try {
        // Step 1: Create PaperBroker
        std::cout << "[1] Creating PaperBroker...\n";
        PaperBrokerConfig broker_config;
        broker_config.initial_capital = config.initial_capital;
        broker_config.commission_rate = 0.001;    // 0.1% commission
        broker_config.slippage_rate = 0.0005;     // 0.05% slippage

        PaperBroker broker(broker_config);
        std::cout << "    Initial capital: $" << config.initial_capital << "\n";
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
        risk_config.order_flow_limit = 50;        // Max 50 orders per second
        risk_config.order_size_limit = 1.0;       // Max 1 BTC per order
        risk_config.active_order_limit = 10;      // Max 10 active orders
        risk_config.prevent_self_trade = true;

        RiskManager risk_manager(risk_config);
        risk_manager.set_log_callback([](const std::string& msg) {
            std::cout << "[Risk] " << msg << std::endl;
        });
        std::cout << "    Order flow limit: " << risk_config.order_flow_limit << "/s\n";
        std::cout << "    Order size limit: " << risk_config.order_size_limit << " BTC\n";
        std::cout << "    Active order limit: " << risk_config.active_order_limit << "\n";

        // Step 3: Create StrategyManager
        std::cout << "\n[3] Creating StrategyManager...\n";
        StrategyManager strategy_manager;
        strategy_manager.set_paper_broker(&broker);
        strategy_manager.set_risk_manager(&risk_manager);
        strategy_manager.set_log_callback([](const std::string& msg) {
            std::cout << "[StrategyManager] " << msg << std::endl;
        });

        // Step 4: Connect to Binance
        std::cout << "\n[4] Connecting to Binance...\n";
        BinanceSpotGateway gateway;
        GatewayConfig gw_config;
        gw_config.api_key = "vxYrtaRfcErhB3u1cgCvPSg82ySR5oQJeeXO51CI2p1FXAm0m9XX8n60I9yFaeGq";
        gw_config.api_secret = "5HOnKSK0AJMZ8shRiH1nlvbmYgxlPKBYlmJSaweIdZFTKcDTI7UlTfJE16FW0pQe";
        gw_config.testnet = true;  // 使用测试网

        if (!gateway.connect(gw_config)) {
            std::cerr << "Failed to connect to Binance: " << gateway.last_error() << std::endl;
            return 1;
        }
        std::cout << "    Connected to " << (true ? "Testnet" : "Production") << "!\n";

        // Get current price if base_price not set
        if (config.base_price <= 0) {
            config.base_price = gateway.get_price(config.symbol);
            std::cout << "    Current " << config.symbol << " price: $" << config.base_price << "\n";
        }

        // Show account balance if API key provided
        if (!config.api_key.empty()) {
            auto accounts = gateway.query_account_all();
            std::cout << "    Account balances:\n";
            for (const auto& acc : accounts) {
                if (acc.balance > 0) {
                    std::cout << "      " << acc.accountid << ": " << acc.balance << "\n";
                }
            }
        }

        // Step 5: Add GridMartinStrategy
        std::cout << "\n[5] Adding GridMartinStrategy...\n";

        // Get current price as base price
        double current_price = gateway.get_price(config.symbol);
        if (current_price <= 0) {
            std::cerr << "Failed to get current price" << std::endl;
            return 1;
        }

        auto strategy = std::make_unique<GridMartinStrategy>();
        strategy->base_price_ = current_price;  // Use current price as base
        strategy->grid_count_ = config.grid_count;
        strategy->grid_spacing_ = config.grid_spacing;
        strategy->amount_per_grid_ = config.amount_per_grid;
        strategy->symbol_ = config.symbol;

        // Connect strategy to PaperBroker for order execution
        strategy->set_paper_broker(&broker);

        std::cout << "    Symbol: " << config.symbol << "\n";
        std::cout << "    Base Price: $" << current_price << " (current market price)\n";
        std::cout << "    Grid Count: " << config.grid_count << "\n";
        std::cout << "    Grid Spacing: " << (config.grid_spacing * 100) << "%\n";
        std::cout << "    Amount per Grid: $" << config.amount_per_grid << "\n";

        // Calculate and display grid levels (symmetric around current price)
        int half_grid = config.grid_count / 2;
        std::cout << "\n    Grid Levels (symmetric around $" << current_price << "):\n";
        for (int i = 0; i < config.grid_count; ++i) {
            double level = current_price * (1.0 + config.grid_spacing * (i - half_grid));
            std::cout << "      Grid " << std::setw(2) << i << ": $" << level << "\n";
        }

        strategy_manager.add_strategy("grid_martin", std::move(strategy),
                                       config.symbol + ".0", {});  // 0 = Exchange::BINANCE
        strategy_manager.init_strategy("grid_martin");
        strategy_manager.start_strategy("grid_martin");

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
        gateway.subscribe_tick(SubscribeRequest{.symbol = config.symbol});
        gateway.subscribe_bar(config.symbol, Interval::MINUTE_1);

        std::cout << "    Subscribed to " << config.symbol << " tick and 1m bar\n";

        // Step 7: Run paper trading
        std::cout << "\n[7] Paper trading running...\n";
        std::cout << "    Press Ctrl+C to stop\n\n";

        // Display loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

            double equity = broker.get_equity();
            double position = broker.get_position(config.symbol);
            int trades = broker.get_trade_count();

            std::cout << "\r[Live] " << std::setw(4) << elapsed << "s | "
                      << "Ticks: " << std::setw(6) << tick_count << " | "
                      << "Bars: " << std::setw(4) << bar_count << " | "
                      << config.symbol << ": $" << std::fixed << std::setprecision(2) << last_price << " | "
                      << "Position: " << std::setprecision(6) << position << " | "
                      << "Equity: $" << std::setprecision(2) << equity << " | "
                      << "Trades: " << trades
                      << std::flush;
        }

        // Step 8: Final report
        std::cout << "\n\n[8] Generating final report...\n";

        // Stop strategy
        strategy_manager.stop_strategy("grid_martin");

        // Close gateway
        gateway.close();

        // Final statistics
        auto end_time = std::chrono::steady_clock::now();
        auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                   Final Results                             ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Initial Capital:     $   " << std::setw(10) << std::fixed << std::setprecision(2)
                  << config.initial_capital << "                        ║\n";
        std::cout << "║ Final Equity:       $   " << std::setw(10) << broker.get_equity() << "                        ║\n";
        std::cout << "║ Cash Balance:       $   " << std::setw(10) << broker.get_balance() << "                        ║\n";
        std::cout << "║ Position:                 " << std::setw(8) << std::setprecision(6)
                  << broker.get_position(config.symbol) << " BTC                   ║\n";
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
        strategy_manager.get_strategy_stats("grid_martin", strategy_trades, strategy_wins, strategy_pnl);

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
