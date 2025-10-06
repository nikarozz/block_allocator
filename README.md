# block_allocator

A simple fixed-size block allocator for Linux, with:
- **Thread-safety** via `std::mutex`.
- **Preallocated pool**: blocks come from a contiguous region.
- **Configurable alignment** for each block.
- **Unit tests** (GoogleTest) including multithreaded and exceptional scenarios.
- **Doxygen**-documented public API.
- **CMake** build with targets for library, example, tests, and docs.
- **Clang-Format** configuration provided.

## Build (Linux)

```bash
# 1) Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2) Build library, example, and tests
cmake --build build --target allocator_example allocator_tests -j

# 3) Run tests
ctest --test-dir build -V

# 4) Generate API docs (requires doxygen)
cmake --build build --target docs
# HTML docs will be in build/docs/html/index.html
```

## Example

```bash
./build/allocator_example
```

## Public API

See Doxygen in `include/block_allocator.hpp`.

## Formatting

The `.clang-format` file is provided and uses the exact settings from the assignment.

## Notes / Decisions

- **posix_memalign** is used for aligned pool allocation because the target is Linux. This keeps the code straightforward.
- `deallocate(nullptr)` is a no-op by convention.
- An internal occupancy vector guards against **double-free** and **foreign-pointer** errors with clear exceptions.
- We keep one `std::mutex` for simplicity. If profiling ever shows contention, the design could be evolved.
- The allocator intentionally does **not** zero memory on allocation; tests that touch memory use `memset` in the test or example.

## License

Test project. No license intended.
