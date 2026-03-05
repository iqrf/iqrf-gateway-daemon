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

#include "DatetimeParser.h"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

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
   * Constructor without ID, and with integer status
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status as integer
   * @param service Access to service mode
   * @param invalidatedAt Timestamp of invalidation
   */
  ApiToken(
    const std::string& owner,
    const std::string& salt,
    const std::string& hash,
    std::chrono::system_clock::time_point createdAt,
    std::chrono::system_clock::time_point expiresAt,
    int status,
    bool service,
    std::optional<std::chrono::system_clock::time_point> invalidatedAt
  )
    : owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(fromValue(status)),
      service_(service),
      invalidatedAt_(invalidatedAt) {}

  /**
   * Constructor without ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status
   * @param service Access to service mode
   * @param invalidatedAt Timestamp of invalidation
   */
  ApiToken(const std::string& owner,
    const std::string& salt,
    const std::string& hash,
    std::chrono::system_clock::time_point createdAt,
    std::chrono::system_clock::time_point expiresAt,
    Status status,
    bool service,
    std::optional<std::chrono::system_clock::time_point> invalidatedAt
  )
    : owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(status),
      service_(service),
      invalidatedAt_(invalidatedAt) {}

  /**
   * Full constructor with integer status
   * @param id Token ID
   * @param owner Token owner
   * @param salt Salt for hashing
   * @param hash Hash from salt and secret
   * @param createdAt Timestamp of creation
   * @param expiresAt Timestamp of expiration
   * @param status Token status as integer
   * @param service Access to service mode
   * @param invalidatedAt Timestamp of invalidation
   */
  ApiToken(
    const uint32_t id,
    const std::string& owner,
    const std::string& salt,
    const std::string& hash,
    std::chrono::system_clock::time_point createdAt,
    std::chrono::system_clock::time_point expiresAt,
    int status,
    bool service,
    std::optional<std::chrono::system_clock::time_point> invalidatedAt
  )
    : id_(id),
      owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(fromValue(status)),
      service_(service),
      invalidatedAt_(invalidatedAt) {}

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
   * @param invalidatedAt Timestamp of invalidation
   */
  ApiToken(
    const uint32_t id,
    const std::string& owner,
    const std::string& salt,
    const std::string& hash,
    std::chrono::system_clock::time_point createdAt,
    std::chrono::system_clock::time_point expiresAt,
    Status status,
    bool service,
    std::optional<std::chrono::system_clock::time_point> invalidatedAt
  )
    : id_(id),
      owner_(owner),
      salt_(salt),
      hash_(hash),
      createdAt_(createdAt),
      expiresAt_(expiresAt),
      status_(status),
      service_(service),
      invalidatedAt_(invalidatedAt) {}

  /**
   * Returns token ID
   * @return `uint32_t` Token ID
   */
  uint32_t getId() const {
    return id_;
  }

  /**
   * Returns token owner
   * @return `std::string` Token owner
   */
  const std::string& getOwner() const {
    return owner_;
  }

  /**
   * Returns salt
   * @return `std::string` Salt
   */
  const std::string& getSalt() const {
    return salt_;
  }

  /**
   * Returns hashed secret
   * @return `std::string` Hashed secret
   */
  const std::string& getHash() const {
    return hash_;
  }

  /**
   * Returns created at timestamp
   * @return `std::chrono::system_clock::time_point` Created at timestamp
   */
  std::chrono::system_clock::time_point getCreatedAt() const {
    return createdAt_;
  }

  /**
   * Returns expires at timestamp
   * @return `std::chrono::system_clock::time_point` Expires at timestamp
   */
  std::chrono::system_clock::time_point getExpiresAt() const {
    return expiresAt_;
  }

  /**
   * Returns token status
   * @return `Status` Token status
   */
  Status getStatus() const {
    return status_;
  }

  /**
   * Returns token display status
   *
   * This method returns status of a token, taking into account current timestamp.
   * Because tokens are invalidated lazily (on attempted use), token status may be
   * set to valid, but timewise may have already expired. This is used for presentation purposes.
   *
   * @param now Current timestamp
   * @return `Status` Token display status
   */
  Status getDisplayStatus(const std::chrono::system_clock::time_point& now) const {
    if (status_ == Status::Valid && now >= expiresAt_) {
      return Status::Expired;
    }
    return status_;
  }

  /**
   * Returns string representation of token display status
   *
   * See docstring for `getDisplayStatus` for more information.
   *
   * @param now Current timestamp
   * @return `std::string_view` Token display status string
   */
  std::string_view getDisplayStatusString(const std::chrono::system_clock::time_point& now) const {
    return toString(getDisplayStatus(now));
  }

  /**
   * Sets token status as expired and updates invalidation timestamp
   *
   * @param timestamp Invalidation timestamp
   */
  void expire(std::chrono::system_clock::time_point timestamp) {
    status_ = Status::Expired;
    invalidatedAt_ = timestamp;
  }

  /**
   * Sets token status as revoked and updates invalidation timestamp
   */
  void revoke(std::chrono::system_clock::time_point timestamp) {
    status_ = Status::Revoked;
    invalidatedAt_ = timestamp;
  }

  /**
   * Returns tokens service mode permission
   * @return `bool` Token service mode permission
   */
  bool canUseServiceMode() const {
    return service_;
  }

  /**
   * Returns timestamp of token invalidation
   * @return `std::chrono::system_clock::time_point` Timestamp of token invalidation
   */
  std::optional<std::chrono::system_clock::time_point> getInvalidatedAt() const {
    return invalidatedAt_;
  }

  /**
   * Factory method for API tokens from SQLiteCpp statement object
   *
   * The method expects a SQLiteCpp statement containing query result
   * produced by query of API token repository's list or get methods.
   *
   * Statement columns are parsed in the following order:
   * 0 - ID - uint |
   * 1 - owner - string |
   * 2 - salt - string |
   * 3 - hash - string |
   * 4 - createdAt - string (passed to helper, parsed into time_point) |
   * 5 - expiresAt - string (passed to helper, parsed into time_point) |
   * 6 - status - int |
   * 7 - service - bool (but really int) |
   * 8 - invalidatedAt - string (passed to helper, parsed into time_point) if not null column |
   *
   * @param stmt SQLiteCpp statement object containing query result
   * @return `ApiToken` Constructed API token
   */
  static ApiToken fromResult(SQLite::Statement& stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto owner = stmt.getColumn(1).getString();
    auto salt = stmt.getColumn(2).getString();
    auto hash = stmt.getColumn(3).getString();
    auto createdAt = DatetimeParser::parse_to_timepoint(stmt.getColumn(4).getString());
    auto expiresAt = DatetimeParser::parse_to_timepoint(stmt.getColumn(5).getString());
    auto status = stmt.getColumn(6).getInt();
    bool service = stmt.getColumn(7).getUInt() != 0;
    std::optional<std::chrono::system_clock::time_point> invalidatedAt = std::nullopt;
    if (!stmt.getColumn(8).isNull()) {
      invalidatedAt = DatetimeParser::parse_to_timepoint(stmt.getColumn(8).getString());
    }
    return ApiToken(id, owner, salt, hash, createdAt, expiresAt, status, service, invalidatedAt);
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
  std::chrono::system_clock::time_point createdAt_;
  /// Expiration timestmap
  std::chrono::system_clock::time_point expiresAt_;
  /// Token status
  Status status_;
  /// Token can use service mode
  bool service_;
  /// Invalidation timestamp
  std::optional<std::chrono::system_clock::time_point> invalidatedAt_ = std::nullopt;
};

}
