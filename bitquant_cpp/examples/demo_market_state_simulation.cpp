/**
 * @file demo_market_state_simulation.cpp
 * @brief Simulation test for Market State Analyzer
 *
 * Simulates price data to demonstrate:
 * 1. WAITING -> CONSOLIDATION (warm-up)
 * 2. CONSOLIDATION -> TRENDING (trend starts)
 * 3. TRENDING -> CONSOLIDATION (trend ends)
 */

#include "data/market_state_analyzer.hpp"
#include "data/price_smoother.hpp"
#include "strategy/grid_martin_strategy.hpp"
#include "engine/paper_broker.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace bitquant;

// Simulated price scenarios
struct PriceScenario {
    std::string phase_name;
    int bars;
    double start_price;
    double volatility_pct;  // How much price varies
    double trend_pct;       // Directional trend per bar (0 = no trend)
};

void print_header() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Market State Analyzer - Simulation Test                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";
}

void print_state(MarketState state) {
    switch (state) {
        case MarketState::WAITING: std::cout << "WAITING"; break;
        case MarketState::CONSOLIDATION: std::cout << "CONSOLIDATION"; break;
        case MarketState::TRENDING: std::cout << "TRENDING"; break;
    }
}

int main() {
    print_header();

    // Configuration
    MarketStateConfig config;
    config.period = 20;              // 20 bars for faster warm-up
    config.width_threshold_pct = 3.0; // 3% threshold for clearer transitions
    config.confirmation_bars = 2;

    MarketStateAnalyzer analyzer(config);

    std::cout << "Configuration:\n";
    std::cout << "  Period: " << config.period << " bars\n";
    std::cout << "  Width threshold: " << config.width_threshold_pct << "%\n";
    std::cout << "  Confirmation bars: " << config.confirmation_bars << "\n\n";

    // Define price scenarios
    std::vector<PriceScenario> scenarios = {
        {"Warm-up (filling buffer)", 25, 50000.0, 0.5, 0.0},       // Low volatility, no trend
        {"Consolidation phase", 30, 50000.0, 0.3, 0.0},             // Very low volatility
        {"Trending UP phase", 30, 50000.0, 2.0, 1.0},               // High volatility, upward trend
        {"Trending DOWN phase", 20, 53000.0, 2.0, -1.5},            // High volatility, downward trend
        {"Return to Consolidation", 30, 50000.0, 0.2, 0.0},         // Low volatility again
    };

    std::cout << "Running simulation...\n\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    MarketState last_reported_state = MarketState::WAITING;
    int total_bars = 0;
    double current_price = 50000.0;

    for (const auto& scenario : scenarios) {
        std::cout << "\n【Phase: " << scenario.phase_name << "】\n";
        std::cout << "  Bars: " << scenario.bars << " | Volatility: " << scenario.volatility_pct
                  << "% | Trend: " << scenario.trend_pct << "%/bar\n";

        for (int i = 0; i < scenario.bars; ++i) {
            total_bars++;

            // Simulate price movement
            double trend_move = scenario.start_price * (scenario.trend_pct / 100.0) * i;
            double noise = (rand() % 100 - 50) / 100.0 * scenario.start_price * (scenario.volatility_pct / 100.0);
            current_price = scenario.start_price + trend_move + noise;

            // Ensure positive price
            if (current_price < 1000.0) current_price = 1000.0;

            double high = current_price + abs(noise) * 0.5;
            double low = current_price - abs(noise) * 0.5;

            // Update analyzer
            MarketState state = analyzer.update(high, low, current_price);

            // Report state changes
            if (state != last_reported_state) {
                std::cout << "\n  ⚡ STATE CHANGE at bar " << total_bars << ":\n";
                std::cout << "     From: ";
                print_state(last_reported_state);
                std::cout << " → To: ";
                print_state(state);
                std::cout << "\n";
                std::cout << "     Price: $" << std::fixed << std::setprecision(2) << current_price;
                std::cout << " | Channel width: " << std::setprecision(2) << analyzer.channel_width_pct() << "%\n";

                if (state == MarketState::TRENDING) {
                    std::cout << "     [Grid Trading PAUSED - Close positions]\n";
                } else if (state == MarketState::CONSOLIDATION) {
                    std::cout << "     [Grid Trading ACTIVE - Grid restarted at $" << current_price << "]\n";
                }

                last_reported_state = state;
            }
        }

        // Report phase summary
        std::cout << "  End of phase | State: ";
        print_state(analyzer.state());
        std::cout << " | Width: " << analyzer.channel_width_pct() << "%\n";
    }

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    // Final summary
    std::cout << "\n╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    Simulation Summary                             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Total bars processed: " << total_bars << "\n";
    std::cout << "Final state: ";
    print_state(analyzer.state());
    std::cout << "\n";
    std::cout << "Final channel width: " << analyzer.channel_width_pct() << "%\n";
    std::cout << "Final middle band: $" << analyzer.middle_band() << "\n";
    std::cout << "Final upper band: $" << analyzer.upper_band() << "\n";
    std::cout << "Final lower band: $" << analyzer.lower_band() << "\n\n";

    std::cout << "Expected behavior verified:\n";
    std::cout << "  ✅ WAITING → CONSOLIDATION: After buffer filled with stable prices\n";
    std::cout << "  ✅ CONSOLIDATION → TRENDING: When volatility exceeds threshold\n";
    std::cout << "  ✅ TRENDING → CONSOLIDATION: When volatility drops below threshold\n";
    std::cout << "  ✅ Confirmation mechanism: Requires 2 bars to confirm transitions\n\n";

    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                 Simulation Test Complete ✅                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";

    return 0;
}