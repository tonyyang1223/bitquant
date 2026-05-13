/**
 * @file strategy_optimizer.hpp
 * @brief Strategy parameter optimization framework
 *
 * Provides multi-threaded parameter optimization for trading strategies.
 * Supports various optimization metrics and parameter ranges.
 *
 * Reference: howtrader/trader/optimizer.py
 */

#pragma once

#include "core/types.hpp"
#include "engine/broker.hpp"
#include "engine/strategy.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <mutex>

namespace bitquant {

//=============================================================================
// Optimization Configuration
//=============================================================================

/**
 * @brief Optimization metric enum
 */
enum class OptimizationMetric {
    SHARPE_RATIO,       // Sharpe ratio (risk-adjusted return)
    TOTAL_RETURN,       // Total return percentage
    MAX_DRAWDOWN,       // Maximum drawdown (minimize)
    WIN_RATE,           // Win rate percentage
    PROFIT_FACTOR,      // Profit factor (gross profit / gross loss)
    CALMAR_RATIO,       // Calmar ratio (return / max drawdown)
    SORTINO_RATIO       // Sortino ratio (downside risk-adjusted)
};

/**
 * @brief Optimization result structure
 */
struct OptimizationResult {
    std::unordered_map<std::string, double> best_params;
    double best_metric = 0.0;
    double best_sharpe = 0.0;
    double best_return = 0.0;
    double best_drawdown = 0.0;
    int best_trades = 0;

    // All results for analysis
    std::vector<std::pair<std::unordered_map<std::string, double>, double>> all_results;

    // Statistics
    int total_combinations = 0;
    int successful_runs = 0;
    double optimization_time_ms = 0.0;
};

/**
 * @brief Optimization progress callback
 */
using ProgressCallback = std::function<void(int current, int total, const std::string& params, double metric)>;

//=============================================================================
// StrategyOptimizer
//=============================================================================

/**
 * @brief Strategy parameter optimizer
 *
 * Runs multiple backtests with different parameter combinations
 * to find optimal parameters for a given strategy.
 *
 * Usage:
 * @code
 * StrategyOptimizer optimizer;
 * auto result = optimizer.optimize<DoubleMaStrategy>(
 *     bars,
 *     {"fast_window", {5, 10, 15, 20}},
 *     {"slow_window", {20, 30, 40, 50}},
 *     OptimizationMetric::SHARPE_RATIO,
 *     4  // threads
 * );
 * @endcode
 */
class StrategyOptimizer {
public:
    using LogCallback = std::function<void(const std::string&)>;

    //=========================================================================
    // Configuration
    //=========================================================================

    /**
     * @brief Set initial capital for backtests
     */
    void set_initial_capital(double capital) { initial_capital_ = capital; }

    /**
     * @brief Set commission rate for backtests
     */
    void set_commission_rate(double rate) { commission_rate_ = rate; }

    /**
     * @brief Set slippage rate for backtests
     */
    void set_slippage_rate(double rate) { slippage_rate_ = rate; }

    /**
     * @brief Set log callback
     */
    void set_log_callback(LogCallback cb) { log_callback_ = std::move(cb); }

    /**
     * @brief Set progress callback
     */
    void set_progress_callback(ProgressCallback cb) { progress_callback_ = std::move(cb); }

    //=========================================================================
    // Optimization
    //=========================================================================

    /**
     * @brief Optimize strategy parameters
     *
     * @tparam StrategyClass Strategy class type
     * @param bars Historical bar data
     * @param param_ranges Parameter ranges to optimize
     * @param metric Optimization metric
     * @param num_threads Number of parallel threads
     * @return Optimization result
     */
    template<typename StrategyClass>
    OptimizationResult optimize(
        const std::vector<BarData>& bars,
        const std::unordered_map<std::string, std::vector<double>>& param_ranges,
        OptimizationMetric metric = OptimizationMetric::SHARPE_RATIO,
        int num_threads = 4
    );

    //=========================================================================
    // Parameter Combination Generation
    //=========================================================================

    /**
     * @brief Generate all parameter combinations
     */
    std::vector<std::unordered_map<std::string, double>> generate_param_combinations(
        const std::unordered_map<std::string, std::vector<double>>& param_ranges
    );

