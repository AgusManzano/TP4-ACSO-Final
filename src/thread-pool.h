/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>       // for queue of tasks
#include <mutex>       // for mutex
#include <condition_variable> //for condition_variable for task availability
#include "Semaphore.h" 
#include <memory>
#include <iostream>
#include <atomic>


class ThreadPool {
public:
    /**
     * Constructs a ThreadPool configured to spawn up to the specified
     * number of threads.
     */
    ThreadPool(size_t numThreads);
    
    /**
     * Waits for all previously scheduled thunks to execute, and then
     * properly brings down the ThreadPool and any resources tapped
     * over the course of its lifetime.
     */
    ~ThreadPool();

    /**
     * Schedules the provided thunk (which is something that can
     * be invoked as a zero-argument function without a return value)
     * to be executed by one of the ThreadPool's threads as soon as
     * all previously scheduled thunks have been handled.
     */
    void schedule(const std::function<void(void)>& thunk);
    
    /**
     * Blocks and waits until all previously scheduled thunks
     * have been executed in full.
     */
    void wait();


private:
    struct Worker_t {
        size_t id;
        bool sleep;
        bool stop;
        std::shared_ptr<std::function<void()>> work;
        std::thread th;
        std::mutex lock;
    };

    struct DispatcherThread {
        std::thread th;
    } dt;


    void dispatcher_func();
    void worker_func(Worker_t* wt);

    std::vector<Worker_t> wts; // Vector of worker threads
    std::queue<std::function<void()>> tasks; // Queue of tasks
    DispatcherThread dpt; // Dispatcher thread
    std::mutex queue_mutex; // Mutex for the task queue
    std::mutex worker_mutex; // Mutex for worker threads
    std::condition_variable cv; // Condition variable for task availability
    bool stop; // Flag to stop the thread pool
    Semaphore sem; // Semaphore for task synchronization
    std::atomic<size_t> tasks_in_progress; // Counter for tasks in progress

    /**
     * ThreadPools are the type of thing that shouldn't be cloneable, since it's
     * not clear what it means to clone a ThreadPool (should copies of all outstanding
     * functions to be executed be copied?).
     *
     * In order to prevent cloning, we remove the copy constructor and the
     * assignment operator.  By doing so, the compiler will ensure we never clone
     * a ThreadPool.
     */
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;

};

#endif // THREAD_POOL_H