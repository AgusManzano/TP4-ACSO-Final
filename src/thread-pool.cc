/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"


using namespace std;

void ThreadPool::dispatcher_func(void) {
    while(1){
        sem.wait();
        queue_mutex.lock();
        if (stop) {
            queue_mutex.unlock();
            break;
        }
        if (tasks.empty()) {
            queue_mutex.unlock();
            continue;
        }
        auto task = move(tasks.front());
        tasks.pop();
        queue_mutex.unlock();

        for (auto& wt : wts) {
            wt.lock.lock();
            if (wt.sleep){
                wt.work = make_shared<function<void()>>(move(task));
                wt.sleep = false;
                wt.lock.unlock();
                break;
            }
            wt.lock.unlock();
        }
    }
}

void ThreadPool::worker_func(Worker_t* wt) {
    while (true) {
        wt->lock.lock();
        if (stop) { // check if we should stop
            wt->lock.unlock();
            break;
        }
        if (!wt->sleep && wt->work != nullptr){
            auto task = move(*wt->work);
            wt->work = nullptr;
            wt->sleep = true;
            wt->lock.unlock();
            task();
            queue_mutex.lock();
            if (tasks.empty()) {
                cv.notify_all(); // notify all waiting threads
            }
            queue_mutex.unlock();
        }else{
            wt->lock.unlock();
        }
    }
}


ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), stop(false) {
    size_t id = 0;
    for (auto& wt : wts) {
        wt.id = id++;
        wt.sleep = true;    
        wt.work = nullptr;
        wt.th = thread([this, &wt] { this->worker_func(&wt); });
    }
    dt.th = thread([this] { this->dispatcher_func(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    unique_lock<mutex> lock(queue_mutex);
    tasks.push(thunk);
    sem.signal();
    lock.unlock(); // Add this line to unlock the mutex
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queue_mutex);
    cv.wait(lock, [this] { return tasks.empty(); });
}


ThreadPool::~ThreadPool() {
    // Stop the dispatcher thread and wait for all worker threads to finish
    stop = true; // set stop to true
    sem.signal();
    for (auto& wt : wts) {
        wt.th.join();
    }
    dt.th.join();
}