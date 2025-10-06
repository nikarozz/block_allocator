#include "block_allocator.hpp"

#include <iostream>
#include <vector>
#include <cstring>

int main() {
  try {
    // Example
    const std::size_t block_size  = 128;
    const std::size_t block_count = 16;
    const std::size_t alignment   = 64;

    mem::BlockAllocator pool( block_size, block_count, alignment );

    std::cout << "BlockAllocator created:\n"
              << "  block_size: " << pool.block_size() << "\n"
              << "  stride:     " << pool.stride() << "\n"
              << "  blocks:     " << pool.block_count() << "\n"
              << "  alignment:  " << pool.alignment() << "\n"
              << "  capacity:   " << pool.capacity_bytes() << " bytes\n"
              << "  free:       " << pool.free_blocks() << "\n";

    // Allocate a few blocks
    std::vector< void * > ptrs;
    for ( int i = 0; i < 4; ++i ) {
      void * p = pool.allocate();
      // Write something trivial to ensure the memory is usable
      std::memset( p, 0xAB, block_size );
      ptrs.push_back( p );
    }
    std::cout << "Allocated 4 blocks. Free now: " << pool.free_blocks() << "\n";

    // Return them
    for ( void * p : ptrs ) {
      pool.deallocate( p );
    }
    std::cout << "Returned 4 blocks. Free now: " << pool.free_blocks() << "\n";

    return 0;
  } catch ( const std::exception & e ) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
