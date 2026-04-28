/**
 * @file performance.cpp
 * @brief Performance statistics implementation
 */

#include "statistics/performance.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace bitquant {

double PerformanceCalculator::calculate_max_drawdown(
    const std::vector<std::pair<int64_t, double>>& equity_curve) {

    if (equity_curve.empty()) return 0.0;

    double peak = equity_curve[0].second;
    double max_dd = 0.0;

    for (const auto& [time, eq] : equity_curve) {
        if (eq > peak) peak = eq;
        double dd = (peak - eq) / peak;
        if (dd > max_dd) max_dd = dd;
    }

    return max_dd;
}

double PerformanceCalculator::calculate_sharpe_ratio(
    const std::vector<std::pair<int64_t, double>>& equity_curve) {

    if (equity_curve.size() < 2) return 0.0;

    std::vector<double> returns;
    returns.reserve(equity_curve.size() - 1);

    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double prev_eq = equity_curve[i - 1].second;
        double curr_eq = equity_curve[i].second;
        if (prev_eq > 0) {
            returns.push_back((curr_eq - prev_eq) / prev_eq);
        }
    }

    if (returns.empty()) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double sq_sum = 0.0;
    for (double r : returns) {
        sq_sum += (r - mean) * (r - mean);
    }
    double std_dev = std::sqrt(sq_sum / returns.size());

    if (std_dev == 0.0) return 0.0;

    // Annualized Sharpe (assuming daily bars)
    return (mean / std_dev) * std::sqrt(252);
}

double PerformanceCalculator::calculate_sortino_ratio(
    const std::vector<std::pair<int64_t, double>>& equity_curve) {

    if (equity_curve.size() < 2) return 0.0;

    std::vector<double> returns;
    returns.reserve(equity_curve.size() - 1);

    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double prev_eq = equity_curve[i - 1].second;
        double curr_eq = equity_curve[i].second;
        if (prev_eq > 0) {
            returns.push_back((curr_eq - prev_eq) / prev_eq);
        }
    }

    if (returns.empty()) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    // Downside deviation
    double downside_sq_sum = 0.0;
    for (double r : returns) {
        if (r < 0) {
            downside_sq_sum += r * r;
        }
    }
    double downside_dev = std::sqrt(downside_sq_sum / returns.size());

    if (downside_dev == 0.0) return 0.0;

    return (mean / downside_dev) * std::sqrt(252);
}

PerformanceMetrics PerformanceCalculator::calculate(
    const std::vector<Trade>& trades,
    const std::vector<std::pair<int64_t, double>>& equity_curve,
    double initial_capital) {

    PerformanceMetrics metrics;

    if (equity_curve.empty()) {
        return metrics;
    }

    metrics.total_trades = static_cast<int>(trades.size());
    metrics.initial_capital = initial_capital;
    metrics.final_capital = equity_curve.back().second;
    metrics.total_return = (metrics.final_capital - initial_capital) / initial_capital;

    // Annualized return (simplified, assumes minute data)
    if (equity_curve.size() > 1) {
        double bars = static_cast<double>(equity_curve.size());
        double years = bars / (252.0 * 240.0);  // 240 trading minutes per day
        if (years > 0) {
            metrics.annualized_return = std::pow(1 + metrics.total_return, 1.0 / years) - 1;
        }
    }

    // Risk metrics
    metrics.max_drawdown = calculate_max_drawdown(equity_curve);
    metrics.sharpe_ratio = calculate_sharpe_ratio(equity_curve);
    metrics.sortino_ratio = calculate_sortino_ratio(equity_curve);

    // Calmar ratio
    if (metrics.max_drawdown > 0) {
        metrics.calmar_ratio = metrics.annualized_return / metrics.max_drawdown;
    }

    // Trade statistics - proper PnL calculation with trade pairing
    if (!trades.empty()) {
        // Pair trades to calculate round-trip PnL
        std::vector<double> round_trip_pnls;
        double current_position = 0.0;
        double avg_cost = 0.0;
        double total_cost = 0.0;

        for (const auto& trade : trades) {
            if (trade.side == Direction::LONG) {
                // Opening or adding to position
                total_cost += trade.price * trade.volume + trade.commission;
                current_position += trade.volume;
                avg_cost = total_cost / current_position;
            } else {
                // Closing position
                double close_volume = std::min(current_position, trade.volume);
                if (close_volume > 0 && avg_cost > 0) {
                    double pnl = (trade.price - avg_cost) * close_volume - trade.commission;
                    round_trip_pnls.push_back(pnl);
                }
                current_position -= close_volume;
                if (current_position <= 0) {
                    current_position = 0;
                    avg_cost = 0;
                    total_cost = 0;
                }
            }
        }

        // Calculate statistics from round-trip PnLs
        if (!round_trip_pnls.empty()) {
            double total_profit = 0.0;
            double total_loss = 0.0;
            int wins = 0;
            int losses = 0;
            int consecutive_wins = 0;
            int consecutive_losses = 0;
            int max_consecutive_wins = 0;
            int max_consecutive_losses = 0;

            for (double pnl : round_trip_pnls) {
                if (pnl > 0) {
                    total_profit += pnl;
                    ++wins;
                    consecutive_wins++;
                    max_consecutive_wins = std::max(max_consecutive_wins, consecutive_wins);
                    consecutive_losses = 0;
                } else {
                    total_loss += std::abs(pnl);
                    ++losses;
                    consecutive_losses++;
                    max_consecutive_losses = std::max(max_consecutive_losses, consecutive_losses);
                    consecutive_wins = 0;
                }
            }

            metrics.winning_trades = wins;
            metrics.losing_trades = losses;
            metrics.win_rate = (wins + losses > 0) ?
                               static_cast<double>(wins) / (wins + losses) : 0.0;
            metrics.avg_win = (wins > 0) ? total_profit / wins : 0.0;
            metrics.avg_loss = (losses > 0) ? total_loss / losses : 0.0;
            metrics.profit_factor = (total_loss > 0) ? total_profit / total_loss :
                                    (total_profit > 0 ? INFINITY : 0.0);
            metrics.avg_trade = (wins + losses > 0) ?
                               (total_profit - total_loss) / (wins + losses) : 0.0;
            metrics.max_consecutive_wins = max_consecutive_wins;
            metrics.max_consecutive_losses = max_consecutive_losses;
        }
    }

    return metrics;
}

} // namespace bitquant
