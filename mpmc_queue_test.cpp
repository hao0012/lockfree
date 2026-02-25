#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include "mpmc_queue.hpp"

TEST(MPMC_QUEUE_TEST, TEST0) {
  size_t data_size = 1000;
  size_t queue_size = 1000;

  hao::queue::MPMCQueue<int> queue(queue_size);
  std::vector<int> consumer_values(data_size, -1);

  for (size_t i = 0; i < data_size; i++) {
    bool res = queue.try_push(i);
    ASSERT_TRUE(res);
  }

  for (size_t i = 0; i < data_size; i++) {
    int j;
    bool res = queue.try_pop(j);
    ASSERT_TRUE(res);
    ASSERT_EQ(i, j);
  }
}

TEST(MPMC_QUEUE_TEST, MULTI_CONSUMER_TEST) {
  size_t data_size = 1000;
  size_t queue_size = 1000;
  size_t consumer_size = 10;

  hao::queue::MPMCQueue<int> queue(queue_size);
  std::vector<int> consumer_values(data_size, -1);

  for (size_t i = 0; i < data_size; i++) {
    bool res = queue.try_push(i);
    ASSERT_TRUE(res);
  }

  std::mutex m;
  std::unordered_set<int> res_set;

  auto consume = [&]() {
    int j;
    for (size_t i = 0; i < data_size; i++) {
      while (!queue.empty()) {
        bool res = queue.try_pop(j);
        if (res) {
          std::scoped_lock guard(m);
          res_set.insert(j);
        }
      }
    }
  };

  std::vector<std::thread> consumers(consumer_size);
  for (auto &t : consumers) {
    t = std::thread(consume);
  }

  for (auto &t : consumers) {
    t.join();
  }

  for (size_t i = 0; i < data_size; i++) {
    ASSERT_TRUE(res_set.find(i) != res_set.end());
  }
}

TEST(MPMC_QUEUE_TEST, MULTI_PRODUCER_TEST) {
  size_t data_size = 1000;
  size_t queue_size = 1000;
  size_t producer_size = 10;

  hao::queue::MPMCQueue<int> queue(queue_size);
  
  auto produce = [&]() {
    for (size_t i = 0; i < data_size; i++) {
      while (!queue.try_push(i))
        ;
    }
  };

  std::vector<std::thread> producers(producer_size);
  for (auto & t : producers) {
    t = std::thread(produce);
  }

  std::unordered_map<int, int> res_count_map;

  int j;
  for (size_t i = 0; i < data_size * producer_size; i++) {
    while (!queue.try_pop(j))
      ;
    res_count_map[j]++;
  }
  for (size_t i = 0; i < data_size; i++) {
    ASSERT_EQ(producer_size, res_count_map[i]);
  }
  for (auto &t : producers) {
    t.join();
  }
}

TEST(MPMC_QUEUE_TEST, CONCURRENT_TEST0) {
  size_t data_size = 1000;
  size_t queue_size = 1000;

  size_t consumer_size = 5;
  size_t producer_size = 6;

  hao::queue::MPMCQueue<int> queue(queue_size);
  std::atomic<size_t> producer_finish_count{0};
  auto produce = [&]() {
    for (size_t i = 0; i < data_size; i++) {
      while (!queue.try_push(i))
        ;
    }
    producer_finish_count++;
  };

  std::mutex m;
  std::unordered_map<int, int> res_count_map;

  auto consume = [&]() {
    int j;
    while (producer_finish_count < producer_size || !queue.empty()) {
      if (queue.try_pop(j)) {
        std::scoped_lock guard(m);
        res_count_map[j]++;
      }
    }
  };

  std::vector<std::thread> producers(producer_size);
  std::vector<std::thread> consumers(consumer_size);

  for (auto & t : producers) {
    t = std::thread(produce);
  }

  for (auto & t : consumers) {
    t = std::thread(consume);
  }

  for (auto &t : producers) {
    t.join();
  }
  for (auto &t: consumers) {
    t.join();
  }

  for (size_t i = 0; i < data_size; i++) {
    ASSERT_EQ(producer_size, res_count_map[i]);
  }
  
}