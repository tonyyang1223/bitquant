/**
 * @file sqlite_database.hpp
 * @brief SQLite 数据库实现
 *
 * 基于 SQLite3 的数据库实现，轻量级、无需额外服务。
 */

#pragma once

#include "database/i_database.hpp"
#include <sqlite3.h>
#include <memory>
#include <mutex>

namespace bitquant {

//=============================================================================
// SQLite Database Implementation
//=============================================================================

/**
 * @brief SQLite 数据库实现
 *
 * 使用 SQLite3 作为存储后端。
 *
 * 数据库表结构:
 * - dbbardata: K线数据表
 * - dbtickdata: Tick数据表
 * - dbstrategydata: 策略状态表
 *
 * 使用方式:
 * @code
 * auto db = create_sqlite_database("trading.db");
 *
 * BarData bar = {...};
 * db->save_bar(bar);
 *
 * auto bars = db->load_bars("BTCUSDT", Exchange::BINANCE, Interval::MINUTE_1);
 * @endcode
 */
class SQLiteDatabase : public IDatabase {
public:
    /**
     * @brief 构造函数
     * @param db_path 数据库文件路径
     */
    explicit SQLiteDatabase(const std::string& db_path);

    /**
     * @brief 析构函数
     */
    ~SQLiteDatabase() override;

    // 禁止拷贝
    SQLiteDatabase(const SQLiteDatabase&) = delete;
    SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;

    //=========================================================================
    // IDatabase Interface Implementation
    //=========================================================================

    bool is_connected() const override;
    std::string last_error() const override;

    // Bar operations
    bool save_bar(const BarData& bar) override;
    int save_bars(const std::vector<BarData>& bars) override;
    std::vector<BarData> load_bars(
        const std::string& symbol,
        Exchange exchange,
        Interval interval,
        int limit = 100
    ) override;
    std::vector<BarData> load_bars_by_time(
        const std::string& symbol,
        Exchange exchange,
        Interval interval,
        int64_t start_time,
        int64_t end_time
    ) override;
    int delete_bars(
        const std::string& symbol,
        Exchange exchange,
        Interval interval
    ) override;
    std::vector<BarOverview> get_bar_overview() override;

    // Tick operations
    bool save_tick(const TickData& tick) override;
    int save_ticks(const std::vector<TickData>& ticks) override;
    std::vector<TickData> load_ticks(
        const std::string& symbol,
        Exchange exchange,
        int limit = 100
    ) override;
    std::vector<TickData> load_ticks_by_time(
        const std::string& symbol,
        Exchange exchange,
        int64_t start_time,
        int64_t end_time
    ) override;
    std::vector<TickOverview> get_tick_overview() override;

    // Strategy state operations
    bool save_strategy_state(
        const std::string& strategy_name,
        const std::string& state_json
    ) override;
    std::optional<std::string> load_strategy_state(
        const std::string& strategy_name
    ) override;
    bool delete_strategy_state(const std::string& strategy_name) override;

    // General operations
    bool execute(const std::string& sql) override;
    bool begin_transaction() override;
    bool commit() override;
    bool rollback() override;

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    /**
     * @brief 初始化数据库表
     */
    bool init_tables();

    /**
     * @brief 设置错误信息
     */
    void set_error(const std::string& error);

    //=========================================================================
    // Members
    //=========================================================================

    sqlite3* db_ = nullptr;
    std::string db_path_;
    std::string last_error_;
    std::mutex mutex_;
    bool connected_ = false;
};

} // namespace bitquant
