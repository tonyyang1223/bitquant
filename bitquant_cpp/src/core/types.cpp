/**
 * @file types.cpp
 * @brief Implementation of core types
 */

#include "core/types.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace bitquant {

//=============================================================================
// Enum Conversion Functions
//=============================================================================

std::string exchange_to_string(Exchange exchange) {
    switch (exchange) {
        case Exchange::BINANCE:    return "BINANCE";
        case Exchange::OKX:        return "OKX";
        case Exchange::BYBIT:      return "BYBIT";
        case Exchange::BITFINEX:   return "BITFINEX";
        case Exchange::LOCAL:      return "LOCAL";
        case Exchange::SHFE:       return "SHFE";
        case Exchange::DCE:        return "DCE";
        case Exchange::CZCE:       return "CZCE";
        case Exchange::CFFEX:      return "CFFEX";
        case Exchange::INE:        return "INE";
        default:                   return "UNKNOWN";
    }
}

Exchange exchange_from_string(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "BINANCE")    return Exchange::BINANCE;
    if (upper == "OKX")        return Exchange::OKX;
    if (upper == "BYBIT")      return Exchange::BYBIT;
    if (upper == "BITFINEX")   return Exchange::BITFINEX;
    if (upper == "LOCAL")      return Exchange::LOCAL;
    if (upper == "SHFE")       return Exchange::SHFE;
    if (upper == "DCE")        return Exchange::DCE;
    if (upper == "CZCE")       return Exchange::CZCE;
    if (upper == "CFFEX")      return Exchange::CFFEX;
    if (upper == "INE")        return Exchange::INE;
    return Exchange::LOCAL;
}

std::string interval_to_string(Interval interval) {
    switch (interval) {
        case Interval::TICK:      return "TICK";
        case Interval::MINUTE_1:  return "1m";
        case Interval::MINUTE_3:  return "3m";
        case Interval::MINUTE_5:  return "5m";
        case Interval::MINUTE_15: return "15m";
        case Interval::MINUTE_30: return "30m";
        case Interval::HOUR_1:    return "1h";
        case Interval::HOUR_2:    return "2h";
        case Interval::HOUR_4:    return "4h";
        case Interval::HOUR_6:    return "6h";
        case Interval::HOUR_12:   return "12h";
        case Interval::DAILY:     return "1d";
        case Interval::WEEKLY:    return "1w";
        case Interval::MONTHLY:   return "1M";
        default:                  return "1m";
    }
}

Interval interval_from_string(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "tick")        return Interval::TICK;
    if (lower == "1m")          return Interval::MINUTE_1;
    if (lower == "3m")          return Interval::MINUTE_3;
    if (lower == "5m")          return Interval::MINUTE_5;
    if (lower == "15m")         return Interval::MINUTE_15;
    if (lower == "30m")         return Interval::MINUTE_30;
    if (lower == "1h")          return Interval::HOUR_1;
    if (lower == "2h")          return Interval::HOUR_2;
    if (lower == "4h")          return Interval::HOUR_4;
    if (lower == "6h")          return Interval::HOUR_6;
    if (lower == "12h")         return Interval::HOUR_12;
    if (lower == "1d" || lower == "daily")   return Interval::DAILY;
    if (lower == "1w" || lower == "weekly")  return Interval::WEEKLY;
    if (lower == "1m" || lower == "monthly") return Interval::MONTHLY;
    return Interval::MINUTE_1;
}

std::string direction_to_string(Direction direction) {
    switch (direction) {
        case Direction::LONG:  return "LONG";
        case Direction::SHORT: return "SHORT";
        case Direction::NET:   return "NET";
        default:               return "NET";
    }
}

Direction direction_from_string(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "LONG" || upper == "BUY")  return Direction::LONG;
    if (upper == "SHORT" || upper == "SELL") return Direction::SHORT;
    if (upper == "NET") return Direction::NET;
    return Direction::NET;
}

//=============================================================================
// Performance Metrics
//=============================================================================

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
