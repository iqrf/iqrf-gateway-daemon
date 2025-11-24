#include "cli_utils.h"
#include "commands.h"

#include "CryptoUtils.h"
#include "DatabaseUtils.h"
#include "DateTimeUtils.h"

#include <api_token_repo.hpp>

using json = nlohmann::json;

void create_token(const std::string& owner, const std::string& expiration, bool service, const SharedParams& params) {
  throw std::invalid_argument(
    "owner value too long: " + std::to_string(owner.length()) + " characters (maximum " + std::to_string(MAX_OWNER_LEN) + ')'
  );
  auto db = create_database_connetion(params.db_path);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  std::size_t created_at = DateTimeUtils::get_current_timestamp();
  auto parsed_expiration = DateTimeUtils::parse_expiration(expiration, created_at);
  auto salt = CryptoUtils::random_data(16);
  auto key = CryptoUtils::random_data(32);
  auto hash = CryptoUtils::sha256_hash_data(salt, key);
  auto encoded_key = CryptoUtils::base64_encode_data(key);
  auto encoded_hash = CryptoUtils::base64_encode_data(hash);

  iqrf::db::models::ApiToken token(
    owner,
    CryptoUtils::base64_encode_data(salt),
    encoded_hash,
    static_cast<int64_t>(created_at),
    static_cast<int64_t>(parsed_expiration),
    false,
    service
  );

  iqrf::db::repos::ApiTokenRepository repo(db);
  repo.insert(token);
  auto id = db->getLastInsertRowid();
  if (params.json_output) {
    std::cout << CryptoUtils::construct_shareable_token(static_cast<uint32_t>(id), encoded_key, true) << '\n';
  } else {
    std::cout << CryptoUtils::construct_shareable_token(static_cast<uint32_t>(id), encoded_key, false) << '\n';
  }
}

void get_token(uint32_t id, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto token = repo.get(id);
  if (!token) {
    throw std::invalid_argument("API token record does not exist.");
  }
  if (params.json_output) {
    std::cout << token_to_json_string(*token) << '\n';
  } else {
    std::cout
      << "ID: " << std::to_string(token->getId()) << '\n'
      << "Owner: " << token->getOwner() << '\n'
      << "Created at: " << std::to_string(token->getCreatedAt()) << '\n'
      << "Expires at: " << std::to_string(token->getExpiresAt()) << '\n'
      << "Revoked: " << (token->isRevoked() ? "YES" : "NO") << '\n'
      << "Service mode: " << (token->canUseServiceMode() ? "YES" : "NO") << "\n";
  }
}

void list_tokens(const SharedParams& params) {
  auto db = create_database_connetion(params.db_path);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto tokens = repo.list();
  if (params.json_output) {
    json doc = json::array();
    for (const auto& token : tokens) {
      doc.push_back(
        token_to_json(token)
      );
    }
    std::cout << doc.dump() << '\n';
    return;
  }
  print_list_header();
  for (const auto& token : tokens) {
    std::cout << '|'
      << pad_end(std::to_string(token.getId()), OUTPUT_ID_LEN) << ' '
      << pad_end(token.getOwner(), MAX_OWNER_LEN) << ' '
      << pad_end(std::to_string(token.getCreatedAt()), OUTPUT_DT_LEN) << ' '
      << pad_end(std::to_string(token.getExpiresAt()), OUTPUT_DT_LEN) << ' '
      << pad_end(token.isRevoked() ? "YES" : "NO", OUTPUT_REVOKED_LEN) << ' '
      << pad_end(token.canUseServiceMode() ? "YES" : "NO", OUTPUT_SERVICE_LEN)
      << "|\n";
  }
  print_table_horizontal_line();
}

void revoke_token(uint32_t id, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto token = repo.get(id);
  if (!token) {
    throw std::runtime_error("API token does not exist.");
  }
  if (token->isRevoked()) {
    std::cout << "API token ID " << std::to_string(id) << " is already revoked.\n";
    return;
  }
  auto success = repo.revoke(id);
  if (!success) {
    throw std::runtime_error("Failed to revoke API token ID " + std::to_string(id) + ".\n");
  }
  std::cout << "API token ID " << std::to_string(id) << " has been revoked.\n";
}

void verify_token(const std::string& token, const SharedParams& params) {
  auto db = create_database_connetion(params.db_path);
  if (!db->tableExists("api_tokens")) {
    throw std::runtime_error("Table api_tokens does not exist in database.");
  }

  uint32_t id = 0;
  std::string secret;
  CryptoUtils::parse_token(token, id, secret);

  iqrf::db::repos::ApiTokenRepository repo(db);
  auto dbToken = repo.get(id);
  if (!dbToken) {
    throw std::invalid_argument("API token record does not exist.");
  }
  if (dbToken->isRevoked()) {
    throw std::runtime_error("API token has been revoked.");
  }
  if (dbToken->getExpiresAt() < DateTimeUtils::get_current_timestamp()) {
    throw std::runtime_error("API token validity has expired.");
  }

  auto salt = CryptoUtils::base64_decode_data(dbToken->getSalt());
  auto hash = CryptoUtils::base64_decode_data(dbToken->getHash());
  auto key = CryptoUtils::base64_decode_data(secret);

  auto candidate = CryptoUtils::sha256_hash_data(salt, key);

  if (hash != candidate) {
    throw std::invalid_argument("Incorrect API token.");
  }
  std::cout << "API token verified.\n";
}
