#pragma once

#include <condition_variable>
#include <mutex>

template <class ConditionVariable = std::condition_variable>
class CyclicBarrier {
public:
    explicit CyclicBarrier(size_t num_threads);
    CyclicBarrier() = delete;
    CyclicBarrier(const CyclicBarrier & other) = delete;
    void Pass();

private:
    std::mutex mutex_;
    std::condition_variable come_to_barrier_cv_;
    size_t num_threads_;
    size_t entered_;
    size_t era_;
};


template <class ConditionVariable> CyclicBarrier<ConditionVariable>::CyclicBarrier(size_t num_threads)
    : num_threads_(num_threads),
      entered_(0),
      era_(0)
    {}


template <class ConditionVariable> void CyclicBarrier<ConditionVariable>::Pass() {
    std::unique_lock<std::mutex> lock(mutex_);
    entered_++;
    std::size_t current_era = era_;

    if (entered_ == num_threads_) {
        entered_ = 0;
        era_++;
        come_to_barrier_cv_.notify_all();
    } else {
        come_to_barrier_cv_.wait(lock, [this, current_era] { return current_era != era_; });
    }
}

