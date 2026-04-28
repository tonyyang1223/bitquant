/**
 * @file logger.cpp
 * @brief Logger implementation
 */

#include "utils/logger.hpp"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace bitquant {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    if (config_.async_mode) {
        async_thread_ = std::thread(&Logger::async_worker, this);
    }
}

Logger::~Logger() {
    shutdown();
}

void Logger::shutdown() {
    running_.store(false);
    queue_cv_.notify_all();
    if (async_thread_.joinable()) {
        async_thread_.join();
    }
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::configure(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    if (config_.file_output && !config_.log_file.empty()) {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        file_stream_.open(config_.log_file, std::ios::app);
    }
}

void Logger::set_level(LogLevel level) {
    config_.min_level = level;
}

void Logger::set_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.file_output = true;
    config_.log_file = filename;
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    file_stream_.open(filename, std::ios::app);
}

void Logger::set_console(bool enabled) {
    config_.console_output = enabled;
}

void Logger::set_async(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (config_.async_mode == enabled) return;

    if (config_.async_mode) {
        // Stop async thread
        running_.store(false);
        queue_cv_.notify_all();
        if (async_thread_.joinable()) {
            async_thread_.join();
        }
    }

    config_.async_mode = enabled;
    running_.store(true);

    if (enabled) {
        async_thread_ = std::thread(&Logger::async_worker, this);
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
    }
}

const char* Logger::level_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (static_cast<uint8_t>(level) < static_cast<uint8_t>(config_.min_level)) {
        return;
    }

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log(level, file, line, std::string(buffer));
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& message) {
    if (static_cast<uint8_t>(level) < static_cast<uint8_t>(config_.min_level)) {
        return;
    }

    LogMessage msg;
    msg.level = level;
    msg.file = file;
    msg.line = line;
    msg.message = message;
    msg.timestamp = std::chrono::system_clock::now();

    if (config_.async_mode) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_queue_.push(msg);
        queue_cv_.notify_one();
    } else {
        write_message(msg);
    }
}

void Logger::write_message(const LogMessage& msg) {
    std::ostringstream oss;

    // Format: [2026-04-27 12:00:00.123] [INFO] [file:line] message
    format_timestamp(oss, msg.timestamp);
    oss << " [" << level_string(msg.level) << "] ";

    // Extract filename from path
    const char* filename = msg.file.c_str();
    const char* last_slash = strrchr(filename, '/');
    if (last_slash) {
        filename = last_slash + 1;
    }

    oss << "[" << filename << ":" << msg.line << "] ";
    oss << msg.message << "\n";

    std::string output = oss.str();

    // Console output
    if (config_.console_output) {
        // Color coding for console
        const char* color = "";
        switch (msg.level) {
            case LogLevel::DEBUG:    color = "\033[36m"; break;   // Cyan
            case LogLevel::INFO:     color = "\033[32m"; break;   // Green
            case LogLevel::WARNING:  color = "\033[33m"; break;   // Yellow
            case LogLevel::ERROR:    color = "\033[31m"; break;   // Red
            case LogLevel::CRITICAL: color = "\033[35m"; break;   // Magenta
        }
        std::cout << color << output << "\033[0m";
    }

    // File output
    if (config_.file_output && file_stream_.is_open()) {
        file_stream_ << output;
    }
}

void Logger::async_worker() {
    while (running_.load()) {
        LogMessage msg;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_cv_.wait(lock, [this] {
                return !message_queue_.empty() || !running_.load();
            });

            if (!running_.load() && message_queue_.empty()) {
                break;
            }

            if (message_queue_.empty()) {
                continue;
            }

            msg = message_queue_.front();
            message_queue_.pop();
        }

        write_message(msg);
    }
}

void Logger::format_timestamp(std::ostringstream& oss, const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_r(&time, &tm);

    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
}

} // namespace bitquant