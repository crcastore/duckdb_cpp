#include "duckdb_cache/duckdb_cache.hpp"

#include <duckdb.hpp>

#include <exception>
#include <sstream>

namespace duckdb_cache {

FloatTableStore::FloatTableStore(const std::string& path)
    : db_(std::make_unique<::duckdb::DuckDB>(path)),
      con_(std::make_unique<::duckdb::Connection>(*db_)) {}

FloatTableStore::~FloatTableStore() = default;
FloatTableStore::FloatTableStore(FloatTableStore&&) noexcept = default;
FloatTableStore& FloatTableStore::operator=(FloatTableStore&&) noexcept = default;

Status FloatTableStore::CreateTable(const std::string& table,
                                    const std::vector<std::string>& columns) {
  if (columns.empty()) {
    return Status::error("at least one column is required");
  }
  std::ostringstream sql;
  sql << "CREATE TABLE " << table << " (";
  for (std::size_t i = 0; i < columns.size(); ++i) {
    if (i > 0) sql << ", ";
    sql << columns[i] << " DOUBLE";
  }
  sql << ");";
  auto result = con_->Query(sql.str());
  if (result->HasError()) {
    return Status::error("CreateTable failed: " + result->GetError());
  }
  return Ok();
}

Status FloatTableStore::DropTable(const std::string& table) {
  auto result = con_->Query("DROP TABLE IF EXISTS " + table + ";");
  if (result->HasError()) {
    return Status::error("DropTable failed: " + result->GetError());
  }
  return Ok();
}

Expected<bool> FloatTableStore::TableExists(const std::string& table) const {
  auto stmt = con_->Prepare(
      "SELECT COUNT(*) FROM information_schema.tables "
      "WHERE table_name = ?;");
  if (stmt->HasError()) {
    return Expected<bool>::error("TableExists prepare failed: " +
                                 stmt->GetError());
  }
  auto result = stmt->Execute(table);
  if (result->HasError()) {
    return Expected<bool>::error("TableExists failed: " + result->GetError());
  }
  auto chunk = result->Fetch();
  if (!chunk || chunk->size() == 0) return false;
  return chunk->GetValue(0, 0).GetValue<int64_t>() > 0;
}

Status FloatTableStore::InsertRow(const std::string& table,
                                  const std::vector<double>& row) {
  return InsertRows(table, {row});
}

Status FloatTableStore::InsertRows(
    const std::string& table,
    const std::vector<std::vector<double>>& rows) {
  if (rows.empty()) return Ok();

  const std::size_t ncols = rows.front().size();
  for (const auto& row : rows) {
    if (row.size() != ncols) {
      return Status::error("inconsistent row width");
    }
  }

  // DuckDB's Appender throws on errors (unknown table, type mismatch, etc.);
  // translate those into the Expected error channel.
  try {
    ::duckdb::Appender appender(*con_, table);
    for (const auto& row : rows) {
      appender.BeginRow();
      for (double v : row) {
        appender.Append<double>(v);
      }
      appender.EndRow();
    }
    appender.Close();
  } catch (const std::exception& e) {
    return Status::error(std::string("InsertRows failed: ") + e.what());
  }
  return Ok();
}

Expected<std::vector<std::vector<double>>> FloatTableStore::SelectAll(
    const std::string& table) const {
  using ResultT = std::vector<std::vector<double>>;
  auto result = con_->Query("SELECT * FROM " + table + ";");
  if (result->HasError()) {
    return Expected<ResultT>::error("SelectAll failed: " + result->GetError());
  }
  const std::size_t ncols = result->ColumnCount();
  ResultT out;
  try {
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
  } catch (const std::exception& e) {
    return Expected<ResultT>::error(std::string("SelectAll failed: ") +
                                    e.what());
  }
  return out;
}

Expected<std::size_t> FloatTableStore::RowCount(
    const std::string& table) const {
  auto result = con_->Query("SELECT COUNT(*) FROM " + table + ";");
  if (result->HasError()) {
    return Expected<std::size_t>::error("RowCount failed: " +
                                        result->GetError());
  }
  auto chunk = result->Fetch();
  if (!chunk || chunk->size() == 0) return std::size_t{0};
  return static_cast<std::size_t>(chunk->GetValue(0, 0).GetValue<int64_t>());
}

}  // namespace duckdb_cache
