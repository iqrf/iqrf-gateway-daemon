/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <cstdint>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

class ApiToken {
public:
  /**
   * Base constructor
   */
  ApiToken() = default;

  /**
   * Constructor without ID
   */
  ApiToken(const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, bool revoked, bool service)
    : owner(owner),
      salt(salt),
      hash(hash),
      createdAt(createdAt),
      expiresAt(expiresAt),
      revoked(revoked),
      service(service) {}

  /**
   * Full constructor
   */
  ApiToken(const uint32_t id, const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, bool revoked, bool service)
    : id(id),
      owner(owner),
      salt(salt),
      hash(hash),
      createdAt(createdAt),
      expiresAt(expiresAt),
      revoked(revoked),
      service(service) {}

  uint32_t getId() const {
    return id;
  }

  const std::string& getOwner() const {
    return owner;
  }

  const std::string& getSalt() const {
    return salt;
  }

  const std::string& getHash() const {
    return hash;
  }

  int64_t getCreatedAt() const {
    return createdAt;
  }

  int64_t getExpiresAt() const {
    return expiresAt;
  }

  bool isRevoked() const {
    return revoked;
  }

  bool canUseServiceMode() const {
    return service;
  }

  static ApiToken fromResult(SQLite::Statement& stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto owner = stmt.getColumn(1).getString();
    auto salt = stmt.getColumn(2).getString();
    auto hash = stmt.getColumn(3).getString();
    auto createdAt = stmt.getColumn(4).getInt64();
    auto expiresAt = stmt.getColumn(5).getInt64();
    bool revoked = stmt.getColumn(6).getUInt() != 0;
    bool service = stmt.getColumn(7).getUInt() != 0;
    return ApiToken(id, owner, salt, hash, createdAt, expiresAt, revoked, service);
  }
private:
  /// Token ID
  uint32_t id;
  /// Token owner
  std::string owner;
  /// Salt
  std::string salt;
  /// Hashed salt and secret
  std::string hash;
  /// Created at timestamp
  int64_t createdAt;
  /// Expiration timestmap
  int64_t expiresAt;
  /// Token revoked
  bool revoked;
  /// Token can use service mode
  bool service;
};

}
