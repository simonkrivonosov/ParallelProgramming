#pragma once


#include <iostream>
#include <mutex>
#include <condition_variable>


class Semaphore {
public:
    Semaphore (int count = 0)
        : count_(count)
    {}

    void Notify() {
        std::unique_lock<std::mutex> lock(mtx_);
        count_++;
        cv_.notify_one();
    }

    void Wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]() { return count_ > 0; });
        count_--;
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
};


class Robot {
public:
    Robot()
        : left_sem_(1)
    {}

    void StepLeft() {
        left_sem_.Wait();
        std::cout << "left" << std::endl;
        right_sem_.Notify();
    }

    void StepRight() {
        right_sem_.Wait();
        std::cout << "right" << std::endl;
        left_sem_.Notify();
    }
private:
    Semaphore left_sem_;
    Semaphore right_sem_;
};




