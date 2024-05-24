/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"

using namespace std;

void ThreadPool::dispatcher_func() {
    while (true) {
        sem.wait(); // Esperar una señal que indica que una nueva tarea está disponible

        unique_lock<mutex> queueLock(queue_mutex);
        if (stop) {
            // Detener todos los hilos de trabajo
            for (auto& wt : wts) {
                wt.lock.lock();
                wt.stop = true;
                wt.lock.unlock();
            }
            cv.notify_all(); // Notificar a todos los hilos de trabajo
            break;
        }

        if (tasks.empty()) {
            continue;
        }

        // Obtener la siguiente tarea de la cola
        auto task = move(tasks.front());
        tasks.pop();

        queueLock.unlock(); // Liberar el bloqueo en la cola de tareas

        unique_lock<mutex> workerLock(worker_mutex);
        cv.wait(workerLock, [this] {
            // Esperar a que un hilo de trabajo esté durmiendo
            for (auto& wt : wts) {
                if (wt.sleep) return true;
            }
            return false;
        });

        // Encontrar un hilo de trabajo durmiendo y asignarle la tarea
        for (auto& wt : wts) {
            wt.lock.lock();
            if (wt.sleep) {
                wt.work = make_shared<function<void()>>(move(task));
                wt.sleep = false;
                wt.lock.unlock();
                cv.notify_all(); // Notificar al hilo de trabajo específico
                break;
            }
            wt.lock.unlock();
        }
    }
}


void ThreadPool::worker_func(Worker_t* wt) {
    while (true) {
        wt->lock.lock();
        if (wt->stop) {
            wt->lock.unlock();
            break;
        }
        if (wt->work == nullptr) {
            wt->sleep = true;
            wt->lock.unlock();
            unique_lock<mutex> lock(worker_mutex);
            cv.notify_one(); // Notify the dispatcher that a worker is sleeping
            cv.wait(lock, [wt] { return wt->work != nullptr || wt->stop; }); // Wait until there is work or the thread should stop
            continue;
        }
        auto work = move(wt->work);
        wt->work = nullptr;
        wt->lock.unlock();
        (*work)(); // Execute the task
        {
            lock_guard<mutex> lock(queue_mutex);
            tasks_in_progress--;
            if (tasks_in_progress == 0){
                cv.notify_all(); // Notify all threads waiting in the wait() function
            }
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), stop(false), tasks_in_progress(0) {
    size_t id = 0;
    for (auto& wt : wts) {
        wt.id = id++;
        wt.sleep = true;
        wt.work = nullptr;
        wt.th = thread([this, &wt] { this->worker_func(&wt); });
    }
    dpt.th = thread([this] { this->dispatcher_func(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        lock_guard<mutex> lock(queue_mutex);
        tasks_in_progress++;
        tasks.push(thunk); // Add the task to the queue
    }
    sem.signal(); // Signal the dispatcher that a new task is available
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queue_mutex);
    while(tasks_in_progress>0){
        cv.wait(lock); // Wait until all tasks are completed
    }
}

ThreadPool::~ThreadPool() {
    wait(); // Wait for all tasks to complete
    {
        unique_lock<mutex> lock(queue_mutex);
        stop = true;
    }
    sem.signal(); // Signal the dispatcher to stop
    dpt.th.join(); // Wait for the dispatcher thread to finish
    for (auto& wt : wts) {
        wt.lock.lock();
        wt.stop = true;
        wt.lock.unlock();
    }
    cv.notify_all(); // Notify all threads once
    for (auto& wt : wts) {
        wt.th.join(); // Wait for all worker threads to finish
    }
}
