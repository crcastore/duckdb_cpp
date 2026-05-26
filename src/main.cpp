#include "duckdb_cache/duckdb_cache.hpp"

#include <cstdio>
#include <iostream>

namespace {

#define RETURN_IF_ERROR(expr)                                           \
  do {                                                                  \
    auto _r = (expr);                                                   \
    if (!_r) {                                                          \
      std::cerr << #expr << " failed: " << _r.error() << "\n";          \
      return 1;                                                         \
    }                                                                   \
  } while (0)

}  // namespace

int main() {
  using duckdb_cache::FloatTableStore;

  // In-memory database; pass a file path to persist instead.
  FloatTableStore store(":memory:");

  RETURN_IF_ERROR(store.CreateTable("measurements", {"x", "y", "z"}));
  RETURN_IF_ERROR(store.InsertRow("measurements", {0.0, 1.0, 2.0}));
  RETURN_IF_ERROR(store.InsertRows("measurements",
                                   {
                                       {1.5, 2.5, 3.5},
                                       {-1.0, 0.5, 9.25},
                                       {3.14159, 2.71828, 1.61803},
                                   }));

  auto count = store.RowCount("measurements");
  if (!count) {
    std::cerr << "RowCount failed: " << count.error() << "\n";
    return 1;
  }
  std::cout << "Row count: " << count.value() << "\n";

  auto rows = store.SelectAll("measurements");
  if (!rows) {
    std::cerr << "SelectAll failed: " << rows.error() << "\n";
    return 1;
  }
  std::cout << "Contents of `measurements`:\n";
  for (const auto& row : rows.value()) {
    for (std::size_t i = 0; i < row.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::printf("%10.5f", row[i]);
    }
    std::cout << "\n";
  }

  return 0;
}
