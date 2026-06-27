#ifndef MPSC_QUEUE_HPP_
#define MPSC_QUEUE_HPP_

#include <unistd.h>
#include <atomic>
#include <iostream>

namespace hao::queue {

// the sync between producer and producer, consumer and consumer is gurantteed by the atomic::fetch_add
template<typename T>
class MPMCQueue {
public:
  MPMCQueue(size_t capacity): capacity_(capacity + 1), head_(0), tail_(0), slots_(static_cast<slot *>(operator new[](capacity_ * sizeof(slot)))) {
    assert(capacity > 0);

    for (size_t i = 0; i < capacity_; i++) {
      slots_[i].turn_ = 0;
    }
  }

  ~MPMCQueue() {
    while (head_ != tail_) {
      slots_[idx(head_)].~slot();
      ++head_;
    }
    delete[] slots_;
  }

  void push(const T &data) {
    const auto tail = tail_.fetch_add(1, std::memory_order_acquire);
    auto &slot = slots_[idx(tail)];
    const auto t = turn(tail) * 2;
    while (t != slot.turn_.load(std::memory_order_acquire))
      ;
    
    slot.construct(data);
    slot.turn_.store(t + 1, std::memory_order_release);
  }

  bool try_push(const T &data) {
    auto tail = tail_.load(std::memory_order_acquire);
    auto &slot = slots_[idx(tail)];
    const auto t = turn(tail) * 2;
    if (t != slot.turn_.load(std::memory_order_acquire)) {
      return false;
    }
    // gurantee no other thread has used slot[tail]
    // no ABA problem because tail does't mod by capacity
    if (!tail_.compare_exchange_strong(tail, tail + 1, std::memory_order_release, std::memory_order_relaxed)) {
      return false;
    }
    slot.construct(data);
    slot.turn_.store(t + 1, std::memory_order_release);
    return true;
  }

  void pop(T &data) {
    const auto head = head_.fetch_add(1, std::memory_order_acquire);
    auto &slot = slots_[idx(head)];
    const auto t = turn(head) * 2 + 1;
    while (t != slot.turn_.load(std::memory_order_acquire))
      ;
      
    slot.move(data);
    slot.destruct();
    slot.turn_.store(t + 1, std::memory_order_release);
  }

  bool try_pop(T &data) {
    auto head = head_.load(std::memory_order_acquire);
    auto &slot = slots_[idx(head)];
    const auto t = turn(head) * 2;
    if (t + 1 != slot.turn_.load(std::memory_order_acquire)) {
      return false;
    }
    // gurantee no other thread has use slot[head]
    if (!head_.compare_exchange_strong(head, head + 1, std::memory_order_release, std::memory_order_relaxed)) {
      return false;
    }
    slot.move(data);
    slot.destruct();
    slot.turn_.store(t + 2, std::memory_order_release);
    return true;
  }

  size_t size() const {
    return tail_.load(std::memory_order_relaxed) - head_.load(std::memory_order_relaxed);
  }

  bool empty() const {
    return size() <= 0;
  }

private:
  size_t idx(size_t i) { return i % capacity_; }
  size_t turn(size_t i) { return i / capacity_; }

  struct slot {
    T data_;
    // turn is used to sync consumer and producer
    std::atomic<size_t> turn_;
    void construct(const T &data) { new(&data_) T(data); }
    void move(T &data) { data = std::move(data_); }
    void destruct() { data_.~T(); }
  };
  size_t capacity_;
  std::atomic<size_t> head_, tail_;
  slot *slots_;
};

}

#endif // MPSC_QUEUE_HPP_