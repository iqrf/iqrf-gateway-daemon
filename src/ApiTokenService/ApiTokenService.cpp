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

#include "ApiTokenService.h"
#include "CryptoUtils.h"
#include "DateTimeUtils.h"
#include "MigrationManager.h"
#include "Trace.h"

#include <mutex>
#include <unordered_map>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__ApiTokenService.hxx"

TRC_INIT_MODULE(iqrf::ApiTokenService);

namespace iqrf {

  class ApiTokenService::Impl {
  private:
    /**
     * @brief Shape launch service interface
     */
    shape::ILaunchService *m_launchService = nullptr;
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
    /**
     * @brief Map storing revoked or expired token IDs
     */
    std::unordered_map<uint32_t, ApiToken::Status> tokenMap_;
    /**
     * @brief Map access mutex
     */
    std::mutex tokenMapMtx_;
  public:
    Impl() {}

    ~Impl() {}

    ///// Component lifecycle /////

    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "ApiTokenService instance activate" << std::endl <<
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

      auto dbDir = m_launchService->getDataDir() + "/DB/";
      migrationDir_ = dbDir + "migrations/auth/";
      dbPath_ = dbDir + "IqrfAuthDb.db";

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "ApiTokenService instance deactivate" << std::endl <<
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
      {
        // check if token was previously seen expired and revoked
        std::lock_guard<std::mutex> lock(tokenMapMtx_);
        auto it = tokenMap_.find(id);
        if (it != tokenMap_.end()) {
          return it->second;
        }
      }

      std::unique_ptr<ApiToken> token;
      // assume token is valid
      ApiToken::Status newStatus = ApiToken::Status::Valid;
      {
        std::lock_guard<std::mutex> lock(dbMutex_);
        SQLite::Transaction transaction(*db_);
        try {
          db::repos::ApiTokenRepository repo(db_);
          token = repo.get(id);
          if (!token) {
            // token does not exist, conclude transaction with no changes and return
            transaction.commit();
            return std::nullopt;
          }

          auto currentStatus = token->getStatus();
          if (currentStatus == ApiToken::Status::Expired || currentStatus == ApiToken::Status::Revoked) {
            // token in db expired or revoked, mark new status for map update and conclude transaction, no changes needed
            newStatus = currentStatus;
            transaction.commit();
          } else {
            // token not expired in DB
            auto now = DateTimeUtils::get_current_timestamp();
            if (now >= token->getExpiresAt()) {
              // token needs to be expired in DB
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

      if (newStatus == ApiToken::Status::Expired) {
        // token
        std::lock_guard<std::mutex> lock(tokenMapMtx_);
        if (tokenMap_.count(id) == 0 || tokenMap_[id] != ApiToken::Status::Revoked) {
          tokenMap_[id] = newStatus;
        }
        return newStatus;
      }

      // check for correct hash
      auto salt = CryptoUtils::base64_decode_data(token->getSalt());
      auto hash = CryptoUtils::base64_decode_data(token->getHash());
      auto key = CryptoUtils::base64_decode_data(secret);
      auto candidate = CryptoUtils::sha256_hash_data(salt, key);
      if (hash != candidate) {
        // hash mismatch, invalid token
        return std::nullopt;
      }
      expiration = token->getExpiresAt();
      return newStatus;
    }

    std::optional<bool> isRevoked(const uint32_t id) {
      {
        // check token map first
        std::lock_guard<std::mutex> lock(tokenMapMtx_);
        if (tokenMap_.count(id)) {
          auto status = tokenMap_[id];
          if (status == ApiToken::Status::Revoked) {
            return true;
          }
        }
      }
      bool revoked;
      {
        // not found in map, get from database
        std::lock_guard<std::mutex> lock(dbMutex_);
        db::repos::ApiTokenRepository repo(db_);
        auto token = repo.get(id);
        if (!token) {
          // token does not exist
          return std::nullopt;
        }
        revoked = token->getStatus() == ApiToken::Status::Revoked;
      }
      if (revoked) {
        // store revoked state before returning
        std::lock_guard<std::mutex> lock(tokenMapMtx_);
        tokenMap_.insert_or_assign(id, ApiToken::Status::Revoked);
      }
      return revoked;
    }

    ///// Interface management /////

    void attachInterface(shape::ILaunchService *iface) {
      m_launchService = iface;
    }

    void detachInterface(shape::ILaunchService *iface) {
      if (m_launchService == iface) {
        m_launchService = nullptr;
      }
    }
  private:
  };

  ///// Object management

  ApiTokenService::ApiTokenService(): impl_(std::make_unique<Impl>()) {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ApiTokenService::~ApiTokenService() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle /////

  void ApiTokenService::activate(const shape::Properties *props) {
    impl_->activate(props);
  }

  void ApiTokenService::modify(const shape::Properties *props) {
    impl_->modify(props);
  }

  void ApiTokenService::deactivate() {
    impl_->deactivate();
  }

  ///// Public API /////

  std::unique_ptr<ApiToken> ApiTokenService::getApiToken(const uint32_t id) {
    return impl_->getApiToken(id);
  }

  std::optional<ApiToken::Status> ApiTokenService::authenticate(const uint32_t id, const std::string& secret, int64_t& expiration) {
    return impl_->authenticate(id, secret, expiration);
  }

  std::optional<bool> ApiTokenService::isRevoked(const uint32_t id) {
    return impl_->isRevoked(id);
  }

  ///// Interface management /////

  void ApiTokenService::attachInterface(shape::ILaunchService *iface) {
    impl_->attachInterface(iface);
  }

  void ApiTokenService::detachInterface(shape::ILaunchService *iface) {
    impl_->detachInterface(iface);
  }

  void ApiTokenService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void ApiTokenService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
