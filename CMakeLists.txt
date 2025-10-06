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

# Tests (GoogleTest via FetchContent)
if (BUILD_TESTING)
  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  )

  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(allocator_tests tests/test_allocator.cpp)
  target_link_libraries(allocator_tests PRIVATE block_allocator GTest::gtest_main)
  add_test(NAME allocator_tests COMMAND allocator_tests)
endif()

# Doxygen docs
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
  set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
else()
  message(STATUS "Doxygen not found. 'docs' target will be unavailable.")
endif()
