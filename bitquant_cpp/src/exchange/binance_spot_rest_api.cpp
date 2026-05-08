/**
 * @file binance_spot_rest_api.cpp
 * @brief Binance Spot REST API client implementation
 *
 * Wraps binapi library for Spot trading:
 * - Market data: ping, time, klines, price, exchange info
 * - Trading: send_order, cancel_order, query_order
 * - Account: query_account, query_open_orders
 * - User stream: start/keep/close user data stream
 */

#include "exchange/binance_spot_rest_api.hpp"
#include <binapi/api.hpp>
#include <binapi/types.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace bitquant {

//=============================================================================
// PIMPL Implementation
//=============================================================================

struct BinanceSpotRestApi::Impl {
    std::unique_ptr<boost::asio::io_context> ioctx;
    std::unique_ptr<binapi::rest::api> api;
};

//=============================================================================
// Constructor / Destructor
//=============================================================================

BinanceSpotRestApi::BinanceSpotRestApi()
    : impl_(std::make_unique<Impl>())
{
}

BinanceSpotRestApi::~BinanceSpotRestApi() = default;

//=============================================================================
// Initialization
//=============================================================================

bool BinanceSpotRestApi::init(const std::string& host,
                              const std::string& port,
                              const std::string& api_key,
                              const std::string& api_secret,
                              size_t timeout_ms) {
    host_ = host;
    port_ = port;
    api_key_ = api_key;
    api_secret_ = api_secret;
    timeout_ms_ = timeout_ms;

    try {
        impl_->ioctx = std::make_unique<boost::asio::io_context>();
        impl_->api = std::make_unique<binapi::rest::api>(
            *impl_->ioctx,
            host_,
            port_,
            api_key_,
            api_secret_,
            timeout_ms_
        );
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Init failed: ") + e.what();
        return false;
    }
}

//=============================================================================
// Market Data
//=============================================================================

bool BinanceSpotRestApi::ping() {
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

int64_t BinanceSpotRestApi::get_server_time() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return 0;
    }

    auto result = impl_->api->server_time();
    if (!result) {
        last_error_ = result.errmsg;
        return 0;
    }

    // Update time offset
    int64_t local_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    time_offset_ = local_time - static_cast<int64_t>(result.v.serverTime);

    return result.v.serverTime;
}

