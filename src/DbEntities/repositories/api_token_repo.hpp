#pragma once

#include <vector>

#include <models/api_token.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::ApiToken;

namespace iqrf::db::repos {

class ApiTokenRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  std::unique_ptr<ApiToken> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, owner, salt, hash, createdAt, expiresAt, status, service
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

  std::vector<ApiToken> list() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, owner, salt, hash, createdAt, expiresAt, status, service
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
    stmt.bind(4, apiToken.getCreatedAt());
    stmt.bind(5, apiToken.getExpiresAt());
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
    stmt.bind(5, apiToken.getCreatedAt());
    stmt.bind(6, apiToken.getExpiresAt());
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

  void update(ApiToken &token) {
    SQLite::Statement stmt(*m_db,
      R"(
        UPDATE api_tokens
        SET status = ?
        WHERE id = ?;
      )"
    );
    stmt.bind(1, static_cast<int>(token.getStatus()));
    stmt.bind(2, token.getId());
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

  bool revoke(uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE api_tokens
      SET status = ?
      WHERE id = ?;
      )"
    );
    stmt.bind(1, static_cast<int>(ApiToken::Status::Revoked));
    stmt.bind(2, id);
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
   * @brief Removes existing api token record by ID
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
