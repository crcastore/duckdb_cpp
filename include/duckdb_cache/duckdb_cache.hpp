#pragma once

#include "duckdb_cache/expected.hpp"

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
//
// Note: table and column names are passed straight through to SQL without
// validation or quoting. Callers are responsible for supplying trusted,
// well-formed identifiers (e.g. matching `[A-Za-z_][A-Za-z0-9_]*`). Do NOT
// pass untrusted input as a table or column name.
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
  // stored as DOUBLE.
  [[nodiscard]] Status CreateTable(const std::string& table,
                                   const std::vector<std::string>& columns);

  // Drop a table if it exists.
  [[nodiscard]] Status DropTable(const std::string& table);

  // Returns whether the named table exists.
  [[nodiscard]] Expected<bool> TableExists(const std::string& table) const;

  // Append a single row of values. Row length must match the column count.
  [[nodiscard]] Status InsertRow(const std::string& table,
                                 const std::vector<double>& row);

  // Append many rows in a single transaction. Each row must match the column
  // count.
  [[nodiscard]] Status InsertRows(
      const std::string& table,
      const std::vector<std::vector<double>>& rows);

  // Read all rows from a table, returned row-major.
  [[nodiscard]] Expected<std::vector<std::vector<double>>> SelectAll(
      const std::string& table) const;

  // Number of rows in the table.
  [[nodiscard]] Expected<std::size_t> RowCount(const std::string& table) const;

 private:
  std::unique_ptr<::duckdb::DuckDB> db_;
  std::unique_ptr<::duckdb::Connection> con_;
};

}  // namespace duckdb_cache
