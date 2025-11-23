#include <gtest/gtest.h>
#include <memory>
#include <SQLiteCpp/SQLiteCpp.h>
#include "MigrationManager.h"

class MigrationManagerTest : public ::testing::Test {
protected:
  static std::shared_ptr<SQLite::Database> db;
  std::string migration_dir;
  iqrf::MigrationManager manager;

  MigrationManagerTest()
    : migration_dir(std::getenv("MIGRATIONS_DIR")),
      manager(migration_dir) {}

  static void SetUpTestSuite() {
    db = std::make_shared<SQLite::Database>(
      ":memory:",
      SQLite::OPEN_READWRITE| SQLite::OPEN_CREATE
    );
  }

  static void TearDownTestSuite() {
    db.reset();
  }
};

std::shared_ptr<SQLite::Database> MigrationManagerTest::db = nullptr;

TEST_F(MigrationManagerTest, test_construct_manager_nonexistent_path) {
  std::string expected_error = "Migration directory invalid does not exist.";
  try {
    iqrf::MigrationManager manager("invalid");
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ(expected_error.c_str(), e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}

TEST_F(MigrationManagerTest, test_migrate_empty_db) {
  SQLite::Statement stmt(*db, "SELECT name FROM sqlite_master WHERE type='table';");
  EXPECT_FALSE(stmt.executeStep());

  auto executed = manager.migrate(db);
  EXPECT_EQ(2, executed);

  EXPECT_TRUE(db->tableExists("migrations"));
  EXPECT_TRUE(db->tableExists("users"));
  EXPECT_TRUE(db->tableExists("posts"));
  EXPECT_TRUE(db->tableExists("comments"));
}
