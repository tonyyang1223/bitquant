/**
 * @file bar_generator.cpp
 * @brief K线生成器实现
 *
 * 从 Tick 数据合成 K 线的实现
 */

#include "data/bar_generator.hpp"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

namespace bitquant {

//=============================================================================
// Time Interval Utilities Implementation
//=============================================================================

namespace time_utils {

int64_t interval_to_milliseconds(Interval interval) {
    switch (interval) {
        case Interval::TICK:
            return 0;  // 每个 Tick 一根 K 线
        case Interval::MINUTE_1:
            return 60 * 1000;
        case Interval::MINUTE_3:
            return 3 * 60 * 1000;
        case Interval::MINUTE_5:
            return 5 * 60 * 1000;
        case Interval::MINUTE_15:
            return 15 * 60 * 1000;
        case Interval::MINUTE_30:
            return 30 * 60 * 1000;
        case Interval::HOUR_1:
            return 60 * 60 * 1000;
        case Interval::HOUR_2:
            return 2 * 60 * 60 * 1000;
        case Interval::HOUR_4:
            return 4 * 60 * 60 * 1000;
        case Interval::HOUR_6:
            return 6 * 60 * 60 * 1000;
        case Interval::HOUR_12:
            return 12 * 60 * 60 * 1000;
        case Interval::DAILY:
            return 24 * 60 * 60 * 1000;
        case Interval::WEEKLY:
            return 7 * 24 * 60 * 60 * 1000;
        case Interval::MONTHLY:
            // 近似值，实际需要按月处理
            return 30LL * 24LL * 60LL * 60LL * 1000LL;
        default:
            return 60 * 1000;  // 默认 1 分钟
    }
}

int interval_to_minutes(Interval interval) {
    return static_cast<int>(interval_to_milliseconds(interval) / (60 * 1000));
}

int64_t align_to_interval(int64_t timestamp, Interval interval) {
    int64_t interval_ms = interval_to_milliseconds(interval);

    if (interval == Interval::MONTHLY) {
        // 月线特殊处理：对齐到月初
        time_t ts = timestamp / 1000;
        struct tm* tm_info = std::localtime(&ts);
        tm_info->tm_mday = 1;
        tm_info->tm_hour = 0;
        tm_info->tm_min = 0;
        tm_info->tm_sec = 0;
        return std::mktime(tm_info) * 1000;
    }

    if (interval == Interval::WEEKLY) {
        // 周线特殊处理：对齐到周一
        time_t ts = timestamp / 1000;
        struct tm* tm_info = std::localtime(&ts);
        int day_of_week = tm_info->tm_wday;
        if (day_of_week == 0) day_of_week = 7;  // 周日 -> 7

        ts -= (day_of_week - 1) * 24 * 60 * 60;
        tm_info = std::localtime(&ts);
        tm_info->tm_hour = 0;
        tm_info->tm_min = 0;
        tm_info->tm_sec = 0;
        return std::mktime(tm_info) * 1000;
    }

    if (interval == Interval::DAILY) {
        // 日线对齐到 UTC 00:00
        time_t ts = timestamp / 1000;
        struct tm* tm_info = std::gmtime(&ts);
        tm_info->tm_hour = 0;
        tm_info->tm_min = 0;
        tm_info->tm_sec = 0;
        return std::mktime(tm_info) * 1000;
    }

    // 其他周期：简单取模对齐
    return (timestamp / interval_ms) * interval_ms;
}

std::string format_timestamp(int64_t timestamp, const std::string& format) {
    time_t ts = timestamp / 1000;
    struct tm* tm_info = std::localtime(&ts);

    std::ostringstream oss;
    oss << std::put_time(tm_info, format.c_str());
    return oss.str();
}

} // namespace time_utils

//=============================================================================
// Bar Generator Implementation
//=============================================================================

BarGenerator::BarGenerator(Interval interval, BarCallback callback, int window_size)
    : interval_(interval)
    , window_size_(window_size)
    , callback_(std::move(callback))
    , has_data_(false)
    , bar_count_(0)
    , tick_mode_(true)
{
    if (window_size > 0) {
        bar_buffer_.reserve(window_size);
    }
}

BarGenerator::BarGenerator(Interval interval, int window_size, BarCallback callback)
    : interval_(interval)
    , window_size_(window_size)
    , callback_(std::move(callback))
    , has_data_(false)
    , bar_count_(0)
    , tick_mode_(false)
{
    bar_buffer_.reserve(window_size);
}

void BarGenerator::on_tick(const TickData& tick) {
    if (!tick_mode_) {
        // 如果是从 Bar 合成模式，忽略 Tick
        return;
    }

    if (!has_data_) {
        // 第一根 K 线
        current_bar_.symbol = tick.symbol;
        current_bar_.exchange = tick.exchange;
        current_bar_.interval = interval_;
        current_bar_.datetime = calculate_bar_start_time(tick.datetime);
        current_bar_.open_price = tick.last_price;
        current_bar_.high_price = tick.last_price;
        current_bar_.low_price = tick.last_price;
        current_bar_.close_price = tick.last_price;
        current_bar_.volume = tick.volume;
        current_bar_.turnover = tick.last_price * tick.volume;
        has_data_ = true;
        return;
    }

    // 检查是否需要生成新 K 线
    if (should_generate_new_bar(tick)) {
        finish_current_bar(tick);
    } else {
        update_current_bar(tick);
    }
}

void BarGenerator::on_bar(const BarData& bar) {
    if (tick_mode_ || window_size_ <= 0) {
        // 不是从 Bar 合成模式，或者没有设置窗口大小
        return;
    }

    update_from_bar(bar);
}

bool BarGenerator::should_generate_new_bar(const TickData& tick) const {
    if (!has_data_) {
        return false;
    }

    // 计算当前 K 线的结束时间
    int64_t bar_end_time = current_bar_.datetime + time_utils::interval_to_milliseconds(interval_);

    // 如果新 Tick 时间超过当前 K 线结束时间，则需要生成新 K 线
    return tick.datetime >= bar_end_time;
}

int64_t BarGenerator::calculate_bar_start_time(int64_t timestamp) const {
    return time_utils::align_to_interval(timestamp, interval_);
}

void BarGenerator::update_current_bar(const TickData& tick) {
    // 更新最高价
    if (tick.last_price > current_bar_.high_price) {
        current_bar_.high_price = tick.last_price;
    }

    // 更新最低价
    if (tick.last_price < current_bar_.low_price) {
        current_bar_.low_price = tick.last_price;
    }

    // 更新收盘价
    current_bar_.close_price = tick.last_price;

    // 累计成交量 (这里需要计算增量)
    // 注意: 实际应用中可能需要更复杂的成交量累计逻辑
    current_bar_.volume += tick.volume;
    current_bar_.turnover += tick.last_price * tick.volume;
}

void BarGenerator::finish_current_bar(const TickData& tick) {
    // 回调完成当前 K 线
    if (callback_) {
        callback_(current_bar_);
    }

    // 生成新 K 线
    current_bar_.symbol = tick.symbol;
    current_bar_.exchange = tick.exchange;
    current_bar_.interval = interval_;
    current_bar_.datetime = calculate_bar_start_time(tick.datetime);
    current_bar_.open_price = tick.last_price;
    current_bar_.high_price = tick.last_price;
    current_bar_.low_price = tick.last_price;
    current_bar_.close_price = tick.last_price;
    current_bar_.volume = tick.volume;
    current_bar_.turnover = tick.last_price * tick.volume;
}

void BarGenerator::update_from_bar(const BarData& bar) {
    bar_buffer_.push_back(bar);
    bar_count_++;

    if (static_cast<int>(bar_buffer_.size()) >= window_size_) {
        finish_bar_synthesis(bar);
    }
}

void BarGenerator::finish_bar_synthesis(const BarData& bar) {
    // 从缓冲区合成大周期 K 线
    BarData synthesized_bar;
    synthesized_bar.symbol = bar.symbol;
    synthesized_bar.exchange = bar.exchange;
    synthesized_bar.interval = interval_;
    synthesized_bar.datetime = bar_buffer_.front().datetime;
    synthesized_bar.open_price = bar_buffer_.front().open_price;
    synthesized_bar.high_price = std::max_element(bar_buffer_.begin(), bar_buffer_.end(),
        [](const BarData& a, const BarData& b) { return a.high_price < b.high_price; })->high_price;
    synthesized_bar.low_price = std::min_element(bar_buffer_.begin(), bar_buffer_.end(),
        [](const BarData& a, const BarData& b) { return a.low_price < b.low_price; })->low_price;
    synthesized_bar.close_price = bar_buffer_.back().close_price;
    synthesized_bar.volume = 0;
    synthesized_bar.turnover = 0;

    for (const auto& b : bar_buffer_) {
        synthesized_bar.volume += b.volume;
        synthesized_bar.turnover += b.turnover;
    }

    // 回调
    if (callback_) {
        callback_(synthesized_bar);
    }

    // 清空缓冲区
    bar_buffer_.clear();
}

std::optional<BarData> BarGenerator::get_current_bar() const {
    if (!has_data_) {
        return std::nullopt;
    }
    return current_bar_;
}

bool BarGenerator::has_data() const {
    return has_data_;
}

void BarGenerator::reset() {
    has_data_ = false;
    bar_count_ = 0;
    bar_buffer_.clear();
}

//=============================================================================
// Multi-Bar Generator Implementation
//=============================================================================

void MultiBarGenerator::add_generator(Interval interval, BarCallback callback) {
    auto generator = std::make_unique<BarGenerator>(interval, std::move(callback));
    generators_.push_back(std::move(generator));
}

void MultiBarGenerator::add_generator(Interval interval, int window_size, BarCallback callback) {
    auto generator = std::make_unique<BarGenerator>(interval, window_size, std::move(callback));
    generators_.push_back(std::move(generator));
}

void MultiBarGenerator::on_tick(const TickData& tick) {
    for (auto& gen : generators_) {
        gen->on_tick(tick);
    }
}

void MultiBarGenerator::on_bar(const BarData& bar) {
    for (auto& gen : generators_) {
        gen->on_bar(bar);
    }
}

void MultiBarGenerator::reset() {
    for (auto& gen : generators_) {
        gen->reset();
    }
}

} // namespace bitquant
