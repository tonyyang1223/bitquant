/**
 * @file test_broker.cpp
 * @brief Unit tests for Broker
 */

#include <gtest/gtest.h>
#include "engine/broker.hpp"
#include "engine/strategy.hpp"

using namespace bitquant;

class TestStrategy : public IStrategy {
public:
    int bar_count = 0;
    int trade_count = 0;

    void on_bar(const BarData& bar) override {
        ++bar_count;

        // Simple strategy: buy on first bar, sell on 10th
        if (bar_count == 1) {
            buy(bar.close_price, 1.0);
        } else if (bar_count == 10) {
            sell(bar.close_price, 1.0);
        }
    }

    void on_trade(const Trade& trade) override {
        ++trade_count;
    }
};

class BrokerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data
        for (int i = 0; i < 20; ++i) {
            BarData bar;
            bar.datetime = i * 60000;
            bar.open_price = 100.0 + i * 0.5;
            bar.high_price = 100.0 + i * 0.5 + 1.0;
            bar.low_price = 100.0 + i * 0.5 - 1.0;
            bar.close_price = 100.0 + i * 0.5;
            bar.volume = 1000.0;
            data_.push_back(bar);
        }
    }

    std::vector<BarData> data_;
};

TEST_F(BrokerTest, BasicSetup) {
    BrokerConfig config;
    config.initial_cash = 100000.0;

    Broker broker(config);
    broker.set_data(data_);

    auto strategy = std::make_unique<TestStrategy>();
    broker.set_strategy(std::move(strategy));

    broker.run();

    // Should have processed all bars
    EXPECT_EQ(broker.performance().total_trades, 2);  // Buy + Sell
}

TEST_F(BrokerTest, PositionTracking) {
    BrokerConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;

    Broker broker(config);
    broker.set_data(data_);

    auto strategy = std::make_unique<TestStrategy>();
    broker.set_strategy(std::move(strategy));

    broker.run();

    // Check equity curve
    const auto& equity_curve = broker.equity_curve();
    EXPECT_EQ(equity_curve.size(), data_.size());
}

TEST_F(BrokerTest, PerformanceMetrics) {
    BrokerConfig config;
    config.initial_cash = 100000.0;

    Broker broker(config);
    broker.set_data(data_);

    auto strategy = std::make_unique<TestStrategy>();
    broker.set_strategy(std::move(strategy));

    broker.run();

    const auto& perf = broker.performance();
    EXPECT_EQ(perf.initial_capital, 100000.0);
    EXPECT_GT(perf.total_trades, 0);
}
