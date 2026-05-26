#include "duckdb_cache/expected.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

using duckdb_cache::Expected;
using duckdb_cache::Ok;
using duckdb_cache::Status;

// ---- Compile-time guarantees ------------------------------------------------

static_assert(!std::is_copy_constructible_v<Expected<int>>,
              "Expected must be move-only");
static_assert(!std::is_copy_assignable_v<Expected<int>>,
              "Expected must be move-only");
static_assert(std::is_move_constructible_v<Expected<int>>,
              "Expected must be move-constructible");
static_assert(std::is_move_assignable_v<Expected<int>>,
              "Expected must be move-assignable");
// Works with non-default-constructible / non-copyable payloads.
static_assert(std::is_move_constructible_v<Expected<std::unique_ptr<int>>>);

// ---- Construction -----------------------------------------------------------

TEST(Expected, ValueConstruction) {
  Expected<int> e(7);
  EXPECT_TRUE(e);
  EXPECT_TRUE(e.has_value());
  EXPECT_FALSE(e.has_error());
  EXPECT_EQ(e.value(), 7);
}

TEST(Expected, ErrorConstruction) {
  auto e = Expected<int>::error("boom");
  EXPECT_FALSE(e);
  EXPECT_FALSE(e.has_value());
  EXPECT_TRUE(e.has_error());
  EXPECT_EQ(e.error(), "boom");
}

// ---- Move semantics ---------------------------------------------------------

TEST(Expected, MoveConstructPreservesValue) {
  Expected<std::string> src(std::string("hello"));
  Expected<std::string> dst(std::move(src));
  ASSERT_TRUE(dst);
  EXPECT_EQ(dst.value(), "hello");
}

TEST(Expected, MoveConstructPreservesError) {
  auto src = Expected<int>::error("nope");
  Expected<int> dst(std::move(src));
  ASSERT_TRUE(dst.has_error());
  EXPECT_EQ(dst.error(), "nope");
}

TEST(Expected, MoveAssignPreservesValue) {
  Expected<std::string> src(std::string("payload"));
  Expected<std::string> dst = Expected<std::string>::error("placeholder");
  dst = std::move(src);
  ASSERT_TRUE(dst);
  EXPECT_EQ(dst.value(), "payload");
}

TEST(Expected, MoveAssignPreservesError) {
  auto src = Expected<int>::error("bad");
  Expected<int> dst(0);
  dst = std::move(src);
  ASSERT_TRUE(dst.has_error());
  EXPECT_EQ(dst.error(), "bad");
}

TEST(Expected, MoveValueTransfersOwnership) {
  Expected<std::unique_ptr<int>> e(std::make_unique<int>(42));
  ASSERT_TRUE(e);
  std::unique_ptr<int> p = e.move_value();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
}

// ---- Const correctness ------------------------------------------------------

TEST(Expected, ConstAccessors) {
  const Expected<int> ok(11);
  EXPECT_TRUE(ok);
  EXPECT_EQ(ok.value(), 11);

  const auto err = Expected<int>::error("oops");
  EXPECT_FALSE(err);
  EXPECT_EQ(err.error(), "oops");
}

// ---- Status / Ok ------------------------------------------------------------

TEST(Status, OkIsTruthy) {
  Status s = Ok();
  EXPECT_TRUE(s);
  EXPECT_TRUE(s.has_value());
  EXPECT_FALSE(s.has_error());
}

TEST(Status, ErrorCarriesMessage) {
  Status s = Status::error("explain");
  EXPECT_FALSE(s);
  EXPECT_TRUE(s.has_error());
  EXPECT_EQ(s.error(), "explain");
}

TEST(Status, MoveAssignAcrossStates) {
  Status s = Ok();
  s = Status::error("now bad");
  ASSERT_TRUE(s.has_error());
  EXPECT_EQ(s.error(), "now bad");

  s = Ok();
  EXPECT_TRUE(s);
}
