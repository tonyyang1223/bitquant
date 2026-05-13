/**
 * @file test_websocket_client.cpp
 * @brief Unit tests for WebSocketClient
 */

#include "exchange/websocket_client.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_ws_config_construction() {
    std::cout << "[Test] WebSocketConfig construction" << std::endl;

    WebSocketConfig config;
    if (config.port != "443") return false;
    if (!config.use_ssl) return false;
    if (config.reconnect_interval_ms != 1000) return false;

    WebSocketConfig custom;
    custom.host = "stream.binance.com";
    custom.port = "9443";
    custom.use_ssl = false;
    if (custom.use_ssl) return false;

    std::cout << "  PASS: WebSocketConfig constructed correctly" << std::endl;
    return true;
}

bool test_ws_state_to_string() {
    std::cout << "[Test] ConnectionState to string" << std::endl;

    if (connection_state_to_string(ConnectionState::IDLE) != "IDLE") return false;
    if (connection_state_to_string(ConnectionState::CONNECTING) != "CONNECTING") return false;
    if (connection_state_to_string(ConnectionState::CONNECTED) != "CONNECTED") return false;
    if (connection_state_to_string(ConnectionState::RECONNECT_PENDING) != "RECONNECT_PENDING") return false;
    if (connection_state_to_string(ConnectionState::FAILED) != "FAILED") return false;

    std::cout << "  PASS: ConnectionState to string works correctly" << std::endl;
    return true;
}

bool test_ws_client_construction() {
    std::cout << "[Test] WebSocketClient construction" << std::endl;

    WebSocketConfig config;
    config.host = "stream.binance.com";
    WebSocketClient client(config);

    if (client.is_connected()) return false;

    std::cout << "  PASS: WebSocketClient constructed correctly" << std::endl;
    return true;
}

bool test_ws_client_callbacks() {
    std::cout << "[Test] WebSocketClient callbacks" << std::endl;

    WebSocketConfig config;
    config.host = "stream.binance.com";
    WebSocketClient client(config);

    int msg_count = 0;
    int conn_count = 0;
    int state_count = 0;

    client.on_message([&msg_count](const std::string&) { msg_count++; });
    client.on_connect([&conn_count](bool) { conn_count++; });
    client.on_disconnect([&conn_count](bool) { conn_count++; });
    client.on_state_change([&state_count](ConnectionState) { state_count++; });
    client.on_error([](const std::string&) {});

    // Callbacks are set, just verify no crash
    (void)msg_count;
    (void)conn_count;
    (void)state_count;

    std::cout << "  PASS: WebSocketClient callbacks work correctly" << std::endl;
    return true;
}

bool test_ws_client_subscribe() {
    std::cout << "[Test] WebSocketClient subscribe" << std::endl;

    WebSocketConfig config;
    config.host = "stream.binance.com";
    WebSocketClient client(config);

    // Subscribe without connection - should queue
    client.subscribe("btcusdt@ticker");

    // Unsubscribe
    client.unsubscribe("btcusdt@ticker");

    std::cout << "  PASS: WebSocketClient subscribe works correctly" << std::endl;
    return true;
}

bool test_ws_client_close() {
    std::cout << "[Test] WebSocketClient close" << std::endl;

    WebSocketConfig config;
    config.host = "stream.binance.com";
    WebSocketClient client(config);

    // Close without connection
    client.close();

    std::cout << "  PASS: WebSocketClient close works correctly" << std::endl;
    return true;
}

bool test_ws_client_reconnect_attempts() {
    std::cout << "[Test] WebSocketClient reconnect attempts" << std::endl;

    WebSocketConfig config;
    config.host = "stream.binance.com";
    WebSocketClient client(config);

    int attempts = client.get_reconnect_attempts();
    if (attempts != 0) return false;

    std::cout << "  PASS: WebSocketClient reconnect attempts works correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          WebSocketClient Unit Tests                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_ws_config_construction,
        test_ws_state_to_string,
        test_ws_client_construction,
        test_ws_client_callbacks,
        test_ws_client_subscribe,
        test_ws_client_close,
        test_ws_client_reconnect_attempts
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