/**
 * @file demo_bar_generator.cpp
 * @brief BarGenerator 演示程序
 *
 * 演示如何使用 BarGenerator 从 Tick 数据生成 K 线
 */

#include "data/bar_generator.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

int main() {
    std::cout << "=== BarGenerator Demo ===" << std::endl << std::endl;

    // 创建 1 分钟 K 线生成器
    int bar_count = 0;
    BarGenerator bg(Interval::MINUTE_1, [&bar_count](const BarData& bar) {
        bar_count++;
        std::cout << "Generated Bar #" << bar_count << std::endl;
        std::cout << "  Time: " << time_utils::format_timestamp(bar.datetime) << std::endl;
        std::cout << "  Open: " << bar.open_price << std::endl;
        std::cout << "  High: " << bar.high_price << std::endl;
        std::cout << "  Low: " << bar.low_price << std::endl;
        std::cout << "  Close: " << bar.close_price << std::endl;
        std::cout << "  Volume: " << bar.volume << std::endl;
        std::cout << std::endl;
    });

    // 模拟 Tick 数据 (模拟 BTCUSDT)
    std::cout << "Simulating Tick data..." << std::endl << std::endl;

    // 第一个 Tick - 第一根 K 线开始
    TickData tick1;
    tick1.symbol = "BTCUSDT";
    tick1.exchange = Exchange::BINANCE;
    tick1.datetime = 1700000000000; // 2023-11-14 22:13:20 UTC (ms)
    tick1.last_price = 37000.0;
    tick1.volume = 0.5;
    bg.on_tick(tick1);

    // 同一分钟内的更多 Tick
    TickData tick2 = tick1;
    tick2.datetime += 10000; // +10 seconds
    tick2.last_price = 37100.0;
    tick2.volume = 0.3;
    bg.on_tick(tick2);

    TickData tick3 = tick1;
    tick3.datetime += 30000; // +30 seconds
    tick3.last_price = 36950.0;
    tick3.volume = 0.2;
    bg.on_tick(tick3);

    TickData tick4 = tick1;
    tick4.datetime += 45000; // +45 seconds
    tick4.last_price = 37050.0;
    tick4.volume = 0.1;
    bg.on_tick(tick4);

    // 触发新 K 线 - 超过 1 分钟
    TickData tick5 = tick1;
    tick5.datetime += 65000; // +65 seconds (超过 1 分钟)
    tick5.last_price = 37200.0;
    tick5.volume = 0.4;
    bg.on_tick(tick5);

    // 再添加一个 Tick
    TickData tick6 = tick5;
    tick6.datetime += 10000; // +10 seconds
    tick6.last_price = 37150.0;
    tick6.volume = 0.2;
    bg.on_tick(tick6);

    // 测试 MultiBarGenerator
    std::cout << "=== MultiBarGenerator Demo ===" << std::endl << std::endl;

    int bar_1min = 0, bar_5min = 0;

    MultiBarGenerator mbg;
    mbg.add_generator(Interval::MINUTE_1, [&bar_1min](const BarData& bar) {
        bar_1min++;
        std::cout << "1-min Bar #" << bar_1min
                  << " Close: " << bar.close_price << std::endl;
    });

    mbg.add_generator(Interval::MINUTE_5, [&bar_5min](const BarData& bar) {
        bar_5min++;
        std::cout << "5-min Bar #" << bar_5min
                  << " Close: " << bar.close_price << std::endl;
    });

    // 重置 Tick 数据，模拟多次输入
    for (int i = 0; i < 10; i++) {
        TickData tick;
        tick.symbol = "ETHUSDT";
        tick.exchange = Exchange::BINANCE;
        tick.datetime = 1700000000000 + i * 60000; // 每分钟一个
        tick.last_price = 2000.0 + i * 10.0;
        tick.volume = 1.0;
        mbg.on_tick(tick);
    }

    std::cout << std::endl << "=== Summary ===" << std::endl;
    std::cout << "Total 1-min bars: " << bar_1min << std::endl;
    std::cout << "Total 5-min bars: " << bar_5min << std::endl;

    return 0;
}
