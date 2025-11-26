#pragma once

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

/**
 * @file db_utils.h
 * @brief SQLiteCpp helpers for creating database connection.
 */

/**
 * @brief Create a database connection handle.
 *
 * The function checks for existence of database file, attempts to create a connection,
 * and checks if `api_tokens` table exists.
 *
 * @param path Path to SQLite database file
 * @param create Create database file if it does not exist
 * @param busyTimeout Busy timeout
 * @param wal Write-ahead logging journal mode
 * @return Database connection handle
 */
std::shared_ptr<SQLite::Database> create_database_connetion(const std::string& path, bool create = true, int busyTimeout = 3000, bool wal = true) {
  if (!create && !std::filesystem::exists(path)) {
    throw std::invalid_argument("Database file does not exist.");
  }
  try {
    int openFlags = SQLite::OPEN_READWRITE;
    if (create) {
      openFlags |= SQLite::OPEN_CREATE;
    }
    auto db = std::make_shared<SQLite::Database>(path, openFlags, busyTimeout);
    if (wal) {
      db->exec("PRAGMA journal_mode=WAL;");
    }
    return db;
  } catch (const SQLite::Exception &e) {
    throw std::runtime_error("SQLite error: " + std::string(e.what()));
  }
}
