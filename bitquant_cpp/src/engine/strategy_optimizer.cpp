/**
 * @file strategy_optimizer.cpp
 * @brief Strategy optimizer implementation
 */

#include "engine/strategy_optimizer.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace bitquant {

//=============================================================================
// Parameter Combination Generation
//=============================================================================

std::vector<std::unordered_map<std::string, double>> StrategyOptimizer::generate_param_combinations(
    const std::unordered_map<std::string, std::vector<double>>& param_ranges
) {
    std::vector<std::unordered_map<std::string, double>> combinations;

    // Start with empty combination
    combinations.push_back({});

    // For each parameter, expand combinations
    for (const auto& [param_name, values] : param_ranges) {
        std::vector<std::unordered_map<std::string, double>> new_combinations;

        for (const auto& existing : combinations) {
            for (double value : values) {
                auto new_params = existing;
                new_params[param_name] = value;
                new_combinations.push_back(new_params);
            }
        }

        combinations = std::move(new_combinations);
    }

    return combinations;
}

//=============================================================================
// Metric Calculation
//=============================================================================

double StrategyOptimizer::calculate_metric(const PerformanceMetrics& perf, OptimizationMetric metric) {
    switch (metric) {
        case OptimizationMetric::SHARPE_RATIO:
            return perf.sharpe_ratio;

        case OptimizationMetric::TOTAL_RETURN:
            return perf.total_return;

        case OptimizationMetric::MAX_DRAWDOWN:
            // Negative because we want to minimize drawdown
            return -perf.max_drawdown_percent;

        case OptimizationMetric::WIN_RATE:
            return perf.win_rate;

        case OptimizationMetric::PROFIT_FACTOR:
            return perf.profit_factor;

        case OptimizationMetric::CALMAR_RATIO:
            if (perf.max_drawdown_percent > 0) {
                return perf.annualized_return / perf.max_drawdown_percent;
            }
            return 0.0;

        case OptimizationMetric::SORTINO_RATIO:
            return perf.sortino_ratio;

        default:
            return perf.sharpe_ratio;
    }
}

//=============================================================================
// Logging
//=============================================================================

void StrategyOptimizer::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    } else {
        std::cout << "[Optimizer] " << msg << std::endl;
    }
}

} // namespace bitquant