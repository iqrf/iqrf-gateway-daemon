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

/**
 * Auth DB API token entity
 */
class ApiToken {
public:
  /**
   * @brief Token status
   *
   * Token can either be valid, expired, or revoked.
   */
  enum class Status : int {
    Valid,
    Expired,
    Revoked,
  };

  /**
   * Get Status enum from value
   * @param value Integer value
   * @return `Status` Status enum member
   * @throws `std::invalid_argument` If value does not exist on enum
   */
  static Status fromValue(int value) {
    switch (value) {
      case 0: return Status::Valid;
      case 1: return Status::Expired;
      case 2: return Status::Revoked;
      default: throw std::invalid_argument("Unknown status value.");
    }
  }

  /**
   * Get string representation of Status enum member
   * @param status Status enum member
   * @return `std::string_view` String representation
   */
  static std::string_view toString(Status status) {
    switch (status) {
      case Status::Valid: return "valid";
      case Status::Expired: return "expired";
      case Status::Revoked: return "revoked";
      default: return "unknown";
    }
  }

  /**
   * Base constructor
   */
  ApiToken() = default;

  /**
   * Constructor without ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status as integer
   * @param service Access to service mode
   */
  ApiToken(const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, int status, bool service)
    : owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(fromValue(status)),
      service_(service) {}

  /**
   * Constructor without ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status
   * @param service Access to service mode
   */
  ApiToken(const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, Status status, bool service)
    : owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(status),
      service_(service) {}

  /**
   * Full constructor
   * @param id Token ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status as integer
   * @param service Access to service mode
   */
  ApiToken(const uint32_t id, const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, int status, bool service)
    : id_(id),
      owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(fromValue(status)),
      service_(service) {}

  /**
   * Full constructor
   * @param id Token ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status
   * @param service Access to service mode
   */
  ApiToken(const uint32_t id, const std::string& owner, const std::string& salt, const std::string& hash,
    int64_t createdAt, int64_t expiresAt, Status status, bool service)
    : id_(id),
      owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(status),
      service_(service) {}

  uint32_t getId() const {
    return id_;
  }

  const std::string& getOwner() const {
    return owner_;
  }

  const std::string& getSalt() const {
    return salt_;
  }

  const std::string& getHash() const {
    return hash_;
  }

  int64_t getCreatedAt() const {
    return createdAt_;
  }

  int64_t getExpiresAt() const {
    return expiresAt_;
  }

  Status getStatus() const {
    return status_;
  }

  bool canUseServiceMode() const {
    return service_;
  }

  static ApiToken fromResult(SQLite::Statement& stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto owner = stmt.getColumn(1).getString();
    auto salt = stmt.getColumn(2).getString();
    auto hash = stmt.getColumn(3).getString();
    auto createdAt = stmt.getColumn(4).getInt64();
    auto expiresAt = stmt.getColumn(5).getInt64();
    auto status = stmt.getColumn(6).getInt();
    bool service = stmt.getColumn(7).getUInt() != 0;
    return ApiToken(id, owner, salt, hash, createdAt, expiresAt, status, service);
  }
private:
  /// Token ID
  uint32_t id_;
  /// Token owner
  std::string owner_;
  /// Salt
  std::string salt_;
  /// Hashed salt and secret
  std::string hash_;
  /// Created at timestamp
  int64_t createdAt_;
  /// Expiration timestmap
  int64_t expiresAt_;
  /// Token status
  Status status_;
  /// Token can use service mode
  bool service_;
};

}
