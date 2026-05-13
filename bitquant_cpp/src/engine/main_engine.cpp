/**
 * @file main_engine.cpp
 * @brief Main trading engine implementation
 */

#include "engine/main_engine.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace bitquant {

//=============================================================================
// MainEngine Implementation
//=============================================================================

MainEngine::MainEngine(const MainEngineConfig& config)
    : config_(config)
{
    // Create event engine
    event_engine_ = std::make_unique<EventEngine>(config.timer_interval_seconds);

    // Initialize engines
    init_engines();
}

MainEngine::~MainEngine() {
    stop();
}

void MainEngine::init_engines() {
    // Create OMS engine
    oms_engine_ = std::make_unique<OmsEngine>(this, event_engine_.get());

    // Create log engine
    log_engine_ = std::make_unique<LogEngine>(
        this, event_engine_.get(),
        config_.log_to_console, config_.log_to_file, config_.log_path
    );

    // Create risk manager
    if (config_.enable_risk_manager) {
        risk_manager_ = std::make_unique<RiskManager>();
        RiskConfig risk_config;
        risk_config.order_flow_limit = config_.order_flow_limit;
        risk_config.order_size_limit = config_.order_size_limit;
        risk_config.active_order_limit = config_.active_order_limit;
        risk_manager_->set_config(risk_config);
    }
}

void MainEngine::start() {
    if (running_) return;

    event_engine_->start();
    running_ = true;

    write_log("MainEngine started");
}

void MainEngine::stop() {
    if (!running_) return;

    // Stop event engine first
    event_engine_->stop();

    // Close all gateways
    for (auto& [name, gateway] : gateways_) {
        if (gateway && gateway->is_connected()) {
            gateway->close();
        }
    }

    running_ = false;
    write_log("MainEngine stopped");
}

bool MainEngine::is_running() const {
    return running_;
}

//=============================================================================
// Gateway Management
//=============================================================================

IExchange* MainEngine::add_gateway(const std::string& name, std::unique_ptr<IExchange> gateway) {
    std::lock_guard<std::mutex> lock(gateways_mutex_);

    if (!gateway) {
        write_log("Cannot add null gateway: " + name);
        return nullptr;
    }

    // Store gateway
    IExchange* ptr = gateway.get();
    gateways_[name] = std::move(gateway);

    // Add supported exchanges
    for (Exchange exchange : ptr->supported_exchanges()) {
        bool found = false;
        for (Exchange e : exchanges_) {
            if (e == exchange) {
                found = true;
                break;
            }
        }
        if (!found) {
            exchanges_.push_back(exchange);
        }
    }

    write_log("Gateway added: " + name);
    return ptr;
}

IExchange* MainEngine::get_gateway(const std::string& name) const {
    auto it = gateways_.find(name);
    if (it != gateways_.end()) {
        return it->second.get();
    }

    // Can't call non-const write_log from const method
    return nullptr;
}

std::vector<std::string> MainEngine::get_gateway_names() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : gateways_) {
        names.push_back(name);
    }
    return names;
}

std::vector<Exchange> MainEngine::get_all_exchanges() const {
    return exchanges_;
}

//=============================================================================
// Connection Management
//=============================================================================

bool MainEngine::connect(const std::string& gateway_name, const std::string& config) {
    IExchange* gateway = get_gateway(gateway_name);
    if (!gateway) {
        return false;
    }

    // Set up callbacks to forward to event engine
    gateway->on_tick([this](const TickData& tick) {
        event_engine_->put(Event(EVENT_TICK, tick));
    });

    gateway->on_order([this](const OrderData& order) {
        event_engine_->put(Event(EVENT_ORDER, order));
    });

    gateway->on_trade([this](const TradeData& trade) {
        event_engine_->put(Event(EVENT_TRADE, trade));
    });

    gateway->on_position([this](const PositionData& pos) {
        event_engine_->put(Event(EVENT_POSITION, pos));
    });

    gateway->on_account([this](const AccountData& acc) {
        event_engine_->put(Event(EVENT_ACCOUNT, acc));
    });

    gateway->on_contract([this](const ContractData& contract) {
        event_engine_->put(Event(EVENT_CONTRACT, contract));
    });

    gateway->on_error([this](const std::string& error) {
        write_log("Gateway error: " + error);
    });

    bool result = gateway->connect(config);
    if (result) {
        write_log("Gateway connected: " + gateway_name);
    } else {
        write_log("Failed to connect gateway: " + gateway_name);
    }

    return result;
}

