/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "AuthService.h"
#include "CryptoUtils.h"
#include "DateTimeUtils.h"
#include "MigrationManager.h"
#include "Trace.h"
#include "api_token.hpp"

#include <mutex>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__AuthService.hxx"

TRC_INIT_MODULE(iqrf::AuthService);

namespace iqrf {

  class AuthService::Impl {
  private:
    /**
     * @brief Shape launch service interface
     */
    shape::ILaunchService *launchService_ = nullptr;
    /**
     * @brief Path to database file
     */
    std::string dbPath_;
    /**
     * @brief Path to migrations directory
     */
    std::string migrationDir_;
    /**
     * @brief Database access mutex
     */
    std::mutex dbMutex_;
    /**
     * @brief Database connection pointer
     */
    std::shared_ptr<SQLite::Database> db_ = nullptr;
  public:
    Impl() {}

    ~Impl() {}

    ///// Component lifecycle /////

    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "AuthService instance activate" << std::endl <<
        "******************************"
      );

      modify(props);
      try {
        db_ = std::make_shared<SQLite::Database>(
          SQLite::Database(
            dbPath_,
            SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE,
            500
          )
        );
        db_->exec("PRAGMA journal_mode=WAL;");
        MigrationManager manager(migrationDir_);
        manager.migrate(db_);
      } catch (const std::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, "[IqrfDb] Failed to migrate database to latest version: " << e.what());
      }
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");

      (void)props;

      auto dbDir = launchService_->getDataDir() + "/DB/";
      migrationDir_ = dbDir + "migrations/auth/";
      dbPath_ = dbDir + "IqrfAuthDb.db";

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "AuthService instance deactivate" << std::endl <<
        "******************************"
      );

      TRC_FUNCTION_LEAVE("")
    }

    ///// Public API /////

    std::unique_ptr<ApiToken> getApiToken(const uint32_t id) {
      std::lock_guard<std::mutex> lock(dbMutex_);
      db::repos::ApiTokenRepository repo(db_);
      return repo.get(id);
    }

    std::optional<ApiToken::Status> authenticate(const uint32_t id, const std::string& secret, int64_t& expiration) {
      std::unique_ptr<ApiToken> token;
      ApiToken::Status newStatus = ApiToken::Status::Valid;
      {
        // run this in a transaction to ensure consistency of database
        std::lock_guard<std::mutex> lock(dbMutex_);
        SQLite::Transaction transaction(*db_);
        try {
          db::repos::ApiTokenRepository repo(db_);
          token = repo.get(id);
          // if candidate token is not found, do nothing
          if (!token) {
            transaction.commit();
            return std::nullopt;
          }

          auto currentStatus = token->getStatus();
          // if token is already expired or revoked in database, change nothing
          if (currentStatus == ApiToken::Status::Expired || currentStatus == ApiToken::Status::Revoked) {
            newStatus = currentStatus;
            transaction.commit();
          } else {
            auto now = DateTimeUtils::get_current_timestamp();
            // if token is not marked as expired in database, but should be
            if (now >= token->getExpiresAt()) {
              newStatus = ApiToken::Status::Expired;
              token->expire();
              repo.update(*token);
            }
            transaction.commit();
          }
        } catch (const std::exception &e) {
          transaction.rollback();
          return std::nullopt;
        }
      }

      // if token is expired or revoked, cannot authenticate
      if (newStatus != ApiToken::Status::Valid) {
        return newStatus;
      }

      auto salt = CryptoUtils::base64_decode_data(token->getSalt());
      auto hash = CryptoUtils::base64_decode_data(token->getHash());
      auto key = CryptoUtils::base64_decode_data(secret);
      auto candidate = CryptoUtils::sha256_hash_data(salt, key);
      // if candidate token hash does not match hash stored in database, invalid
      if (hash != candidate) {
        return std::nullopt;
      }
      // set expiration
      expiration = token->getExpiresAt();
      return newStatus;
    }

    std::optional<bool> isRevoked(const uint32_t id) {
      std::lock_guard<std::mutex> lock(dbMutex_);
      db::repos::ApiTokenRepository repo(db_);
      auto token = repo.get(id);
      // token does not exist, cannot decide revoked
      if (!token) {
        return std::nullopt;
      }
      return token->getStatus() == ApiToken::Status::Revoked;
    }

    ///// Interface management /////

    void attachInterface(shape::ILaunchService *iface) {
      launchService_ = iface;
    }

    void detachInterface(shape::ILaunchService *iface) {
      if (launchService_ == iface) {
        launchService_ = nullptr;
      }
    }
  private:
  };

  ///// Object management

  AuthService::AuthService(): impl_(std::make_unique<Impl>()) {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  AuthService::~AuthService() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle /////

  void AuthService::activate(const shape::Properties *props) {
    impl_->activate(props);
  }

  void AuthService::modify(const shape::Properties *props) {
    impl_->modify(props);
  }

  void AuthService::deactivate() {
    impl_->deactivate();
  }

  ///// Public API /////

  std::unique_ptr<ApiToken> AuthService::getApiToken(const uint32_t id) {
    return impl_->getApiToken(id);
  }

  std::optional<ApiToken::Status> AuthService::authenticate(const uint32_t id, const std::string& secret, int64_t& expiration) {
    return impl_->authenticate(id, secret, expiration);
  }

  std::optional<bool> AuthService::isRevoked(const uint32_t id) {
    return impl_->isRevoked(id);
  }

  ///// Interface management /////

  void AuthService::attachInterface(shape::ILaunchService *iface) {
    impl_->attachInterface(iface);
  }

  void AuthService::detachInterface(shape::ILaunchService *iface) {
    impl_->detachInterface(iface);
  }

  void AuthService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void AuthService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
