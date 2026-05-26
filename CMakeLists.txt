cmake_minimum_required(VERSION 3.16)
project(duckdb_cache CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
# Sanitizers
# ----------------------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer + UBSan" ON)
option(ENABLE_TSAN "Enable ThreadSanitizer (mutually exclusive with ASAN)" OFF)

add_library(sanitizer_flags INTERFACE)
if(ENABLE_TSAN)
  target_compile_options(sanitizer_flags INTERFACE -fsanitize=thread -fno-omit-frame-pointer -g)
  target_link_options(sanitizer_flags INTERFACE -fsanitize=thread)
elseif(ENABLE_ASAN)
  target_compile_options(sanitizer_flags INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -g)
  target_link_options(sanitizer_flags INTERFACE -fsanitize=address,undefined)
endif()

# ----------------------------------------------------------------------------
# Warnings
# ----------------------------------------------------------------------------
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)

# ----------------------------------------------------------------------------
# DuckDB
# ----------------------------------------------------------------------------
include(FetchContent)

find_package(DuckDB QUIET)
if(NOT DuckDB_FOUND)
  # Try pkg-config-ish discovery via header/library
  find_path(DUCKDB_INCLUDE_DIR duckdb.hpp
    HINTS /usr/local/include /opt/homebrew/include /usr/include)
  find_library(DUCKDB_LIBRARY duckdb
    HINTS /usr/local/lib /opt/homebrew/lib /usr/lib)
endif()

if(NOT DUCKDB_INCLUDE_DIR OR NOT DUCKDB_LIBRARY)
  message(STATUS "System DuckDB not found; fetching prebuilt release...")

  if(APPLE)
    set(_duckdb_url "https://github.com/duckdb/duckdb/releases/download/v1.1.3/libduckdb-osx-universal.zip")
  elseif(UNIX)
    set(_duckdb_url "https://github.com/duckdb/duckdb/releases/download/v1.1.3/libduckdb-linux-amd64.zip")
  else()
    message(FATAL_ERROR "Unsupported platform for prebuilt DuckDB fetch")
  endif()

  FetchContent_Declare(duckdb_prebuilt URL "${_duckdb_url}")
  FetchContent_MakeAvailable(duckdb_prebuilt)

  set(DUCKDB_INCLUDE_DIR "${duckdb_prebuilt_SOURCE_DIR}")
  find_library(DUCKDB_LIBRARY duckdb
    HINTS "${duckdb_prebuilt_SOURCE_DIR}"
    NO_DEFAULT_PATH REQUIRED)
endif()

message(STATUS "DuckDB include: ${DUCKDB_INCLUDE_DIR}")
message(STATUS "DuckDB lib:     ${DUCKDB_LIBRARY}")

add_library(duckdb::duckdb UNKNOWN IMPORTED)
set_target_properties(duckdb::duckdb PROPERTIES
  IMPORTED_LOCATION "${DUCKDB_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${DUCKDB_INCLUDE_DIR}")

# ----------------------------------------------------------------------------
# Library
# ----------------------------------------------------------------------------
add_library(duckdb_cache src/duckdb_cache.cpp)
target_include_directories(duckdb_cache PUBLIC include)
target_link_libraries(duckdb_cache
  PUBLIC duckdb::duckdb
  PRIVATE project_warnings sanitizer_flags)

# ----------------------------------------------------------------------------
# Example executable
# ----------------------------------------------------------------------------
add_executable(duckdb_cache_demo src/main.cpp)
target_link_libraries(duckdb_cache_demo
  PRIVATE duckdb_cache project_warnings sanitizer_flags)

# ----------------------------------------------------------------------------
# GoogleTest
# ----------------------------------------------------------------------------
enable_testing()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(duckdb_cache_tests tests/test_duckdb_cache.cpp)
target_link_libraries(duckdb_cache_tests
  PRIVATE duckdb_cache GTest::gtest_main project_warnings sanitizer_flags)

include(GoogleTest)
gtest_discover_tests(duckdb_cache_tests)
