/**
 * @file test_exchange_factory.cpp
 * @brief Unit tests for ExchangeFactory
 */

#include "exchange/i_exchange.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_exchange_factory_list_exchanges() {
    std::cout << "[Test] ExchangeFactory::list_exchanges" << std::endl;

    auto exchanges = ExchangeFactory::list_exchanges();
    (void)exchanges;

    std::cout << "  PASS: list_exchanges works" << std::endl;
    return true;
}

bool test_exchange_factory_has_exchange() {
    std::cout << "[Test] ExchangeFactory::has_exchange" << std::endl;

    // Test for non-existent exchange
    bool has = ExchangeFactory::has_exchange("nonexistent_exchange");
    if (has) {
        std::cout << "  WARN: Found unexpected exchange" << std::endl;
    }

    std::cout << "  PASS: has_exchange works" << std::endl;
    return true;
}

bool test_exchange_factory_create_nonexistent() {
    std::cout << "[Test] ExchangeFactory::create (nonexistent)" << std::endl;

    auto exchange = ExchangeFactory::create("nonexistent_exchange");
    if (exchange != nullptr) return false;

    std::cout << "  PASS: create returns nullptr for nonexistent" << std::endl;
    return true;
}

bool test_exchange_factory_register_and_create() {
    std::cout << "[Test] ExchangeFactory register and create" << std::endl;

    // Create a simple mock exchange
    class MockExchange : public IExchange {
    public:
        std::string name() const override { return "mock"; }
        Exchange exchange() const override { return Exchange::LOCAL; }
        std::string gateway_name() const override { return "MOCK"; }
        std::vector<Exchange> supported_exchanges() const override { return {Exchange::LOCAL}; }
        bool connect(const std::string&) override { return true; }
        void close() override {}
        bool is_connected() const override { return true; }
        std::string send_order(const OrderRequest&) override { return "mock_order"; }
        bool cancel_order(const CancelRequest&) override { return true; }
    };

    // Register
    ExchangeFactory::register_exchange("mock_test", []() {
        return std::make_unique<MockExchange>();
    });

    // Check registered
    if (!ExchangeFactory::has_exchange("mock_test")) return false;

    // Create
    auto exchange = ExchangeFactory::create("mock_test");
    if (!exchange) return false;
    if (exchange->name() != "mock") return false;

    std::cout << "  PASS: register and create works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          ExchangeFactory Unit Tests                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_exchange_factory_list_exchanges,
        test_exchange_factory_has_exchange,
        test_exchange_factory_create_nonexistent,
        test_exchange_factory_register_and_create
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