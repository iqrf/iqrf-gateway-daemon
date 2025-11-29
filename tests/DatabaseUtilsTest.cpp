#include <filesystem>

#include <gtest/gtest.h>
#include <DatabaseUtils.h>
#include "sqlite3.h"

class DatabaseUtilsTest : public ::testing::Test {
protected:
  std::string dbPath;

  DatabaseUtilsTest() : dbPath(std::string(std::getenv("TESTS_DATA_DIR")) + "database.db") {}

  void TearDown() {
    std::filesystem::remove(dbPath);
  }
};

TEST_F(DatabaseUtilsTest, create_database_connection_no_create_nonexistent) {
  try {
    create_database_connetion("nonexistent_path_to_db.db", false);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Database file does not exist.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception was thrown.";
  }
}

TEST_F(DatabaseUtilsTest, create_database_connection_with_defaults) {
  auto db = create_database_connetion(dbPath);

  SQLite::Statement journalPragma(*db, "PRAGMA journal_mode;");
  EXPECT_TRUE(journalPragma.executeStep());
  EXPECT_STREQ("delete", journalPragma.getColumn(0).getText());

  SQLite::Statement syncPragma(*db, "PRAGMA synchronous;");
  EXPECT_TRUE(syncPragma.executeStep());
  EXPECT_EQ(2, syncPragma.getColumn(0).getInt());

  SQLite::Statement busyTimeoutPragma(*db, "PRAGMA busy_timeout;");
  EXPECT_TRUE(busyTimeoutPragma.executeStep());
  EXPECT_EQ(3000, busyTimeoutPragma.getColumn(0).getInt());
}

TEST_F(DatabaseUtilsTest, create_database_connetion_wal_mode) {
  auto db = create_database_connetion(dbPath, true, 500, true);

  SQLite::Statement journalPragma(*db, "PRAGMA journal_mode;");
  EXPECT_TRUE(journalPragma.executeStep());
  EXPECT_STREQ("wal", journalPragma.getColumn(0).getText());

  SQLite::Statement syncPragma(*db, "PRAGMA synchronous;");
  EXPECT_TRUE(syncPragma.executeStep());
  EXPECT_EQ(2, syncPragma.getColumn(0).getInt());

  SQLite::Statement busyTimeoutPragma(*db, "PRAGMA busy_timeout;");
  EXPECT_TRUE(busyTimeoutPragma.executeStep());
  EXPECT_EQ(500, busyTimeoutPragma.getColumn(0).getInt());
}