double BinanceSpotRestApi::get_price(const std::string& symbol) {
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

std::vector<BarData> BinanceSpotRestApi::get_klines(const std::string& symbol,
                                                    Interval interval,
                                                    size_t limit) {
    std::vector<BarData> bars;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return bars;
    }

    try {
        std::string interval_str = interval_to_binance(interval);
        auto result = impl_->api->klines(
            symbol.c_str(),
            interval_str.c_str(),
            std::min(limit, size_t(1000))
        );

        if (!result) {
            last_error_ = result.errmsg;
            return bars;
        }

        bars.reserve(result.v.klines.size());
        for (const auto& k : result.v.klines) {
            BarData bar;
            bar.symbol = symbol;
            bar.exchange = Exchange::BINANCE;
            bar.interval = interval;
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
    }

    return bars;
}

std::vector<ContractData> BinanceSpotRestApi::get_exchange_info() {
    std::vector<ContractData> contracts;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return contracts;
    }

    try {
        auto result = impl_->api->exchange_info();
        if (!result) {
            last_error_ = result.errmsg;
            return contracts;
        }

        for (const auto& [sym_name, sym] : result.v.symbols) {
            ContractData contract;
            contract.symbol = sym.symbol;
            contract.exchange = Exchange::BINANCE;
            contract.product = Product::SPOT;
            contract.history_data = true;

            // Parse filters
            for (const auto& f : sym.filters) {
                const std::string& filter_type = f.filterType;

                if (filter_type == "PRICE_FILTER") {
                    // Use boost::get to extract price filter
                    const auto* price_filter = boost::get<binapi::rest::exchange_info_t::symbol_t::filter_t::price_t>(&f.filter);
                    if (price_filter) {
                        contract.pricetick = static_cast<double>(price_filter->tickSize);
                    }
                } else if (filter_type == "LOT_SIZE") {
                    const auto* lot_filter = boost::get<binapi::rest::exchange_info_t::symbol_t::filter_t::lot_size_t>(&f.filter);
                    if (lot_filter) {
                        contract.min_volume = static_cast<double>(lot_filter->stepSize);
                    }
                } else if (filter_type == "MIN_NOTIONAL" || filter_type == "NOTIONAL") {
                    // Try MIN_NOTIONAL first
                    const auto* min_notional = boost::get<binapi::rest::exchange_info_t::symbol_t::filter_t::min_notional_t>(&f.filter);
                    if (min_notional) {
                        contract.min_notional = static_cast<double>(min_notional->minNotional);
                    } else {
                        // Try NOTIONAL filter
                        const auto* notional_filter = boost::get<binapi::rest::exchange_info_t::symbol_t::filter_t::notional_t>(&f.filter);
                        if (notional_filter) {
                            contract.min_notional = static_cast<double>(notional_filter->minNotional);
                        }
                    }
                }
            }

            contracts.push_back(contract);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("get_exchange_info failed: ") + e.what();
    }

    return contracts;
}

//=============================================================================
// Trading
//=============================================================================

std::string BinanceSpotRestApi::send_order(const OrderRequest& req) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return "";
    }

    try {
        // Determine order side
        binapi::e_side e_side;
        if (req.direction == Direction::LONG) {
            e_side = binapi::e_side::buy;
        } else {
            e_side = binapi::e_side::sell;
        }

        // Determine order type
        binapi::e_type e_type;
        if (req.type == OrderType::LIMIT) {
            e_type = binapi::e_type::limit;
        } else if (req.type == OrderType::TAKER) {
            e_type = binapi::e_type::market;
        } else if (req.type == OrderType::MAKER) {
            e_type = binapi::e_type::limit_maker;
        } else if (req.type == OrderType::STOP) {
            e_type = binapi::e_type::stop_loss;
        } else if (req.type == OrderType::STOP_LIMIT) {
            e_type = binapi::e_type::stop_loss_limit;
        } else {
            e_type = binapi::e_type::limit;
        }

        // Time in force (default GTC for limit orders)
        binapi::e_time e_time = binapi::e_time::GTC;

        // Format volume and price as strings
        std::ostringstream volume_ss, price_ss;
        volume_ss << std::fixed << std::setprecision(8) << req.volume;
        price_ss << std::fixed << std::setprecision(8) << req.price;

        // Send order
        auto result = impl_->api->new_order(
            req.symbol.c_str(),
            e_side,
            e_type,
            e_time,
            binapi::e_trade_resp_type::RESULT,
            volume_ss.str().c_str(),
            (req.type == OrderType::LIMIT || req.type == OrderType::MAKER || req.type == OrderType::STOP_LIMIT)
                ? price_ss.str().c_str() : nullptr,
            nullptr,  // client_order_id
            nullptr,  // stop_price
            nullptr   // iceberg_amount
        );

        if (!result) {
            last_error_ = result.errmsg;
            return "";
        }

        return std::to_string(result.v.get_order_id());
    } catch (const std::exception& e) {
        last_error_ = std::string("send_order failed: ") + e.what();
        return "";
    }
}

bool BinanceSpotRestApi::cancel_order(const std::string& symbol,
                                       const std::string& order_id) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        size_t oid = std::stoull(order_id);
        auto result = impl_->api->cancel_order(
            symbol.c_str(),
            oid,
            nullptr,  // client_order_id
            nullptr   // new_client_order_id
        );

        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("cancel_order failed: ") + e.what();
        return false;
    }
}

bool BinanceSpotRestApi::cancel_order(const CancelRequest& req) {
    return cancel_order(req.symbol, req.orderid);
}

std::optional<OrderData> BinanceSpotRestApi::query_order(const std::string& symbol,
                                                          const std::string& order_id) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return std::nullopt;
    }

    try {
        size_t oid = std::stoull(order_id);
        auto result = impl_->api->order_info(
            symbol.c_str(),
            oid,
            nullptr  // client_order_id
        );

        if (!result) {
            last_error_ = result.errmsg;
            return std::nullopt;
        }

        OrderData order;
        order.symbol = result.v.symbol;
        order.exchange = Exchange::BINANCE;
        order.orderid = std::to_string(result.v.orderId);
        order.type = order_type_from_binance(result.v.type);
        order.direction = direction_from_binance(result.v.side);
        order.price = static_cast<double>(result.v.price);
        order.volume = static_cast<double>(result.v.origQty);
        order.traded = static_cast<double>(result.v.executedQty);
        order.status = status_from_binance(result.v.status);
        order.datetime = static_cast<int64_t>(result.v.time);

        // Calculate traded_price
        if (order.traded > 0) {
            order.traded_price = static_cast<double>(result.v.cummulativeQuoteQty) / order.traded;
        }

        return order;
    } catch (const std::exception& e) {
        last_error_ = std::string("query_order failed: ") + e.what();
        return std::nullopt;
    }
}

