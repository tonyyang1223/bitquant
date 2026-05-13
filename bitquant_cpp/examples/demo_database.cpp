/**
 * @file demo_database.cpp
 * @brief 数据库功能演示
 *
 * 演示如何使用 SQLite 数据库进行数据持久化
 */

#include "database/sqlite_database.hpp"
#include "data/bar_generator.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

int main() {
    std::cout << "=== Database Demo ===" << std::endl << std::endl;

    // 创建数据库
    auto db = create_sqlite_database("trading_data.db");

    if (!db->is_connected()) {
        std::cerr << "Failed to connect to database: " << db->last_error() << std::endl;
        return 1;
    }

    std::cout << "Database connected successfully!" << std::endl << std::endl;

    // 创建测试 Bar 数据
    std::cout << "--- Saving Bar Data ---" << std::endl;

    std::vector<BarData> bars;
    for (int i = 0; i < 10; i++) {
        BarData bar;
        bar.symbol = "BTCUSDT";
        bar.exchange = Exchange::BINANCE;
        bar.interval = Interval::MINUTE_1;
        bar.datetime = 1700000000000 + i * 60000; // 每分钟
        bar.open_price = 37000.0 + i * 10;
        bar.high_price = bar.open_price + 50;
        bar.low_price = bar.open_price - 30;
        bar.close_price = bar.open_price + 20;
        bar.volume = 100.0 + i * 10;
        bar.turnover = bar.volume * bar.close_price;

        bars.push_back(bar);
    }

    int saved = db->save_bars(bars);
    std::cout << "Saved " << saved << " bars to database" << std::endl << std::endl;

    // 查询 Bar 数据
    std::cout << "--- Loading Bar Data ---" << std::endl;

    auto loaded_bars = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 5);
    std::cout << "Loaded " << loaded_bars.size() << " bars:" << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    for (const auto& bar : loaded_bars) {
        std::cout << "  Time: " << time_utils::format_timestamp(bar.datetime)
                  << " | O: " << bar.open_price
                  << " H: " << bar.high_price
                  << " L: " << bar.low_price
                  << " C: " << bar.close_price
                  << " V: " << bar.volume
                  << std::endl;
    }
    std::cout << std::endl;

    // 获取数据概览
    std::cout << "--- Bar Data Overview ---" << std::endl;

    auto overviews = db->get_bar_overview();
    for (const auto& overview : overviews) {
        std::cout << "  Symbol: " << overview.symbol
                  << " | Exchange: " << exchange_to_string(overview.exchange)
                  << " | Interval: " << interval_to_string(overview.interval)
                  << " | Count: " << overview.count
                  << " | Start: " << time_utils::format_timestamp(overview.start_time)
                  << " | End: " << time_utils::format_timestamp(overview.end_time)
                  << std::endl;
    }
    std::cout << std::endl;

    // 测试策略状态保存
    std::cout << "--- Strategy State ---" << std::endl;

    std::string strategy_state = R"({"position":1.5,"entry_price":37050,"stop_loss":36500})";
    db->save_strategy_state("sma_cross", strategy_state);
    std::cout << "Saved strategy state: " << strategy_state << std::endl;

    auto loaded_state = db->load_strategy_state("sma_cross");
    if (loaded_state) {
        std::cout << "Loaded strategy state: " << *loaded_state << std::endl;
    }
    std::cout << std::endl;

    // 测试时间范围查询
    std::cout << "--- Time Range Query ---" << std::endl;

    int64_t start_time = 1700000000000;
    int64_t end_time = 1700000000000 + 5 * 60000;

    auto range_bars = db->load_bars_by_time("BTCUSDT", Exchange::BINANCE,
                                             Interval::MINUTE_1, start_time, end_time);
    std::cout << "Bars in time range: " << range_bars.size() << std::endl << std::endl;

    // 清理
    std::cout << "--- Cleanup ---" << std::endl;
    int deleted = db->delete_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1);
    std::cout << "Deleted " << deleted << " bars" << std::endl;

    db->delete_strategy_state("sma_cross");
    std::cout << "Deleted strategy state" << std::endl;

    std::cout << std::endl << "Database demo completed!" << std::endl;

    return 0;
}
