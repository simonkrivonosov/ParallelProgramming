#pragma once


#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>
#include <utility>
#include <memory>

class Semaphore {
public:
    explicit Semaphore (int count)
        : count_(count)
        {}
    Semaphore()
        : Semaphore(0)
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
    Robot(const std::size_t num_foots)
        : num_foots_(num_foots)
    {
        semaphores_.push_back(std::make_unique<Semaphore>(1));
        for (size_t i = 1; i < num_foots; ++i)
        {
            semaphores_.push_back(std::make_unique<Semaphore>(0));
        }

    }
    void Step(const std::size_t foot) {
        semaphores_[foot]->Wait();
        std::cout << "foot " << foot << std::endl;
        semaphores_[(foot + 1) % num_foots_]->Notify();
    }
private:
    size_t num_foots_;
    std::vector<std::unique_ptr<Semaphore>> semaphores_;
};