std::optional<OrderData> BinanceSpotRestApi::query_order(const OrderQueryRequest& req) {
    return query_order(req.symbol, req.orderid);
}

std::vector<OrderData> BinanceSpotRestApi::query_open_orders(const std::string& symbol) {
    std::vector<OrderData> orders;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return orders;
    }

    try {
        auto result = impl_->api->open_orders(
            symbol.empty() ? nullptr : symbol.c_str()
        );

        if (!result) {
            last_error_ = result.errmsg;
            return orders;
        }

        // open_orders returns orders_info_t with a map of symbol -> vector<order_info_t>
        for (const auto& [sym, sym_orders] : result.v.orders) {
            for (const auto& o : sym_orders) {
                OrderData order;
                order.symbol = o.symbol;
                order.exchange = Exchange::BINANCE;
                order.orderid = std::to_string(o.orderId);
                order.type = order_type_from_binance(o.type);
                order.direction = direction_from_binance(o.side);
                order.price = static_cast<double>(o.price);
                order.volume = static_cast<double>(o.origQty);
                order.traded = static_cast<double>(o.executedQty);
                order.status = status_from_binance(o.status);
                order.datetime = static_cast<int64_t>(o.time);

                if (order.traded > 0) {
                    order.traded_price = static_cast<double>(o.cummulativeQuoteQty) / order.traded;
                }

                orders.push_back(order);
            }
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("query_open_orders failed: ") + e.what();
    }

    return orders;
}

//=============================================================================
// Account
//=============================================================================

std::optional<AccountData> BinanceSpotRestApi::query_account(const std::string& asset) {
    auto accounts = query_account_all();
    if (asset.empty()) {
        // Return first non-zero account if no specific asset requested
        for (const auto& acc : accounts) {
            if (acc.balance > 0 || acc.frozen > 0) {
                return acc;
            }
        }
        return accounts.empty() ? std::nullopt : std::optional(accounts[0]);
    }

    for (const auto& acc : accounts) {
        if (acc.accountid == asset) {
            return acc;
        }
    }
    return std::nullopt;
}

std::vector<AccountData> BinanceSpotRestApi::query_account_all() {
    std::vector<AccountData> accounts;
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return accounts;
    }

    try {
        auto result = impl_->api->account_info();
        if (!result) {
            last_error_ = result.errmsg;
            return accounts;
        }

        for (const auto& [asset, balance] : result.v.balances) {
            double free = static_cast<double>(balance.free);
            double locked = static_cast<double>(balance.locked);

            if (free == 0 && locked == 0) continue;  // Skip empty balances

            AccountData account;
            account.accountid = asset;
            account.balance = free + locked;
            account.frozen = locked;
            accounts.push_back(account);
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("query_account_all failed: ") + e.what();
    }

    return accounts;
}

//=============================================================================
// User Data Stream
//=============================================================================

std::string BinanceSpotRestApi::start_user_stream() {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return "";
    }

    try {
        auto result = impl_->api->start_user_data_stream();
        if (!result) {
            last_error_ = result.errmsg;
            return "";
        }
        return result.v.listenKey;
    } catch (const std::exception& e) {
        last_error_ = std::string("start_user_stream failed: ") + e.what();
        return "";
    }
}

bool BinanceSpotRestApi::keep_user_stream(const std::string& listen_key) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        auto result = impl_->api->ping_user_data_stream(listen_key.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("keep_user_stream failed: ") + e.what();
        return false;
    }
}

bool BinanceSpotRestApi::close_user_stream(const std::string& listen_key) {
    if (!impl_->api) {
        last_error_ = "API not initialized";
        return false;
    }

    try {
        auto result = impl_->api->close_user_data_stream(listen_key.c_str());
        if (!result) {
            last_error_ = result.errmsg;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("close_user_stream failed: ") + e.what();
        return false;
    }
}

} // namespace bitquant
