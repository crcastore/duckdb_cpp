#pragma once

#include <memory>
#include <string>
#include <vector>

namespace duckdb {
class DuckDB;
class Connection;
}  // namespace duckdb

namespace duckdb_cache {

// A simple wrapper around DuckDB for storing and retrieving tables of
// floating-point data. Each row consists of one or more `double` columns.
class FloatTableStore {
 public:
  // Open (or create) a DuckDB database at the given path. Use ":memory:" for
  // an in-memory database.
  explicit FloatTableStore(const std::string& path = ":memory:");
  ~FloatTableStore();

  FloatTableStore(const FloatTableStore&) = delete;
  FloatTableStore& operator=(const FloatTableStore&) = delete;
  FloatTableStore(FloatTableStore&&) noexcept;
  FloatTableStore& operator=(FloatTableStore&&) noexcept;

  // Create a table with the given name and column names. All columns are
  // stored as DOUBLE. Throws std::runtime_error on failure.
  void CreateTable(const std::string& table,
                   const std::vector<std::string>& columns);

  // Drop a table if it exists.
  void DropTable(const std::string& table);

  // Returns true if the named table exists.
  bool TableExists(const std::string& table) const;

  // Append a single row of values. Row length must match the column count.
  void InsertRow(const std::string& table, const std::vector<double>& row);

  // Append many rows in a single transaction. Each row must match the column
  // count.
  void InsertRows(const std::string& table,
                  const std::vector<std::vector<double>>& rows);

  // Read all rows from a table, returned row-major.
  std::vector<std::vector<double>> SelectAll(const std::string& table) const;

  // Number of rows in the table.
  std::size_t RowCount(const std::string& table) const;

 private:
  std::unique_ptr<::duckdb::DuckDB> db_;
  std::unique_ptr<::duckdb::Connection> con_;
};

}  // namespace duckdb_cache
