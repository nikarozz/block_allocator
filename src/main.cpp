#include "block_allocator.hpp"

#include <iostream>
#include <vector>
#include <cstring>

int main() {
  const std::size_t block_size  = 128;
  const std::size_t block_count = 16;
  const std::size_t alignment   = 64;

  mem::BlockAllocator pool( block_size, block_count, alignment );

  return 0;
}
