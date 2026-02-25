#include <benchmark/benchmark.h>
#include <chrono>
#include <thread>
#include <typeinfo>

#include <boost/lockfree/spsc_queue.hpp>
#include "spsc_queue.hpp"

using namespace hao::queue;

template<size_t Size>
struct Data {
  char c[Size];
};

template<template<typename> class Queue, typename T>
static void create_benchmark(benchmark::State &state) {
  int queue_size = state.range(0);

  for (auto _ : state) {
    Queue<T> queue(queue_size);
    benchmark::DoNotOptimize(queue);
  }
}

template<template<typename> class Queue, typename T>
static void push_benchmark(benchmark::State &state) {
  int queue_size = state.range(0);
  
  for (auto _ : state) {
    Queue<T> queue(queue_size);
    benchmark::DoNotOptimize(queue);
    T t;
    auto start = std::chrono::high_resolution_clock::now();
    queue.push(t);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

template<template<typename> class Queue, typename T>
static void pop_benchmark(benchmark::State &state) {
  int queue_size = state.range(0);

  for (auto _ : state) {
    Queue<T> queue(queue_size);
    T t;
    queue.push(t);
    
    auto start = std::chrono::high_resolution_clock::now();
    queue.pop(t);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }
}

template<template<typename> class Queue, typename T>
void register_single_size_benchmark() {
  const std::string queue_name = typeid(Queue<T>).name();
  benchmark::RegisterBenchmark(queue_name + " create", create_benchmark<Queue, T>)->Range(1, 1000000);
  benchmark::RegisterBenchmark(queue_name + " push", push_benchmark<Queue, T>)->Range(1, 1000)->UseManualTime();
  benchmark::RegisterBenchmark(queue_name + " pop", pop_benchmark<Queue, T>)->Range(1, 1000)->UseManualTime();
}

void register_all_benchmarks() {
  register_single_size_benchmark<hao::queue::SPSCQueue, Data<1>>();
  register_single_size_benchmark<hao::queue::SPSCQueue, Data<32>>();
  register_single_size_benchmark<hao::queue::SPSCQueue, Data<64>>();
  register_single_size_benchmark<hao::queue::SPSCQueue, Data<128>>();

  register_single_size_benchmark<boost::lockfree::spsc_queue, Data<1>>();
  register_single_size_benchmark<boost::lockfree::spsc_queue, Data<32>>();
  register_single_size_benchmark<boost::lockfree::spsc_queue, Data<64>>();
  register_single_size_benchmark<boost::lockfree::spsc_queue, Data<128>>();
}

// 程序启动时自动注册
static bool init_benchmarks = [](){
  register_all_benchmarks();
  return true;
}();

// 基准测试主函数
BENCHMARK_MAIN();