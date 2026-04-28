/**
 * @file trading_engine.cpp
 * @brief Trading engine implementation
 *
 * Unified trading engine coordinating all components
 */

#include "engine/trading_engine.hpp"
#include "exchange/binance_gateway.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace bitquant {

//=============================================================================
// Constructor/Destructor
//=============================================================================

TradingEngine::TradingEngine() {
    initialize_components();
}

TradingEngine::TradingEngine(const TradingEngineConfig& config)
    : config_(config) {
    initialize_components();
}

TradingEngine::~TradingEngine() {
    stop_all_strategies();
    disconnect();
}

//=============================================================================
// Initialization
//=============================================================================

void TradingEngine::initialize_components() {
    // Create event engine
    event_engine_ = std::make_unique<EventEngine>(config_.timer_interval_seconds);

    // Create broker
    BrokerConfig broker_config;
    broker_config.initial_cash = config_.initial_capital;
    broker_config.commission_rate = config_.commission_rate;
    broker_config.slippage_rate = config_.slippage_rate;
    broker_config.leverage = config_.leverage;
    broker_config.allow_short = config_.allow_short;
    broker_ = std::make_unique<Broker>(broker_config);

    // Create strategy manager
    strategy_manager_ = std::make_unique<StrategyManager>(
        broker_.get(),
        event_engine_.get()
    );

    // Create risk manager
    if (config_.enable_risk_manager) {
        RiskConfig risk_config;
        risk_config.active = true;
        risk_config.order_flow_limit = config_.order_flow_limit;
        risk_config.order_size_limit = config_.order_size_limit;
        risk_config.trade_limit = config_.trade_limit;
        risk_config.active_order_limit = config_.active_order_limit;
        risk_config.order_cancel_limit = config_.order_cancel_limit;
        risk_manager_ = std::make_unique<RiskManager>(risk_config);

        strategy_manager_->set_risk_manager(risk_manager_.get());
    }

    initialized_ = true;
    write_log("Trading engine initialized");
}

void TradingEngine::configure(const TradingEngineConfig& config) {
    config_ = config;
    initialize_components();
}

//=============================================================================
// Connection Management
//=============================================================================

bool TradingEngine::connect() {
    if (config_.mode == EngineMode::BACKTESTING) {
        write_log("Backtesting mode - no connection needed");
        return true;
    }

    // Create exchange gateway
    exchange_ = ExchangeFactory::create(config_.exchange_name);
    if (!exchange_) {
        write_log("Failed to create exchange: " + config_.exchange_name);
        return false;
    }

    // Build connection config
    std::ostringstream oss;
    oss << "{";
    oss << "\"api_key\":\"" << config_.api_key << "\",";
    oss << "\"api_secret\":\"" << config_.api_secret << "\",";
    oss << "\"testnet\":" << (config_.use_testnet ? "true" : "false");
    oss << "}";

    if (!exchange_->connect(oss.str())) {
        write_log("Failed to connect to exchange");
        return false;
    }

    // Register callbacks
    exchange_->on_tick([this](const TickData& tick) { on_tick(tick); });
    exchange_->on_bar([this](const BarData& bar) { on_bar(bar); });
    exchange_->on_order([this](const OrderData& order) { on_order(order); });
    exchange_->on_trade([this](const TradeData& trade) { on_trade(trade); });

    // Start event engine
    event_engine_->start();

    write_log("Connected to " + config_.exchange_name);
    return true;
}

void TradingEngine::disconnect() {
    if (exchange_) {
        exchange_->close();
        exchange_.reset();
    }

    if (event_engine_) {
        event_engine_->stop();
    }

    write_log("Disconnected");
}

bool TradingEngine::is_connected() const {
    return exchange_ && exchange_->is_connected();
}

//=============================================================================
// Data Management
//=============================================================================

void TradingEngine::load_data(
    const std::string& symbol,
    const std::vector<BarData>& bars
) {
    data_store_[symbol] = bars;
    write_log("Loaded " + std::to_string(bars.size()) + " bars for " + symbol);
}

size_t TradingEngine::load_data_from_csv(
    const std::string& symbol,
    const std::string& filename
) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        write_log("Failed to open file: " + filename);
        return 0;
    }

    std::vector<BarData> bars;
    std::string line;

    // Skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 6) {
            BarData bar;
            bar.datetime = std::stoll(tokens[0]);
            bar.open_price = std::stod(tokens[1]);
            bar.high_price = std::stod(tokens[2]);
            bar.low_price = std::stod(tokens[3]);
            bar.close_price = std::stod(tokens[4]);
            bar.volume = std::stod(tokens[5]);
            bar.symbol = symbol;
            bars.push_back(bar);
        }
    }

    data_store_[symbol] = bars;
    write_log("Loaded " + std::to_string(bars.size()) + " bars from " + filename);
    return bars.size();
}

//=============================================================================
// Strategy Management
//=============================================================================

bool TradingEngine::add_strategy(
    const std::string& name,
    std::unique_ptr<IStrategy> strategy,
    const std::string& symbol,
    const std::unordered_map<std::string, double>& params
) {
    return strategy_manager_->add_strategy(name, std::move(strategy), symbol, params);
}

void TradingEngine::init_all_strategies() {
    strategy_manager_->init_all_strategies();
}

void TradingEngine::start_all_strategies() {
    strategy_manager_->start_all_strategies();
}

