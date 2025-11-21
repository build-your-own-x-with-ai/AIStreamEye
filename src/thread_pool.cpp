#include "video_analyzer/thread_pool.h"
#include <algorithm>

namespace video_analyzer {

ThreadPool::ThreadPool(size_t numThreads)
    : stop_(false), activeTasks_(0) {
    
    // Auto-detect hardware threads if numThreads is 0
    if (numThreads == 0) {
        numThreads = detectHardwareThreads();
    }
    
    // Limit to hardware threads to avoid oversubscription
    size_t hardwareThreads = detectHardwareThreads();
    numThreads = std::min(numThreads, hardwareThreads);
    
    // Ensure at least one thread
    numThreads = std::max(numThreads, size_t(1));
    
    // Create worker threads
    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    
    // Wake up all threads
    condition_.notify_all();
    
    // Wait for all threads to finish
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    allTasksComplete_.wait(lock, [this] {
        return tasks_.empty() && activeTasks_ == 0;
    });
}

size_t ThreadPool::getThreadCount() const {
    return workers_.size();
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for a task or stop signal
            condition_.wait(lock, [this] {
                return stop_ || !tasks_.empty();
            });
            
            // Exit if stopped and no more tasks
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            // Get next task
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
                ++activeTasks_;
            }
        }
        
        // Execute task outside the lock
        if (task) {
            task();
            
            // Decrement active tasks and notify if all complete
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                --activeTasks_;
                if (tasks_.empty() && activeTasks_ == 0) {
                    allTasksComplete_.notify_all();
                }
            }
        }
    }
}

size_t ThreadPool::detectHardwareThreads() {
    unsigned int threads = std::thread::hardware_concurrency();
    
    // hardware_concurrency() returns 0 if it cannot detect
    if (threads == 0) {
        return 1;  // Fallback to single thread
    }
    
    return static_cast<size_t>(threads);
}

} // namespace video_analyzer
