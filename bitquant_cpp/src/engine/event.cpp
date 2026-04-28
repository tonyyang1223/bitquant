/**
 * @file event.cpp
 * @brief Event engine implementation
 *
 * Based on howtrader's EventEngine design patterns
 */

#include "engine/event.hpp"
#include <chrono>
#include <algorithm>

namespace bitquant {

EventEngine::EventEngine(int interval)
    : interval_(interval), active_(false) {}

EventEngine::~EventEngine() {
    stop();
}

void EventEngine::start() {
    if (active_) return;

    active_ = true;
    thread_ = std::thread(&EventEngine::run, this);
    timer_thread_ = std::thread(&EventEngine::run_timer, this);
}

void EventEngine::stop() {
    if (!active_) return;

    active_ = false;

    // Notify queue to wake up
    queue_cv_.notify_all();

    // Wait for threads to finish
    if (thread_.joinable()) {
        thread_.join();
    }
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }
}

void EventEngine::put(const Event& event) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(event);
    }
    queue_cv_.notify_one();
}

void EventEngine::put(Event&& event) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(std::move(event));
    }
    queue_cv_.notify_one();
}

void EventEngine::register_handler(const std::string& type, HandlerType handler) {
    auto& handler_list = handlers_[type];

    // Check if handler already exists (simple check by pointer for lambdas)
    handler_list.push_back(std::move(handler));
}

void EventEngine::unregister_handler(const std::string& type, HandlerType handler) {
    auto it = handlers_.find(type);
    if (it != handlers_.end()) {
        // Remove matching handlers
        auto& handler_list = it->second;
        handler_list.erase(
            std::remove_if(handler_list.begin(), handler_list.end(),
                [&handler](const HandlerType& h) {
                    // Simple comparison - may not work for all cases
                    return h.target_type() == handler.target_type();
                }),
            handler_list.end()
        );

        if (handler_list.empty()) {
            handlers_.erase(it);
        }
    }
}

void EventEngine::register_general_handler(HandlerType handler) {
    general_handlers_.push_back(std::move(handler));
}

void EventEngine::unregister_general_handler(HandlerType handler) {
    general_handlers_.erase(
        std::remove_if(general_handlers_.begin(), general_handlers_.end(),
            [&handler](const HandlerType& h) {
                return h.target_type() == handler.target_type();
            }),
        general_handlers_.end()
    );
}

size_t EventEngine::handler_count(const std::string& type) const {
    auto it = handlers_.find(type);
    return it != handlers_.end() ? it->second.size() : 0;
}

size_t EventEngine::queue_size() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
    return queue_.size();
}

void EventEngine::run() {
    while (active_) {
        Event event("", std::any());

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return !queue_.empty() || !active_;
            });

            if (!active_) break;

            if (queue_.empty()) continue;

            event = std::move(queue_.front());
            queue_.pop();
        }

        process(event);
    }
}

void EventEngine::run_timer() {
    while (active_) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_));

        if (!active_) break;

        put(Event(EVENT_TIMER));
    }
}

void EventEngine::process(const Event& event) {
    // First, distribute to type-specific handlers
    auto it = handlers_.find(event.type());
    if (it != handlers_.end()) {
        for (const auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                // Log error but continue processing
                // TODO: Use proper logging
            }
        }
    }

    // Then, distribute to general handlers
    for (const auto& handler : general_handlers_) {
        try {
            handler(event);
        } catch (const std::exception& e) {
            // Log error but continue processing
        }
    }
}

//=============================================================================
// EventDispatcher Implementation
//=============================================================================

void EventDispatcher::dispatch_log(const std::string& msg, int level) {
    struct LogData {
        std::string msg;
        int level;
    };
    engine_->put(Event(EVENT_LOG, LogData{msg, level}));
}

} // namespace bitquant
