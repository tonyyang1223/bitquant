/**
 * @file demo_exchange.cpp
 * @brief Demonstration of unified exchange interface
 *
 * Shows how to use:
 * - ExchangeFactory to create exchange instances
 * - IExchange interface for market data
 * - WebSocket for real-time streaming
 */

#include "exchange/i_exchange.hpp"
#include "exchange/binance_gateway.hpp"
#include "exchange/websocket_client.hpp"
#include <iostream>
#include <iomanip>

using namespace bitquant;

void print_separator(const std::string& title) {
    std::cout << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demo_exchange_factory() {
    print_separator("Exchange Factory Demo");

    // List registered exchanges
    auto exchanges = ExchangeFactory::list_exchanges();
    std::cout << "Registered exchanges: ";
    for (const auto& name : exchanges) {
        std::cout << name << " ";
    }
    std::cout << "\n";

    // Create Binance gateway
    auto binance = ExchangeFactory::create("binance");
    if (binance) {
        std::cout << "Created exchange: " << binance->name() << "\n";
        std::cout << "Gateway name: " << binance->gateway_name() << "\n";

        auto caps = binance->capabilities();
        std::cout << "Capabilities:\n";
        std::cout << "  - Market data: " << (caps.market_data ? "Yes" : "No") << "\n";
        std::cout << "  - Trading: " << (caps.trading ? "Yes" : "No") << "\n";
        std::cout << "  - WebSocket: " << (caps.websocket ? "Yes" : "No") << "\n";
        std::cout << "  - Futures: " << (caps.futures ? "Yes" : "No") << "\n";
    }
}

void demo_market_data() {
    print_separator("Market Data Demo");

    // Create and connect Binance gateway
    BinanceGateway binance;
    std::cout << "Connecting to Binance...\n";

    if (!binance.connect("")) {
        std::cerr << "Failed to connect: " << binance.last_error() << "\n";
        return;
    }

    std::cout << "Connected!\n";

    // Get server time
    uint64_t server_time = binance.get_server_time();
    std::cout << "Server time: " << server_time << " ms\n";

    // Get current price
    std::string symbol = "BTCUSDT";
    double price = binance.get_price(symbol);
    std::cout << "Current " << symbol << " price: $" << std::fixed << std::setprecision(2) << price << "\n";

    // Get tick data
    auto tick = binance.get_tick(symbol);
    if (tick) {
        std::cout << "Tick data:\n";
        std::cout << "  Symbol: " << tick->symbol << "\n";
        std::cout << "  Last price: $" << tick->last_price << "\n";
        std::cout << "  Timestamp: " << tick->datetime << "\n";
    }

    // Get historical bars
    HistoryRequest req;
    req.symbol = symbol;
    req.interval = Interval::MINUTE_1;
    req.limit = 10;

    std::cout << "\nFetching last " << req.limit << " bars for " << symbol << "...\n";
    auto bars = binance.get_bars(req);
    std::cout << "Received " << bars.size() << " bars\n";

    if (!bars.empty()) {
        std::cout << "\nLatest bars:\n";
        std::cout << std::setw(15) << "Time"
                  << std::setw(12) << "Open"
                  << std::setw(12) << "High"
                  << std::setw(12) << "Low"
                  << std::setw(12) << "Close"
                  << std::setw(12) << "Volume" << "\n";

        int count = 0;
        for (const auto& bar : bars) {
            if (count++ >= 5) break;

            // Convert timestamp to readable time
            time_t time = bar.datetime / 1000;
            struct tm* tm_info = localtime(&time);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%m-%d %H:%M", tm_info);

            std::cout << std::setw(15) << time_str
                      << std::setw(12) << std::fixed << std::setprecision(2) << bar.open_price
                      << std::setw(12) << bar.high_price
                      << std::setw(12) << bar.low_price
                      << std::setw(12) << bar.close_price
                      << std::setw(12) << std::setprecision(4) << bar.volume << "\n";
        }
    }

    binance.close();
    std::cout << "Disconnected.\n";
}

void demo_websocket_streams() {
    print_separator("WebSocket Stream Names Demo");

    std::string symbol = "BTCUSDT";

    std::cout << "Binance WebSocket streams for " << symbol << ":\n";
    std::cout << "  Aggregate trade: " << BinanceStream::agg_trade(symbol) << "\n";
    std::cout << "  Trade:           " << BinanceStream::trade(symbol) << "\n";
    std::cout << "  Kline (1m):      " << BinanceStream::kline(symbol, "1m") << "\n";
    std::cout << "  Mini ticker:     " << BinanceStream::mini_ticker(symbol) << "\n";
    std::cout << "  Book ticker:     " << BinanceStream::book_ticker(symbol) << "\n";
    std::cout << "  Depth (5):       " << BinanceStream::depth(symbol, 5) << "\n";

    std::vector<std::string> streams = {
        BinanceStream::agg_trade(symbol),
        BinanceStream::kline(symbol, "1m"),
        BinanceStream::book_ticker(symbol)
    };
    std::cout << "\nCombined stream: " << BinanceStream::combine(streams) << "\n";
}

void demo_websocket_client() {
    print_separator("WebSocket Client Demo");

    WebSocketConfig config;
    config.host = "stream.binance.com";
    config.port = "9443";
    config.use_ssl = true;

    WebSocketClient ws(config);

    ws.on_message([](const std::string& msg) {
        std::cout << "Received: " << msg.substr(0, 100) << "...\n";
    });

    ws.on_connect([](bool connected) {
        std::cout << "WebSocket " << (connected ? "connected" : "disconnected") << "\n";
    });

    ws.on_error([](const std::string& error) {
        std::cerr << "WebSocket error: " << error << "\n";
    });

    std::cout << "Connecting to WebSocket...\n";
    ws.connect();

    // Subscribe to BTC/USDT streams
    ws.subscribe(BinanceStream::ticker("BTCUSDT"));
    ws.subscribe(BinanceStream::kline("BTCUSDT", "1m"));

    std::cout << "Active subscriptions:\n";
    for (const auto& sub : ws.get_subscriptions()) {
        std::cout << "  - " << sub << "\n";
    }

    // Let it run for a few seconds
    std::cout << "\nWaiting for data (3 seconds)...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    ws.close();
    std::cout << "WebSocket closed.\n";
}

void demo_iexchange_interface() {
    print_separator("IExchange Interface Demo");

    // Create exchange using factory
    auto exchange = ExchangeFactory::create("binance");

    // Register callbacks
    exchange->on_tick([](const TickData& tick) {
        std::cout << "Tick: " << tick.symbol << " @ $" << tick.last_price << "\n";
    });

    exchange->on_bar([](const BarData& bar) {
        std::cout << "Bar: " << bar.symbol
                  << " O:" << bar.open_price
                  << " H:" << bar.high_price
                  << " L:" << bar.low_price
                  << " C:" << bar.close_price << "\n";
    });

    // Connect
    std::cout << "Connecting to exchange...\n";
    if (!exchange->connect("")) {
        std::cerr << "Failed to connect\n";
        return;
    }
    std::cout << "Connected!\n";

    // Subscribe to data
    SubscribeRequest req;
    req.symbol = "BTCUSDT";
    exchange->subscribe_tick(req);
    exchange->subscribe_bar("BTCUSDT", Interval::MINUTE_1);

    // Query market data
    double price = exchange->get_price("BTCUSDT");
    std::cout << "BTC/USDT price: $" << price << "\n";

    // Close
    exchange->close();
    std::cout << "Disconnected.\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       BitQuant C++ Unified Exchange Interface Demo         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    try {
        demo_exchange_factory();
        demo_market_data();
        demo_websocket_streams();
        demo_websocket_client();
        demo_iexchange_interface();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    print_separator("Demo Complete");
    std::cout << "All demonstrations completed successfully!\n";

    return 0;
}
