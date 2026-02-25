cmake_minimum_required(VERSION 3.14)
project(hqueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# 引入google test
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

include(GoogleTest)

add_subdirectory(third_party/benchmark-main)

find_package(Boost REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -fsanitize=address -fsanitize=null -fno-omit-frame-pointer")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX COMPILER: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

add_executable(spsc_benchmark spsc_benchmark.cpp)

target_link_libraries(spsc_benchmark 
  benchmark::benchmark
  benchmark::benchmark_main
)

add_executable(spsc_qps_benchmark spsc_qps_benchmark.cpp)

add_executable(spsc_queue_test spsc_queue_test.cpp)
add_executable(mpmc_queue_test mpmc_queue_test.cpp)
target_link_libraries(spsc_queue_test 
  GTest::gtest_main
  asan
)
target_link_libraries(mpmc_queue_test 
  GTest::gtest_main
  asan
)