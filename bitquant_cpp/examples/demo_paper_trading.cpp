/**
 * @file demo_paper_trading.cpp
 * @brief Paper trading demonstration
 *
 * Demonstrates:
 * - Real-time market data from Binance WebSocket
 * - Simulated order execution (no real orders sent)
 * - Strategy running with live prices
 */

#include "engine/paper_broker.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "engine/strategy.hpp"
#include "data/array_manager.hpp"
#include <iostream>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <thread>

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
// Simple SMA Crossover Strategy for Paper Trading
//=============================================================================

class PaperSMAStrategy {
public:
    int short_period = 10;
    int long_period = 20;
    double trade_size = 0.01;  // BTC

    PaperSMAStrategy(PaperBroker* broker) : broker_(broker) {}

    void on_tick(const TickData& tick) {
        // Use tick prices for strategy logic
        last_price_ = tick.last_price;
        update_position_value(tick);
    }

    void on_bar(const BarData& bar) {
        am_.update_bar(bar);
        if (!am_.inited()) return;

        double short_ma = am_.sma(short_period);
        double long_ma = am_.sma(long_period);

        // Simple crossover logic
        if (short_ma > long_ma && position_ <= 0) {
            // Golden cross - buy signal
            double size = trade_size;
            if (position_ < 0) {
                size += std::abs(position_);  // Cover short first
            }
            broker_->send_order(OrderRequest{
                .symbol = bar.symbol,
                .exchange = Exchange::BINANCE,
                .direction = Direction::LONG,
                .type = OrderType::TAKER,
                .volume = size,
                .price = 0.0,  // Market order
            });
            position_ += size;
            entry_price_ = bar.close_price;
            std::cout << "[Strategy] BUY signal at " << bar.close_price
                      << " (SMA" << short_period << "=" << short_ma
                      << " > SMA" << long_period << "=" << long_ma << ")" << std::endl;
        } else if (short_ma < long_ma && position_ >= 0) {
            // Death cross - sell signal
            double size = trade_size;
            if (position_ > 0) {
                size = position_;  // Close all long
            }
            broker_->send_order(OrderRequest{
                .symbol = bar.symbol,
                .exchange = Exchange::BINANCE,
                .direction = Direction::SHORT,
                .type = OrderType::TAKER,
                .volume = size,
                .price = 0.0,
            });
            position_ -= size;
            entry_price_ = bar.close_price;
            std::cout << "[Strategy] SELL signal at " << bar.close_price
                      << " (SMA" << short_period << "=" << short_ma
                      << " < SMA" << long_period << "=" << long_ma << ")" << std::endl;
        }
    }

    double get_pnl() const {
        if (position_ == 0) return 0.0;
        return (last_price_ - entry_price_) * position_;
    }

    double get_position() const { return position_; }

private:
    void update_position_value(const TickData& tick) {
        if (position_ != 0 && entry_price_ > 0) {
            double unrealized_pnl = (tick.last_price - entry_price_) * position_;
            // Could display this in real-time
        }
    }

    PaperBroker* broker_;
    ArrayManager am_{100};
    double position_ = 0.0;
    double entry_price_ = 0.0;
    double last_price_ = 0.0;
};

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BitQuant Paper Trading Demonstration              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "NOTE: This demo uses simulated order execution.\n";
    std::cout << "No real orders will be sent to the exchange.\n";
    std::cout << "\n";

    // Register signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Step 1: Create PaperBroker
        std::cout << "[1] Creating PaperBroker...\n";

        PaperBrokerConfig config;
        config.initial_capital = 100'000.0;  // $100,000
        config.commission_rate = 0.001;       // 0.1%
        config.slippage_rate = 0.0005;        // 0.05%

        PaperBroker broker(config);

        // Track order and trade updates
        broker.on_order([](const OrderData& order) {
            std::cout << "[Order] " << order.orderid
                      << " status: " << static_cast<int>(order.status)
                      << " filled: " << order.traded << "/" << order.volume
                      << std::endl;
        });

        broker.on_trade([](const TradeData& trade) {
            std::cout << "[Trade] " << trade.tradeid
                      << " " << (trade.direction == Direction::LONG ? "BUY" : "SELL")
                      << " " << trade.volume << " @ " << trade.price
                      << std::endl;
        });

        std::cout << "    Initial capital: $100,000\n";
        std::cout << "    Commission: 0.1%\n";
        std::cout << "    Slippage: 0.05%\n";

        // Step 2: Create strategy
        std::cout << "\n[2] Creating SMA Crossover Strategy...\n";

        PaperSMAStrategy strategy(&broker);
        std::cout << "    Short period: 10\n";
        std::cout << "    Long period: 20\n";
        std::cout << "    Trade size: 0.01 BTC\n";

        // Step 3: Connect to Binance for real-time data
        std::cout << "\n[3] Connecting to Binance for real-time data...\n";

        BinanceSpotGateway gateway;
        GatewayConfig gw_config;
        gw_config.api_key = "";      // Public data only
        gw_config.api_secret = "";

        if (!gateway.connect(gw_config)) {
            std::cerr << "Failed to connect to Binance" << std::endl;
            return 1;
        }
        std::cout << "    Connected!\n";

        // Step 4: Subscribe to market data
        std::cout << "\n[4] Subscribing to BTCUSDT real-time data...\n";

        gateway.on_tick([&](const TickData& tick) {
            broker.on_tick(tick);
            strategy.on_tick(tick);

            // Display current status
            static int counter = 0;
            if (++counter % 100 == 0) {  // Update every 100 ticks
                std::cout << "\r[Live] BTCUSDT: $" << std::fixed << std::setprecision(2)
                          << tick.last_price
                          << " | Position: " << std::setprecision(4) << strategy.get_position() << " BTC"
                          << " | PnL: $" << std::setprecision(2) << strategy.get_pnl()
                          << " | Equity: $" << broker.get_equity()
                          << std::flush;
            }
        });

        gateway.on_bar([&](const BarData& bar) {
            broker.on_bar(bar);
            strategy.on_bar(bar);
        });

        // Subscribe to ticker updates
        gateway.subscribe_tick(SubscribeRequest{.symbol = "BTCUSDT"});

        std::cout << "    Subscribed to BTCUSDT ticker\n";

        // Step 5: Run paper trading loop
        std::cout << "\n[5] Paper trading running...\n";
        std::cout << "    Press Ctrl+C to stop\n\n";

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Keep-alive for user stream (if we had one)
            gateway.keep_user_stream();
        }

        // Step 6: Print final results
        gateway.close();

        std::cout << "\n\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                   Final Results                             ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Initial Capital:     $" << std::setw(12) << std::fixed << std::setprecision(2)
                  << 100'000.0 << "                        ║\n";
        std::cout << "║ Final Equity:       $" << std::setw(12) << broker.get_equity()
                  << "                        ║\n";
        std::cout << "║ Cash Balance:       $" << std::setw(12) << broker.get_balance()
                  << "                        ║\n";
        std::cout << "║ Position:           " << std::setw(12) << std::setprecision(4)
                  << strategy.get_position() << " BTC                   ║\n";
        std::cout << "║ Total Trades:        " << std::setw(12) << broker.get_trade_count()
                  << "                        ║\n";
        std::cout << "║ Total Commission:   $" << std::setw(12) << std::setprecision(2)
                  << broker.get_total_commission() << "                        ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
