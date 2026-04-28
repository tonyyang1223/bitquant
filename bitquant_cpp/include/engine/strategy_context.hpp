/**
 * @file strategy_context.hpp
 * @brief Strategy execution context for isolated strategy environment
 *
 * Provides each strategy with isolated:
 * - Data feed
 * - Order tracking
 * - Position management
 * - State persistence
 */

#pragma once

#include "core/types.hpp"
#include "data/array_manager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace bitquant {

//=============================================================================
// Forward Declarations
//=============================================================================

class Broker;
class StrategyManager;

//=============================================================================
// Strategy Context
//=============================================================================

/**
 * @brief Execution context for individual strategy
 *
 * Provides isolated environment for strategy execution:
 * - Separate position tracking
 * - Order history
 * - PnL calculation
 * - State persistence
 */
class StrategyContext {
public:
    StrategyContext(
        const std::string& name,
        Broker* broker,
        StrategyManager* manager
    );

    //=========================================================================
    // Identification
    //=========================================================================

    const std::string& name() const { return name_; }

    //=========================================================================
    // Position Management
    //=========================================================================

    /**
     * @brief Get current position volume
     */
    double position() const { return position_; }

    /**
     * @brief Get position direction
     */
    Direction position_direction() const { return position_direction_; }

    /**
     * @brief Get average entry price
     */
    double avg_price() const { return avg_price_; }

    /**
     * @brief Update position from trade
     */
    void update_position(const TradeData& trade);

    //=========================================================================
    // PnL Tracking
    //=========================================================================

    /**
     * @brief Get realized PnL
     */
    double realized_pnl() const { return realized_pnl_; }

    /**
     * @brief Get unrealized PnL at given price
     */
    double unrealized_pnl(double current_price) const;

    /**
     * @brief Get total PnL
     */
    double total_pnl(double current_price) const;

    //=========================================================================
    // Order Tracking
    //=========================================================================

    /**
     * @brief Get active order IDs
     */
    const std::unordered_set<std::string>& active_orders() const {
        return active_orders_;
    }

    /**
     * @brief Add order to tracking
     */
    void add_order(const std::string& vt_orderid);

    /**
     * @brief Remove order from tracking
     */
    void remove_order(const std::string& vt_orderid);

    //=========================================================================
    // Trade History
    //=========================================================================

    /**
     * @brief Get all trades
     */
    const std::vector<TradeData>& trades() const { return trades_; }

    /**
     * @brief Add trade
     */
    void add_trade(const TradeData& trade);

    //=========================================================================
    // Statistics
    //=========================================================================

    int total_trades() const { return trades_.size(); }
    int winning_trades() const { return winning_trades_; }
    int losing_trades() const { return losing_trades_; }
    double win_rate() const;

    //=========================================================================
    // State Persistence
    //=========================================================================

    /**
     * @brief Save state to map
     */
    std::unordered_map<std::string, double> save_state() const;

    /**
     * @brief Load state from map
     */
    void load_state(const std::unordered_map<std::string, double>& state);

private:
    std::string name_;
    Broker* broker_;
    StrategyManager* manager_;

    // Position state
    double position_ = 0.0;
    Direction position_direction_ = Direction::NET;
    double avg_price_ = 0.0;

    // PnL tracking
    double realized_pnl_ = 0.0;
    double total_cost_ = 0.0;

    // Order tracking
    std::unordered_set<std::string> active_orders_;

    // Trade history
    std::vector<TradeData> trades_;
    int winning_trades_ = 0;
    int losing_trades_ = 0;

    // Custom state variables
    std::unordered_map<std::string, double> variables_;
};

//=============================================================================
// Strategy Context Factory
//=============================================================================

/**
 * @brief Factory for creating strategy contexts
 */
class StrategyContextFactory {
public:
    static std::unique_ptr<StrategyContext> create(
        const std::string& name,
        Broker* broker,
        StrategyManager* manager
    );
};

} // namespace bitquant
