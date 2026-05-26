#include "duckdb_cache/duckdb_cache.hpp"

#include <cstdio>
#include <iostream>

int main() {
  using duckdb_cache::FloatTableStore;

  // In-memory database; pass a file path to persist instead.
  FloatTableStore store(":memory:");

  store.CreateTable("measurements", {"x", "y", "z"});

  store.InsertRow("measurements", {0.0, 1.0, 2.0});
  store.InsertRows("measurements", {
                                      {1.5, 2.5, 3.5},
                                      {-1.0, 0.5, 9.25},
                                      {3.14159, 2.71828, 1.61803},
                                  });

  std::cout << "Row count: " << store.RowCount("measurements") << "\n";

  const auto rows = store.SelectAll("measurements");
  std::cout << "Contents of `measurements`:\n";
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::printf("%10.5f", row[i]);
    }
    std::cout << "\n";
  }

  return 0;
}
