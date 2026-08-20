#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>

namespace nn {

class ThreadPool {
   public:
    // TODO: Implement Singleton pattern (static instance() method)
    static ThreadPool& instance();

    // Prevent copying
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F>
    void parallelFor(int start, int end, F func) {
        int total = end - start;
        if (total <= 0) return;

        unsigned int numThreads = workers_.size();
        if (total <= 4096 || numThreads == 0) {
            func(start, end);
            return;
        }

        int chunkSize = std::max(1, total / (int)numThreads);

        std::shared_ptr<std::atomic<int>> completedChunks = std::make_shared<std::atomic<int>>(0);
        int expectedChunks = (total + chunkSize - 1) / chunkSize;

        std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        for (int i = start; i < end; i += chunkSize) {
            int chunkEnd = std::min(i + chunkSize, end);
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                tasks_.push([i, chunkEnd, func, completedChunks, expectedChunks, promise]() {
                    func(i, chunkEnd);
                    if (++(*completedChunks) == expectedChunks) {
                        promise->set_value();
                    }
                });
            }
        }

        condition_.notify_all();
        future.wait();
    }

   private:
    // Private constructor for Singleton
    ThreadPool();
    ~ThreadPool();

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
};

}  // namespace nn
