/**
 * @file performance.hpp
 * @brief Performance statistics calculation
 */

#pragma once

#include "core/types.hpp"
#include <vector>

namespace bitquant {

/**
 * @brief Calculate performance metrics from trades and equity curve
 */
class PerformanceCalculator {
public:
    static PerformanceMetrics calculate(
        const std::vector<Trade>& trades,
        const std::vector<std::pair<int64_t, double>>& equity_curve,
        double initial_capital
    );

private:
    static double calculate_max_drawdown(
        const std::vector<std::pair<int64_t, double>>& equity_curve
    );

    static double calculate_sharpe_ratio(
        const std::vector<std::pair<int64_t, double>>& equity_curve
    );

    static double calculate_sortino_ratio(
        const std::vector<std::pair<int64_t, double>>& equity_curve
    );
};

} // namespace bitquant
