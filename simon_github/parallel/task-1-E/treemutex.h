#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>


class PetersonMutex{
public:
    PetersonMutex()
        : victim_(0) {
        want_[0].store(false);
        want_[1].store(false);
    }

    void lock(size_t current_thread);
    void unlock(size_t current_thread);

private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<size_t> victim_;
};


void PetersonMutex::lock(size_t current_thread){
    want_[current_thread].store(true);
    victim_.store(current_thread);
    while(want_[1 - current_thread].load() && victim_.load() == current_thread){
        std::this_thread::yield();
    }
}

void PetersonMutex::unlock(size_t current_thread){
    want_[current_thread].store(false);
}

class TreeMutex{
public:
    explicit TreeMutex(size_t n_threads)
        : tree_depth_(GetDepth(n_threads)), tree_(GetDegree(n_threads))
        {}
    static int GetDegree(int n);
    static int GetDepth(int threads_num);
    void lock(size_t current_thread);
    void unlock(size_t current_thread);

private:
    int tree_depth_;
    std::vector<PetersonMutex> tree_;
};


void TreeMutex::lock(size_t current_thread){
    size_t id = tree_.size() + current_thread;
    while(id != 0){
        tree_[(id - 1) / 2].lock((id + 1) & 1);
        id = (id - 1) / 2;
    }
}

int TreeMutex::GetDegree(int n) {
     int64_t degree = 1;
     while(degree < n + 1 / 2){
         degree *= 2 ;
     }
     return 2 * degree - 1;
 }

int TreeMutex::GetDepth(int threads_num)
{
    int depth = 0;
    while ((1 << depth) < threads_num) depth++;
    return depth;
}

void TreeMutex::unlock(size_t current_thread) {
    size_t mutex_id = 0;
    for (int i = tree_depth_ - 1; i >= 0; i--) {
        tree_[mutex_id].unlock((current_thread >> i) & 1);
        mutex_id = mutex_id * 2 + 1 + ((current_thread >> i) & 1);
    }
}

