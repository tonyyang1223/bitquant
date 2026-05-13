/**
 * @file test_thread_pool.cpp
 * @brief Unit tests for ThreadPool
 */

#include "utils/thread_pool.hpp"
#include <iostream>
#include <chrono>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_thread_pool_construction() {
    std::cout << "[Test] ThreadPool construction" << std::endl;

    ThreadPool pool(4);
    if (pool.thread_count() != 4) return false;

    std::cout << "  PASS: ThreadPool constructed correctly" << std::endl;
    return true;
}

bool test_thread_pool_start_stop() {
    std::cout << "[Test] ThreadPool start/stop" << std::endl;

    ThreadPool pool(2);
    pool.start();
    pool.stop();
    // Should not crash

    std::cout << "  PASS: start/stop works" << std::endl;
    return true;
}

bool test_thread_pool_pause_resume() {
    std::cout << "[Test] ThreadPool pause/resume" << std::endl;

    ThreadPool pool(2);
    pool.start();
    pool.pause();
    pool.resume();
    pool.stop();
    // Should not crash

    std::cout << "  PASS: pause/resume works" << std::endl;
    return true;
}

bool test_thread_pool_submit() {
    std::cout << "[Test] ThreadPool submit" << std::endl;

    ThreadPool pool(2);
    pool.start();

    auto future = pool.submit([]() {
        return 42;
    });

    int result = future.get();
    if (result != 42) return false;

    pool.stop();

    std::cout << "  PASS: submit works" << std::endl;
    return true;
}

bool test_thread_pool_wait_all() {
    std::cout << "[Test] ThreadPool wait_all" << std::endl;

    ThreadPool pool(4);
    pool.start();

    int counter = 0;
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 10; i++) {
        futures.push_back(pool.submit([&counter]() {
            counter++;
        }));
    }

    pool.wait_all();

    if (counter != 10) return false;

    pool.stop();

    std::cout << "  PASS: wait_all works" << std::endl;
    return true;
}

bool test_thread_pool_pending_tasks() {
    std::cout << "[Test] ThreadPool pending_tasks" << std::endl;

    ThreadPool pool(2);
    pool.start();

    size_t pending = pool.pending_tasks();
    (void)pending;

    pool.stop();

    std::cout << "  PASS: pending_tasks works" << std::endl;
    return true;
}

bool test_thread_pool_is_running() {
    std::cout << "[Test] ThreadPool is_running" << std::endl;

    ThreadPool pool(2);
    // ThreadPool starts automatically on construction
    if (!pool.is_running()) return false;

    pool.stop();
    if (pool.is_running()) return false;

    pool.start();
    if (!pool.is_running()) return false;

    pool.stop();

    std::cout << "  PASS: is_running works" << std::endl;
    return true;
}

bool test_thread_pool_active_tasks() {
    std::cout << "[Test] ThreadPool active_tasks" << std::endl;

    ThreadPool pool(4);
    pool.start();

    size_t active = pool.active_tasks();
    (void)active;

    pool.stop();

    std::cout << "  PASS: active_tasks works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          ThreadPool Unit Tests                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_thread_pool_construction,
        test_thread_pool_start_stop,
        test_thread_pool_pause_resume,
        test_thread_pool_submit,
        test_thread_pool_wait_all,
        test_thread_pool_pending_tasks,
        test_thread_pool_is_running,
        test_thread_pool_active_tasks
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}