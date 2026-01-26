#pragma once

#include "TestUtils.h"
#include "api_token_repo.hpp"

#include <optional>

namespace test_utils::api_token {

  /**
   * @brief Insert new token into database
   *
   * If the token already exists, do nothing.
   * Created at and expiration timestamps should be in seconds.
   *
   * @param db Database pointer
   * @param token Token to insert
   * @param createdAt Created at timestamp
   * @param expiration Expiration
   * @return `true` if statement was successful, false otherwise
   */
  inline Result insert(
    std::shared_ptr<SQLite::Database> db,
    iqrf::db::models::ApiToken& token,
    int64_t createdAt,
    int64_t expiration
  ) {
    SQLite::Statement stmt(*db,
      R"(
      INSERT OR IGNORE INTO api_tokens (id, owner, salt, hash, createdAt, expiresAt, status, service)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?);
      )"
    );

    stmt.bind(1, token.getId());
    stmt.bind(2, token.getOwner());
    stmt.bind(3, token.getSalt());
    stmt.bind(4, token.getHash());
    stmt.bind(5, createdAt);
    stmt.bind(6, expiration);
    stmt.bind(7, static_cast<int>(token.getStatus()));
    stmt.bind(8, token.canUseServiceMode());
    try {
      stmt.exec();
      return {true, std::nullopt};
    } catch (const std::exception &e) {
      return {false, e.what()};
    }
  }

  /**
   * @brief Revoke an existing token
   *
   * @param db Database pointer
   * @param id Token ID
   */
  inline Result revoke(
    std::shared_ptr<SQLite::Database> db,
    uint32_t id
  ) {
    iqrf::db::repos::ApiTokenRepository repo(db);
    try {
      repo.revoke(id);
      return {true, std::nullopt};
    } catch (const std::exception &e) {
      return {false, e.what()};
    }
  }

  /**
   * @brief Remove an existing token
   *
   * @param db Database pointer
   * @param id Token ID
   */
  inline Result remove(
    std::shared_ptr<SQLite::Database> db,
    uint32_t id
  ) {
    iqrf::db::repos::ApiTokenRepository repo(db);
    try {
      repo.remove(id);
      return {true, std::nullopt};
    } catch (const std::exception &e) {
      return {false, e.what()};
    }
  }
} // namespace api_token
