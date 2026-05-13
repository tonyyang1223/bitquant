/**
 * @file offset_converter.cpp
 * @brief Offset converter implementation
 *
 * Based on howtrader's OffsetConverter logic from:
 * howtrader/trader/converter.py
 */

#include "exchange/offset_converter.hpp"
#include "engine/main_engine.hpp"
#include <algorithm>
#include <sstream>

namespace bitquant {

//=============================================================================
// PositionHolding Implementation
//=============================================================================

PositionHolding::PositionHolding(const ContractData& contract)
    : vt_symbol_(contract.vt_symbol())
    , exchange_(contract.exchange) {}

void PositionHolding::update_position(const PositionData& position) {
    if (position.direction == Direction::LONG) {
        long_pos_ = position.volume;
        long_yd_ = position.yd_volume;
        long_td_ = long_pos_ - long_yd_;
    } else {
        short_pos_ = position.volume;
        short_yd_ = position.yd_volume;
        short_td_ = short_pos_ - short_yd_;
    }
}

void PositionHolding::update_trade(const TradeData& trade) {
    if (trade.direction == Direction::LONG) {
        if (trade.offset == Offset::OPEN) {
            long_td_ += trade.volume;
        } else if (trade.offset == Offset::CLOSETODAY) {
            short_td_ -= trade.volume;
        } else if (trade.offset == Offset::CLOSEYESTERDAY) {
            short_yd_ -= trade.volume;
        } else if (trade.offset == Offset::CLOSE) {
            // For non-SHFE/INE exchanges, close today first
            if (exchange_ == Exchange::SHFE || exchange_ == Exchange::INE) {
                short_yd_ -= trade.volume;
            } else {
                short_td_ -= trade.volume;
                if (short_td_ < 0) {
                    short_yd_ += short_td_;
                    short_td_ = 0;
                }
            }
        }
    } else {
        if (trade.offset == Offset::OPEN) {
            short_td_ += trade.volume;
        } else if (trade.offset == Offset::CLOSETODAY) {
            long_td_ -= trade.volume;
        } else if (trade.offset == Offset::CLOSEYESTERDAY) {
            long_yd_ -= trade.volume;
        } else if (trade.offset == Offset::CLOSE) {
            if (exchange_ == Exchange::SHFE || exchange_ == Exchange::INE) {
                long_yd_ -= trade.volume;
            } else {
                long_td_ -= trade.volume;
                if (long_td_ < 0) {
                    long_yd_ += long_td_;
                    long_td_ = 0;
                }
            }
        }
    }

    // Update total position
    long_pos_ = long_td_ + long_yd_;
    short_pos_ = short_td_ + short_yd_;

    // Update frozen volume
    sum_pos_frozen();
}

void PositionHolding::update_order(const OrderData& order) {
    std::string vt_orderid = order.vt_orderid();

    if (order.is_active()) {
        active_orders_[vt_orderid] = order;
    } else {
        active_orders_.erase(vt_orderid);
    }

    calculate_frozen();
}

void PositionHolding::update_order_request(const OrderRequest& req, const std::string& vt_orderid) {
    // Parse gateway name and order id
    size_t dot_pos = vt_orderid.find('.');
    std::string gateway_name = (dot_pos != std::string::npos) ? vt_orderid.substr(0, dot_pos) : "";
    std::string orderid = (dot_pos != std::string::npos) ? vt_orderid.substr(dot_pos + 1) : vt_orderid;

    OrderData order = req.create_order_data(orderid, gateway_name);
    update_order(order);
}

void PositionHolding::calculate_frozen() {
    // Reset frozen
    long_pos_frozen_ = 0.0;
    long_yd_frozen_ = 0.0;
    long_td_frozen_ = 0.0;

    short_pos_frozen_ = 0.0;
    short_yd_frozen_ = 0.0;
    short_td_frozen_ = 0.0;

    for (const auto& [vt_orderid, order] : active_orders_) {
        // Ignore position open orders
        if (order.offset == Offset::OPEN) {
            continue;
        }

        double frozen = order.volume - order.traded;

        if (order.direction == Direction::LONG) {
            // Buy to close short position
            if (order.offset == Offset::CLOSETODAY) {
                short_td_frozen_ += frozen;
            } else if (order.offset == Offset::CLOSEYESTERDAY) {
                short_yd_frozen_ += frozen;
            } else if (order.offset == Offset::CLOSE) {
                short_td_frozen_ += frozen;
                if (short_td_frozen_ > short_td_) {
                    short_yd_frozen_ += (short_td_frozen_ - short_td_);
                    short_td_frozen_ = short_td_;
                }
            }
        } else {
            // Sell to close long position
            if (order.offset == Offset::CLOSETODAY) {
                long_td_frozen_ += frozen;
            } else if (order.offset == Offset::CLOSEYESTERDAY) {
                long_yd_frozen_ += frozen;
            } else if (order.offset == Offset::CLOSE) {
                long_td_frozen_ += frozen;
                if (long_td_frozen_ > long_td_) {
                    long_yd_frozen_ += (long_td_frozen_ - long_td_);
                    long_td_frozen_ = long_td_;
                }
            }
        }
    }

    sum_pos_frozen();
}

void PositionHolding::sum_pos_frozen() {
    // Frozen volume should be no more than total volume
    long_td_frozen_ = std::min(long_td_frozen_, long_td_);
    long_yd_frozen_ = std::min(long_yd_frozen_, long_yd_);

    short_td_frozen_ = std::min(short_td_frozen_, short_td_);
    short_yd_frozen_ = std::min(short_yd_frozen_, short_yd_);

    long_pos_frozen_ = long_td_frozen_ + long_yd_frozen_;
    short_pos_frozen_ = short_td_frozen_ + short_yd_frozen_;
}

std::vector<OrderRequest> PositionHolding::convert_order_request_shfe(const OrderRequest& req) {
    // Open position - no conversion needed
    if (req.offset == Offset::OPEN) {
        return {req};
    }

    // Calculate available position
    double pos_available = 0.0;
    double td_available = 0.0;

    if (req.direction == Direction::LONG) {
        // Buy to close short position
        pos_available = short_pos_ - short_pos_frozen_;
        td_available = short_td_ - short_td_frozen_;
    } else {
        // Sell to close long position
        pos_available = long_pos_ - long_pos_frozen_;
        td_available = long_td_ - long_td_frozen_;
    }

    // Not enough position
    if (req.volume > pos_available) {
        return {};
    }

    // All can be closed as today
    if (req.volume <= td_available) {
        OrderRequest req_td = req;
        req_td.offset = Offset::CLOSETODAY;
        return {req_td};
    }

    // Need to split into today + yesterday
    std::vector<OrderRequest> req_list;

    if (td_available > 0) {
        OrderRequest req_td = req;
        req_td.offset = Offset::CLOSETODAY;
        req_td.volume = td_available;
        req_list.push_back(req_td);
    }

    OrderRequest req_yd = req;
    req_yd.offset = Offset::CLOSEYESTERDAY;
    req_yd.volume = req.volume - td_available;
    req_list.push_back(req_yd);

    return req_list;
}

std::vector<OrderRequest> PositionHolding::convert_order_request_lock(const OrderRequest& req) {
    double td_volume = 0.0;
    double yd_available = 0.0;

    if (req.direction == Direction::LONG) {
        td_volume = short_td_;
        yd_available = short_yd_ - short_yd_frozen_;
    } else {
        td_volume = long_td_;
        yd_available = long_yd_ - long_yd_frozen_;
    }

    // If there is td_volume, we can only lock position
    if (td_volume > 0) {
        OrderRequest req_open = req;
        req_open.offset = Offset::OPEN;
        return {req_open};
    }

    // If no td_volume, close opposite yd position first, then open new position
    double close_volume = std::min(req.volume, yd_available);
    double open_volume = std::max(0.0, req.volume - yd_available);

    std::vector<OrderRequest> req_list;

    if (yd_available > 0) {
        OrderRequest req_yd = req;
        if (exchange_ == Exchange::SHFE || exchange_ == Exchange::INE) {
            req_yd.offset = Offset::CLOSEYESTERDAY;
        } else {
            req_yd.offset = Offset::CLOSE;
        }
        req_yd.volume = close_volume;
        req_list.push_back(req_yd);
    }

    if (open_volume > 0) {
        OrderRequest req_open = req;
        req_open.offset = Offset::OPEN;
        req_open.volume = open_volume;
        req_list.push_back(req_open);
    }

    return req_list;
}

std::vector<OrderRequest> PositionHolding::convert_order_request_net(const OrderRequest& req) {
    double pos_available = 0.0;
    double td_available = 0.0;
    double yd_available = 0.0;

    if (req.direction == Direction::LONG) {
        pos_available = short_pos_ - short_pos_frozen_;
        td_available = short_td_ - short_td_frozen_;
        yd_available = short_yd_ - short_yd_frozen_;
    } else {
        pos_available = long_pos_ - long_pos_frozen_;
        td_available = long_td_ - long_td_frozen_;
        yd_available = long_yd_ - long_yd_frozen_;
    }

    std::vector<OrderRequest> reqs;
    double volume_left = req.volume;

    // For SHFE/INE, need to split close into today/yesterday
    if (exchange_ == Exchange::SHFE || exchange_ == Exchange::INE) {
        if (td_available > 0) {
            double td_volume = std::min(td_available, volume_left);
            volume_left -= td_volume;

            OrderRequest td_req = req;
            td_req.offset = Offset::CLOSETODAY;
            td_req.volume = td_volume;
            reqs.push_back(td_req);
        }

        if (volume_left > 0 && yd_available > 0) {
            double yd_volume = std::min(yd_available, volume_left);
            volume_left -= yd_volume;

            OrderRequest yd_req = req;
            yd_req.offset = Offset::CLOSEYESTERDAY;
            yd_req.volume = yd_volume;
            reqs.push_back(yd_req);
        }

        if (volume_left > 0) {
            OrderRequest open_req = req;
            open_req.offset = Offset::OPEN;
            open_req.volume = volume_left;
            reqs.push_back(open_req);
        }
    } else {
        // For other exchanges, just use CLOSE
        if (pos_available > 0) {
            double close_volume = std::min(pos_available, volume_left);
            volume_left -= pos_available;

            OrderRequest close_req = req;
            close_req.offset = Offset::CLOSE;
            close_req.volume = close_volume;
            reqs.push_back(close_req);
        }

        if (volume_left > 0) {
            OrderRequest open_req = req;
            open_req.offset = Offset::OPEN;
            open_req.volume = volume_left;
            reqs.push_back(open_req);
        }
    }

    return reqs;
}

//=============================================================================
// OffsetConverter Implementation
//=============================================================================

OffsetConverter::OffsetConverter(MainEngine* main_engine)
    : main_engine_(main_engine) {}

void OffsetConverter::update_position(const PositionData& position) {
    if (!is_convert_required(position.vt_symbol())) {
        return;
    }

    PositionHolding& holding = get_position_holding(position.vt_symbol());
    holding.update_position(position);
}

void OffsetConverter::update_trade(const TradeData& trade) {
    if (!is_convert_required(trade.vt_symbol())) {
        return;
    }

    PositionHolding& holding = get_position_holding(trade.vt_symbol());
    holding.update_trade(trade);
}

void OffsetConverter::update_order(const OrderData& order) {
    if (!is_convert_required(order.vt_symbol())) {
        return;
    }

    PositionHolding& holding = get_position_holding(order.vt_symbol());
    holding.update_order(order);
}

void OffsetConverter::update_order_request(const OrderRequest& req, const std::string& vt_orderid) {
    if (!is_convert_required(req.vt_symbol())) {
        return;
    }

    PositionHolding& holding = get_position_holding(req.vt_symbol());
    holding.update_order_request(req, vt_orderid);
}

std::vector<OrderRequest> OffsetConverter::convert_order_request(
    const OrderRequest& req,
    bool lock,
    bool net
) {
    if (!is_convert_required(req.vt_symbol())) {
        return {req};
    }

    PositionHolding& holding = get_position_holding(req.vt_symbol());

    if (lock) {
        return holding.convert_order_request_lock(req);
    } else if (net) {
        return holding.convert_order_request_net(req);
    } else if (req.exchange == Exchange::SHFE || req.exchange == Exchange::INE) {
        return holding.convert_order_request_shfe(req);
    } else {
        return {req};
    }
}

PositionHolding& OffsetConverter::get_position_holding(const std::string& vt_symbol) {
    auto it = holdings_.find(vt_symbol);
    if (it == holdings_.end()) {
        // Try to get contract from main engine or registered contracts
        ContractData contract;
        contract.symbol = vt_symbol.substr(0, vt_symbol.find('.'));
        contract.exchange = Exchange::BINANCE;  // Default

        // Check registered contracts
        auto contract_it = contracts_.find(vt_symbol);
        if (contract_it != contracts_.end()) {
            contract = contract_it->second;
        }

        it = holdings_.emplace(vt_symbol, PositionHolding(contract)).first;
    }
    return it->second;
}

bool OffsetConverter::is_convert_required(const std::string& vt_symbol) const {
    // Check registered contracts
    auto it = contracts_.find(vt_symbol);
    if (it != contracts_.end()) {
        // Net position mode doesn't require conversion
        if (it->second.net_position) {
            return false;
        }
        return true;
    }

    // If no contract info, assume conversion is required for futures
    // (Spot trading typically doesn't need offset conversion)
    return true;
}

void OffsetConverter::register_contract(const ContractData& contract) {
    contracts_[contract.vt_symbol()] = contract;
}

void OffsetConverter::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    }
}

} // namespace bitquant