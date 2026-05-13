/**
 * @file bar_generator.hpp
 * @brief K线生成器
 *
 * 从 Tick 数据合成 K 线数据，支持多种时间周期。
 * 基于 Python howtrader 的 BarGenerator 设计。
 *
 * 功能:
 * - Tick → Bar 转换 (生成任意周期 K 线)
 * - Bar → Bar 转换 (从小周期合成大周期)
 * - 支持成交量、成交额累计
 * - 支持新 K 线生成回调
 */

#pragma once

#include "core/types.hpp"
#include <functional>
#include <chrono>
#include <string>
#include <vector>
#include <memory>

namespace bitquant {

//=============================================================================
// Callback Types
//=============================================================================

/// Bar 生成回调
using BarCallback = std::function<void(const BarData&)>;

//=============================================================================
// Bar Generator
//=============================================================================

/**
 * @brief K线生成器
 *
 * 从实时 Tick 数据合成 K 线。
 *
 * 使用方式:
 * @code
 * BarGenerator bg(Interval::MINUTE_1, [](const BarData& bar) {
 *     // 处理新生成的 K 线
 * });
 *
 * // 在 Tick 回调中
 * void on_tick(const TickData& tick) {
 *     bg.on_tick(tick);
 * }
 * @endcode
 */
class BarGenerator {
public:
    /**
     * @brief 构造函数
     * @param interval K 线周期
     * @param callback K 线生成回调
     * @param window_size 窗口大小 (用于合成大周期，如 5 表示 5 根 1 分钟合成 5 分钟)
     */
    explicit BarGenerator(Interval interval, BarCallback callback, int window_size = 0);

    /**
     * @brief 构造函数 (使用 Bar 输入，用于合成大周期)
     * @param interval 目标周期
     * @param window_size 窗口大小
     * @param callback K 线生成回调
     */
    BarGenerator(Interval interval, int window_size, BarCallback callback);

    ~BarGenerator() = default;

    //=========================================================================
    // Data Input
    //=========================================================================

    /**
     * @brief 处理 Tick 数据
     * @param tick Tick 行情
     *
     * 当新 K 线生成时，会调用回调函数
     */
    void on_tick(const TickData& tick);

    /**
     * @brief 处理 Bar 数据 (用于合成大周期)
     * @param bar K 线数据
     *
     * 当用小周期合成大周期时使用
     */
    void on_bar(const BarData& bar);

    //=========================================================================
    // Data Access
    //=========================================================================

    /**
     * @brief 获取当前未完成的 K 线
     * @return 当前 K 线数据 (如果还没有数据则返回 std::nullopt)
     */
    std::optional<BarData> get_current_bar() const;

    /**
     * @brief 检查是否有数据
     */
    bool has_data() const;

    /**
     * @brief 重置生成器状态
     */
    void reset();

    //=========================================================================
    // Properties
    //=========================================================================

    /**
     * @brief 获取 K 线周期
     */
    Interval interval() const { return interval_; }

    /**
     * @brief 获取窗口大小
     */
    int window_size() const { return window_size_; }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    /**
     * @brief 检查是否需要生成新的 K 线
     * @param tick 新的 Tick 数据
     * @return true 如果需要生成新 K 线
     */
    bool should_generate_new_bar(const TickData& tick) const;

    /**
     * @brief 生成新 K 线的起始时间戳
     * @param timestamp Tick 时间戳
     * @return K 线的起始时间戳
     */
    int64_t calculate_bar_start_time(int64_t timestamp) const;

    /**
     * @brief 更新当前 K 线
     * @param tick Tick 数据
     */
    void update_current_bar(const TickData& tick);

    /**
     * @brief 完成当前 K 线并生成新的 K 线
     * @param tick 新 K 线的第一个 Tick
     */
    void finish_current_bar(const TickData& tick);

    /**
     * @brief 从 Bar 合成大周期 K 线
     */
    void update_from_bar(const BarData& bar);

    /**
     * @brief 完成 Bar 合成
     */
    void finish_bar_synthesis(const BarData& bar);

    //=========================================================================
    // Members
    //=========================================================================

    Interval interval_;
    int window_size_ = 0;
    BarCallback callback_;

    // 当前 K 线状态
    BarData current_bar_;
    bool has_data_ = false;

    // Bar 合成状态 (用于从 Bar 合成大周期)
    std::vector<BarData> bar_buffer_;
    int bar_count_ = 0;

    // Tick 输入模式标志
    bool tick_mode_ = true;
};

//=============================================================================
// Multi-Timeframe Bar Generator
//=============================================================================

/**
 * @brief 多时间框架 K 线生成器
 *
 * 同时生成多个周期的 K 线。
 *
 * 使用方式:
 * @code
 * MultiBarGenerator mbg;
 * mbg.add_generator(Interval::MINUTE_1, on_1min_bar);
 * mbg.add_generator(Interval::MINUTE_5, on_5min_bar);
 *
 * void on_tick(const TickData& tick) {
 *     mbg.on_tick(tick);
 * }
 * @endcode
 */
class MultiBarGenerator {
public:
    MultiBarGenerator() = default;
    ~MultiBarGenerator() = default;

    /**
     * @brief 添加时间框架
     * @param interval K 线周期
     * @param callback K 线生成回调
     */
    void add_generator(Interval interval, BarCallback callback);

    /**
     * @brief 添加时间框架 (从 Bar 合成)
     * @param interval K 线周期
     * @param window_size 窗口大小
     * @param callback K 线生成回调
     */
    void add_generator(Interval interval, int window_size, BarCallback callback);

    /**
     * @brief 处理 Tick 数据
     */
    void on_tick(const TickData& tick);

    /**
     * @brief 处理 Bar 数据
     */
    void on_bar(const BarData& bar);

    /**
     * @brief 重置所有生成器
     */
    void reset();

private:
    std::vector<std::unique_ptr<BarGenerator>> generators_;
};

//=============================================================================
// Time Interval Utilities
//=============================================================================

namespace time_utils {

/**
 * @brief 获取周期对应的毫秒数
 * @param interval K 线周期
 * @return 周期的毫秒数
 */
int64_t interval_to_milliseconds(Interval interval);

/**
 * @brief 获取周期对应的分钟数
 * @param interval K 线周期
 * @return 周期的分钟数
 */
int interval_to_minutes(Interval interval);

/**
 * @brief 对齐时间戳到周期起点
 * @param timestamp 时间戳 (毫秒)
 * @param interval K 线周期
 * @return 对齐后的时间戳
 */
int64_t align_to_interval(int64_t timestamp, Interval interval);

/**
 * @brief 格式化时间戳为字符串
 * @param timestamp 时间戳 (毫秒)
 * @param format 格式字符串
 * @return 格式化后的字符串
 */
std::string format_timestamp(int64_t timestamp, const std::string& format = "%Y-%m-%d %H:%M:%S");

} // namespace time_utils

} // namespace bitquant
