/**
 * @file sqlite_database.cpp
 * @brief SQLite 数据库实现
 *
 * 使用 SQLite3 提供数据持久化功能
 */

#include "database/sqlite_database.hpp"
#include "core/types.hpp"
#include <iostream>
#include <sstream>

namespace bitquant {

//=============================================================================
// Constructor/Destructor
//=============================================================================

SQLiteDatabase::SQLiteDatabase(const std::string& db_path)
    : db_path_(db_path)
{
    int rc = sqlite3_open(db_path.c_str(), &db_);

    if (rc != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }

    connected_ = true;

    // 初始化表结构
    if (!init_tables()) {
        connected_ = false;
    }
}

SQLiteDatabase::~SQLiteDatabase() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    connected_ = false;
}

//=============================================================================
// Connection Management
//=============================================================================

bool SQLiteDatabase::is_connected() const {
    return connected_;
}

std::string SQLiteDatabase::last_error() const {
    return last_error_;
}

void SQLiteDatabase::set_error(const std::string& error) {
    last_error_ = error;
    std::cerr << "[Database] Error: " << error << std::endl;
}

//=============================================================================
// Table Initialization
//=============================================================================

bool SQLiteDatabase::init_tables() {
    // K线数据表
    const char* create_bar_table = R"(
        CREATE TABLE IF NOT EXISTS dbbardata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            exchange TEXT NOT NULL,
            interval TEXT NOT NULL,
            datetime INTEGER NOT NULL,
            open_price REAL NOT NULL,
            high_price REAL NOT NULL,
            low_price REAL NOT NULL,
            close_price REAL NOT NULL,
            volume REAL NOT NULL,
            turnover REAL,
            UNIQUE(symbol, exchange, interval, datetime)
        );
    )";

    // Tick 数据表
    const char* create_tick_table = R"(
        CREATE TABLE IF NOT EXISTS dbtickdata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            exchange TEXT NOT NULL,
            datetime INTEGER NOT NULL,
            last_price REAL NOT NULL,
            volume REAL NOT NULL,
            turnover REAL,
            open_interest REAL,
            bid_price_1 REAL,
            bid_volume_1 REAL,
            ask_price_1 REAL,
            ask_volume_1 REAL,
            UNIQUE(symbol, exchange, datetime)
        );
    )";

    // 策略状态表
    const char* create_strategy_table = R"(
        CREATE TABLE IF NOT EXISTS dbstrategydata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            strategy_name TEXT NOT NULL UNIQUE,
            state_json TEXT NOT NULL,
            updated_at INTEGER NOT NULL
        );
    )";

    // 创建索引
    const char* create_bar_index = R"(
        CREATE INDEX IF NOT EXISTS idx_bar_symbol_exchange_interval
        ON dbbardata(symbol, exchange, interval, datetime);
    )";

    const char* create_tick_index = R"(
        CREATE INDEX IF NOT EXISTS idx_tick_symbol_exchange
        ON dbtickdata(symbol, exchange, datetime);
    )";

    if (!execute(create_bar_table)) return false;
    if (!execute(create_tick_table)) return false;
    if (!execute(create_strategy_table)) return false;
    if (!execute(create_bar_index)) return false;
    if (!execute(create_tick_index)) return false;

    return true;
}

//=============================================================================
// Bar Data Operations
//=============================================================================

bool SQLiteDatabase::save_bar(const BarData& bar) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT OR REPLACE INTO dbbardata
        (symbol, exchange, interval, datetime, open_price, high_price, low_price, close_price, volume, turnover)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    // Store strings in temporary variables to ensure they persist
    std::string exchange_str = exchange_to_string(bar.exchange);
    std::string interval_str = interval_to_string(bar.interval);

    sqlite3_bind_text(stmt, 1, bar.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, bar.datetime);
    sqlite3_bind_double(stmt, 5, bar.open_price);
    sqlite3_bind_double(stmt, 6, bar.high_price);
    sqlite3_bind_double(stmt, 7, bar.low_price);
    sqlite3_bind_double(stmt, 8, bar.close_price);
    sqlite3_bind_double(stmt, 9, bar.volume);
    sqlite3_bind_double(stmt, 10, bar.turnover);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    return true;
}

int SQLiteDatabase::save_bars(const std::vector<BarData>& bars) {
    if (!begin_transaction()) {
        return 0;
    }

    int count = 0;
    for (const auto& bar : bars) {
        if (save_bar(bar)) {
            count++;
        }
    }

    if (!commit()) {
        rollback();
        return 0;
    }

    return count;
}

