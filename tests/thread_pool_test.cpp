#include "video_analyzer/thread_pool.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

using namespace video_analyzer;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }
    
    void TearDown() override {
        // Cleanup
    }
};

// Test: ThreadPool construction with auto-detection
TEST_F(ThreadPoolTest, ConstructionAutoDetect) {
    ThreadPool pool(0);  // 0 = auto-detect
    
    // Should have at least 1 thread
    EXPECT_GE(pool.getThreadCount(), 1u);
    
    // Should not exceed hardware threads
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads > 0) {
        EXPECT_LE(pool.getThreadCount(), hardwareThreads);
    }
}

// Test: ThreadPool construction with specific thread count
TEST_F(ThreadPoolTest, ConstructionSpecificCount) {
    ThreadPool pool(4);
    
    // Should have at most 4 threads (or hardware limit)
    EXPECT_LE(pool.getThreadCount(), 4u);
    EXPECT_GE(pool.getThreadCount(), 1u);
}

// Test: ThreadPool limits thread count to hardware threads
TEST_F(ThreadPoolTest, ThreadCountLimitedToHardware) {
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads == 0) {
        GTEST_SKIP() << "Cannot detect hardware threads";
    }
    
    // Request more threads than available
    ThreadPool pool(hardwareThreads * 2);
    
    // Should be limited to hardware threads
    EXPECT_LE(pool.getThreadCount(), hardwareThreads);
}

// Test: Submit and execute simple task
TEST_F(ThreadPoolTest, SubmitSimpleTask) {
    ThreadPool pool(2);
    
    std::atomic<int> counter{0};
    
    auto future = pool.submit([&counter]() {
        counter++;
        return 42;
    });
    
    int result = future.get();
    
    EXPECT_EQ(result, 42);
    EXPECT_EQ(counter, 1);
}

// Test: Submit multiple tasks
TEST_F(ThreadPoolTest, SubmitMultipleTasks) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::future<int>> futures;
    
    // Submit 10 tasks
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([&counter, i]() {
            counter++;
            return i * 2;
        }));
    }
    
    // Wait for all tasks and verify results
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * 2);
    }
    
    EXPECT_EQ(counter, 10);
}

// Test: Tasks execute in parallel
TEST_F(ThreadPoolTest, ParallelExecution) {
    ThreadPool pool(4);
    
    std::atomic<int> activeCount{0};
    std::atomic<int> maxActive{0};
    
    std::vector<std::future<void>> futures;
    
    // Submit tasks that sleep briefly
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.submit([&activeCount, &maxActive]() {
            int current = ++activeCount;
            
            // Update max if needed
            int expected = maxActive.load();
            while (current > expected && 
                   !maxActive.compare_exchange_weak(expected, current)) {
                expected = maxActive.load();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            --activeCount;
        }));
    }
    
    // Wait for all tasks
    for (auto& f : futures) {
        f.get();
    }
    
    // With 4 threads and 8 tasks, we should have had at least 2 active at once
    EXPECT_GE(maxActive, 2);
}

// Test: waitAll() waits for all tasks to complete
TEST_F(ThreadPoolTest, WaitAllCompletesAllTasks) {
    ThreadPool pool(2);
    
    std::atomic<int> counter{0};
    
    // Submit tasks
    for (int i = 0; i < 5; ++i) {
        pool.submit([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter++;
        });
    }
    
    // Wait for all tasks
    pool.waitAll();
    
    // All tasks should be complete
    EXPECT_EQ(counter, 5);
}

// Test: Task with return value
TEST_F(ThreadPoolTest, TaskWithReturnValue) {
    ThreadPool pool(2);
    
    auto future = pool.submit([]() -> std::string {
        return "Hello, ThreadPool!";
    });
    
    std::string result = future.get();
    EXPECT_EQ(result, "Hello, ThreadPool!");
}

// Test: Task with parameters
TEST_F(ThreadPoolTest, TaskWithParameters) {
    ThreadPool pool(2);
    
    auto add = [](int a, int b) { return a + b; };
    
    auto future = pool.submit(add, 10, 32);
    
    EXPECT_EQ(future.get(), 42);
}

// Test: Exception in task is propagated
TEST_F(ThreadPoolTest, ExceptionPropagation) {
    ThreadPool pool(2);
    
    auto future = pool.submit([]() -> int {
        throw std::runtime_error("Test exception");
        return 42;
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

// Test: RAII cleanup - destructor waits for tasks
TEST_F(ThreadPoolTest, RAIICleanup) {
    std::atomic<int> counter{0};
    
    {
        ThreadPool pool(2);
        
        // Submit tasks
        for (int i = 0; i < 5; ++i) {
            pool.submit([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                counter++;
            });
        }
        
        // Pool destructor should wait for all tasks
    }
    
    // All tasks should have completed
    EXPECT_EQ(counter, 5);
}

// Test: Thread safety - concurrent submissions
TEST_F(ThreadPoolTest, ConcurrentSubmissions) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::thread> submitters;
    
    // Multiple threads submitting tasks
    for (int t = 0; t < 4; ++t) {
        submitters.emplace_back([&pool, &counter]() {
            for (int i = 0; i < 10; ++i) {
                pool.submit([&counter]() {
                    counter++;
                });
            }
        });
    }
    
    // Wait for all submitters
    for (auto& t : submitters) {
        t.join();
    }
    
    // Wait for all tasks
    pool.waitAll();
    
    // Should have executed 40 tasks
    EXPECT_EQ(counter, 40);
}

// Test: Cannot submit after destruction
TEST_F(ThreadPoolTest, CannotSubmitAfterStop) {
    auto pool = std::make_unique<ThreadPool>(2);
    
    // Destroy the pool
    pool.reset();
    
    // Cannot test submission after destruction as pool is destroyed
    // This test verifies that destruction completes without hanging
    SUCCEED();
}

// Test: Large number of tasks
TEST_F(ThreadPoolTest, LargeNumberOfTasks) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    // Submit 1000 tasks
    for (int i = 0; i < 1000; ++i) {
        futures.push_back(pool.submit([&counter]() {
            counter++;
        }));
    }
    
    // Wait for all
    for (auto& f : futures) {
        f.get();
    }
    
    EXPECT_EQ(counter, 1000);
}

// Test: Task ordering is not guaranteed but all execute
TEST_F(ThreadPoolTest, AllTasksExecute) {
    ThreadPool pool(2);
    
    std::vector<std::atomic<bool>> executed(100);
    for (auto& e : executed) {
        e = false;
    }
    
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < executed.size(); ++i) {
        futures.push_back(pool.submit([&executed, i]() {
            executed[i] = true;
        }));
    }
    
    // Wait for all
    for (auto& f : futures) {
        f.get();
    }
    
    // All should have executed
    for (const auto& e : executed) {
        EXPECT_TRUE(e);
    }
}
