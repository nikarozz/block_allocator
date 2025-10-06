#include "block_allocator.hpp"
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <random>
#include <thread>
#include <vector>

using mem::BlockAllocator;

TEST( BlockAllocator, BasicAllocateFree ) {
  BlockAllocator alloc( 64, 32, 64 );
  EXPECT_EQ( alloc.free_blocks(), 32u );

  void * p = alloc.allocate();
  ASSERT_NE( p, nullptr );
  EXPECT_EQ( reinterpret_cast< std::uintptr_t >( p ) % alloc.alignment(), 0u );
  EXPECT_EQ( alloc.free_blocks(), 31u );

  alloc.deallocate( p );
  EXPECT_EQ( alloc.free_blocks(), 32u );
}

TEST( BlockAllocator, OutOfMemoryThrows ) {
  BlockAllocator alloc( 16, 2, 16 );
  void *         a = alloc.allocate();
  void *         b = alloc.allocate();
  EXPECT_THROW( alloc.allocate(), std::bad_alloc );
  alloc.deallocate( a );
  alloc.deallocate( b );
}

TEST( BlockAllocator, DoubleFreeThrows ) {
  BlockAllocator alloc( 32, 4, 32 );
  void *         p = alloc.allocate();
  alloc.deallocate( p );
  EXPECT_THROW( alloc.deallocate( p ), std::runtime_error );
}

TEST( BlockAllocator, ForeignPointerThrows ) {
  BlockAllocator alloc( 32, 4, 32 );
  int            x;
  EXPECT_THROW( alloc.deallocate( &x ), std::runtime_error );
}

TEST( BlockAllocator, AlignmentAndStride ) {
  const std::size_t block_size = 24;
  const std::size_t alignment  = 64;
  BlockAllocator    alloc( block_size, 8, alignment );
  EXPECT_EQ( alloc.stride() % alignment, 0u );

  void * p = alloc.allocate();
  EXPECT_EQ( reinterpret_cast< std::uintptr_t >( p ) % alignment, 0u );
  alloc.deallocate( p );
}

TEST( BlockAllocator, MultithreadedAllocFree ) {
  const std::size_t blocks = 256;
  BlockAllocator    alloc( 128, blocks, 64 );

  const int threads = 8;
  const int iters   = 2000;

  std::atomic< bool >        start{ false };
  std::vector< std::thread > ts;
  ts.reserve( threads );

  for ( int t = 0; t < threads; ++t ) {
    ts.emplace_back( [&]() {
      // Spin until start
      while ( !start.load( std::memory_order_acquire ) ) {
        std::this_thread::yield();
      }
      for ( int i = 0; i < iters; ++i ) {
        void * p = alloc.allocate();
        // Touch memory
        std::memset( p, 0xCD, 128 );
        alloc.deallocate( p );
      }
    } );
  }

  start.store( true, std::memory_order_release );
  for ( auto & th : ts )
    th.join();

  EXPECT_EQ( alloc.free_blocks(), blocks );
}

TEST( BlockAllocator, StressWithContentionAndOOM ) {
  const std::size_t blocks = 16;
  BlockAllocator    alloc( 64, blocks, 64 );

  const int threads = 8;
  const int iters   = 2000;

  std::atomic< bool > start{ false };
  std::atomic< int >  allocations{ 0 };

  std::vector< std::thread > ts;
  for ( int t = 0; t < threads; ++t ) {
    ts.emplace_back( [&]() {
      while ( !start.load( std::memory_order_acquire ) ) {
        std::this_thread::yield();
      }
      for ( int i = 0; i < iters; ++i ) {
        try {
          void * p = alloc.allocate();
          allocations.fetch_add( 1, std::memory_order_relaxed );
          // Simulate work
          std::this_thread::sleep_for( std::chrono::microseconds( 10 ) );
          alloc.deallocate( p );
        } catch ( const std::bad_alloc & ) {
          // Expected under contention when all blocks are in use.
          std::this_thread::yield();
        }
      }
    } );
  }
  start.store( true, std::memory_order_release );
  for ( auto & th : ts )
    th.join();

  EXPECT_EQ( alloc.free_blocks(), blocks );
  EXPECT_GT( allocations.load(), 0 );
