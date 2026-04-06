/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "api_token.hpp"
#include "cli_utils.h"
#include "commands.h"
#include "exceptions.h"

#include "CryptoUtils.h"
#include "DatabaseUtils.h"
#include "DateTimeUtils.h"

#include <api_token_repo.hpp>

#include <chrono>
#include <iostream>
#include <optional>

using json = nlohmann::json;

void create_token(const std::string& owner, const std::string& expiration, bool service, const SharedParams& params) {
  if (owner.length() > MAX_OWNER_LEN) {
    throw std::invalid_argument(
      "owner value too long: " + std::to_string(owner.length()) + " characters (maximum " + std::to_string(MAX_OWNER_LEN) + ')'
    );
  }

  auto created_at = std::chrono::system_clock::now();
  auto expires_at = DateTimeUtils::parse_expiration(expiration, created_at);
  auto key = CryptoUtils::generateRandomBytes(32);
  auto encoded_key = CryptoUtils::base64Encode(key);
  auto encoded_hash = CryptoUtils::argon2idHash(encoded_key);

  iqrf::db::models::ApiToken token(
    owner,
    encoded_hash,
    created_at,
    expires_at,
    ApiToken::Status::Valid,
    service,
    std::nullopt
  );

  auto db = create_database_connetion(params.db_path, true, 3000, true);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto id = repo.insert(token);
  std::cout << construct_shareable_token(static_cast<uint32_t>(id), encoded_key, params.json_output) << '\n';
}

void get_token(uint32_t id, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path, true, 3000, true);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto token = repo.get(id);
  if (!token) {
    throw token_not_found("API token record does not exist.");
  }

  auto now = std::chrono::system_clock::now();
  if (params.json_output) {
    std::cout << token_to_json_string(*token, now) << '\n';
  } else {
    const auto invalidated_at = token->getInvalidatedAt();
    std::cout
      << "ID: " << std::to_string(token->getId()) << '\n'
      << "Owner: " << token->getOwner() << '\n'
      << "Created at: " << DatetimeParser::toISO8601(token->getCreatedAt()) << '\n'
      << "Expires at: " << DatetimeParser::toISO8601(token->getExpiresAt()) << '\n'
      << "Status: " << ApiToken::toString(token->getDisplayStatus(now)) << '\n'
      << "Service mode: " << (token->canUseServiceMode() ? "YES" : "NO") << '\n'
      << "Invalidated at: " << (invalidated_at.has_value() ? DatetimeParser::toISO8601(invalidated_at.value()) : "--") << '\n';
  }
}

void list_tokens(const SharedParams& params) {
  auto db = create_database_connetion(params.db_path, true, 3000, true);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  auto now = std::chrono::system_clock::now();
  iqrf::db::repos::ApiTokenRepository repo(db);
  auto tokens = repo.list();
  if (params.json_output) {
    json doc = json::array();
    for (const auto& token : tokens) {
      doc.push_back(
        token_to_json(token, now)
      );
    }
    std::cout << doc.dump() << '\n';
    return;
  }

  print_list_header();
  for (const auto& token : tokens) {
    auto invalidated_at = token.getInvalidatedAt();
    std::cout << '|'
      << pad_end(std::to_string(token.getId()), OUTPUT_ID_LEN) << ' '
      << pad_end(token.getOwner(), MAX_OWNER_LEN) << ' '
      << pad_end(DatetimeParser::toISO8601(token.getCreatedAt()), OUTPUT_DT_LEN) << ' '
      << pad_end(DatetimeParser::toISO8601(token.getExpiresAt()), OUTPUT_DT_LEN) << ' '
      << pad_end(std::string(token.getDisplayStatusString(now)), OUTPUT_STATUS_LEN) << ' '
      << pad_end(token.canUseServiceMode() ? "YES" : "NO", OUTPUT_SERVICE_LEN) << ' '
      << pad_end(invalidated_at.has_value() ? DatetimeParser::toISO8601(invalidated_at.value()) : "--", OUTPUT_DT_LEN)
      << "|\n";
  }
  print_table_horizontal_line();
}

void revoke_token(uint32_t id, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path, true, 3000, true);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto token = repo.get(id);
  if (!token) {
    throw token_not_found("API token record does not exist.\n");
  }
  auto status = token->getStatus();
  if (status == ApiToken::Status::Expired) {
    throw token_expired("API token ID " + std::to_string(id) + " is expired.\n");
  }
  if (status == ApiToken::Status::Revoked) {
    throw token_revoked("API token ID " + std::to_string(id) + " is already revoked.\n");
  }
  auto now = std::chrono::system_clock::now();
  if (now >= token->getExpiresAt()) {
    throw token_expired("API token ID " + std::to_string(id) + " is expired.\n");
  }
  auto success = repo.revoke(id, now);
  if (!success) {
    throw std::runtime_error("Failed to revoke API token ID " + std::to_string(id) + ".\n");
  }
  std::cout << "API token ID " << std::to_string(id) << " has been revoked.\n";
}

void rotate_token(const uint32_t id, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path, true, 3000, true);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto token = repo.get(id);
  if (!token) {
    throw token_not_found("API token record does not exist.\n");
  }
  auto status = token->getStatus();
  if (status == ApiToken::Status::Expired) {
    throw token_expired("API token ID " + std::to_string(id) + " is expired.\n");
  }
  if (status == ApiToken::Status::Revoked) {
    throw token_revoked("API token ID " + std::to_string(id) + " is revoked.\n");
  }
  auto now = std::chrono::system_clock::now();
  if (now >= token->getExpiresAt()) {
    throw token_expired("API token ID " + std::to_string(id) + " is expired.\n");
  }
  auto ttl = token->getExpiresAt() - token->getCreatedAt();
  SQLite::Transaction transaction(*db);
  try {
    // revoke old
    repo.revoke(id, now);
    // create new token
    auto expiration = now + ttl;
    auto key = CryptoUtils::generateRandomBytes(32);
    auto encoded_key = CryptoUtils::base64Encode(key);
    auto encoded_hash = CryptoUtils::argon2idHash(encoded_key);
    iqrf::db::models::ApiToken newToken(
      token->getOwner(),
      encoded_hash,
      now,
      expiration,
      ApiToken::Status::Valid,
      token->canUseServiceMode(),
      std::nullopt
    );
    auto id = repo.insert(newToken);
    transaction.commit();
    std::cout << construct_shareable_token(static_cast<uint32_t>(id), encoded_key, params.json_output) << '\n';
  } catch (const std::exception &e) {
    transaction.rollback();
    throw std::runtime_error(e.what());
  }
}
