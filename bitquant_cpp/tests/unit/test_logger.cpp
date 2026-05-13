/**
 * @file test_logger.cpp
 * @brief Unit tests for Logger
 */

#include "utils/logger.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_logger_instance() {
    std::cout << "[Test] Logger::instance" << std::endl;

    Logger& logger = Logger::instance();
    (void)logger;

    std::cout << "  PASS: Logger instance works" << std::endl;
    return true;
}

bool test_logger_configure() {
    std::cout << "[Test] Logger::configure" << std::endl;

    LogConfig config;
    config.min_level = LogLevel::INFO;
    config.console_output = false;
    config.file_output = false;

    Logger::instance().configure(config);
    // Should not crash

    std::cout << "  PASS: Logger configure works" << std::endl;
    return true;
}

bool test_logger_set_level() {
    std::cout << "[Test] Logger::set_level" << std::endl;

    Logger::instance().set_level(LogLevel::DEBUG);
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_level(LogLevel::ERROR);
    // Should not crash

    std::cout << "  PASS: set_level works" << std::endl;
    return true;
}

bool test_logger_set_console() {
    std::cout << "[Test] Logger::set_console" << std::endl;

    Logger::instance().set_console(true);
    Logger::instance().set_console(false);
    // Should not crash

    std::cout << "  PASS: set_console works" << std::endl;
    return true;
}

bool test_logger_set_async() {
    std::cout << "[Test] Logger::set_async" << std::endl;

    Logger::instance().set_async(false);
    Logger::instance().set_async(true);
    // Should not crash

    std::cout << "  PASS: set_async works" << std::endl;
    return true;
}

bool test_logger_log() {
    std::cout << "[Test] Logger::log" << std::endl;

    LogConfig config;
    config.min_level = LogLevel::INFO;
    config.console_output = false;
    Logger::instance().configure(config);

    Logger::instance().log(LogLevel::INFO, __FILE__, __LINE__, "Test message");
    Logger::instance().log(LogLevel::ERROR, __FILE__, __LINE__, "Test error");
    // Should not crash

    std::cout << "  PASS: log works" << std::endl;
    return true;
}

bool test_logger_flush() {
    std::cout << "[Test] Logger::flush" << std::endl;

    Logger::instance().flush();
    // Should not crash

    std::cout << "  PASS: flush works" << std::endl;
    return true;
}

bool test_logger_level_string() {
    std::cout << "[Test] Logger::level_string" << std::endl;

    if (Logger::level_string(LogLevel::DEBUG) == nullptr) return false;
    if (Logger::level_string(LogLevel::INFO) == nullptr) return false;
    if (Logger::level_string(LogLevel::ERROR) == nullptr) return false;

    std::cout << "  PASS: level_string works" << std::endl;
    return true;
}

bool test_logger_shutdown() {
    std::cout << "[Test] Logger::shutdown" << std::endl;

    Logger::instance().shutdown();
    // Should not crash

    std::cout << "  PASS: shutdown works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Logger Unit Tests                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_logger_instance,
        test_logger_configure,
        test_logger_set_level,
        test_logger_set_console,
        test_logger_set_async,
        test_logger_log,
        test_logger_flush,
        test_logger_level_string,
        test_logger_shutdown
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}