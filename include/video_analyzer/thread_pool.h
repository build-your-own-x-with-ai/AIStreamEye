#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <stdexcept>

namespace video_analyzer {

/**
 * @brief Thread pool for parallel task execution
 * 
 * Manages a pool of worker threads that execute submitted tasks.
 * Automatically detects system core count if numThreads is 0.
 * Uses RAII to ensure proper thread cleanup.
 */
class ThreadPool {
public:
    /**
     * @brief Construct a thread pool
     * @param numThreads Number of worker threads (0 = auto-detect from hardware)
     */
    explicit ThreadPool(size_t numThreads = 0);
    
    /**
     * @brief Destructor - waits for all tasks to complete and joins threads
     */
    ~ThreadPool();
    
    // Disable copy
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // Disable move (threads cannot be moved safely)
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    /**
     * @brief Submit a task for execution
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to function
     * @return Future that will contain the result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        
        // Create a packaged task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    /**
     * @brief Wait for all currently queued tasks to complete
     * 
     * Note: This does not prevent new tasks from being submitted.
     * It only waits for tasks that were queued at the time of the call.
     */
    void waitAll();
    
    /**
     * @brief Get the number of worker threads
     * @return Number of threads in the pool
     */
    size_t getThreadCount() const;
    
private:
    // Worker threads
    std::vector<std::thread> workers_;
    
    // Task queue
    std::queue<std::function<void()>> tasks_;
    
    // Synchronization
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
    // Track active tasks for waitAll()
    std::atomic<size_t> activeTasks_;
    std::condition_variable allTasksComplete_;
    
    /**
     * @brief Worker thread function
     */
    void workerThread();
    
    /**
     * @brief Detect number of hardware threads
     * @return Number of hardware threads, or 1 if detection fails
     */
    static size_t detectHardwareThreads();
};

} // namespace video_analyzer
