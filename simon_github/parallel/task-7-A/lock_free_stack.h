#pragma once

#include <atomic>
#include <thread>


template <typename T>
class LockFreeStack {
    struct Node {
        T element_;
        std::atomic<Node*> next_;
        explicit Node(T & element):
            element_(element)
        {}
    };

 public:
    ~LockFreeStack() {
        T elem;
        while (Pop(elem));
        Node* top_to_delete = to_be_deleted_.load();
        while (top_to_delete) {
            Node* next = top_to_delete->next_.load();
            delete top_to_delete;
            top_to_delete = next;
        }
    }

    void Push(T element) {
        Node* new_node = new Node(element);
        new_node->next_.store(top_.load());
        Node* next_node = new_node->next_.load();
        while (!top_.compare_exchange_weak(next_node,new_node)) {
            new_node->next_.store(next_node);
        }
    }

    bool Pop(T& element) {
        Node* old_top = top_.load();
        if (!old_top)
            return false;
        while (!top_.compare_exchange_weak(old_top,old_top->next_)) {
            if (!old_top)
                return false;
        }
        element = old_top->element_;
        Node * top_to_delete = to_be_deleted_.load();
        old_top->next_.store(top_to_delete);
        while (!to_be_deleted_.compare_exchange_weak(top_to_delete, old_top)) {
            old_top->next_.store(top_to_delete);
        }
        return true;
    }

 private:
    std::atomic<Node*> top_{nullptr};
    std::atomic<Node*> to_be_deleted_{nullptr};
};


template <typename T>
using ConcurrentStack = LockFreeStack<T>;

