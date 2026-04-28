/**
 * @file test_array_manager.cpp
 * @brief Unit tests for ArrayManager
 */

#include <gtest/gtest.h>
#include "data/array_manager.hpp"

using namespace bitquant;

class ArrayManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        am_ = std::make_unique<ArrayManager>(100);

        // Fill with test data
        for (int i = 0; i < 100; ++i) {
            double price = 100.0 + i * 0.1;
            am_->update_bar(i * 60000,  // 1 minute intervals
                           price - 0.5,  // open
                           price + 1.0,  // high
                           price - 1.0,  // low
                           price,        // close
                           1000.0);      // volume
        }
    }

    std::unique_ptr<ArrayManager> am_;
};

TEST_F(ArrayManagerTest, Initialization) {
    EXPECT_TRUE(am_->inited());
    EXPECT_EQ(am_->count(), 100);
    EXPECT_EQ(am_->size(), 100);
}

TEST_F(ArrayManagerTest, SMA) {
    double sma = am_->sma(20);
    EXPECT_GT(sma, 0.0);

    auto sma_arr = am_->sma_array(20);
    EXPECT_EQ(sma_arr.size(), 100 - 19);  // 20-period SMA has 19 lookback
}

TEST_F(ArrayManagerTest, EMA) {
    double ema = am_->ema(20);
    EXPECT_GT(ema, 0.0);
}

TEST_F(ArrayManagerTest, RSI) {
    double rsi = am_->rsi(14);
    EXPECT_GE(rsi, 0.0);
    EXPECT_LE(rsi, 100.0);
}

TEST_F(ArrayManagerTest, MACD) {
    auto [macd_line, signal, hist] = am_->macd(12, 26, 9);
    // MACD should have some value
    EXPECT_NE(macd_line, 0.0);
}

TEST_F(ArrayManagerTest, ATR) {
    double atr = am_->atr(14);
    EXPECT_GT(atr, 0.0);
}

TEST_F(ArrayManagerTest, BollingerBands) {
    auto bb = am_->bollinger(20, 2.0);
    EXPECT_GT(bb.upper, bb.middle);
    EXPECT_GT(bb.middle, bb.lower);
}

TEST_F(ArrayManagerTest, DonchianChannel) {
    auto dc = am_->donchian(20);
    EXPECT_GT(dc.upper, dc.lower);
}

TEST_F(ArrayManagerTest, LatestClose) {
    double close = am_->latest_close();
    EXPECT_GT(close, 0.0);
}