void TradingEngine::stop_all_strategies() {
    strategy_manager_->stop_all_strategies();
}

std::vector<std::string> TradingEngine::get_strategy_names() const {
    return strategy_manager_->get_strategy_names();
}

//=============================================================================
// Market Data Subscription
//=============================================================================

void TradingEngine::subscribe(const std::string& symbol) {
    if (exchange_) {
        SubscribeRequest req;
        req.symbol = symbol;
        exchange_->subscribe_tick(req);
    }
}

void TradingEngine::subscribe_bar(const std::string& symbol, Interval interval) {
    if (exchange_) {
        exchange_->subscribe_bar(symbol, interval);
    }
}

//=============================================================================
// Trading Operations
//=============================================================================

std::string TradingEngine::send_order(const OrderRequest& req) {
    // Risk check
    if (risk_manager_ && !risk_manager_->check_risk(req, "")) {
        write_log("Order rejected by risk manager");
        return "";
    }

    // Route to exchange or broker
    if (exchange_) {
        return exchange_->send_order(req);
    } else if (config_.mode == EngineMode::BACKTESTING) {
        // In backtesting, orders go through broker
        // This is handled by the strategy calling broker methods
        return "";
    }

    return "";
}

bool TradingEngine::cancel_order(const std::string& vt_orderid) {
    if (exchange_) {
        CancelRequest req;
        req.orderid = vt_orderid;
        return exchange_->cancel_order(req);
    }
    return false;
}

void TradingEngine::cancel_all_orders(const std::string& symbol) {
    if (exchange_) {
        exchange_->cancel_all_orders(symbol);
    }
}

//=============================================================================
// Query Methods
//=============================================================================

double TradingEngine::get_price(const std::string& symbol) {
    if (exchange_) {
        return exchange_->get_price(symbol);
    }
    return 0.0;
}

double TradingEngine::get_balance() const {
    if (broker_) {
        return broker_->cash();
    }
    return 0.0;
}

double TradingEngine::get_position(const std::string& symbol) const {
    (void)symbol;
    if (broker_) {
        return broker_->position();
    }
    return 0.0;
}

std::vector<OrderData> TradingEngine::get_open_orders(const std::string& symbol) const {
    if (exchange_) {
        return exchange_->query_open_orders(symbol);
    }
    return {};
}

//=============================================================================
// Run Backtest
//=============================================================================

void TradingEngine::run_backtest() {
    run_backtest(nullptr);
}

void TradingEngine::run_backtest(ProgressCallback progress) {
    if (config_.mode != EngineMode::BACKTESTING) {
        write_log("Not in backtesting mode");
        return;
    }

    // Get data
    if (data_store_.empty()) {
        write_log("No data loaded");
        return;
    }

    // Use first symbol's data
    auto& [symbol, bars] = *data_store_.begin();
    write_log("Running backtest with " + std::to_string(bars.size()) + " bars");

    // Set data to broker
    broker_->set_data(bars);

    // Initialize and start strategies
    init_all_strategies();
    start_all_strategies();

    // Run backtest
    running_ = true;
    broker_->run();
    running_ = false;

    // Stop strategies
    stop_all_strategies();

    write_log("Backtest completed");
}

//=============================================================================
// Results
//=============================================================================

const PerformanceMetrics& TradingEngine::get_performance() const {
    return broker_->performance();
}

const std::vector<std::pair<int64_t, double>>& TradingEngine::get_equity_curve() const {
    return broker_->equity_curve();
}

const std::vector<Trade>& TradingEngine::get_trades() const {
    return broker_->trades();
}

//=============================================================================
// Event Handlers
//=============================================================================

void TradingEngine::on_tick(const TickData& tick) {
    // Dispatch to risk manager
    if (risk_manager_) {
        risk_manager_->on_timer();
    }

    // Dispatch to strategies
    strategy_manager_->on_tick(tick);
}

void TradingEngine::on_bar(const BarData& bar) {
    // Dispatch to strategies
    strategy_manager_->on_bar(bar);
}

void TradingEngine::on_order(const OrderData& order) {
    // Dispatch to risk manager
    if (risk_manager_) {
        risk_manager_->on_order(order);
    }

    // Dispatch to strategies
    strategy_manager_->on_order(order);

    // Callback
    if (order_callback_) {
        order_callback_(order);
    }
}

void TradingEngine::on_trade(const TradeData& trade) {
    // Dispatch to risk manager
    if (risk_manager_) {
        risk_manager_->on_trade(trade);
    }

    // Dispatch to strategies
    strategy_manager_->on_trade(trade);

    // Callback
    if (trade_callback_) {
        trade_callback_(trade);
    }
}

//=============================================================================
// Callbacks
//=============================================================================

void TradingEngine::set_log_callback(LogCallback callback) {
    log_callback_ = std::move(callback);
    if (strategy_manager_) {
        strategy_manager_->set_log_callback(log_callback_);
    }
}

void TradingEngine::set_order_callback(OrderCallback callback) {
    order_callback_ = std::move(callback);
}

void TradingEngine::set_trade_callback(TradeCallback callback) {
    trade_callback_ = std::move(callback);
}

//=============================================================================
// Internal Methods
//=============================================================================

void TradingEngine::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    } else {
        std::cout << "[TradingEngine] " << msg << std::endl;
    }
}

} // namespace bitquant
