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
#include "MigrationManager.h"
#include "Trace.h"

#include <mutex>

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
    std::string m_dbPath;
    /**
     * @brief Path to migrations directory
     */
    std::string m_migrationDir;
    /**
     * @brief Database access mutex
     */
    std::mutex m_mtx;
    /**
     * @brief Database connection pointer
     */
    std::shared_ptr<SQLite::Database> m_db = nullptr;
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
        m_db = std::make_shared<SQLite::Database>(
          SQLite::Database(
            m_dbPath,
            SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE,
            500
          )
        );
        m_db->exec("PRAGMA journal_mode=WAL;");
        MigrationManager manager(m_migrationDir);
        manager.migrate(m_db);
      } catch (const std::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, "[IqrfDb] Failed to migrate database to latest version: " << e.what());
      }
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");

      (void)props;

      auto dbDir = m_launchService->getDataDir() + "/DB/";
      m_migrationDir = dbDir + "migrations/auth/";
      m_dbPath = dbDir + "IqrfAuthDb.db";

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
      std::lock_guard<std::mutex> lock(m_mtx);
      db::repos::ApiTokenRepository repo(m_db);
      return repo.get(id);
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
