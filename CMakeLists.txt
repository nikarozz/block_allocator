cmake_minimum_required(VERSION 3.16)
project(BlockAllocator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# Options
option(BUILD_TESTING "Build tests" ON)

# Warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast)
endif()

find_package(Threads REQUIRED)

# Library
add_library(block_allocator
  src/block_allocator.cpp
)
target_include_directories(block_allocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(block_allocator PUBLIC Threads::Threads)

# Example executable
add_executable(allocator_example src/main.cpp)
target_link_libraries(allocator_example PRIVATE block_allocator)
