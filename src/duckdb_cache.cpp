#include "duckdb_cache/duckdb_cache.hpp"

#include <duckdb.hpp>

#include <sstream>
#include <stdexcept>

namespace duckdb_cache {

namespace {

// Validate that an identifier is safe to embed directly in a SQL statement.
// DuckDB doesn't allow parameter binding for identifiers, so we restrict
// table/column names to a conservative character set to prevent injection.
void ValidateIdentifier(const std::string& id) {
  if (id.empty()) {
    throw std::invalid_argument("identifier must not be empty");
  }
  for (char c : id) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_';
    if (!ok) {
      throw std::invalid_argument("invalid identifier: " + id);
    }
  }
  if (id[0] >= '0' && id[0] <= '9') {
    throw std::invalid_argument("identifier must not start with a digit: " + id);
  }
}

}  // namespace

FloatTableStore::FloatTableStore(const std::string& path)
    : db_(std::make_unique<::duckdb::DuckDB>(path)),
      con_(std::make_unique<::duckdb::Connection>(*db_)) {}

FloatTableStore::~FloatTableStore() = default;
FloatTableStore::FloatTableStore(FloatTableStore&&) noexcept = default;
FloatTableStore& FloatTableStore::operator=(FloatTableStore&&) noexcept = default;

void FloatTableStore::CreateTable(const std::string& table,
                                  const std::vector<std::string>& columns) {
  ValidateIdentifier(table);
  if (columns.empty()) {
    throw std::invalid_argument("at least one column is required");
  }
  std::ostringstream sql;
  sql << "CREATE TABLE " << table << " (";
  for (std::size_t i = 0; i < columns.size(); ++i) {
    ValidateIdentifier(columns[i]);
    if (i > 0) sql << ", ";
    sql << columns[i] << " DOUBLE";
  }
  sql << ");";
  auto result = con_->Query(sql.str());
  if (result->HasError()) {
    throw std::runtime_error("CreateTable failed: " + result->GetError());
  }
}

void FloatTableStore::DropTable(const std::string& table) {
  ValidateIdentifier(table);
  auto result = con_->Query("DROP TABLE IF EXISTS " + table + ";");
  if (result->HasError()) {
    throw std::runtime_error("DropTable failed: " + result->GetError());
  }
}

bool FloatTableStore::TableExists(const std::string& table) const {
  ValidateIdentifier(table);
  auto stmt = con_->Prepare(
      "SELECT COUNT(*) FROM information_schema.tables "
      "WHERE table_name = ?;");
  if (stmt->HasError()) {
    throw std::runtime_error("TableExists prepare failed: " + stmt->GetError());
  }
  auto result = stmt->Execute(table);
  if (result->HasError()) {
    throw std::runtime_error("TableExists failed: " + result->GetError());
  }
  auto chunk = result->Fetch();
  if (!chunk || chunk->size() == 0) return false;
  return chunk->GetValue(0, 0).GetValue<int64_t>() > 0;
}

void FloatTableStore::InsertRow(const std::string& table,
                                const std::vector<double>& row) {
  InsertRows(table, {row});
}

void FloatTableStore::InsertRows(
    const std::string& table,
    const std::vector<std::vector<double>>& rows) {
  ValidateIdentifier(table);
  if (rows.empty()) return;

  // Use the Appender API for efficient bulk insertion.
  ::duckdb::Appender appender(*con_, table);
  const std::size_t ncols = rows.front().size();
  for (const auto& row : rows) {
    if (row.size() != ncols) {
      throw std::invalid_argument("inconsistent row width");
    }
    appender.BeginRow();
    for (double v : row) {
      appender.Append<double>(v);
    }
    appender.EndRow();
  }
  appender.Close();
}

std::vector<std::vector<double>> FloatTableStore::SelectAll(
    const std::string& table) const {
  ValidateIdentifier(table);
  auto result = con_->Query("SELECT * FROM " + table + ";");
  if (result->HasError()) {
    throw std::runtime_error("SelectAll failed: " + result->GetError());
  }
  const std::size_t ncols = result->ColumnCount();
  std::vector<std::vector<double>> out;
  while (auto chunk = result->Fetch()) {
    const std::size_t nrows = chunk->size();
    for (std::size_t r = 0; r < nrows; ++r) {
      std::vector<double> row;
      row.reserve(ncols);
      for (std::size_t c = 0; c < ncols; ++c) {
        row.push_back(chunk->GetValue(c, r).GetValue<double>());
      }
      out.push_back(std::move(row));
    }
  }
  return out;
}

std::size_t FloatTableStore::RowCount(const std::string& table) const {
  ValidateIdentifier(table);
  auto result = con_->Query("SELECT COUNT(*) FROM " + table + ";");
  if (result->HasError()) {
    throw std::runtime_error("RowCount failed: " + result->GetError());
  }
  auto chunk = result->Fetch();
  if (!chunk || chunk->size() == 0) return 0;
  return static_cast<std::size_t>(chunk->GetValue(0, 0).GetValue<int64_t>());
}

}  // namespace duckdb_cache