std::vector<BarData> SQLiteDatabase::load_bars(
    const std::string& symbol,
    Exchange exchange,
    Interval interval,
    int limit
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BarData> bars;

    std::ostringstream sql;
    sql << R"(
        SELECT symbol, exchange, interval, datetime, open_price, high_price,
               low_price, close_price, volume, turnover
        FROM dbbardata
        WHERE symbol = ? AND exchange = ? AND interval = ?
        ORDER BY datetime DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return bars;
    }

    std::string exchange_str = exchange_to_string(exchange);
    std::string interval_str = interval_to_string(interval);

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BarData bar;
        bar.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        bar.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        bar.interval = interval_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        bar.datetime = sqlite3_column_int64(stmt, 3);
        bar.open_price = sqlite3_column_double(stmt, 4);
        bar.high_price = sqlite3_column_double(stmt, 5);
        bar.low_price = sqlite3_column_double(stmt, 6);
        bar.close_price = sqlite3_column_double(stmt, 7);
        bar.volume = sqlite3_column_double(stmt, 8);
        bar.turnover = sqlite3_column_double(stmt, 9);

        bars.push_back(bar);
    }

    sqlite3_finalize(stmt);

    // 反转顺序，使时间正序
    std::reverse(bars.begin(), bars.end());
    return bars;
}

std::vector<BarData> SQLiteDatabase::load_bars_by_time(
    const std::string& symbol,
    Exchange exchange,
    Interval interval,
    int64_t start_time,
    int64_t end_time
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BarData> bars;

    const char* sql = R"(
        SELECT symbol, exchange, interval, datetime, open_price, high_price,
               low_price, close_price, volume, turnover
        FROM dbbardata
        WHERE symbol = ? AND exchange = ? AND interval = ?
              AND datetime >= ? AND datetime <= ?
        ORDER BY datetime ASC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return bars;
    }

    std::string exchange_str = exchange_to_string(exchange);
    std::string interval_str = interval_to_string(interval);

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, start_time);
    sqlite3_bind_int64(stmt, 5, end_time);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BarData bar;
        bar.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        bar.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        bar.interval = interval_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        bar.datetime = sqlite3_column_int64(stmt, 3);
        bar.open_price = sqlite3_column_double(stmt, 4);
        bar.high_price = sqlite3_column_double(stmt, 5);
        bar.low_price = sqlite3_column_double(stmt, 6);
        bar.close_price = sqlite3_column_double(stmt, 7);
        bar.volume = sqlite3_column_double(stmt, 8);
        bar.turnover = sqlite3_column_double(stmt, 9);

        bars.push_back(bar);
    }

    sqlite3_finalize(stmt);
    return bars;
}

int SQLiteDatabase::delete_bars(
    const std::string& symbol,
    Exchange exchange,
    Interval interval
) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        DELETE FROM dbbardata
        WHERE symbol = ? AND exchange = ? AND interval = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return 0;
    }

    std::string exchange_str = exchange_to_string(exchange);
    std::string interval_str = interval_to_string(interval);

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval_str.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        set_error(sqlite3_errmsg(db_));
        return 0;
    }

    return changes;
}

std::vector<BarOverview> SQLiteDatabase::get_bar_overview() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BarOverview> overviews;

    const char* sql = R"(
        SELECT symbol, exchange, interval, COUNT(*) as count,
               MIN(datetime) as start_time, MAX(datetime) as end_time
        FROM dbbardata
        GROUP BY symbol, exchange, interval
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return overviews;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BarOverview overview;
        overview.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        overview.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        overview.interval = interval_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        overview.count = sqlite3_column_int64(stmt, 3);
        overview.start_time = sqlite3_column_int64(stmt, 4);
        overview.end_time = sqlite3_column_int64(stmt, 5);

        overviews.push_back(overview);
    }

    sqlite3_finalize(stmt);
    return overviews;
}

//=============================================================================
// Tick Data Operations
//=============================================================================

bool SQLiteDatabase::save_tick(const TickData& tick) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT OR REPLACE INTO dbtickdata
        (symbol, exchange, datetime, last_price, volume, turnover,
         open_interest, bid_price_1, bid_volume_1, ask_price_1, ask_volume_1)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    std::string exchange_str = exchange_to_string(tick.exchange);

    sqlite3_bind_text(stmt, 1, tick.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, tick.datetime);
    sqlite3_bind_double(stmt, 4, tick.last_price);
    sqlite3_bind_double(stmt, 5, tick.volume);
    sqlite3_bind_double(stmt, 6, tick.turnover);
    sqlite3_bind_double(stmt, 7, tick.open_interest);
    sqlite3_bind_double(stmt, 8, tick.bid_price_1);
    sqlite3_bind_double(stmt, 9, tick.bid_volume_1);
    sqlite3_bind_double(stmt, 10, tick.ask_price_1);
    sqlite3_bind_double(stmt, 11, tick.ask_volume_1);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    return true;
}

int SQLiteDatabase::save_ticks(const std::vector<TickData>& ticks) {
    if (!begin_transaction()) {
        return 0;
    }

    int count = 0;
    for (const auto& tick : ticks) {
        if (save_tick(tick)) {
            count++;
        }
    }

    if (!commit()) {
        rollback();
        return 0;
    }

    return count;
}