void MainEngine::disconnect(const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway && gateway->is_connected()) {
        gateway->close();
        write_log("Gateway disconnected: " + gateway_name);
    }
}

//=============================================================================
// Market Data Subscription
//=============================================================================

void MainEngine::subscribe(const std::string& symbol, const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway) {
        SubscribeRequest req;
        req.symbol = symbol;
        req.exchange = gateway->exchange();
        gateway->subscribe_tick(req);
        write_log("Subscribed: " + symbol + " on " + gateway_name);
    }
}

void MainEngine::subscribe_bar(const std::string& symbol, Interval interval,
                                const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway) {
        gateway->subscribe_bar(symbol, interval);
        write_log("Subscribed bar: " + symbol + " on " + gateway_name);
    }
}

void MainEngine::unsubscribe(const std::string& symbol, const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway) {
        gateway->unsubscribe_tick(symbol);
        write_log("Unsubscribed: " + symbol + " on " + gateway_name);
    }
}

//=============================================================================
// Order Management
//=============================================================================

std::string MainEngine::send_order(const OrderRequest& req, const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (!gateway) {
        write_log("Cannot send order: gateway not found - " + gateway_name);
        return "";
    }

    // Risk check
    if (risk_manager_ && !risk_manager_->check_risk(req, gateway_name)) {
        write_log("Order rejected by risk manager: " + req.symbol);
        return "";
    }

    std::string orderid = gateway->send_order(req);
    if (!orderid.empty()) {
        write_log("Order sent: " + orderid + " " + req.symbol + " " +
                  direction_to_string(req.direction) + " " + std::to_string(req.volume));
    }

    return orderid;
}

bool MainEngine::cancel_order(const CancelRequest& req, const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (!gateway) {
        return false;
    }

    bool result = gateway->cancel_order(req);
    if (result) {
        write_log("Order cancelled: " + req.orderid);
    }

    return result;
}

void MainEngine::cancel_all_orders(const std::string& symbol, const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway) {
        gateway->cancel_all_orders(symbol);
        write_log("Cancelled all orders: " + (symbol.empty() ? "ALL" : symbol));
    }
}

//=============================================================================
// Query Methods (delegated to OMS)
//=============================================================================

std::optional<TickData> MainEngine::get_tick(const std::string& vt_symbol) const {
    return oms_engine_ ? oms_engine_->get_tick(vt_symbol) : std::nullopt;
}

std::optional<OrderData> MainEngine::get_order(const std::string& vt_orderid) const {
    return oms_engine_ ? oms_engine_->get_order(vt_orderid) : std::nullopt;
}

std::optional<TradeData> MainEngine::get_trade(const std::string& vt_tradeid) const {
    return oms_engine_ ? oms_engine_->get_trade(vt_tradeid) : std::nullopt;
}

std::optional<PositionData> MainEngine::get_position(const std::string& vt_positionid) const {
    return oms_engine_ ? oms_engine_->get_position(vt_positionid) : std::nullopt;
}

std::optional<AccountData> MainEngine::get_account(const std::string& vt_accountid) const {
    return oms_engine_ ? oms_engine_->get_account(vt_accountid) : std::nullopt;
}

std::optional<ContractData> MainEngine::get_contract(const std::string& vt_symbol) const {
    return oms_engine_ ? oms_engine_->get_contract(vt_symbol) : std::nullopt;
}

std::vector<TickData> MainEngine::get_all_ticks() const {
    return oms_engine_ ? oms_engine_->get_all_ticks() : std::vector<TickData>{};
}

