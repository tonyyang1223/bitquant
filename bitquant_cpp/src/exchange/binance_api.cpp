/**
 * @file binance_api.cpp
 * @brief Binance API implementation using binapi
 */

#include "exchange/binance_api.hpp"
#include <binapi/api.hpp>
#include <binapi/types.hpp>
#include <boost/asio/io_context.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace bitquant {

const char* kline_interval_to_string(KlineInterval interval) {
    switch (interval) {
        case KlineInterval::MIN_1:   return "1m";
        case KlineInterval::MIN_3:   return "3m";
        case KlineInterval::MIN_5:   return "5m";
        case KlineInterval::MIN_15:  return "15m";
        case KlineInterval::MIN_30:  return "30m";
        case KlineInterval::HOUR_1:  return "1h";
        case KlineInterval::HOUR_2:  return "2h";
        case KlineInterval::HOUR_4:  return "4h";
        case KlineInterval::HOUR_6:  return "6h";
        case KlineInterval::HOUR_8:  return "8h";
        case KlineInterval::HOUR_12: return "12h";
        case KlineInterval::DAY_1:   return "1d";
        case KlineInterval::DAY_3:   return "3d";
        case KlineInterval::WEEK_1:  return "1w";
        case KlineInterval::MONTH_1: return "1M";
        default: return "1m";
    }
}

/**
 * @brief Private implementation (PIMPL pattern)
 */
struct BinanceApiClient::Impl {
    std::unique_ptr<boost::asio::io_context> ioctx;
    std::unique_ptr<binapi::rest::api> api;
};

BinanceApiClient::BinanceApiClient()
    : impl_(std::make_unique<Impl>())
{
}

BinanceApiClient::BinanceApiClient(const BinanceConfig& config)
    : impl_(std::make_unique<Impl>())
    , config_(config)
{
}

BinanceApiClient::~BinanceApiClient() = default;

BinanceApiClient::BinanceApiClient(BinanceApiClient&&) noexcept = default;
BinanceApiClient& BinanceApiClient::operator=(BinanceApiClient&&) noexcept = default;

bool BinanceApiClient::init() {
    try {
        impl_->ioctx = std::make_unique<boost::asio::io_context>();
        impl_->api = std::make_unique<binapi::rest::api>(
            *impl_->ioctx,
            config_.host,
            config_.port,
            config_.api_key,
            config_.api_secret,
            config_.timeout_ms
        );
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Init failed: ") + e.what();
        return false;
    }
}

bool BinanceApiClient::ping() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    auto result = impl_->api->ping();
    if (!result) {
        last_error_ = result.errmsg;
        return false;
    }
    return true;
}

uint64_t BinanceApiClient::get_server_time() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0;
    }

    auto result = impl_->api->server_time();
    if (!result) {
        last_error_ = result.errmsg;
        return 0;
    }
    return result.v.serverTime;
}

std::vector<BarData> BinanceApiClient::get_klines(
    const std::string& symbol,
    KlineInterval interval,
    size_t limit
) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return {};
    }

    std::vector<BarData> bars;
    bars.reserve(limit);

    try {
        auto result = impl_->api->klines(
            symbol,
            kline_interval_to_string(interval),
            std::min(limit, size_t(1000))
        );

        if (!result) {
            last_error_ = result.errmsg;
            return {};
        }

        for (const auto& k : result.v.klines) {
            BarData bar;
            bar.datetime = static_cast<int64_t>(k.start_time);
            bar.open_price = static_cast<double>(k.open);
            bar.high_price = static_cast<double>(k.high);
            bar.low_price = static_cast<double>(k.low);
            bar.close_price = static_cast<double>(k.close);
            bar.volume = static_cast<double>(k.volume);
            bars.push_back(bar);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("get_klines failed: ") + e.what();
        return {};
    }

    return bars;
}

double BinanceApiClient::get_price(const std::string& symbol) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0.0;
    }

    try {
        auto result = impl_->api->price(symbol.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return 0.0;
        }
        return static_cast<double>(result.v.price);
    } catch (const std::exception& e) {
        last_error_ = std::string("get_price failed: ") + e.what();
        return 0.0;
    }
}

size_t BinanceApiClient::fetch_historical_data(
    const std::string& symbol,
    KlineInterval interval,
    size_t total_bars,
    const std::string& output_dir
) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0;
    }

    const size_t batch_size = 1000;
    size_t fetched = 0;

    // binapi only supports fetching latest klines, not historical with time range
    // We can only fetch up to 1000 most recent bars per interval
    size_t to_fetch = std::min(total_bars, batch_size);

    auto klines = get_klines(symbol, interval, to_fetch);
    if (klines.empty()) {
        return 0;
    }

    // Save to CSV with end_time as filename (like Python version)
    uint64_t end_time = static_cast<uint64_t>(klines.back().datetime);
    std::ostringstream filename;
    filename << output_dir << "/" << end_time << ".csv";
    save_klines_to_csv(klines, filename.str());

    fetched = klines.size();
    std::cout << "Fetched " << fetched << " bars, saved to " << filename.str() << std::endl;

    // Note: For fetching more historical data, you would need to:
    // 1. Modify binapi to support startTime/endTime parameters, or
    // 2. Use a different approach (e.g., store timestamps and fetch incrementally)

    if (total_bars > batch_size) {
        std::cout << "Note: binapi only supports fetching latest " << batch_size
                  << " bars. Requested " << total_bars << " bars." << std::endl;
    }

    return fetched;
}

bool save_klines_to_csv(
    const std::vector<BarData>& klines,
    const std::string& filename
) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Header
    file << "open_time,open,high,low,close,volume\n";

    // Data
    file << std::fixed << std::setprecision(8);
    for (const auto& bar : klines) {
        file << bar.datetime << ","
             << bar.open_price << ","
             << bar.high_price << ","
             << bar.low_price << ","
             << bar.close_price << ","
             << bar.volume << "\n";
    }

    return true;
}

std::vector<BarData> load_klines_from_csv(const std::string& filename) {
    std::vector<BarData> bars;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return bars;
    }

    std::string line;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        BarData bar;

        std::getline(ss, token, ',');
        bar.datetime = std::stoll(token);

        std::getline(ss, token, ',');
        bar.open_price = std::stod(token);

        std::getline(ss, token, ',');
        bar.high_price = std::stod(token);

        std::getline(ss, token, ',');
        bar.low_price = std::stod(token);

        std::getline(ss, token, ',');
        bar.close_price = std::stod(token);

        std::getline(ss, token, ',');
        bar.volume = std::stod(token);

        bars.push_back(bar);
    }

    return bars;
}

} // namespace bitquant
