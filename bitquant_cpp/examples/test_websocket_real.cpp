/**
 * @file test_websocket_real.cpp
 * @brief Test real WebSocket connection to Binance
 */

#include "exchange/websocket_client.hpp"
#include <iostream>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <chrono>

using namespace bitquant;

std::atomic<bool> running{true};
std::atomic<int> message_count{0};

void signal_handler(int) {
    running = false;
    std::cout << "\n[Main] Shutdown signal received..." << std::endl;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     WebSocket Real Connection Test - Binance Live Data     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Configure WebSocket
    WebSocketConfig config;
    config.host = "stream.binance.com";
    config.port = "443";
    config.use_ssl = true;
    config.reconnect_interval_ms = 1000;
    config.max_reconnect_attempts = 10;
    config.ping_interval_ms = 30000;
    config.pong_timeout_ms = 10000;

    WebSocketClient ws(config);

    // Track message stats
    auto start_time = std::chrono::steady_clock::now();
    double last_price = 0.0;

    // Message callback
    ws.on_message([&](const std::string& msg) {
        message_count++;

        // Parse simple JSON to extract price
        // Format: {"stream":"btcusdt@ticker",...,"c":"12345.67",...}
        size_t c_pos = msg.find("\"c\":\"");
        if (c_pos != std::string::npos) {
            c_pos += 5;
            size_t end = msg.find("\"", c_pos);
            if (end != std::string::npos) {
                std::string price_str = msg.substr(c_pos, end - c_pos);
                try {
                    last_price = std::stod(price_str);
                } catch (...) {}
            }
        }
    });

    // State callbacks
    ws.on_state_change([](ConnectionState state) {
        std::cout << "[WebSocket] State: " << connection_state_to_string(state) << std::endl;
    });

    ws.on_connect([](bool connected) {
        std::cout << "[WebSocket] Connected: " << (connected ? "YES" : "NO") << std::endl;
    });

    ws.on_disconnect([](bool) {
        std::cout << "[WebSocket] Disconnected" << std::endl;
    });

    ws.on_reconnect_attempt([](int attempt, int max) {
        std::cout << "[WebSocket] Reconnect attempt " << attempt << "/" << max << std::endl;
    });

    ws.on_error([](const std::string& err) {
        std::cerr << "[WebSocket] Error: " << err << std::endl;
    });

    // Connect
    std::cout << "[1] Connecting to Binance WebSocket..." << std::endl;
    if (!ws.connect()) {
        std::cerr << "Failed to connect!" << std::endl;
        return 1;
    }

    // Subscribe to BTCUSDT ticker
    std::cout << "[2] Subscribing to BTCUSDT ticker..." << std::endl;
    ws.subscribe("btcusdt@ticker");

    // Also subscribe to aggTrade for more frequent updates
    std::cout << "[3] Subscribing to BTCUSDT aggTrade..." << std::endl;
    ws.subscribe("btcusdt@aggTrade");

    std::cout << "\n[4] Waiting for live data (press Ctrl+C to stop)...\n" << std::endl;

    // Display loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        int count = message_count.load();

        if (count > 0) {
            std::cout << "\r[Live] "
                      << std::setw(3) << elapsed << "s | "
                      << "Messages: " << std::setw(6) << count << " | "
                      << "Rate: " << std::fixed << std::setprecision(1)
                      << (elapsed > 0 ? count / static_cast<double>(elapsed) : 0.0) << "/s | "
                      << "BTCUSDT: $" << std::setprecision(2) << last_price
                      << std::flush;
        }
    }

    std::cout << "\n\n[5] Closing connection..." << std::endl;
    ws.close();

    // Final stats
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    int total_count = message_count.load();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    Final Statistics                         ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Total Messages:     " << std::setw(10) << total_count << "                  ║\n";
    std::cout << "║ Duration:           " << std::setw(10) << total_elapsed << " seconds             ║\n";
    std::cout << "║ Average Rate:       " << std::fixed << std::setprecision(1)
              << std::setw(10) << (total_elapsed > 0 ? total_count / static_cast<double>(total_elapsed) : 0.0)
              << " msg/s               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return 0;
}