std::vector<TickData> SQLiteDatabase::load_ticks(
    const std::string& symbol,
    Exchange exchange,
    int limit
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TickData> ticks;

    const char* sql = R"(
        SELECT symbol, exchange, datetime, last_price, volume, turnover,
               open_interest, bid_price_1, bid_volume_1, ask_price_1, ask_volume_1
        FROM dbtickdata
        WHERE symbol = ? AND exchange = ?
        ORDER BY datetime DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return ticks;
    }

    std::string exchange_str = exchange_to_string(exchange);

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TickData tick;
        tick.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tick.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        tick.datetime = sqlite3_column_int64(stmt, 2);
        tick.last_price = sqlite3_column_double(stmt, 3);
        tick.volume = sqlite3_column_double(stmt, 4);
        tick.turnover = sqlite3_column_double(stmt, 5);
        tick.open_interest = sqlite3_column_double(stmt, 6);
        tick.bid_price_1 = sqlite3_column_double(stmt, 7);
        tick.bid_volume_1 = sqlite3_column_double(stmt, 8);
        tick.ask_price_1 = sqlite3_column_double(stmt, 9);
        tick.ask_volume_1 = sqlite3_column_double(stmt, 10);

        ticks.push_back(tick);
    }

    sqlite3_finalize(stmt);
    std::reverse(ticks.begin(), ticks.end());
    return ticks;
}

std::vector<TickData> SQLiteDatabase::load_ticks_by_time(
    const std::string& symbol,
    Exchange exchange,
    int64_t start_time,
    int64_t end_time
) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TickData> ticks;

    const char* sql = R"(
        SELECT symbol, exchange, datetime, last_price, volume, turnover,
               open_interest, bid_price_1, bid_volume_1, ask_price_1, ask_volume_1
        FROM dbtickdata
        WHERE symbol = ? AND exchange = ?
              AND datetime >= ? AND datetime <= ?
        ORDER BY datetime ASC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return ticks;
    }

    std::string exchange_str = exchange_to_string(exchange);

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, start_time);
    sqlite3_bind_int64(stmt, 4, end_time);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TickData tick;
        tick.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tick.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        tick.datetime = sqlite3_column_int64(stmt, 2);
        tick.last_price = sqlite3_column_double(stmt, 3);
        tick.volume = sqlite3_column_double(stmt, 4);
        tick.turnover = sqlite3_column_double(stmt, 5);
        tick.open_interest = sqlite3_column_double(stmt, 6);
        tick.bid_price_1 = sqlite3_column_double(stmt, 7);
        tick.bid_volume_1 = sqlite3_column_double(stmt, 8);
        tick.ask_price_1 = sqlite3_column_double(stmt, 9);
        tick.ask_volume_1 = sqlite3_column_double(stmt, 10);

        ticks.push_back(tick);
    }

    sqlite3_finalize(stmt);
    return ticks;
}

std::vector<TickOverview> SQLiteDatabase::get_tick_overview() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TickOverview> overviews;

    const char* sql = R"(
        SELECT symbol, exchange, COUNT(*) as count,
               MIN(datetime) as start_time, MAX(datetime) as end_time
        FROM dbtickdata
        GROUP BY symbol, exchange
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return overviews;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TickOverview overview;
        overview.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        overview.exchange = exchange_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        overview.count = sqlite3_column_int64(stmt, 2);
        overview.start_time = sqlite3_column_int64(stmt, 3);
        overview.end_time = sqlite3_column_int64(stmt, 4);

        overviews.push_back(overview);
    }

    sqlite3_finalize(stmt);
    return overviews;
}

//=============================================================================
// Strategy State Operations
//=============================================================================

bool SQLiteDatabase::save_strategy_state(
    const std::string& strategy_name,
    const std::string& state_json
) {
    std::lock_guard<std::mutex> lock(mutex_);

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    const char* sql = R"(
        INSERT OR REPLACE INTO dbstrategydata
        (strategy_name, state_json, updated_at)
        VALUES (?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    sqlite3_bind_text(stmt, 1, strategy_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, state_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    return true;
}

std::optional<std::string> SQLiteDatabase::load_strategy_state(
    const std::string& strategy_name
) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        SELECT state_json FROM dbstrategydata
        WHERE strategy_name = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, strategy_name.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (json) {
            result = json;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

bool SQLiteDatabase::delete_strategy_state(const std::string& strategy_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM dbstrategydata WHERE strategy_name = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    sqlite3_bind_text(stmt, 1, strategy_name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        set_error(sqlite3_errmsg(db_));
        return false;
    }

    return true;
}

//=============================================================================
// General Operations
//=============================================================================

bool SQLiteDatabase::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        set_error(err_msg ? err_msg : "Unknown error");
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool SQLiteDatabase::begin_transaction() {
    return execute("BEGIN TRANSACTION");
}

bool SQLiteDatabase::commit() {
    return execute("COMMIT");
}

bool SQLiteDatabase::rollback() {
    return execute("ROLLBACK");
}

//=============================================================================
// Factory Function
//=============================================================================

std::unique_ptr<IDatabase> create_sqlite_database(const std::string& db_path) {
    return std::make_unique<SQLiteDatabase>(db_path);
}

} // namespace bitquant