std::vector<OrderData> MainEngine::get_all_orders() const {
    return oms_engine_ ? oms_engine_->get_all_orders() : std::vector<OrderData>{};
}

std::vector<OrderData> MainEngine::get_all_active_orders(const std::string& symbol) const {
    return oms_engine_ ? oms_engine_->get_all_active_orders(symbol) : std::vector<OrderData>{};
}

std::vector<TradeData> MainEngine::get_all_trades() const {
    return oms_engine_ ? oms_engine_->get_all_trades() : std::vector<TradeData>{};
}

std::vector<PositionData> MainEngine::get_all_positions() const {
    return oms_engine_ ? oms_engine_->get_all_positions() : std::vector<PositionData>{};
}

std::vector<AccountData> MainEngine::get_all_accounts() const {
    return oms_engine_ ? oms_engine_->get_all_accounts() : std::vector<AccountData>{};
}

std::vector<ContractData> MainEngine::get_all_contracts() const {
    return oms_engine_ ? oms_engine_->get_all_contracts() : std::vector<ContractData>{};
}

//=============================================================================
// History Data Query
//=============================================================================

std::vector<BarData> MainEngine::query_history(const HistoryRequest& req,
                                                const std::string& gateway_name) {
    IExchange* gateway = get_gateway(gateway_name);
    if (gateway) {
        return gateway->get_bars(req);
    }
    return {};
}

//=============================================================================
// Logging
//=============================================================================

void MainEngine::write_log(const std::string& msg, const std::string& source) {
    LogData log;
    log.msg = msg;
    log.gateway_name = source;
    log.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    event_engine_->put(Event(EVENT_LOG, log));
}

//=============================================================================
// OmsEngine Implementation
//=============================================================================

OmsEngine::OmsEngine(MainEngine* main_engine, EventEngine* event_engine)
    : main_engine_(main_engine)
    , event_engine_(event_engine)
{
    register_event();
}

OmsEngine::~OmsEngine() = default;

void OmsEngine::register_event() {
    event_engine_->register_handler(EVENT_TICK, [this](const Event& e) {
        process_tick_event(e);
    });

    event_engine_->register_handler(EVENT_ORDER, [this](const Event& e) {
        process_order_event(e);
    });

    event_engine_->register_handler(EVENT_TRADE, [this](const Event& e) {
        process_trade_event(e);
    });

    event_engine_->register_handler(EVENT_POSITION, [this](const Event& e) {
        process_position_event(e);
    });

    event_engine_->register_handler(EVENT_ACCOUNT, [this](const Event& e) {
        process_account_event(e);
    });

    event_engine_->register_handler(EVENT_CONTRACT, [this](const Event& e) {
        process_contract_event(e);
    });
}

