#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include "spsc_queue.hpp"

TEST(SPSC_QUEUE_TEST, TEST1) {
  size_t size = 1000;
  hao::queue::SPSCQueue<int> queue(size);
  std::vector<int> consumer_values(size, -1);

  std::thread producer([&]() {
    for (int i = 0; i < 1000; i++) {
      while (!queue.push(i));
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < 1000; i++) {
      while (!queue.pop(consumer_values[i]));
    }
  });

  producer.join();
  consumer.join();

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(i, consumer_values[i]);
  }
}

TEST(SPSC_QUEUE_TEST, TEST2) {
  size_t data_size = 1000000;
  size_t queue_size = 1000;
  hao::queue::SPSCQueue<int> queue(queue_size);
  std::vector<int> consumer_values(data_size, -1);

  std::thread producer([&]() {
    for (size_t i = 0; i < data_size; i++) {
      while (!queue.push(i));
    }
  });

  std::thread consumer([&]() {
    for (size_t i = 0; i < data_size; i++) {
      while (!queue.pop(consumer_values[i]));
    }
  });

  producer.join();
  consumer.join();

  for (size_t i = 0; i < data_size; i++) {
    ASSERT_EQ(i, consumer_values[i]);
  }
}