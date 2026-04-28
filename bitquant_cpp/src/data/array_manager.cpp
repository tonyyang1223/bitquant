/**
 * @file array_manager.cpp
 * @brief Implementation of ArrayManager with TA-Lib integration
 */

#include "data/array_manager.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace bitquant {

ArrayManager::ArrayManager(size_t size)
    : buffer_(size),
      output_(size),
      output2_(size),
      output3_(size) {}

void ArrayManager::update_bar(const BarData& bar) {
    buffer_.update(bar.datetime, bar.open_price, bar.high_price,
                   bar.low_price, bar.close_price, bar.volume);
}

void ArrayManager::update_bar(int64_t datetime, double open, double high,
                               double low, double close, double volume) {
    buffer_.update(datetime, open, high, low, close, volume);
}

bool ArrayManager::check_ret(TA_RetCode ret) const {
    if (ret != TA_SUCCESS) {
        TA_RetCodeInfo info;
        TA_SetRetCodeInfo(ret, &info);
        std::cerr << "TA-Lib error: " << info.enumStr << " - " << info.infoStr << std::endl;
        return false;
    }
    return true;
}

// ==================== Trend Indicators ====================

double ArrayManager::sma(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_SMA(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

std::vector<double> ArrayManager::sma_array(int period) const {
    std::vector<double> result;
    if (!inited()) return result;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_SMA(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (check_ret(ret)) {
        result.assign(output_.begin(), output_.begin() + outNbElement);
    }
    return result;
}

double ArrayManager::ema(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_EMA(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

std::vector<double> ArrayManager::ema_array(int period) const {
    std::vector<double> result;
    if (!inited()) return result;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_EMA(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (check_ret(ret)) {
        result.assign(output_.begin(), output_.begin() + outNbElement);
    }
    return result;
}

std::tuple<double, double, double> ArrayManager::macd(int fast_period,
                                                       int slow_period,
                                                       int signal_period) const {
    if (!inited()) return {0.0, 0.0, 0.0};

    int outBeg, outNbElement;
    TA_RetCode ret = TA_MACD(0, static_cast<int>(size()) - 1,
                              close(), fast_period, slow_period, signal_period,
                              &outBeg, &outNbElement,
                              output_.data(), output2_.data(), output3_.data());

    if (!check_ret(ret) || outNbElement == 0) return {0.0, 0.0, 0.0};

    int last = outNbElement - 1;
    return {output_[last], output2_[last], output3_[last]};
}

double ArrayManager::adx(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_ADX(0, static_cast<int>(size()) - 1,
                            high(), low(), close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

// ==================== Momentum Indicators ====================

double ArrayManager::rsi(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_RSI(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

std::vector<double> ArrayManager::rsi_array(int period) const {
    std::vector<double> result;
    if (!inited()) return result;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_RSI(0, static_cast<int>(size()) - 1,
                            close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (check_ret(ret)) {
        result.assign(output_.begin(), output_.begin() + outNbElement);
    }
    return result;
}

double ArrayManager::cci(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_CCI(0, static_cast<int>(size()) - 1,
                            high(), low(), close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

std::tuple<double, double> ArrayManager::stoch(int k_period, int d_period) const {
    if (!inited()) return {0.0, 0.0};

    int outBeg, outNbElement;
    TA_RetCode ret = TA_STOCH(0, static_cast<int>(size()) - 1,
                               high(), low(), close(),
                               k_period, 3, TA_MAType_SMA,
                               d_period, TA_MAType_SMA,
                               &outBeg, &outNbElement,
                               output_.data(), output2_.data());

    if (!check_ret(ret) || outNbElement == 0) return {0.0, 0.0};

    int last = outNbElement - 1;
    return {output_[last], output2_[last]};
}

// ==================== Volatility Indicators ====================

double ArrayManager::atr(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_ATR(0, static_cast<int>(size()) - 1,
                            high(), low(), close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

std::vector<double> ArrayManager::atr_array(int period) const {
    std::vector<double> result;
    if (!inited()) return result;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_ATR(0, static_cast<int>(size()) - 1,
                            high(), low(), close(), period,
                            &outBeg, &outNbElement, output_.data());

    if (check_ret(ret)) {
        result.assign(output_.begin(), output_.begin() + outNbElement);
    }
    return result;
}

double ArrayManager::stddev(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_STDDEV(0, static_cast<int>(size()) - 1,
                               close(), period, 1.0,
                               &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

double ArrayManager::natr(int period) const {
    if (!inited()) return 0.0;

    int outBeg, outNbElement;
    TA_RetCode ret = TA_NATR(0, static_cast<int>(size()) - 1,
                             high(), low(), close(), period,
                             &outBeg, &outNbElement, output_.data());

    if (!check_ret(ret) || outNbElement == 0) return 0.0;
    return output_[outNbElement - 1];
}

// ==================== Channel Indicators ====================

ArrayManager::BollingerBands ArrayManager::bollinger(int period, double dev) const {
    if (!inited()) return {0.0, 0.0, 0.0};

    int outBeg, outNbElement;
    TA_RetCode ret = TA_BBANDS(0, static_cast<int>(size()) - 1,
                               close(), period,
                               dev, dev, TA_MAType_SMA,
                               &outBeg, &outNbElement,
                               output_.data(), output2_.data(), output3_.data());

    if (!check_ret(ret) || outNbElement == 0) return {0.0, 0.0, 0.0};

    int last = outNbElement - 1;
    return {output_[last], output2_[last], output3_[last]};
}

ArrayManager::KeltnerChannel ArrayManager::keltner(int period, double dev) const {
    if (!inited()) return {0.0, 0.0, 0.0};

    double mid = sma(period);
    double atr_val = atr(period);

    return {mid + atr_val * dev, mid, mid - atr_val * dev};
}

ArrayManager::DonchianChannel ArrayManager::donchian(int period) const {
    if (!inited()) return {0.0, 0.0};

    int outBeg, outNbElement;

    TA_RetCode ret1 = TA_MAX(0, static_cast<int>(size()) - 1,
                             high(), period,
                             &outBeg, &outNbElement, output_.data());

    TA_RetCode ret2 = TA_MIN(0, static_cast<int>(size()) - 1,
                             low(), period,
                             &outBeg, &outNbElement, output2_.data());

    if (!check_ret(ret1) || !check_ret(ret2) || outNbElement == 0) {
        return {0.0, 0.0};
    }

    int last = outNbElement - 1;
    return {output_[last], output2_[last]};
}

// ==================== Custom Indicators ====================

double ArrayManager::cmi(int period, int ma_period) const {
    if (!inited() || count() < static_cast<size_t>(period + 1)) return 0.0;

    // CMI = 100 * abs(close - close[period]) / (HH - LL)
    double curr_close = latest_close();
    double prev_close = close_at(period);

    // Calculate highest high and lowest low over period
    double highest = buffer_.latest_high();
    double lowest = buffer_.latest_low();

    for (int i = 0; i < period; ++i) {
        // Access from ring buffer
        size_t offset = static_cast<size_t>(i);
        double h = buffer_.high_data()[(buffer_.capacity() - 1 - offset)];
        double l = buffer_.low_data()[(buffer_.capacity() - 1 - offset)];
        highest = std::max(highest, h);
        lowest = std::min(lowest, l);
    }

    double range = highest - lowest;
    if (range == 0.0) return 0.0;

    return 100.0 * std::abs(curr_close - prev_close) / range;
}

std::pair<double, double> ArrayManager::stoch_rsi(int rsi_period, int stoch_period,
                                                   int k_smooth, int d_smooth) const {
    if (!inited()) return {0.0, 0.0};

    // Calculate RSI array
    auto rsi_vals = rsi_array(rsi_period);
    if (rsi_vals.size() < static_cast<size_t>(stoch_period)) {
        return {0.0, 0.0};
    }

    // Calculate Stochastic of RSI
    std::vector<double> k_values(rsi_vals.size() - stoch_period + 1);

    for (size_t i = stoch_period - 1; i < rsi_vals.size(); ++i) {
        double rsi_high = rsi_vals[i - stoch_period + 1];
        double rsi_low = rsi_vals[i - stoch_period + 1];
        double rsi_curr = rsi_vals[i];

        for (size_t j = i - stoch_period + 1; j <= i; ++j) {
            rsi_high = std::max(rsi_high, rsi_vals[j]);
            rsi_low = std::min(rsi_low, rsi_vals[j]);
        }

        double range = rsi_high - rsi_low;
        if (range == 0.0) {
            k_values[i - stoch_period + 1] = 50.0;
        } else {
            k_values[i - stoch_period + 1] = 100.0 * (rsi_curr - rsi_low) / range;
        }
    }

    // Smooth K values
    if (k_values.size() < static_cast<size_t>(k_smooth)) {
        return {0.0, 0.0};
    }

    double fast_k = 0.0;
    for (int i = 0; i < k_smooth; ++i) {
        fast_k += k_values[k_values.size() - 1 - i];
    }
    fast_k /= k_smooth;

    // Smooth D values
    double fast_d = fast_k;  // Simplified, would need more K values for proper smoothing

    return {fast_k, fast_d};
}

} // namespace bitquant
