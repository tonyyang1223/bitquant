/**
 * @file types.cpp
 * @brief Implementation of core types
 */

#include "core/types.hpp"
#include <sstream>
#include <iomanip>

namespace bitquant {

std::string PerformanceMetrics::to_string() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║                  回测绩效报告 (Backtest Report)              ║\n";
    oss << "╠══════════════════════════════════════════════════════════════╣\n";
    oss << "║ 【资金统计 (Capital)】                                       ║\n";
    oss << "║   初始资金 (Initial): " << std::setw(14) << initial_capital << "                          ║\n";
    oss << "║   最终资金 (Final):   " << std::setw(14) << final_capital << "                          ║\n";
    oss << "║   总收益率 (Return):  " << std::setw(12) << (total_return * 100) << "%                          ║\n";
    oss << "║   年化收益 (Annual):  " << std::setw(12) << (annualized_return * 100) << "%                          ║\n";
    oss << "╠══════════════════════════════════════════════════════════════╣\n";
    oss << "║ 【风险指标 (Risk Metrics)】                                   ║\n";
    oss << "║   最大回撤 (Max DD): " << std::setw(12) << (max_drawdown_percent * 100) << "%                          ║\n";
    oss << "║   夏普比率 (Sharpe): " << std::setw(14) << sharpe_ratio << "                          ║\n";
    oss << "║   索提诺比 (Sortino): " << std::setw(14) << sortino_ratio << "                          ║\n";
    oss << "║   卡玛比率 (Calmar):  " << std::setw(14) << calmar_ratio << "                          ║\n";
    oss << "╠══════════════════════════════════════════════════════════════╣\n";
    oss << "║ 【交易统计 (Trade Stats)】                                   ║\n";
    oss << "║   总交易数 (Trades): " << std::setw(14) << total_trades << "                          ║\n";
    oss << "║   盈利次数 (Wins):    " << std::setw(14) << winning_trades << "                          ║\n";
    oss << "║   亏损次数 (Losses):  " << std::setw(14) << losing_trades << "                          ║\n";
    oss << "║   胜率 (Win Rate):    " << std::setw(12) << (win_rate * 100) << "%                          ║\n";
    oss << "║   盈亏比 (PF):        " << std::setw(14) << profit_factor << "                          ║\n";
    oss << "║   平均盈利 (Avg Win): " << std::setw(14) << avg_win << "                          ║\n";
    oss << "║   平均亏损 (Avg Loss):" << std::setw(14) << avg_loss << "                          ║\n";
    oss << "║   平均交易 (Avg Trade):" << std::setw(14) << avg_trade << "                          ║\n";
    oss << "║   连续盈利 (Consec W):" << std::setw(14) << max_consecutive_wins << "                          ║\n";
    oss << "║   连续亏损 (Consec L):" << std::setw(14) << max_consecutive_losses << "                          ║\n";
    oss << "╠══════════════════════════════════════════════════════════════╣\n";
    oss << "║ 【成本统计 (Costs)】                                         ║\n";
    oss << "║   手续费 (Commission): " << std::setw(14) << total_commission << "                          ║\n";
    oss << "║   滑点成本 (Slippage): " << std::setw(14) << total_slippage << "                          ║\n";
    oss << "║   总成交额 (Turnover): " << std::setw(14) << total_turnover << "                          ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n";

    return oss.str();
}

} // namespace bitquant
