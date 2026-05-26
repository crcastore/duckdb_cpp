#include "duckdb_cache/duckdb_cache.hpp"

#include <gtest/gtest.h>

#include <cmath>

using duckdb_cache::Expected;
using duckdb_cache::FloatTableStore;
using duckdb_cache::Status;

TEST(FloatTableStore, CreateAndCheckExistence) {
  FloatTableStore store;
  auto exists = store.TableExists("t");
  ASSERT_TRUE(exists);
  EXPECT_FALSE(exists.value());

  ASSERT_TRUE(store.CreateTable("t", {"a", "b"}));

  exists = store.TableExists("t");
  ASSERT_TRUE(exists);
  EXPECT_TRUE(exists.value());

  auto count = store.RowCount("t");
  ASSERT_TRUE(count);
  EXPECT_EQ(count.value(), 0u);
}

TEST(FloatTableStore, CreateTableReturnsErrorOnEmptyColumns) {
  FloatTableStore store;
  auto result = store.CreateTable("t", {});
  ASSERT_TRUE(result.has_error());
  EXPECT_NE(result.error().find("column"), std::string::npos);
  auto exists = store.TableExists("t");
  ASSERT_TRUE(exists);
  EXPECT_FALSE(exists.value());
}

TEST(FloatTableStore, CreateTableReturnsErrorOnDuplicate) {
  FloatTableStore store;
  ASSERT_TRUE(store.CreateTable("dup", {"a"}));
  auto result = store.CreateTable("dup", {"a"});
  ASSERT_TRUE(result.has_error());
  EXPECT_FALSE(result.error().empty());
}

TEST(FloatTableStore, InsertAndSelectSingleRow) {
  FloatTableStore store;
  ASSERT_TRUE(store.CreateTable("t", {"a", "b", "c"}));
  ASSERT_TRUE(store.InsertRow("t", {1.0, 2.5, -3.75}));

  auto rows = store.SelectAll("t");
  ASSERT_TRUE(rows);
  ASSERT_EQ(rows.value().size(), 1u);
  ASSERT_EQ(rows.value()[0].size(), 3u);
  EXPECT_DOUBLE_EQ(rows.value()[0][0], 1.0);
  EXPECT_DOUBLE_EQ(rows.value()[0][1], 2.5);
  EXPECT_DOUBLE_EQ(rows.value()[0][2], -3.75);
}

TEST(FloatTableStore, BulkInsertPreservesValues) {
  FloatTableStore store;
  ASSERT_TRUE(store.CreateTable("m", {"x", "y"}));

  std::vector<std::vector<double>> data;
  for (int i = 0; i < 1000; ++i) {
    data.push_back({static_cast<double>(i), std::sin(static_cast<double>(i))});
  }
  ASSERT_TRUE(store.InsertRows("m", data));

  auto count = store.RowCount("m");
  ASSERT_TRUE(count);
  EXPECT_EQ(count.value(), 1000u);

  auto rows = store.SelectAll("m");
  ASSERT_TRUE(rows);
  ASSERT_EQ(rows.value().size(), 1000u);
  for (std::size_t i = 0; i < rows.value().size(); ++i) {
    EXPECT_DOUBLE_EQ(rows.value()[i][0], static_cast<double>(i));
    EXPECT_DOUBLE_EQ(rows.value()[i][1], std::sin(static_cast<double>(i)));
  }
}

TEST(FloatTableStore, DropTable) {
  FloatTableStore store;
  ASSERT_TRUE(store.CreateTable("doomed", {"v"}));
  auto exists = store.TableExists("doomed");
  ASSERT_TRUE(exists);
  EXPECT_TRUE(exists.value());

  ASSERT_TRUE(store.DropTable("doomed"));

  exists = store.TableExists("doomed");
  ASSERT_TRUE(exists);
  EXPECT_FALSE(exists.value());

  // Idempotent.
  ASSERT_TRUE(store.DropTable("doomed"));
}

TEST(FloatTableStore, RejectsInconsistentRowWidth) {
  FloatTableStore store;
  ASSERT_TRUE(store.CreateTable("t", {"a", "b"}));
  auto result = store.InsertRows("t", {{1.0, 2.0}, {3.0}});
  ASSERT_TRUE(result.has_error());
  EXPECT_NE(result.error().find("row width"), std::string::npos);
}

TEST(FloatTableStore, InsertRowsReportsUnknownTable) {
  FloatTableStore store;
  auto result = store.InsertRows("nope", {{1.0}});
  ASSERT_TRUE(result.has_error());
  EXPECT_NE(result.error().find("InsertRows failed"), std::string::npos);
}

TEST(FloatTableStore, SelectAllReportsUnknownTable) {
  FloatTableStore store;
  auto result = store.SelectAll("nope");
  ASSERT_TRUE(result.has_error());
  EXPECT_NE(result.error().find("SelectAll failed"), std::string::npos);
}

TEST(FloatTableStore, MoveConstruction) {
  FloatTableStore a;
  ASSERT_TRUE(a.CreateTable("t", {"v"}));
  ASSERT_TRUE(a.InsertRow("t", {42.0}));

  FloatTableStore b(std::move(a));
  auto count = b.RowCount("t");
  ASSERT_TRUE(count);
  EXPECT_EQ(count.value(), 1u);
}
