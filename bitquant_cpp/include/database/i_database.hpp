/**
 * @file i_database.hpp
 * @brief 数据库抽象接口
 *
 * 提供统一的数据库访问接口，支持多种数据库实现。
 * 基于 Python howtrader 的 database.py 设计。
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <memory>

namespace bitquant {

//=============================================================================
// Data Overview Structures
//=============================================================================

/**
 * @brief K线数据概览
 */
struct BarOverview {
    std::string symbol;
    Exchange exchange;
    Interval interval;
    int64_t count;         // 数据条数
    int64_t start_time;    // 最早数据时间
    int64_t end_time;      // 最新数据时间
};

/**
 * @brief Tick数据概览
 */
struct TickOverview {
    std::string symbol;
    Exchange exchange;
    int64_t count;
    int64_t start_time;
    int64_t end_time;
};

//=============================================================================
// Database Interface
//=============================================================================

/**
 * @brief 数据库抽象接口
 *
 * 定义数据库操作的统一接口，支持多种数据库实现。
 *
 * 使用方式:
 * @code
 * std::unique_ptr<IDatabase> db = create_sqlite_database("trading.db");
 *
 * // 保存 Bar 数据
 * BarData bar = {...};
 * db->save_bar(bar);
 *
 * // 查询 Bar 数据
 * auto bars = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1, 100);
 * @endcode
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief 检查数据库连接是否正常
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief 获取最后的错误信息
     */
    virtual std::string last_error() const = 0;

    //=========================================================================
    // Bar Data Operations
    //=========================================================================

    /**
     * @brief 保存单条 Bar 数据
     * @param bar K线数据
     * @return 成功返回 true
     */
    virtual bool save_bar(const BarData& bar) = 0;

    /**
     * @brief 批量保存 Bar 数据
     * @param bars K线数据列表
     * @return 成功保存的数量
     */
    virtual int save_bars(const std::vector<BarData>& bars) = 0;

    /**
     * @brief 加载 Bar 数据
     * @param symbol 品种代码
     * @param exchange 交易所
     * @param interval K线周期
     * @param limit 最大数量
     * @return K线数据列表
     */
    virtual std::vector<BarData> load_bars(
        const std::string& symbol,
        Exchange exchange,
        Interval interval,
        int limit = 100
    ) = 0;

    /**
     * @brief 加载指定时间范围的 Bar 数据
     * @param symbol 品种代码
     * @param exchange 交易所
     * @param interval K线周期
     * @param start_time 开始时间 (毫秒)
     * @param end_time 结束时间 (毫秒)
     * @return K线数据列表
     */
    virtual std::vector<BarData> load_bars_by_time(
        const std::string& symbol,
        Exchange exchange,
        Interval interval,
        int64_t start_time,
        int64_t end_time
    ) = 0;

    /**
     * @brief 删除 Bar 数据
     * @param symbol 品种代码
     * @param exchange 交易所
     * @param interval K线周期
     * @return 删除的数量
     */
    virtual int delete_bars(
        const std::string& symbol,
        Exchange exchange,
        Interval interval
    ) = 0;

    /**
     * @brief 获取 Bar 数据概览
     * @return 所有品种的 Bar 数据概览列表
     */
    virtual std::vector<BarOverview> get_bar_overview() = 0;

    //=========================================================================
    // Tick Data Operations
    //=========================================================================

    /**
     * @brief 保存单条 Tick 数据
     * @param tick Tick数据
     * @return 成功返回 true
     */
    virtual bool save_tick(const TickData& tick) = 0;

    /**
     * @brief 批量保存 Tick 数据
     * @param ticks Tick数据列表
     * @return 成功保存的数量
     */
    virtual int save_ticks(const std::vector<TickData>& ticks) = 0;

    /**
     * @brief 加载 Tick 数据
     * @param symbol 品种代码
     * @param exchange 交易所
     * @param limit 最大数量
     * @return Tick数据列表
     */
    virtual std::vector<TickData> load_ticks(
        const std::string& symbol,
        Exchange exchange,
        int limit = 100
    ) = 0;

    /**
     * @brief 加载指定时间范围的 Tick 数据
     */
    virtual std::vector<TickData> load_ticks_by_time(
        const std::string& symbol,
        Exchange exchange,
        int64_t start_time,
        int64_t end_time
    ) = 0;

    /**
     * @brief 获取 Tick 数据概览
     */
    virtual std::vector<TickOverview> get_tick_overview() = 0;

    //=========================================================================
    // Strategy State Operations
    //=========================================================================

    /**
     * @brief 保存策略状态
     * @param strategy_name 策略名称
     * @param state_json 状态 JSON 字符串
     */
    virtual bool save_strategy_state(
        const std::string& strategy_name,
        const std::string& state_json
    ) = 0;

    /**
     * @brief 加载策略状态
     * @param strategy_name 策略名称
     * @return 状态 JSON 字符串，不存在返回空
     */
    virtual std::optional<std::string> load_strategy_state(
        const std::string& strategy_name
    ) = 0;

    /**
     * @brief 删除策略状态
     */
    virtual bool delete_strategy_state(const std::string& strategy_name) = 0;

    //=========================================================================
    // General Operations
    //=========================================================================

    /**
     * @brief 执行原始 SQL (谨慎使用)
     * @param sql SQL 语句
     * @return 成功返回 true
     */
    virtual bool execute(const std::string& sql) = 0;

    /**
     * @brief 开始事务
     */
    virtual bool begin_transaction() = 0;

    /**
     * @brief 提交事务
     */
    virtual bool commit() = 0;

    /**
     * @brief 回滚事务
     */
    virtual bool rollback() = 0;
};

//=============================================================================
// Database Factory
//=============================================================================

/**
 * @brief 创建 SQLite 数据库实例
 * @param db_path 数据库文件路径
 * @return 数据库实例
 */
std::unique_ptr<IDatabase> create_sqlite_database(const std::string& db_path);

} // namespace bitquant