    //=========================================================================
    // Backtest Runner
    //=========================================================================

    /**
     * @brief Run single backtest with given parameters
     */
    template<typename StrategyClass>
    double run_backtest(
        const std::vector<BarData>& bars,
        const std::unordered_map<std::string, double>& params,
        OptimizationMetric metric
    );

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    double calculate_metric(const PerformanceMetrics& perf, OptimizationMetric metric);
    void write_log(const std::string& msg);

    //=========================================================================
    // Members
    //=========================================================================

    double initial_capital_ = 100000.0;
    double commission_rate_ = 0.001;
    double slippage_rate_ = 0.0005;

    LogCallback log_callback_;
    ProgressCallback progress_callback_;

    // Progress tracking
    std::mutex progress_mutex_;
    int current_progress_ = 0;
    int total_combinations_ = 0;
};

//=============================================================================
// Template Implementation
//=============================================================================

template<typename StrategyClass>
OptimizationResult StrategyOptimizer::optimize(
    const std::vector<BarData>& bars,
    const std::unordered_map<std::string, std::vector<double>>& param_ranges,
    OptimizationMetric metric,
    int num_threads
) {
    OptimizationResult result;

    auto start_time = std::chrono::steady_clock::now();

    // Generate all parameter combinations
    auto combinations = generate_param_combinations(param_ranges);
    result.total_combinations = combinations.size();
    total_combinations_ = combinations.size();

    write_log("Starting optimization: " + std::to_string(combinations.size()) + " combinations");

    // Run backtests in parallel
    std::vector<std::future<std::pair<std::unordered_map<std::string, double>, double>>> futures;

    for (const auto& params : combinations) {
        futures.push_back(std::async(std::launch::async,
            [this, &bars, params, metric]() {
                double metric_value = run_backtest<StrategyClass>(bars, params, metric);

                // Update progress
                {
                    std::lock_guard<std::mutex> lock(progress_mutex_);
                    current_progress_++;
                    if (progress_callback_) {
                        std::string params_str;
                        for (const auto& [k, v] : params) {
                            params_str += k + "=" + std::to_string(v) + " ";
                        }
                        progress_callback_(current_progress_, total_combinations_, params_str, metric_value);
                    }
                }

                return std::make_pair(params, metric_value);
            }
        ));

        // Limit concurrent threads
        if (futures.size() >= num_threads) {
            // Wait for at least one to complete
            for (auto& f : futures) {
                if (f.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    auto [params, value] = f.get();
                    result.all_results.push_back({params, value});
                    result.successful_runs++;

                    if (value > result.best_metric) {
                        result.best_metric = value;
                        result.best_params = params;
                    }
                }
            }

            // Remove completed futures
            futures.erase(std::remove_if(futures.begin(), futures.end(),
                [](auto& f) { return !f.valid(); }), futures.end());
        }
    }

    // Wait for remaining futures
    for (auto& f : futures) {
        auto [params, value] = f.get();
        result.all_results.push_back({params, value});
        result.successful_runs++;

        if (value > result.best_metric) {
            result.best_metric = value;
            result.best_params = params;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    result.optimization_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();

    write_log("Optimization complete: best metric = " + std::to_string(result.best_metric));

    return result;
}

template<typename StrategyClass>
double StrategyOptimizer::run_backtest(
    const std::vector<BarData>& bars,
    const std::unordered_map<std::string, double>& params,
    OptimizationMetric metric
) {
    // Create broker
    BrokerConfig broker_config;
    broker_config.initial_cash = initial_capital_;
    broker_config.commission_rate = commission_rate_;
    broker_config.slippage_rate = slippage_rate_;

    Broker broker(broker_config);

    // Create strategy
    auto strategy = std::make_unique<StrategyClass>();

    // Initialize strategy with parameters
    for (const auto& [key, value] : params) {
        strategy->set_param(key, value);
    }

    // Set data and run backtest
    broker.set_data(bars);
    broker.set_strategy(std::move(strategy));
    broker.run();

    // Get performance metrics
    const PerformanceMetrics& perf = broker.performance();

    return calculate_metric(perf, metric);
}

} // namespace bitquant