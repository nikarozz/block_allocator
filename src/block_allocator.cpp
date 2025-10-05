#include "block_allocator.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

namespace mem {

static void * allocate_aligned( std::size_t alignment, std::size_t size ) {
  void * p  = nullptr;
  int    rc = posix_memalign( &p, alignment, size );
  if ( rc != 0 ) {
    return nullptr;
  }
  return p;
}

std::size_t BlockAllocator::round_up( std::size_t value, std::size_t align ) noexcept {
  const std::size_t mask = align - 1;
  return ( value + mask ) & ~mask;
}

BlockAllocator::BlockAllocator( std::size_t block_size, std::size_t block_count, std::size_t alignment )
    : block_size_{ block_size }, block_count_{ block_count }, alignment_{ alignment }, stride_{ 0 }, region_{ nullptr },
      free_list_{ nullptr }, free_count_{ 0 }, occupancy_( block_count, static_cast< std::uint8_t >( 0 ) ) {
  if ( block_size_ == 0 || block_count_ == 0 ) {
    throw std::invalid_argument( "BlockAllocator: block_size and block_count must be > 0" );
  }
  if ( !is_power_of_two( alignment_ ) || alignment_ < alignof( void * ) ) {
    throw std::invalid_argument( "BlockAllocator: alignment must be a power of two and >= alignof(void*)" );
  }

  const std::size_t min_stride = std::max< std::size_t >( block_size_, sizeof( FreeNode ) );
  stride_                      = round_up( min_stride, alignment_ );

  if ( stride_ > static_cast< std::size_t >( -1 ) / block_count_ ) {
    throw std::invalid_argument( "BlockAllocator: size overflow" );
  }
  const std::size_t total_size = stride_ * block_count_;

  region_ = static_cast< std::byte * >( allocate_aligned( alignment_, total_size ) );
  if ( !region_ ) {
    throw std::bad_alloc();
  }

  free_list_ = nullptr;
  for ( std::size_t i = 0; i < block_count_; ++i ) {
    auto * node = reinterpret_cast< FreeNode * >( region_ + i * stride_ );
    node->next  = free_list_;
    free_list_  = node;
  }
  free_count_ = block_count_;
}

BlockAllocator::~BlockAllocator() noexcept {
  std::free( region_ );
  region_     = nullptr;
  free_list_  = nullptr;
  free_count_ = 0;
  occupancy_.clear();
}

void * BlockAllocator::allocate() {
  std::lock_guard< std::mutex > lock( mtx_ );
  if ( !free_list_ ) {
    throw std::bad_alloc();
  }

  FreeNode * node = free_list_;
  free_list_      = free_list_->next;
  --free_count_;

  const std::size_t idx = ( reinterpret_cast< std::byte * >( node ) - region_ ) / stride_;
  occupancy_[idx]       = 1;

  return static_cast< void * >( node );
}

void BlockAllocator::deallocate( void * p ) {
  if ( !p ) {
    return;
  }
  std::lock_guard< std::mutex > lock( mtx_ );

  if ( !is_from_region_unlocked( p ) ) {
    throw std::runtime_error( "BlockAllocator::deallocate: pointer does not belong to this allocator" );
  }

  const std::size_t idx = index_from_ptr_unlocked( p );
  if ( occupancy_[idx] == 0 ) {
    throw std::runtime_error( "BlockAllocator::deallocate: double free or corruption detected" );
  }

  auto * node = reinterpret_cast< FreeNode * >( p );
  node->next  = free_list_;
  free_list_  = node;
  --occupancy_[idx]; // becomes 0
  ++free_count_;
}

std::size_t BlockAllocator::free_blocks() const noexcept {
  std::lock_guard< std::mutex > lock( mtx_ );
  return free_count_;
}

bool BlockAllocator::is_from_region_unlocked( const void * p ) const noexcept {
  auto addr = reinterpret_cast< const std::byte * >( p );
  return addr >= region_ && addr < ( region_ + stride_ * block_count_ ) && ( ( addr - region_ ) % stride_ == 0 );
}

std::size_t BlockAllocator::index_from_ptr_unlocked( const void * p ) const {
  auto addr = reinterpret_cast< const std::byte * >( p );
  if ( addr < region_ || addr >= ( region_ + stride_ * block_count_ ) ) {
    throw std::runtime_error( "BlockAllocator: pointer out of range" );
  }
  const std::size_t diff = static_cast< std::size_t >( addr - region_ );
  if ( diff % stride_ != 0 ) {
    throw std::runtime_error( "BlockAllocator: pointer is not properly aligned to stride" );
  }
  return diff / stride_;
}

} // namespace mem
