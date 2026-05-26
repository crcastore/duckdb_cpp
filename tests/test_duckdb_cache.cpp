#include "duckdb_cache/duckdb_cache.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

using duckdb_cache::FloatTableStore;

TEST(FloatTableStore, CreateAndCheckExistence) {
  FloatTableStore store;
  EXPECT_FALSE(store.TableExists("t"));
  store.CreateTable("t", {"a", "b"});
  EXPECT_TRUE(store.TableExists("t"));
  EXPECT_EQ(store.RowCount("t"), 0u);
}

TEST(FloatTableStore, InsertAndSelectSingleRow) {
  FloatTableStore store;
  store.CreateTable("t", {"a", "b", "c"});
  store.InsertRow("t", {1.0, 2.5, -3.75});

  auto rows = store.SelectAll("t");
  ASSERT_EQ(rows.size(), 1u);
  ASSERT_EQ(rows[0].size(), 3u);
  EXPECT_DOUBLE_EQ(rows[0][0], 1.0);
  EXPECT_DOUBLE_EQ(rows[0][1], 2.5);
  EXPECT_DOUBLE_EQ(rows[0][2], -3.75);
}

TEST(FloatTableStore, BulkInsertPreservesValues) {
  FloatTableStore store;
  store.CreateTable("m", {"x", "y"});

  std::vector<std::vector<double>> data;
  for (int i = 0; i < 1000; ++i) {
    data.push_back({static_cast<double>(i), std::sin(static_cast<double>(i))});
  }
  store.InsertRows("m", data);
  EXPECT_EQ(store.RowCount("m"), 1000u);

  auto rows = store.SelectAll("m");
  ASSERT_EQ(rows.size(), 1000u);
  for (std::size_t i = 0; i < rows.size(); ++i) {
    EXPECT_DOUBLE_EQ(rows[i][0], static_cast<double>(i));
    EXPECT_DOUBLE_EQ(rows[i][1], std::sin(static_cast<double>(i)));
  }
}

TEST(FloatTableStore, DropTable) {
  FloatTableStore store;
  store.CreateTable("doomed", {"v"});
  EXPECT_TRUE(store.TableExists("doomed"));
  store.DropTable("doomed");
  EXPECT_FALSE(store.TableExists("doomed"));
  // Idempotent.
  store.DropTable("doomed");
}

TEST(FloatTableStore, RejectsInvalidIdentifiers) {
  FloatTableStore store;
  EXPECT_THROW(store.CreateTable("bad name", {"a"}), std::invalid_argument);
  EXPECT_THROW(store.CreateTable("t", {"a;b"}), std::invalid_argument);
  EXPECT_THROW(store.CreateTable("1leading", {"a"}), std::invalid_argument);
  EXPECT_THROW(store.CreateTable("", {"a"}), std::invalid_argument);
}

TEST(FloatTableStore, RejectsInconsistentRowWidth) {
  FloatTableStore store;
  store.CreateTable("t", {"a", "b"});
  EXPECT_THROW(store.InsertRows("t", {{1.0, 2.0}, {3.0}}),
               std::invalid_argument);
}

TEST(FloatTableStore, MoveConstruction) {
  FloatTableStore a;
  a.CreateTable("t", {"v"});
  a.InsertRow("t", {42.0});

  FloatTableStore b(std::move(a));
  EXPECT_EQ(b.RowCount("t"), 1u);
}
