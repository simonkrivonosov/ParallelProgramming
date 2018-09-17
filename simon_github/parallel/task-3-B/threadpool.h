#pragma once

#include <functional>
#include <future>
#include "safequeue.h"
#include <thread>
#include <utility>

#define queue_size 1000

template <class T>
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();
    explicit ThreadPool(const std::size_t num_threads);
    ThreadPool & operator=(const ThreadPool  & pool) = delete;
    ThreadPool(const ThreadPool & pool) = delete;
    std::future<T> Submit(std::function<T ()> task);
    void Shutdown();

private:
    std::vector<std::thread> workers_;
    std::atomic<bool> shutdown_;
    BlockingQueue<std::packaged_task<T ()>> task_queue_;
    int default_num_workers();
};

template <class T>
ThreadPool<T>::ThreadPool()
    : ThreadPool(default_num_workers())
    {}

template <class T>
ThreadPool<T>::ThreadPool(size_t num_threads)
    : shutdown_(false), task_queue_(queue_size)
{
    if (num_threads == 0)
        num_threads = default_num_workers();
    std::packaged_task<T ()> task();
    auto WorkerFunction = [this]() -> void {
        std::packaged_task<T ()> task;
        while(task_queue_.Get(task)) {
            task();
        }
    };
    for (unsigned int i = 0; i < num_threads; i++)
        workers_.emplace_back(std::thread(WorkerFunction));
}

template <class T>
int ThreadPool<T>::default_num_workers() {
    int cores = std::thread::hardware_concurrency();
    return cores > 0 ? cores : 4;
}

template <class T>
void ThreadPool<T>::Shutdown() {
    if(!shutdown_) {
        task_queue_.Shutdown();
        shutdown_.store(true);
        for (unsigned int i = 0; i < workers_.size(); i++)
            if (workers_[i].joinable())
                workers_[i].join();
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    Shutdown();
}

template<typename T>
std::future<T> ThreadPool<T>::Submit(const std::function<T ()> function) {
    std::packaged_task<T ()> task(function);
    std::future<T> future = task.get_future();
    task_queue_.Put(std::move(task));
    return std::move(future);
}

