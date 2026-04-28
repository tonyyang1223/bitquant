/**
 * @file thread_pool.hpp
 * @brief Thread pool for parallel task execution
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace bitquant {

/**
 * @brief Thread pool for concurrent task execution
 *
 * Usage:
 *   ThreadPool pool(4);
 *   auto result = pool.submit([]{ return compute(); });
 *   pool.wait_all();
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    // Wait for all tasks to complete
    void wait_all();

    // Control
    void start();
    void stop();
    void pause();
    void resume();

    // Status
    bool is_running() const { return running_.load(); }
    size_t pending_tasks() const;
    size_t active_tasks() const { return active_count_.load(); }
    size_t thread_count() const { return threads_.size(); }

private:
    void worker_loop();

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable done_cv_;

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<size_t> active_count_{0};
    std::atomic<size_t> total_tasks_{0};
    std::atomic<size_t> completed_tasks_{0};
};

// Implementation
template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace([task]() { (*task)(); });
        total_tasks_++;
    }

    cv_.notify_one();
    return result;
}

} // namespace bitquant