void OmsEngine::process_tick_event(const Event& event) {
    try {
        const TickData& tick = std::any_cast<const TickData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);
        ticks_[tick.symbol] = tick;
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

void OmsEngine::process_order_event(const Event& event) {
    try {
        const OrderData& order = std::any_cast<const OrderData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);

        std::string vt_orderid = order.orderid;
        orders_[vt_orderid] = order;

        // Update active orders
        if (order.is_active()) {
            active_orders_[vt_orderid] = order;
        } else {
            active_orders_.erase(vt_orderid);
        }
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

void OmsEngine::process_trade_event(const Event& event) {
    try {
        const TradeData& trade = std::any_cast<const TradeData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);
        trades_[trade.tradeid] = trade;
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

void OmsEngine::process_position_event(const Event& event) {
    try {
        const PositionData& pos = std::any_cast<const PositionData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);
        std::string vt_positionid = pos.symbol + "_" + std::to_string(static_cast<int>(pos.direction));
        positions_[vt_positionid] = pos;
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

void OmsEngine::process_account_event(const Event& event) {
    try {
        const AccountData& acc = std::any_cast<const AccountData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);
        accounts_[acc.accountid] = acc;
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

void OmsEngine::process_contract_event(const Event& event) {
    try {
        const ContractData& contract = std::any_cast<const ContractData&>(event.data());
        std::lock_guard<std::mutex> lock(data_mutex_);
        contracts_[contract.symbol] = contract;
    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

//=============================================================================
// OMS Data Access
//=============================================================================

std::optional<TickData> OmsEngine::get_tick(const std::string& vt_symbol) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = ticks_.find(vt_symbol);
    if (it != ticks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<OrderData> OmsEngine::get_order(const std::string& vt_orderid) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = orders_.find(vt_orderid);
    if (it != orders_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<TradeData> OmsEngine::get_trade(const std::string& vt_tradeid) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = trades_.find(vt_tradeid);
    if (it != trades_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<PositionData> OmsEngine::get_position(const std::string& vt_positionid) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = positions_.find(vt_positionid);
    if (it != positions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AccountData> OmsEngine::get_account(const std::string& vt_accountid) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = accounts_.find(vt_accountid);
    if (it != accounts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ContractData> OmsEngine::get_contract(const std::string& vt_symbol) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = contracts_.find(vt_symbol);
    if (it != contracts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TickData> OmsEngine::get_all_ticks() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<TickData> result;
    result.reserve(ticks_.size());
    for (const auto& [_, tick] : ticks_) {
        result.push_back(tick);
    }
    return result;
}

std::vector<OrderData> OmsEngine::get_all_orders() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<OrderData> result;
    result.reserve(orders_.size());
    for (const auto& [_, order] : orders_) {
        result.push_back(order);
    }
    return result;
}

std::vector<OrderData> OmsEngine::get_all_active_orders(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<OrderData> result;
    for (const auto& [_, order] : active_orders_) {
        if (symbol.empty() || order.symbol == symbol) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<TradeData> OmsEngine::get_all_trades() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<TradeData> result;
    result.reserve(trades_.size());
    for (const auto& [_, trade] : trades_) {
        result.push_back(trade);
    }
    return result;
}

std::vector<PositionData> OmsEngine::get_all_positions() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<PositionData> result;
    result.reserve(positions_.size());
    for (const auto& [_, pos] : positions_) {
        result.push_back(pos);
    }
    return result;
}

std::vector<AccountData> OmsEngine::get_all_accounts() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<AccountData> result;
    result.reserve(accounts_.size());
    for (const auto& [_, acc] : accounts_) {
        result.push_back(acc);
    }
    return result;
}

std::vector<ContractData> OmsEngine::get_all_contracts() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<ContractData> result;
    result.reserve(contracts_.size());
    for (const auto& [_, contract] : contracts_) {
        result.push_back(contract);
    }
    return result;
}

//=============================================================================
// LogEngine Implementation
//=============================================================================

LogEngine::LogEngine(MainEngine* main_engine, EventEngine* event_engine,
                     bool console, bool file, const std::string& path)
    : main_engine_(main_engine)
    , event_engine_(event_engine)
    , console_(console)
    , file_(file)
    , log_path_(path)
{
    register_event();
}

LogEngine::~LogEngine() = default;

void LogEngine::register_event() {
    event_engine_->register_handler(EVENT_LOG, [this](const Event& e) {
        process_log_event(e);
    });
}

void LogEngine::process_log_event(const Event& event) {
    try {
        const LogData& log = std::any_cast<const LogData&>(event.data());

        // Format timestamp
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::time_point(
                std::chrono::milliseconds(log.datetime)
            )
        );
        std::tm* tm = std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        if (!log.gateway_name.empty()) {
            oss << " [" << log.gateway_name << "]";
        }
        oss << " " << log.msg;

        std::string output = oss.str();

        // Output to console
        if (console_) {
            std::cout << output << std::endl;
        }

        // TODO: Output to file if enabled
        // if (file_) { ... }

    } catch (const std::bad_any_cast&) {
        // Ignore cast errors
    }
}

} // namespace bitquant
