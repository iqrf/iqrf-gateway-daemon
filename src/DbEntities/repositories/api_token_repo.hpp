#pragma once

#include "TimeConversion.h"
#include <chrono>
#include <vector>

#include <models/api_token.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::ApiToken;

namespace iqrf::db::repos {

/**
 * API token repository class
 */
class ApiTokenRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * Constructs a query to retrieve API token record from database using token ID
   * and returns an `ApiToken` object if the record exists.
   *
   * @param id Token ID
   * @return `nullptr` If token record does not exist
   * @return `ApiToken` Token object if record exists
   */
  std::unique_ptr<ApiToken> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, owner, salt, hash, createdAt, expiresAt, status, service, invalidatedAt
      FROM api_tokens
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, id);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<ApiToken>(ApiToken::fromResult(stmt));
  }

  /**
   * Constructs a query to retrieve all API token records from database and
   * returns a vector of `ApiToken` objects.
   *
   * Token records are processed one at a time, before fetching the next query result.
   *
   * @return `std::vector<ApiToken>` API token objects
   */
  std::vector<ApiToken> list() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, owner, salt, hash, createdAt, expiresAt, status, service, invalidatedAt
      FROM api_tokens
      ORDER BY id;
      )"
    );
    std::vector<ApiToken> vec;
    while (stmt.executeStep()) {
      vec.emplace_back(ApiToken::fromResult(stmt));
    }
    return vec;
  }

  /**
   * Constructs a query to insert API token record into database and returns
   * ID of the new token record.
   *
   * @param apiToken API token object to persist
   * @return `uint32_t` API token record iD
   */
  uint32_t insert(ApiToken& apiToken) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO api_tokens (owner, salt, hash, createdAt, expiresAt, status, service)
      VALUES (?, ?, ?, ?, ?, ?, ?);
      )"
    );
    stmt.bind(1, apiToken.getOwner());
    stmt.bind(2, apiToken.getSalt());
    stmt.bind(3, apiToken.getHash());
    stmt.bind(4, TimeConversion::getISO8601TimestampSafe(apiToken.getCreatedAt()));
    stmt.bind(5, TimeConversion::getISO8601TimestampSafe(apiToken.getExpiresAt()));
    stmt.bind(6, static_cast<int>(apiToken.getStatus()));
    stmt.bind(7, apiToken.canUseServiceMode());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new ApiToken entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * Constructs a query to insert API token record into database with explicitly specified ID
   * and returns ID of the new token record.
   *
   * This method is primarily used for testing purposes.
   *
   * @param apiToken API token object to persist
   * @return `uint32_t` API token record ID
   */
  uint32_t insertWithId(ApiToken& apiToken) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO api_tokens (id, owner, salt, hash, createdAt, expiresAt, status, service)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?);
      )"
    );
    stmt.bind(1, apiToken.getId());
    stmt.bind(2, apiToken.getOwner());
    stmt.bind(3, apiToken.getSalt());
    stmt.bind(4, apiToken.getHash());
    stmt.bind(5, TimeConversion::getISO8601TimestampSafe(apiToken.getCreatedAt()));
    stmt.bind(6, TimeConversion::getISO8601TimestampSafe(apiToken.getExpiresAt()));
    stmt.bind(7, static_cast<int>(apiToken.getStatus()));
    stmt.bind(8, apiToken.canUseServiceMode());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new ApiToken entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * Constructs a query to update API token record in database.
   *
   * The method only changes token status and date of invalidation
   * as the other properties / columns are not intended to be modified.
   *
   * @param token API token object
   */
  void update(ApiToken &token) {
    SQLite::Statement stmt(*m_db,
      R"(
        UPDATE api_tokens
        SET status = ?, invalidatedAt = ?
        WHERE id = ?;
      )"
    );
    auto invalidated_at = token.getInvalidatedAt();
    stmt.bind(1, static_cast<int>(token.getStatus()));
    stmt.bind(2, invalidated_at.has_value() ? TimeConversion::getISO8601TimestampSafe(invalidated_at.value()) : nullptr);
    stmt.bind(3, token.getId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to update ApiToken entity ID " + std::to_string(token.getId()),
          e.what()
        )
      );
    }
  }

  /**
   * Constructs a query to revoke API token record in database with a timestamp.
   *
   * @param id Token ID
   * @param now Revocation timestamp
   */
  bool revoke(uint32_t id, const std::chrono::system_clock::time_point &now) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE api_tokens
      SET status = ?, invalidatedAt = ?
      WHERE id = ?;
      )"
    );
    stmt.bind(1, static_cast<int>(ApiToken::Status::Revoked));
    stmt.bind(2, TimeConversion::getISO8601TimestampSafe(now));
    stmt.bind(3, id);
    try {
      auto changes = stmt.exec();
      return changes != 0;
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to revoke api token ID " + std::to_string(id),
          e.what()
        )
      );
    }
  }

  /**
   * Constructs a query to remove API token record from database.
   *
   * This method is currently only intended for testing purposes.
   *
   * @param id Record ID
   */
  void remove(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM api_tokens
      WHERE id = ?;
      )"
    );
    stmt.bind(1, id);
    stmt.exec();
  }
};

}
