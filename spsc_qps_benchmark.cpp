#include <thread>
#include <chrono>
#include <iostream>
#include "spsc_queue.hpp"

using namespace hao::queue;

template<size_t Size>
struct Data {
  constexpr static size_t size = Size;
  char c[Size];
};

void pinThread(int cpu) {
  if (cpu < 0) {
    return;
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
      -1) {
    perror("pthread_setaffinity_no");
    exit(1);
  }
}

template<typename T>
void spsc_queue_qps_benchmark(int queue_size, int d) {
  std::cout << "queue size: " << queue_size << ", Data<" << T::size << ">" << std::endl;
  const std::chrono::seconds duration(d);
  SPSCQueue<T> queue(queue_size);
  size_t push_count = 0, pop_count = 0;
  size_t push_fail = 0, pop_fail = 0;

  std::thread producer([&]() {
    pinThread(0);
    auto start_time = std::chrono::high_resolution_clock::now();
    T t;
    while (std::chrono::high_resolution_clock::now() - start_time < duration) {
      bool res = queue.push(t);
      if (res) {
        push_count++;
      } else {
        push_fail++;
      }
    }
  });

  std::thread consumer([&]() {
    pinThread(1);
    auto start_time = std::chrono::high_resolution_clock::now();
    T t;
    while (std::chrono::high_resolution_clock::now() - start_time < duration) {
      bool res = queue.pop(t);
      if (res) {
        pop_count++;
      } else {
        pop_fail++;
      }
    }
  });

  producer.join();
  consumer.join();

  std::cout << "duration: " << d << "s" << std::endl;
  std::cout << "push success op total count: " << push_count << std::endl;
  std::cout << "pop success op total count: " << pop_count << std::endl;
  std::cout << "push fail op total count: " << push_fail << std::endl;
  std::cout << "pop fail op total count: " << pop_fail << std::endl;
  std::cout << "push qps: " << push_count / duration.count() << std::endl;
  std::cout << "pop qps: " << pop_count / duration.count() << std::endl;
}

int main() {
  for (int i = 1; i <= 10000000; i *= 100) {
    spsc_queue_qps_benchmark<Data<1>>(i, 1);
    spsc_queue_qps_benchmark<Data<8>>(i, 1);
    spsc_queue_qps_benchmark<Data<16>>(i, 1);
    spsc_queue_qps_benchmark<Data<32>>(i, 1);
    spsc_queue_qps_benchmark<Data<64>>(i, 1);
  }
}