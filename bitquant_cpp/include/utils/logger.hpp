/**
 * @file logger.hpp
 * @brief High-performance logging system for BitQuant
 *
 * Features:
 * - Multi-level logging (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * - File and console output
 * - Thread-safe
 * - Performance optimized with async logging
 */

#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <ctime>
#include <sstream>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>

namespace bitquant {

/**
 * @brief Log level enumeration
 */
enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Log configuration
 */
struct LogConfig {
    LogLevel min_level = LogLevel::INFO;
    bool console_output = true;
    bool file_output = false;
    std::string log_file = "bitquant.log";
    bool async_mode = true;
    size_t max_file_size = 10 * 1024 * 1024;  // 10MB
};

/**
 * @brief Log message structure
 */
struct LogMessage {
    LogLevel level;
    std::string file;
    int line;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief High-performance logger
 *
 * Thread-safe logging with optional async mode for better performance.
 * Usage:
 *   LOG_INFO("Strategy started with params: {}", params);
 *   LOG_ERROR("Order failed: {}", error_msg);
 */
class Logger {
public:
    static Logger& instance();

    // Configuration
    void configure(const LogConfig& config);
    void set_level(LogLevel level);
    void set_file(const std::string& filename);
    void set_console(bool enabled);
    void set_async(bool enabled);

    // Logging methods
    void log(LogLevel level, const char* file, int line, const char* fmt, ...);
    void log(LogLevel level, const char* file, int line, const std::string& message);

    // Flush pending messages (for async mode)
    void flush();

    // Shutdown
    void shutdown();

    // Level to string
    static const char* level_string(LogLevel level);

private:
    Logger();
    ~Logger();

    void write_message(const LogMessage& msg);
    void async_worker();
    void format_timestamp(std::ostringstream& oss, const std::chrono::system_clock::time_point& tp);

    LogConfig config_;
    std::mutex mutex_;
    std::ofstream file_stream_;
    std::atomic<bool> running_{true};

    // Async mode
    std::queue<LogMessage> message_queue_;
    std::condition_variable queue_cv_;
    std::thread async_thread_;

    // Disable copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// Convenient macros for logging
#define LOG_DEBUG(...)  bitquant::Logger::instance().log(bitquant::LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)   bitquant::Logger::instance().log(bitquant::LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)   bitquant::Logger::instance().log(bitquant::LogLevel::WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)  bitquant::Logger::instance().log(bitquant::LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) bitquant::Logger::instance().log(bitquant::LogLevel::CRITICAL, __FILE__, __LINE__, __VA_ARGS__)

// Conditional logging (for performance)
#define LOG_DEBUG_IF(cond, ...) if(cond) LOG_DEBUG(__VA_ARGS__)
#define LOG_INFO_IF(cond, ...) if(cond) LOG_INFO(__VA_ARGS__)

} // namespace bitquant