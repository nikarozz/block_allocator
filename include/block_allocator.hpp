#pragma once
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <vector>
/**
 * @file block_allocator.hpp
 * @brief Fixed-size block memory allocator operating on a preallocated pool.
 *
 * This module provides a simple thread-safe block allocator that manages a pool of
 * fixed-size blocks within a single contiguous region. It is intended as a practical
 * building block for systems where predictable allocation behavior is required.
 *
 * Design notes:
 *  - Thread-safety: guarded by a single std::mutex. Simplicity > lock-free cleverness.
 *  - Each allocated block start is aligned to the user-specified alignment.
 *  - A free-list is embedded in the blocks themselves.
 *  - For safety, a small occupancy bitmap prevents double-free and invalid deallocation.
 *
 * @copyright
 * No license. See README.md for details.
 */
namespace mem {
/**
 * @class BlockAllocator
 * @brief Simple fixed-size block allocator with alignment and thread-safety.
 *
 * Blocks are carved from a single pre-allocated, aligned region. Allocation and
 * deallocation are O(1) (pop/push from a singly-linked free-list). The allocator
 * is thread-safe via a single internal mutex.
 *
 * @note All methods are safe to call from multiple threads concurrently.
 */
class BlockAllocator final {
public:
  /**
   * @brief Construct a block allocator.
   * @param block_size The requested size (in bytes) for each block (payload).
   * @param block_count Number of blocks to reserve in the pool.
   * @param alignment Desired alignment (power of two, >= alignof(void*)). Every block start will satisfy this.
   *
   * @throw std::invalid_argument if parameters are invalid, sizes overflow, or alignment is not a power of two / too small.
   * @throw std::bad_alloc if the underlying memory region cannot be allocated.
   */
  BlockAllocator( std::size_t block_size, std::size_t block_count, std::size_t alignment );

  /// Non-copyable / non-movable by design.
  BlockAllocator( const BlockAllocator & )             = delete;
  BlockAllocator & operator=( const BlockAllocator & ) = delete;
  BlockAllocator( BlockAllocator && )                  = delete;
  BlockAllocator & operator=( BlockAllocator && )      = delete;

  /// Destructor frees the underlying region.
  ~BlockAllocator() noexcept;

  /**
   * @brief Allocate one block.
   * @return Pointer to a block of size() bytes, aligned to alignment().
   * @throw std::bad_alloc if no blocks are available.
   */
  void * allocate();

  /**
   * @brief Return a previously allocated block to the pool.
   * @param p Pointer previously obtained from allocate() of this allocator. nullptr is ignored.
   * @throw std::runtime_error if @p p does not belong to this allocator, is misaligned, or was already freed.
   */
  void deallocate( void * p );

  /// @return Requested payload size in bytes (before internal rounding).
  std::size_t block_size() const noexcept { return block_size_; }

  /// @return Number of blocks in the pool.
  std::size_t block_count() const noexcept { return block_count_; }

  /// @return Alignment (in bytes) guaranteed for each block.
  std::size_t alignment() const noexcept { return alignment_; }

  /// @return Actual stride in bytes (internal rounded block size).
  std::size_t stride() const noexcept { return stride_; }

  /// @return Total capacity of the region in bytes.
  std::size_t capacity_bytes() const noexcept { return stride_ * block_count_; }

  /// @return Number of currently free blocks.
  std::size_t free_blocks() const noexcept;

private:
  struct FreeNode {
    FreeNode * next;
  };

  std::size_t block_size_;
  std::size_t block_count_;
  std::size_t alignment_;
  std::size_t stride_;

  std::byte * region_;     // base of the pool
  FreeNode *  free_list_;  // head of embedded free-list
  std::size_t free_count_; // number of free blocks

  std::vector< std::uint8_t > occupancy_; // 0 = free, 1 = allocated (guard against double-free)

  std::mutex mtx_;

  static constexpr bool is_power_of_two( std::size_t x ) noexcept { return x && ( ( x & ( x - 1 ) ) == 0 ); }

  static std::size_t round_up( std::size_t value, std::size_t align ) noexcept;

  bool        is_from_region_unlocked( const void * p ) const noexcept;
  std::size_t index_from_ptr_unlocked( const void * p ) const; // throws std::runtime_error on invalid
};
} // namespace mem
