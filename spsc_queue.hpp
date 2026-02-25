#ifndef SPSC_QUEUE_HPP_
#define SPSC_QUEUE_HPP_

#include <atomic>
#include <cassert>

namespace hao::queue {

// wait-free single producer single consumer queue
template<typename T>
class SPSCQueue {
public:
  explicit SPSCQueue(size_t capacity = 1023)
    : capacity_(capacity + 1), data_(static_cast<T *>(operator new[](capacity_ * sizeof(T)))), head_(0), tail_(0), head_cache_(0), tail_cache_(0) {
    assert(capacity > 0);
  }

  ~SPSCQueue() {
    while (head_ != tail_) {
      data_[head_].~T();
      head_ = (head_ + 1) % capacity_;
    }
    delete[] data_;
  }

  bool push(const T& data) {
    // const auto head = head_.load(std::memory_order_acquire); // 1a
    const auto head = head_cache_;
    const auto tail = tail.load(std::memory_order_relaxed);
    if (((tail + 1) % capacity_) == head) {
      head = head_cache_ = head_.load(std::memory_order_acquire);
      if ((tail + 1) % capacity_ == head) {
        return false;
      }
    }
    new(&data_[tail]) T(data);
    tail_.store((tail + 1) % capacity_, std::memory_order_release); // 2a
    return true;
  }

  template<typename... Args>
  bool push(Args&&... args) {
    // const auto head = head_.load(std::memory_order_acquire); // 1a
    auto head = head_cache_;
    const auto tail = tail_.load(std::memory_order_relaxed);
    if (((tail + 1) % capacity_) == head) {
      head = head_cache_ = head_.load(std::memory_order_acquire);
      if ((tail + 1) % capacity_ == head) {
        return false;
      }
    }
    new(&data_[tail_]) T(std::forward<Args>(args)...);
    tail_.store((tail + 1) % capacity_, std::memory_order_release); // 2a
    return true;
  }

  bool pop(T& data) {
    const auto head = head_.load(std::memory_order_relaxed);
    // const auto tail = tail_.load(std::memory_order_acquire); // 2b
    auto tail = tail_cache_;
    if (head == tail) {
      tail = tail_cache_ = tail_.load(std::memory_order_acquire);
      if (head == tail) {
        return false;
      }
    }
    data = std::move(data_[head]);
    data_[head].~T();
    head_.store((head + 1) % capacity_, std::memory_order_release); // 1b
    return true;
  }
private:
  size_t capacity_;
  T* data_;
  static constexpr size_t hardware_destructive_interference_size = 64;
  static_assert(std::atomic<size_t>::is_always_lock_free);

  alignas(hardware_destructive_interference_size) 
  std::atomic<size_t> head_;
  alignas(hardware_destructive_interference_size) 
  std::atomic<size_t> tail_;
  
  size_t head_cache_;
  size_t tail_cache_;

  // avoid tail_ to false share with adjacent object
  char padding_[hardware_destructive_interference_size - sizeof(tail_)];
};

}

#endif // SPSC_QUEUE_HPP_