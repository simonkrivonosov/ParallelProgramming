#pragma once


#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optimistic_linked_list.h>
#include <shared_mutex>
#include <thread>
#include <vector>


ArenaAllocator allocator{};


template <typename T, class Hash = std::hash<T>>
class StripedHashSet {
public:
    StripedHashSet ()
        : StripedHashSet (10, 2, 1)
    {}
    explicit StripedHashSet (std::size_t buckets_number, std::size_t growth_factor = 2, std::size_t max_load_factor = 1)
        : growth_factor_(growth_factor),
          max_load_factor_(max_load_factor)
    {

        for (size_t i = 0; i < buckets_number; ++i) {
            buckets_.emplace_back(std::make_unique<OptimisticLinkedSet<T>>(allocator));
        }
    }

    bool Remove (const T& element) {
        std::shared_lock<std::shared_timed_mutex> lock(mtx_);
        std::size_t bucket_index = GetBucketIndex(element);
        if (buckets_[bucket_index]->Remove(element)) {
            amount_--;
        } else
            return false;
        return true;
    }

    bool Insert (const T& element) {
        std::shared_lock<std::shared_timed_mutex> lock(mtx_);
        if (buckets_[GetBucketIndex(element)]->Insert(element)) {
            amount_++;
        } else
            return false;
        if (GetLoadFactor() > max_load_factor_) {
            if (need_to_rehash_.exchange(true) == false) {
                std::unique_lock<std::shared_timed_mutex> lock(mtx_);
                Resize();
            }
        }
        return true;
    }

    bool Contains (const T& element) {
        std::shared_lock<std::shared_timed_mutex> lock(mtx_);
        std::size_t bucket_index = GetBucketIndex(element);
        return buckets_[bucket_index]->Contains(element);
    }

    void Resize () {
        if (GetLoadFactor() > max_load_factor_) {
            std::vector<std::unique_ptr<OptimisticLinkedSet<T>>> tmp_buckets;
            size_t new_size = buckets_.size() * growth_factor_;
            for (size_t i = 0; i < new_size; ++i) {
                tmp_buckets.emplace_back(std::make_unique<OptimisticLinkedSet<T>>(allocator));
            }

            for (std::size_t i = 0; i < buckets_.size(); ++i) {
                for (auto it = buckets_[i]->Begin(); it != buckets_[i]->End(); ++it) {
                    std::size_t bucket_index = hash_function_(it->value_) % tmp_buckets.size();
                    tmp_buckets[bucket_index]->Insert(it->value_);
                }
            }

            buckets_ = std::move(tmp_buckets);
        }
        need_to_rehash_.store(false);
    }

    size_t Size() {
        return amount_;
    }

private:

    double GetLoadFactor () {
        return amount_ / buckets_.size();
    }

    std::size_t GetBucketIndex (const T& hash_value) {
        return hash_function_(hash_value) % buckets_.size();
    }

    std::size_t growth_factor_;
    std::shared_timed_mutex mtx_;
    std::vector<std::unique_ptr<OptimisticLinkedSet<T>>> buckets_;
    std::atomic<std::size_t> amount_{0};
    std::size_t max_load_factor_;
    Hash hash_function_;
    std::atomic<bool> need_to_rehash_{false};
};

template <typename T> using ConcurrentSet = StripedHashSet<T>;

