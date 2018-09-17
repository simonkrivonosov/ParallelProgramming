
#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <limits>
#include <mutex>
#include <memory>
#include <vector>
#include <thread>

class ArenaAllocator {
public:
    explicit ArenaAllocator(const size_t capacity = 4 * 1024 * 1024)
        : arena_(new unsigned char[capacity])
    {}

    ArenaAllocator(const ArenaAllocator& that ) = delete;
    ArenaAllocator(ArenaAllocator&&  that ) = delete;

    ~ArenaAllocator() {
        delete[] arena_;
    }

    template <typename TObject>
    void* Allocate(const size_t alignment = alignof(TObject)) {
        size_t block_size = sizeof(TObject) + alignment;
        void* addr = arena_ + next_offset_.fetch_add(block_size);
        return std::align(alignment, sizeof(TObject), addr, block_size);
    }

    template <typename TObject, typename... Args>
    TObject* New(Args&&... args) {
        void* addr = Allocate<TObject>();
        new (addr) TObject(std::forward<Args>(args)...);
        return static_cast<TObject*>(addr);
    }

    size_t SpaceUsed() const {
        return next_offset_.load();
    }

private:
    unsigned char* arena_;
    std::atomic<size_t> next_offset_{0};
};


class TicketSpinlock {
public:
    explicit TicketSpinlock()
        : owner_ticket_(0)
        , next_free_ticket_(0)
    {}

    void Lock() {
        int this_thread_ticket = next_free_ticket_.fetch_add(1);
        while (owner_ticket_.load() != this_thread_ticket) {
        }
    }

    void Unlock() {
        owner_ticket_.store(owner_ticket_.load() + 1);
    }

    void lock() {
        Lock();
    }

    void unlock() {
        Unlock();
    }

private:
    std::atomic<int> owner_ticket_;
    std::atomic<int> next_free_ticket_;
};
template <typename T>
struct ElementTraits {
    static T Min() {
        return std::numeric_limits<T>::min();
    }
    static T Max() {
        return std::numeric_limits<T>::max();
    }
};

template <typename T>
class OptimisticLinkedSet {
 public:
    explicit OptimisticLinkedSet(ArenaAllocator& allocator);
    bool Insert(const T& element);
    bool Remove(const T& element);
    bool Contains(const T& element) const;
    size_t Size() const;

 private:
    struct Node {
        T value_;
        std::atomic<Node*> next_;
        TicketSpinlock spinlock_{};
        std::atomic<bool> marked_{false};

        Node(const T& value, Node* next = nullptr)
            : value_(value),
              next_(next)
        {}
    };

    struct Edge {
        Node* pred_;
        Node* curr_;

        Edge(Node* pred, Node* curr)
            : pred_(pred),
              curr_(curr)
        {}
    };

    void CreateEmptyList();
    Edge Locate(const T& key) const;
    bool Validate(const Edge& edge) const;
    ArenaAllocator& allocator_;
    Node* head_{nullptr};
    std::atomic<std::size_t> size_{0};
};


template<typename T>
void OptimisticLinkedSet<T>::CreateEmptyList() {
    head_ = allocator_.New<Node>(ElementTraits<T>::Min());
    head_->next_ = allocator_.New<Node>(ElementTraits<T>::Max());
}

template<typename T>
OptimisticLinkedSet<T>::OptimisticLinkedSet(ArenaAllocator& allocator)
    : allocator_(allocator) {
            CreateEmptyList();
}

template<typename T>
typename OptimisticLinkedSet<T>::Edge OptimisticLinkedSet<T>::Locate(const T& key) const {
    Node* pred = head_;
    Node* curr = head_->next_;
    while (curr->value_ < key) {
        pred = curr;
        curr = curr->next_;
    }
    return Edge{pred, curr};
}

template<typename T>
bool OptimisticLinkedSet<T>::Validate(const Edge& edge) const {
    return (!edge.curr_->marked_ && !edge.pred_->marked_ && edge.pred_->next_ == edge.curr_);
}

template<typename T>
bool OptimisticLinkedSet<T>::Contains(const T& element) const {
    Node* curr = head_;
    while (curr->value_ < element) {
        curr = curr->next_;
    }
    return (curr->value_ == element && !curr->marked_);
}

template<typename T>
bool OptimisticLinkedSet<T>::Insert(const T& element) {
    while (true) {
        Edge edge = Locate(element);
        std::unique_lock<TicketSpinlock> lock_pred(edge.pred_->spinlock_);
        std::unique_lock<TicketSpinlock> lock_curr(edge.curr_->spinlock_);
        if (Validate(edge)) {
            if (edge.curr_->value_ == element) {
                return false;
            } else {
                Node* new_node = allocator_.New<Node>(element);
                new_node->next_.store(edge.curr_);
                edge.pred_->next_.store(new_node);
                size_++;
                return true;
            }
        }
    }
}

template<typename T>
bool OptimisticLinkedSet<T>::Remove(const T& element) {
    while(true) {
        Edge edge = Locate(element);
        std::unique_lock<TicketSpinlock> lock_pred(edge.pred_->spinlock_);
        std::unique_lock<TicketSpinlock> lock_curr(edge.curr_->spinlock_);
        if (Validate(edge)) {
            if (edge.curr_->value_ == element) {
                edge.curr_->marked_.store(true);
                edge.pred_->next_.store(edge.curr_->next_);
                size_--;
                return true;
            } else
                return false;
        }
    }

}

template<typename T>
size_t OptimisticLinkedSet<T>::Size() const {
    return size_;
}

template <typename T> using ConcurrentSet = OptimisticLinkedSet<T>;


