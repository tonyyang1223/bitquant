/**
 * @file thread_pool.cpp
 * @brief Thread pool implementation
 */

#include "utils/thread_pool.hpp"

namespace bitquant {

ThreadPool::ThreadPool(size_t num_threads) {
    threads_.reserve(num_threads);
    running_.store(true);

    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start() {
    if (!running_.load()) {
        running_.store(true);
        for (auto& t : threads_) {
            if (!t.joinable()) {
                t = std::thread(&ThreadPool::worker_loop, this);
            }
        }
    }
}

void ThreadPool::stop() {
    running_.store(false);
    cv_.notify_all();

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::pause() {
    paused_.store(true);
}

void ThreadPool::resume() {
    paused_.store(false);
    cv_.notify_all();
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(mutex_);
    done_cv_.wait(lock, [this] {
        return tasks_.empty() && active_count_.load() == 0;
    });
}

size_t ThreadPool::pending_tasks() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return tasks_.size();
}

void ThreadPool::worker_loop() {
    while (running_.load()) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return (!running_.load()) || (!paused_.load() && !tasks_.empty());
            });

            if (!running_.load()) return;

            if (tasks_.empty()) continue;

            task = std::move(tasks_.front());
            tasks_.pop();
            active_count_++;
        }

        task();
        active_count_--;
        completed_tasks_++;
        done_cv_.notify_all();
    }
}

} // namespace bitquant