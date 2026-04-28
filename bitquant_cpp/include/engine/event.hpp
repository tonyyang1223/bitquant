/**
 * @file event.hpp
 * @brief Event-driven framework for BitQuant C++ trading platform
 *
 * Based on howtrader's EventEngine design:
 * - Event types: Bar, Order, Trade, Position, Account, Timer, Log
 * - EventEngine with register/unregister handlers
 * - Thread-based event processing
 */

#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <any>

namespace bitquant {

//=============================================================================
// Event Types (Based on howtrader/trader/event.py)
//=============================================================================

// Timer event (generated every interval)
constexpr const char* EVENT_TIMER = "eTimer";

// Market data events
constexpr const char* EVENT_TICK = "eTick";
constexpr const char* EVENT_BAR = "eBar";
constexpr const char* EVENT_KLINE = "eKline";

// Order events
constexpr const char* EVENT_ORDER = "eOrder";
constexpr const char* EVENT_TRADE = "eTrade";

// Position/Account events
constexpr const char* EVENT_POSITION = "ePosition";
constexpr const char* EVENT_ACCOUNT = "eAccount";
constexpr const char* EVENT_CONTRACT = "eContract";

// System events
constexpr const char* EVENT_LOG = "eLog";
constexpr const char* EVENT_ERROR = "eError";
constexpr const char* EVENT_QUOTE = "eQuote";

// Strategy events
constexpr const char* EVENT_STRATEGY = "eStrategy";
constexpr const char* EVENT_STOP_ORDER = "eStopOrder";

//=============================================================================
// Event Class
//=============================================================================

/**
 * @brief Event object consists of a type string and data
 *
 * Based on howtrader's Event class:
 * - type: Event type string (e.g., "eTick", "eOrder")
 * - data: Any data object attached to event
 */
class Event {
public:
    Event(std::string type, std::any data = std::any())
        : type_(std::move(type)), data_(std::move(data)) {}

    const std::string& type() const { return type_; }
    std::any& data() { return data_; }
    const std::any& data() const { return data_; }

private:
    std::string type_;
    std::any data_;
};

//=============================================================================
// Handler Type
//=============================================================================

using HandlerType = std::function<void(const Event&)>;

//=============================================================================
// EventEngine
//=============================================================================

/**
 * @brief Event engine for distributing events to registered handlers
 *
 * Based on howtrader's EventEngine design:
 * - Thread-based event processing queue
 * - Register/unregister handlers for specific event types
 * - Timer event generation
 * - General handlers for all events
 */
class EventEngine {
public:
    /**
     * @brief Construct event engine with timer interval
     * @param interval Timer interval in seconds (default: 1)
     */
    explicit EventEngine(int interval = 1);

    /**
     * @brief Destructor - stops engine if running
     */
    ~EventEngine();

    //=========================================================================
    // Engine control
    //=========================================================================

    /**
     * @brief Start event engine
     * Starts processing thread and timer thread
     */
    void start();

    /**
     * @brief Stop event engine
     * Stops all threads and waits for completion
     */
    void stop();

    /**
     * @brief Check if engine is running
     */
    bool is_running() const { return active_; }

    //=========================================================================
    // Event processing
    //=========================================================================

    /**
     * @brief Put event into processing queue
     * @param event Event to process
     */
    void put(const Event& event);

    /**
     * @brief Put event into processing queue (move)
     */
    void put(Event&& event);

    //=========================================================================
    // Handler registration
    //=========================================================================

    /**
     * @brief Register handler for specific event type
     * @param type Event type string
     * @param handler Handler function
     */
    void register_handler(const std::string& type, HandlerType handler);

    /**
     * @brief Unregister handler from event type
     * @param type Event type string
     * @param handler Handler function to remove
     */
    void unregister_handler(const std::string& type, HandlerType handler);

    /**
     * @brief Register general handler for all event types
     * @param handler Handler function
     */
    void register_general_handler(HandlerType handler);

    /**
     * @brief Unregister general handler
     * @param handler Handler function to remove
     */
    void unregister_general_handler(HandlerType handler);

    //=========================================================================
    // Statistics
    //=========================================================================

    /**
     * @brief Get number of registered handlers for type
     */
    size_t handler_count(const std::string& type) const;

    /**
     * @brief Get queue size
     */
    size_t queue_size() const;

private:
    //=========================================================================
    // Internal processing
    //=========================================================================

    /**
     * @brief Main processing loop
     */
    void run();

    /**
     * @brief Timer loop for generating timer events
     */
    void run_timer();

    /**
     * @brief Process single event
     */
    void process(const Event& event);

    //=========================================================================
    // Member variables
    //=========================================================================

    int interval_;              // Timer interval in seconds
    bool active_;               // Engine active flag
    std::queue<Event> queue_;   // Event queue

    std::thread thread_;        // Processing thread
    std::thread timer_thread_;  // Timer thread

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Handlers for specific event types
    std::unordered_map<std::string, std::vector<HandlerType>> handlers_;

    // General handlers for all events
    std::vector<HandlerType> general_handlers_;
};

//=============================================================================
// Event Dispatcher Helper
//=============================================================================

/**
 * @brief Helper class for easy event dispatching
 */
class EventDispatcher {
public:
    explicit EventDispatcher(EventEngine* engine) : engine_(engine) {}

    /**
     * @brief Dispatch tick event
     */
    template<typename TickType>
    void dispatch_tick(const TickType& tick) {
        engine_->put(Event(EVENT_TICK, tick));
    }

    /**
     * @brief Dispatch bar event
     */
    template<typename BarType>
    void dispatch_bar(const BarType& bar) {
        engine_->put(Event(EVENT_BAR, bar));
    }

    /**
     * @brief Dispatch order event
     */
    template<typename OrderType>
    void dispatch_order(const OrderType& order) {
        engine_->put(Event(EVENT_ORDER, order));
    }

    /**
     * @brief Dispatch trade event
     */
    template<typename TradeType>
    void dispatch_trade(const TradeType& trade) {
        engine_->put(Event(EVENT_TRADE, trade));
    }

    /**
     * @brief Dispatch log event
     */
    void dispatch_log(const std::string& msg, int level = 0);

private:
    EventEngine* engine_;
};

} // namespace bitquant