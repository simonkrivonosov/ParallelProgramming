#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>


class QueueShutdowException:public std::exception {
public:
    QueueShutdowException(const char * p)
        : str_(p)
    {}
    const char* what() const noexcept override {
        return str_.c_str();
    }
    private:
        std::string str_;
};


template <class T, class Container = std::deque<T>>
class BlockingQueue {
public:
    explicit BlockingQueue(const std::size_t& capacity);
    BlockingQueue(const BlockingQueue & source) = delete;
    void Put(T&& element);
    bool Get(T& result);
    void Shutdown();

private:
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
    std::size_t capacity_;
    //std::size_t cur_size_;
    Container queue_;
    std::mutex mtx_;
    std::atomic<bool> shutdown_;
};


template <class T, class Container>
BlockingQueue<T, Container>::BlockingQueue(const std::size_t& capacity)
    : capacity_(capacity)
{
    shutdown_.store(false);
}


template<class T, class Container> void
BlockingQueue<T, Container>::Shutdown() {
    shutdown_.store(true);
    not_empty_cv_.notify_all();
}


template <class T, class Container>
void BlockingQueue<T, Container>::Put(T&& element) {
    std::unique_lock<std::mutex> lock(mtx_);
    if(shutdown_.load() == true) {
        throw QueueShutdowException("Queue is shut down");
    }
    while (queue_.size() == capacity_ && shutdown_.load() == false) {
        not_full_cv_.wait(lock);
    }
    queue_.push_back(std::move(element));
    if (queue_.size() == 1) {
        not_empty_cv_.notify_all();
    }
}


template<class T, class Container>
bool BlockingQueue<T, Container>:: Get(T& result) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (queue_.empty() && shutdown_.load() == false) {
        not_empty_cv_.wait(lock);
    }
    if(queue_.empty() && shutdown_.load() == true) {
        return false;
    }
    result = std::move(queue_.front());
    queue_.pop_front();
    not_full_cv_.notify_all();
    return true;
}
