#include "nn/Core/ThreadPool.hpp"
#include <atomic>
#include <cmath>
#include <algorithm>

namespace nn {

ThreadPool& ThreadPool::instance() {
    static ThreadPool pool;
    return pool;
}

ThreadPool::ThreadPool() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8; // Default for M1 if undefined

    for (unsigned int i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this, &stoken]() { return !tasks_.empty() || stoken.stop_requested(); });
                    if (stoken.stop_requested() && tasks_.empty()) return;
                    
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    // std::jthread requests stop and joins automatically on destruction,
    // but we need to notify the condition variable so threads wake up and check stop_token.
    for (auto& worker : workers_) {
        worker.request_stop();
    }
    condition_.notify_all();
}

} // namespace nn
