#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>

enum step_type {left, right, none};


class Robot {
public:
    Robot ()
        : last_step_(right)
    {}
    void StepLeft() {
        std::unique_lock<std::mutex> lock(mtx);
        left_cond_var_.wait(lock, [this]() { return last_step_ == right; });
        std::cout << "left" << std::endl;
        last_step_ = left;
        right_cond_var_.notify_one();
    }

    void StepRight() {
        std::unique_lock<std::mutex> lock(mtx);
        right_cond_var_.wait(lock, [this]() { return last_step_ == left; });
        std::cout << "right" << std::endl;
        last_step_ = right;
        left_cond_var_.notify_one();
    }

private:
    std::mutex mtx;
    step_type last_step_;
    std::condition_variable left_cond_var_;
    std::condition_variable right_cond_var_;
};

