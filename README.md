# duckdb_cache

A small C++17 wrapper around [DuckDB](https://duckdb.org/) for storing tables of
floating-point data. Includes GoogleTest unit tests and ASan/UBSan enabled by
default.

## Layout

```
include/duckdb_cache/duckdb_cache.hpp   public API
src/duckdb_cache.cpp                    implementation
src/main.cpp                            demo executable
tests/test_duckdb_cache.cpp             gtest unit tests
CMakeLists.txt                          build + GTest fetch + sanitizers
```

## Build & run

```sh
cmake -S . -B build
cmake --build build -j
./build/duckdb_cache_demo
ctest --test-dir build --output-on-failure
```

## Sanitizers

ASan + UBSan are enabled by default. To switch to ThreadSanitizer or disable
sanitizers entirely:

```sh
cmake -S . -B build -DENABLE_ASAN=OFF                  # no sanitizers
cmake -S . -B build -DENABLE_ASAN=OFF -DENABLE_TSAN=ON  # TSan
```

## DuckDB

The build first looks for a system-installed DuckDB (e.g. `brew install duckdb`
which provides `libduckdb`). If not found, it downloads the official prebuilt
release archive.
