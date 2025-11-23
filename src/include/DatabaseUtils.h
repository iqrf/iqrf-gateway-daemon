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
 * @return Database connection handle
 */
std::shared_ptr<SQLite::Database> create_database_connetion(const std::string& path) {
  if (!std::filesystem::exists(path)) {
    throw std::invalid_argument("Database file does not exist.");
  }
  try {
    auto db = std::make_shared<SQLite::Database>(path, SQLite::OPEN_READWRITE);
    db->setBusyTimeout(3000);
    return db;
  } catch (const SQLite::Exception &e) {
    throw std::runtime_error("SQLite error: " + std::string(e.what()));
  }
}
