#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace duckdb_cache {

// A lightweight result type holding either a value of type T or an error
// message. Move-only.
template <typename T>
class Expected {
 public:
  Expected(T value) : value_(std::move(value)) {}

  static Expected error(std::string message) {
    Expected result;
    result.error_ = std::move(message);
    return result;
  }

  Expected(Expected&&) = default;
  Expected& operator=(Expected&&) = default;

  Expected(const Expected&) = delete;
  Expected& operator=(const Expected&) = delete;

  explicit operator bool() const noexcept { return has_value(); }

  bool has_value() const noexcept { return value_.has_value(); }
  bool has_error() const noexcept { return !has_value(); }

  T& value() noexcept {
    assert(value_);
    return *value_;
  }

  const T& value() const noexcept {
    assert(value_);
    return *value_;
  }

  T&& move_value() noexcept {
    assert(value_);
    return std::move(*value_);
  }

  const std::string& error() const noexcept {
    assert(!value_);
    return error_;
  }

 private:
  Expected() = default;

  std::optional<T> value_;
  std::string error_;
};

// Alias used for operations that succeed without producing a value.
using Status = Expected<std::monostate>;

inline Status Ok() { return Status(std::monostate{}); }

}  // namespace duckdb_cache
