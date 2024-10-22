/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#define IJsCacheService_EXPORTS
#define ISchedulerService_EXPORTS

#include "JsCache.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 33

#include "iqrf__JsCache.hxx"

TRC_INIT_MODULE(iqrf::JsCache);

using json = nlohmann::json;

namespace iqrf {
  static const char *COMPANIES_DIR = "cache/companies";
  static const char *MANUFACTURERS_DIR = "cache/manufacturers";
  static const char *PRODUCTS_DIR = "cache/products";
  static const char *OSDPA_DIR = "cache/osdpa";
  static const char *STANDARDS_DIR = "cache/standards";
  static const char *PACKAGES_DIR = "cache/packages/id";
  static const char *QUANTITIES_DIR = "cache/quantities";
  static const char *SERVER_DIR = "cache/server";

  static const char *SERVER_URL = "server";
  static const char *ZIP_URL = "zip";

  JsCache::JsCache() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  JsCache::~JsCache() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

    ///// Component lifecycle /////

  void JsCache::activate(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsCache instance activate" << std::endl <<
      "******************************"
    );

    modify(props);

    if (!cacheExists()) {
      if (m_downloadIfRepoCacheEmpty) {
        TRC_INFORMATION("[IQRF Repository cache] Cache does not exist, will attempt to download.");
        std::cout << "[IQRF Repository cache] Cache does not exist, will attempt to download." << std::endl;
        try {
          downloadCache();
        } catch (const std::exception &e) {
          CATCH_EXC_TRC_WAR(std::logic_error, e, e.what())
          TRC_WARNING("[IQRF Repository cache] Failed to download remote cache or validate cache content.");
          std::cerr << "[IQRF Repository cache] Failed to download remote cache or validate cache content." << std::endl;
        }
      } else {
        TRC_INFORMATION("[IQRF Repository cache] Cache download if empty not allowed, this feature can be enabled in configuration.");
        std::cout << "[IQRF Repository cache] Cache download if empty not allowed, this feature can be enabled in configuration." << std::endl;
      }
    } else {
      try {
        m_serverState = getCacheServer(m_serverStateFilePath);
      } catch (const std::exception &e) {
        TRC_WARNING("[IQRF Repository cache] Cache exists, but server state file does not.");
        std::cerr << "[IQRF Repository cache] Cache exists, but server state file does not." << std::endl;
      }
      try {
        checkCache();
      } catch (const std::logic_error &e) {
        m_cacheStatus = CacheStatus::UPDATE_FAILED;
      }
      if (m_cacheStatus == CacheStatus::UPDATE_NEEDED) {
        TRC_INFORMATION("[IQRF Repository cache] Cache exists, but is out of date.");
        std::cout << "[IQRF Repository cache] Cache exists, but is out of date." << std::endl;
        try {
          downloadCache();
        } catch (const std::exception &e) {
          TRC_WARNING("[IQRF Repository cache] Failed to download or save cache to filesystem, or validate cache content.");
          std::cerr << "[IQRF Repository cache] Failed to download or save cache to filesystem, or validate cache content." << std::endl;
        }
      } else if (m_cacheStatus == CacheStatus::UPDATE_FAILED) {
        TRC_WARNING("[IQRF Repository cache] Failed to get remote cache status, using local cache if available...");
        std::cout << "[IQRF Repository cache] Failed to get remote cache status, using local cache if available..." << std::endl;
      } else {
        TRC_INFORMATION("[IQRF Repository cache] Cache is up to date.");
        std::cout << "[IQRF Repository cache] Cache is up to date." << std::endl;
      }
    }

    if (!cacheExists()) {
      TRC_ERROR("[IQRF Repository cache] No local cache found and remote cache could not be retrieved, exiting...");
      std::cerr << "[IQRF Repository cache] No local cache found and remote cache could not be retrieved, exiting..." << std::endl;
      m_iLaunchService->exit();
      return;
    }
    try {
      loadCache(true);
    } catch (const std::exception &e) {
      TRC_ERROR("[IQRF Repository cache] Failed to load and initialize cache, deleting cache and exiting...");
      std::cerr << "[IQRF Repository cache] Failed to load and initialize cache, deleting cache and exiting..." << std::endl;
      deleteCache();
      m_iLaunchService->exit();
      return;
    }

    m_cacheUpdateFlag = true;
    m_cacheUpdateThread = std::thread([&]() {
      worker();
    });

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::modify(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");

    using namespace rapidjson;
    const std::string CHECK_CACHE("checkCache");
    const Document &doc = props->getAsJson();

    const Value *v = Pointer("/instance").Get(doc);
    if (v && v->IsString()) {
      m_name = v->GetString();
    }
    v = Pointer("/iqrfRepoCache").Get(doc);
    if (v && v->IsString()) {
      m_iqrfRepoCache = v->GetString();
    }
    v = Pointer("/urlRepo").Get(doc);
    if (v && v->IsString()) {
      m_urlRepo = v->GetString();
    }
    v = Pointer("/checkPeriodInMinutes").Get(doc);
    if (v && v->IsNumber()) {
      m_checkPeriodInMinutes = v->GetDouble();
      if (m_checkPeriodInMinutes > 0 && m_checkPeriodInMinutes < m_checkPeriodInMinutesMin) {
        TRC_WARNING(PAR(m_checkPeriodInMinutes) << " from configuration forced to: " << PAR(m_checkPeriodInMinutesMin));
        m_checkPeriodInMinutes = m_checkPeriodInMinutesMin;
      }
    }
    v = Pointer("/downloadIfRepoCacheEmpty").Get(doc);
    if (v && v->IsBool()) {
      m_downloadIfRepoCacheEmpty = v->GetBool();
    }

    m_cacheDir = m_iLaunchService->getCacheDir() + "/" + m_iqrfRepoCache;
    TRC_DEBUG("Using cache directory: " << PAR(m_cacheDir));
    m_schemaDir = m_iLaunchService->getDataDir() + "/cacheSchemas";
    TRC_DEBUG("Using cache schema directory: " << PAR(m_schemaDir));

    m_serverStateFilePath = getCachePath("serverCheck.json");

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::deactivate() {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsCache instance deactivate" << std::endl <<
      "******************************"
    );

    m_cacheUpdateFlag = false;
    m_cacheUpdateCv.notify_all();
    if (m_cacheUpdateThread.joinable()) {
      m_cacheUpdateThread.join();
    }

    TRC_FUNCTION_LEAVE("")
  }

  ///// Interfaces /////

  void JsCache::attachInterface(iqrf::IIqrfDpaService *iface) {
    m_iIqrfDpaService = iface;
  }

  void JsCache::detachInterface(iqrf::IIqrfDpaService *iface) {
    if (m_iIqrfDpaService == iface) {
      m_iIqrfDpaService = nullptr;
    }
  }

  void JsCache::attachInterface(iqrf::IJsRenderService *iface) {
    m_iJsRenderService = iface;
  }

  void JsCache::detachInterface(iqrf::IJsRenderService *iface) {
    if (m_iJsRenderService == iface) {
      m_iJsRenderService = nullptr;
    }
  }

  void JsCache::attachInterface(iqrf::ISchedulerService *iface) {
    m_iSchedulerService = iface;
  }

  void JsCache::detachInterface(iqrf::ISchedulerService *iface) {
    if (m_iSchedulerService == iface) {
      m_iSchedulerService = nullptr;
    }
  }

  void JsCache::attachInterface(shape::ILaunchService *iface) {
    m_iLaunchService = iface;
  }

  void JsCache::detachInterface(shape::ILaunchService *iface) {
    if (m_iLaunchService == iface) {
      m_iLaunchService = nullptr;
    }
  }

  void JsCache::attachInterface(shape::IRestApiService *iface) {
    m_iRestApiService = iface;
  }

  void JsCache::detachInterface(shape::IRestApiService *iface) {
    if (m_iRestApiService == iface) {
      m_iRestApiService = nullptr;
    }
  }

  void JsCache::attachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsCache::detachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().removeTracerService(iface);
  }

  ///// API /////

  std::shared_ptr<IJsCacheService::StdDriver> JsCache::getDriver(int id, double ver) const {
    TRC_FUNCTION_ENTER(PAR(id) << std::fixed << std::setprecision(2) << PAR(ver));
    std::shared_ptr<StdDriver> driver = nullptr;
    auto foundDrv = m_standardMap.find(id);
    if (foundDrv != m_standardMap.end()) {
      const StdItem &stdItem = foundDrv->second;
      auto foundVer = stdItem.m_drivers.find(ver);
      if (foundVer != stdItem.m_drivers.end()) {
        driver = std::make_shared<StdDriver>(foundVer->second);
      }
    }
    TRC_FUNCTION_LEAVE("");
    return driver;
  }

  std::shared_ptr<IJsCacheService::Manufacturer> JsCache::getManufacturer(uint16_t hwpid) const {
    TRC_FUNCTION_ENTER(PAR(hwpid));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<Manufacturer> manufacturer = nullptr;
    auto found = m_productMap.find(hwpid);
    if (found != m_productMap.end()) {
      int manufacturerId = found->second.m_manufacturerId;
      auto foundManuf = m_manufacturerMap.find(manufacturerId);
      if (foundManuf != m_manufacturerMap.end()) {
        manufacturer = std::make_shared<Manufacturer>(foundManuf->second);
      }
    }

    int manufacturerId = manufacturer == nullptr ? -1 : manufacturer->m_manufacturerId;

    TRC_FUNCTION_LEAVE(PAR(manufacturerId));
    return manufacturer;
  }

  std::shared_ptr<IJsCacheService::Product> JsCache::getProduct(uint16_t hwpid) const {
    TRC_FUNCTION_ENTER(PAR(hwpid));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<Product> product = nullptr;
    auto found = m_productMap.find(hwpid);
    if (found != m_productMap.end()) {
      product = std::make_shared<Product>(found->second);
    }

    int productId = product == nullptr ? -1 : product->m_manufacturerId;

    TRC_FUNCTION_LEAVE(PAR(productId));
    return product;
  }

  std::shared_ptr<IJsCacheService::Package> JsCache::getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string &os, const std::string &dpa) const {
    TRC_FUNCTION_ENTER(PAR(hwpid) << PAR(hwpidVer) << PAR(os) << PAR(dpa));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<Package> package = nullptr;
    for (const auto &[id, pkg] : m_packageMap) {
      if (pkg.m_hwpid == hwpid && pkg.m_hwpidVer == hwpidVer && pkg.m_os == os && pkg.m_dpa == dpa) {
        package = std::make_shared<Package>(pkg);
        break;
      }
    }

    int packageId = package == nullptr ? -1 : package->m_packageId;

    TRC_FUNCTION_LEAVE(PAR(packageId));
    return package;
  }

  std::shared_ptr<IJsCacheService::Package> JsCache::getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const {
    TRC_FUNCTION_ENTER(PAR(hwpid) << PAR(hwpidVer) << PAR(os) << PAR(dpa));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<Package> package = nullptr;
    std::string osStr = embed::os::Read::getOsBuildAsString(os);
    std::string dpaStr = embed::explore::Enumerate::getDpaVerAsHexaString(dpa);
    for (const auto &[id, pkg] : m_packageMap) {
      if (pkg.m_hwpid == hwpid && pkg.m_hwpidVer == hwpidVer && pkg.m_os == osStr && pkg.m_dpa == dpaStr) {
        package = std::make_shared<Package>(pkg);
        break;
      }
    }

    TRC_FUNCTION_LEAVE("");
    return package;
  }

  std::map<int, std::map<double, std::vector<std::pair<int, int>>>> JsCache::getDrivers(const std::string &os, const std::string &dpa) const {
    TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

    // DriverId, DriverVersion, hwpid, hwpidVer
    std::map<int, std::map<double, std::vector<std::pair<int, int>>>> map2;

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::ostringstream ostr;
    for (const auto &pck : m_packageMap) {
      const Package &p = pck.second;
      if (p.m_os == os && p.m_dpa == dpa) {
        for (const auto &drv : p.m_stdDriverVect) {
          map2[drv.getId()][drv.getVersion()].push_back(std::make_pair(p.m_hwpid, p.m_hwpidVer));
          ostr << '[' << drv.getId() << ',' << std::fixed << std::setprecision(2) << drv.getVersion() << "] ";
        }
      }
    }

    TRC_INFORMATION("Loading provisory drivers (no context): " << std::endl << ostr.str());
    TRC_FUNCTION_LEAVE("");
    return map2;
  }

  std::map<int, std::map<int, std::string>> JsCache::getCustomDrivers(const std::string &os, const std::string &dpa) const {
    TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

    // hwpid, hwpidVer, driver
    std::map<int, std::map<int, std::string>> map2;

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    for (const auto &pck : m_packageMap) {
      const Package &p = pck.second;
      if (p.m_os == os && p.m_dpa == dpa) {
        if (!p.m_driver.empty() && p.m_driver.size() > 20) {
          map2[p.m_hwpid].insert(std::make_pair(p.m_hwpidVer, p.m_driver));
        }
      }
    }

    TRC_FUNCTION_LEAVE("");
    return map2;
  }

  IJsCacheService::MapOsListDpa JsCache::getOsDpa() const {
    TRC_FUNCTION_ENTER("");

    MapOsListDpa retval;

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    for (auto it : m_osDpaMap) {
      int os = 0;
      int dpa = 0;
      std::string osStr = it.second.m_os;
      std::string dpaStr = it.second.m_dpa;
      try {
        os = std::stoi(osStr, nullptr, 16);
        dpa = std::stoi(dpaStr, nullptr, 16);
        retval[os].insert(dpa);
      } catch (std::invalid_argument &e) {
        CATCH_EXC_TRC_WAR(std::invalid_argument, e, "cannot convert: " << PAR(osStr) << PAR(dpaStr));
        continue;
      }
    }

    TRC_FUNCTION_LEAVE("");
    return retval;
  }

  std::shared_ptr<IJsCacheService::OsDpa> JsCache::getOsDpa(int id) const {
    TRC_FUNCTION_ENTER(PAR(id));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<OsDpa> osDpa;
    auto found = m_osDpaMap.find(id);
    if (found != m_osDpaMap.end()) {
      osDpa = std::make_shared<OsDpa>(found->second);
    }

    int osDpaId = osDpa == nullptr ? -1 : osDpa->m_osdpaId;

    TRC_FUNCTION_LEAVE(PAR(osDpaId));
    return osDpa;
  }

  std::shared_ptr<IJsCacheService::OsDpa> JsCache::getOsDpa(const std::string &os, const std::string &dpa) const {
    TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<OsDpa> osDpa;
    for (auto &[id, item] : m_osDpaMap) {
      if (os == item.m_os && dpa == item.m_dpa) {
        osDpa = std::make_shared<OsDpa>(item);
        break;
      }
    }

    int osDpaId = osDpa == nullptr ? -1 : osDpa->m_osdpaId;

    TRC_FUNCTION_LEAVE(PAR(osDpaId));
    return osDpa;
  }

  std::shared_ptr<IJsCacheService::Quantity> JsCache::getQuantity(const uint8_t &type) const {
    TRC_FUNCTION_ENTER(PAR(type));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<Quantity> quantity;
    auto found = m_quantityMap.find(type);
    if (found != m_quantityMap.end()) {
      quantity = std::make_shared<Quantity>(found->second);
    }

    int quantityId = quantity == nullptr ? -1 : quantity->m_type;
    TRC_FUNCTION_LEAVE(PAR(quantityId));
    return quantity;
  }

  IJsCacheService::ServerState JsCache::getServerState() const {
    TRC_FUNCTION_ENTER("");

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    TRC_FUNCTION_LEAVE("");
    return m_serverState;
  }

  std::tuple<IJsCacheService::CacheStatus, std::string> JsCache::invokeWorker() {
    TRC_FUNCTION_ENTER("");
    std::unique_lock<std::mutex> invokeWorkerLock(m_cacheUpdateMtx);
    // set manual invocation flag
    m_invoked = true;
    // wake up cache update thread
    m_cacheUpdateCv.notify_all();
    // sleep until cache status is known
    m_invokeWorkerCv.wait(invokeWorkerLock);
    // get cache status
    CacheStatus status = m_cacheStatus;
    if (status == CacheStatus::UPDATE_NEEDED) {
      // wake up worker thread to perform cache update
      m_cacheUpdateCv.notify_all();
      // wait for cache to be updated
      m_invokeWorkerCv.wait(invokeWorkerLock);
      // get status after update
      status = m_cacheStatus;
    }
    // Get error str
    std::string errorStr = m_cacheUpdateError;
    // reset manual invocation flag
    m_invoked = false;
    // wake up cache update thread to return to original state
    m_cacheUpdateCv.notify_all();
    TRC_FUNCTION_LEAVE("");
    return std::make_tuple(status, errorStr);
  }

  void JsCache::registerCacheReloadedHandler(const std::string &clientId, CacheReloadedFunc hndl) {
    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);
    m_cacheReloadedHndlMap[clientId] = hndl;
  }

  void JsCache::unregisterCacheReloadedHandler(const std::string &clientId) {
    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);
    m_cacheReloadedHndlMap.erase(clientId);
  }

  ///// Private methods /////

  IJsCacheService::ServerState JsCache::getCacheServer(const std::string &fileName) {
    TRC_FUNCTION_ENTER("");

    ServerState serverState;

    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Server state file does not exist. " << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse server state file: [" << e.byte << "] " << e.what());
    }

    try {
      serverState.m_apiVersion = doc["apiVersion"];
      serverState.m_hostname = doc["hostname"];
      serverState.m_user = doc["user"];
      serverState.m_buildDateTime = doc["buildDateTime"];
      serverState.m_startDateTime = doc["startDateTime"];
      serverState.m_dateTime = doc["dateTime"];
      serverState.m_databaseChecksum = doc["databaseChecksum"];
      serverState.m_databaseChangeDateTime = doc["databaseChangeDateTime"];
    } catch (const json::exception &e) {
      THROW_EXC_TRC_WAR(std::logic_error, e.what())
    } catch (const std::logic_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, e.what());
    }

    TRC_FUNCTION_LEAVE("");
    return serverState;
  }

  std::shared_ptr<IJsCacheService::StdDriver> JsCache::getStandard(int standardId, double version) {
    TRC_FUNCTION_ENTER(PAR(standardId) << std::fixed << std::setprecision(2) << PAR(version));

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    std::shared_ptr<StdDriver> stdDriver = nullptr;
    auto found = m_standardMap.find(standardId);
    if (found != m_standardMap.end()) {
      auto foundVer = found->second.m_drivers.find(version);
      if (foundVer != found->second.m_drivers.end()) {
        stdDriver = std::make_shared<StdDriver>(foundVer->second);
      }
    }

    unsigned int stdDriverId = stdDriver == nullptr ? -1 : stdDriver->getId();

    TRC_FUNCTION_LEAVE(PAR(stdDriverId));
    return stdDriver;
  }

  void JsCache::updateCacheServer() {
    TRC_FUNCTION_ENTER("");

    std::string fname = getCacheDataFilePath(SERVER_DIR);
    if (!std::filesystem::exists(fname)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Cache server data file does not exist. " << PAR(fname));
    }
    m_serverState = getCacheServer(fname);

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::updateCacheCompanies() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(COMPANIES_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Companies information file does not exist. " << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse companies information file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Companies information file (" << fileName << ") should be an array root.");
    }

    std::map<unsigned int, Company> companyMap;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr) {
      json companyDoc = itr.value();
      try {
        unsigned int companyId = companyDoc["companyID"];
        std::string name = companyDoc["name"];
        std::string homePage = companyDoc["homePage"];
        companyMap.insert(
          std::make_pair(companyId, Company(companyId, name, homePage))
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_companyMap = companyMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCacheManufacturers() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(MANUFACTURERS_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Manufacturers information file does not exist." << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse manufacturers information file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Manufacturers information file (" << fileName << ") should be an array root.");
    }

    std::map<unsigned int, Manufacturer> manufacturerMap;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr) {
      json manufacturerDoc = itr.value();
      try {
        unsigned int manufacturerId = manufacturerDoc["manufacturerID"];
        unsigned int companyId = manufacturerDoc["companyID"];
        std::string name = manufacturerDoc["name"];
        manufacturerMap.insert(
          std::make_pair(manufacturerId, Manufacturer(manufacturerId, companyId, name))
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_manufacturerMap = manufacturerMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCacheProducts() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(PRODUCTS_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Products information file does not exist." << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse products information file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Products information file (" << fileName << ") should be an array root.");
    }

    std::map<uint16_t, Product> productMap;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr) {
      json productDoc = itr.value();
      try {
        uint16_t hwpid = productDoc["hwpid"];
        unsigned int manufacturerId = productDoc["manufacturerID"];
        std::string name = productDoc["name"];
        std::string homePage = productDoc["homePage"];
        std::string picture = productDoc["picture"];
        std::shared_ptr<Metadata> metadata = nullptr;

        if (!productDoc["metadata"].empty()) {
          json metadataDoc = productDoc["metadata"]["static"];

          for (auto metadataItemItr = metadataDoc.begin(); metadataItemItr != metadataDoc.end(); ++metadataItemItr) {
            json metadataItemDoc = metadataItemItr.value();

            uint16_t version = metadataItemDoc["version"];
            std::vector<MetadataProfile> profiles;

            for (auto profileItr = metadataItemDoc["profiles"].begin(); profileItr != metadataItemDoc["profiles"].end(); ++profileItr) {
              json profileDoc = profileItr.value();
              ProfileHwpidVersion hwpidVer(profileDoc["hwpidVersions"]["min"], profileDoc["hwpidVersions"]["max"]);
              bool routing = profileDoc["routing"];
              bool beaming = profileDoc["beaming"];
              bool repeater = profileDoc["repeater"];
              bool frcAggregation = profileDoc["frcAggregation"];
              bool iqarosCompatible = profileDoc["iqarosCompatible"];
              std::vector<uint8_t> iqrfSensors = profileDoc["iqrfSensor"];
              uint8_t binouts = profileDoc["iqrfBinaryOutput"];
              // Mains power
              ProfilePowerMains mainsPower(profileDoc["powerSupply"]["mains"]);
              // Accumulator power
              std::shared_ptr<std::string> accuType;
              if (!profileDoc["powerSupply"]["accumulator"]["type"].is_null()) {
                accuType = std::make_shared<std::string>(profileDoc["powerSupply"]["accumulator"]["type"]);
              }
              std::shared_ptr<double> accuLowLevel;
              if (!profileDoc["powerSupply"]["accumulator"]["lowLevel"].is_null()) {
                accuLowLevel = std::make_shared<double>(profileDoc["powerSupply"]["accumulator"]["lowLevel"]);
              }
              ProfilePowerAccumulator accuPower(
                profileDoc["powerSupply"]["accumulator"]["present"],
                accuType,
                accuLowLevel
              );
              // Battery power
              std::shared_ptr<std::string> batteryType;
              if (!profileDoc["powerSupply"]["battery"]["type"].is_null()) {
                batteryType = std::make_shared<std::string>(profileDoc["powerSupply"]["battery"]["type"]);
              }
              std::shared_ptr<double> batteryChangeThreshold;
              if (!profileDoc["powerSupply"]["battery"]["changeThreshold"].is_null()) {
                batteryChangeThreshold = std::make_shared<double>(profileDoc["powerSupply"]["battery"]["changeThreshold"]);
              }
              ProfilePowerBattery batteryPower(
                profileDoc["powerSupply"]["battery"]["present"],
                batteryType,
                batteryChangeThreshold
              );
              // power
              double minVoltage = profileDoc["powerSupply"]["minVoltage"];
              ProfilePower powerSupply(mainsPower, accuPower, batteryPower, minVoltage);
              // profile
              profiles.emplace_back(
                MetadataProfile(hwpidVer, powerSupply, routing, beaming, repeater, frcAggregation, iqarosCompatible, iqrfSensors, binouts)
              );
            }

            metadata = std::make_shared<Metadata>(Metadata(version, profiles));
            if (metadata->profiles[0].powerSupply.accumulator.type != nullptr) {
              std::string aType = (*metadata->profiles[0].powerSupply.accumulator.type.get());
            }
          }
        }
        productMap.insert(
          std::make_pair(hwpid, Product(hwpid, manufacturerId, name, homePage, picture, metadata))
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_productMap = productMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCacheOsDpa() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(OSDPA_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "OsDpa information file does not exist." << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse OsDpa information file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "OsDpa information file (" << fileName << ") should be an array root.");
    }

    std::map<unsigned int, OsDpa> osDpaMap;
    unsigned int idx = 0;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr, ++idx) {
      json osDpaDoc = itr.value();
      try {
        std::string os = osDpaDoc["os"];
        std::string dpa = osDpaDoc["dpa"];
        std::string notes = osDpaDoc["notes"];
        osDpaMap.insert(
          std::make_pair(idx, OsDpa(idx, os, dpa, notes))
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }

    m_osDpaMap = osDpaMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCacheStandards() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(STANDARDS_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Standards information file does not exist. " << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse standards information file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Standards information file (" << fileName << ") should be an array root.");
    }

    std::map<int, StdItem> standardMap;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr) {
      // get standards
      json standardDoc = itr.value();
      int standardId = standardDoc["standardID"];
      std::string name = standardDoc["name"];
      standardMap.insert(
        std::make_pair(standardId, StdItem(name))
      );
    }

    for (auto &[id, item] : standardMap) {
      // get standard version
      std::ostringstream os;
      os << STANDARDS_DIR << '/' << id;
      std::string url = os.str();
      fileName = getCacheDataFilePath(url);

      if (!std::filesystem::exists(fileName)) {
        THROW_EXC_TRC_WAR(std::logic_error, "Standard file does not exist: " << PAR(fileName));
      }

      std::ifstream file(fileName);
      json standardDoc;
      try {
        standardDoc = json::parse(file);
      } catch (const json::parse_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse standard file: [" << e.byte << "] " << e.what());
      }

      try {
        std::vector<double> versions = standardDoc["versions"];
        for (auto &version : versions) {

          std::ostringstream oss;
          oss << STANDARDS_DIR << '/' << id << '/' << std::fixed << std::setprecision(2) << version;
          fileName = getCacheDataFilePath(oss.str());

          if (!std::filesystem::exists(fileName)) {
            THROW_EXC_TRC_WAR(std::logic_error, "Standard version file does not exist. " << PAR(fileName));
          }

          std::ifstream file(fileName);
          json driverDoc;
          try {
            driverDoc = json::parse(file);
          } catch (const json::parse_error &e) {
            THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse standard version file: [" << e.byte << "] " << e.what());
          }

          double drvVersion = driverDoc["version"];
          int versionFlags = driverDoc["versionFlags"];
          std::shared_ptr<std::string> driver = std::make_shared<std::string>(driverDoc["driver"]);
          std::shared_ptr<std::string> notes = std::make_shared<std::string>(driverDoc["notes"]);

          item.m_drivers.insert(
            std::make_pair(version, StdDriver(id, item.m_name, drvVersion, driver, notes, versionFlags))
          );
        }
        item.m_valid = true;
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_standardMap = standardMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCachePackages() {
    TRC_FUNCTION_ENTER("");

    std::string fname = getCachePath(PACKAGES_DIR);

    std::filesystem::path p(fname);
    std::vector<std::filesystem::directory_entry> v; // To save the file names in a vector.

    if (is_directory(p)) {
      std::copy(std::filesystem::directory_iterator(p), std::filesystem::directory_iterator(), std::back_inserter(v));
    }

    std::vector<std::string> vstr;
    for (std::vector<std::filesystem::directory_entry>::const_iterator it = v.begin(); it != v.end(); ++it) {
      vstr.push_back((*it).path().string() + "/data.json");
    }

    std::ostringstream auxtrc;
    std::map<unsigned int, Package> packageMap;

    for (auto pkgFile : vstr) {
      std::ifstream file(pkgFile);
      json doc;
      try {
        doc = json::parse(file);
      } catch (const json::parse_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse package information file: [" << e.byte << "] " << e.what());
      }

      try {
        unsigned int packageId = doc["packageID"];
        uint16_t hwpid = doc["hwpid"];
        uint16_t hwpidVer = doc["hwpidVer"];
        std::string handlerUrl = doc["handlerUrl"];
        std::string handlerHash = doc["handlerHash"];
        std::string os = doc["os"];
        std::string dpa = doc["dpa"];
        std::string notes = doc["notes"];
        std::string driver = doc["driver"];

        auxtrc << std::endl
          << NAME_PAR(package, packageId)
          << NAME_PAR(os, os)
          << NAME_PAR(dpa, dpa)
          << NAME_PAR(hwpid, hwpid)
          << NAME_PAR(hwpidVer, hwpidVer)
          << std::endl
          << "    standards: ";

        json standardsDoc = doc["standards"];
        if (!standardsDoc.is_array()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Package standards should be an array. " << PAR(packageId));
        }
        std::vector<StdDriver> stdDrivers;
        for (auto itr = standardsDoc.begin(); itr != standardsDoc.end(); ++itr) {
          json standardDoc = itr.value();
          int standardId = standardDoc["standardID"];
          double version = standardDoc["version"];

          std::shared_ptr<StdDriver> stdDrv = getStandard(standardId, version);
          if (stdDrv != nullptr) {
            stdDrivers.emplace_back(*stdDrv);
            auxtrc << '[' << standardId << ',' << std::fixed << std::setprecision(2) << version << "], ";
          } else {
            auxtrc << '[' << standardId << ',' << std::fixed << std::setprecision(2) << version << ", N/F], ";
          }
        }
        Package package(packageId, hwpid, hwpidVer, handlerUrl, handlerHash, os, dpa, notes, driver, stdDrivers);
        packageMap.insert(
          std::make_pair(packageId, package)
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_packageMap = packageMap;

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::updateCacheQuantities() {
    TRC_FUNCTION_ENTER("");

    std::string fileName = getCacheDataFilePath(QUANTITIES_DIR);
    if (!std::filesystem::exists(fileName)) {
      THROW_EXC_TRC_WAR(std::logic_error, "Quantities data file does not exist." << PAR(fileName));
    }

    std::ifstream file(fileName);
    json doc;
    try {
      doc = json::parse(file);
    } catch (const json::parse_error &e) {
      THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse quantities data file: [" << e.byte << "] " << e.what());
    }

    if (!doc.is_array()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Quantities data file (" << fileName << ") should be an array root.");
    }

    std::map<uint8_t, Quantity> quantityMap;
    for (auto itr = doc.begin(); itr != doc.end(); ++itr) {
      try {
        json quantityDoc = itr.value();
        uint8_t type = quantityDoc["idValue"];
        std::string textId = quantityDoc["id"];
        std::string name = quantityDoc["name"];
        std::string shortName = quantityDoc["shortName"];
        std::string unit = quantityDoc["unit"];
        uint8_t precision = quantityDoc["decimalPlaces"];
        std::vector<uint8_t> frcs = quantityDoc["frcs"];
        uint8_t width = quantityDoc["width"];
        std::string driverKey = quantityDoc["idDriver"];
        quantityMap.insert(
          std::make_pair(
            type,
            Quantity(type, textId, name, shortName, unit, precision, frcs, width, driverKey)
          )
        );
      } catch (const json::exception &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what())
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }
    m_quantityMap = quantityMap;

    TRC_FUNCTION_LEAVE("");
  }

  std::string JsCache::getTmpPath(const std::string &path) {
    return m_tmpDir + path;
  }

  std::string JsCache::getTmpCachePath(const std::string &path) {
    return m_tmpDir + "cache/" + path;
  }

  std::string JsCache::getTmpCacheDataFilePath(const std::string &relativeDir) {
    return m_tmpDir + "cache/" + relativeDir + "/data.json";
  }

  std::string JsCache::getSchemaFilePath(const std::string &path) {
    return m_schemaDir + '/' + path;
  }

  std::string JsCache::getCachePath(const std::string &path) {
    std::ostringstream os;
    os << m_cacheDir << '/' << path;
    return os.str();
  }

  std::string JsCache::getCacheDataFilePath(const std::string &relativeDir) {
    std::ostringstream os;
    os << m_cacheDir << '/' << relativeDir << "/data.json";
    return os.str();
  }

  std::string JsCache::getAbsoluteUrl(const std::string &relativeUrl) {
    std::ostringstream os;
    os << m_urlRepo << '/' << relativeUrl;
    return os.str();
  }

  void JsCache::createFile(const std::string &path) {
    std::filesystem::path createdFile(path);
    std::filesystem::path parent(createdFile.parent_path());

    try {
      if (!(std::filesystem::exists(parent))) {
        if (std::filesystem::create_directories(parent)) {
          TRC_DEBUG("Created: " << PAR(parent));
        } else {
          TRC_DEBUG("Cannot create: " << PAR(parent));
        }
      }
    } catch (std::exception &e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "cannot create: " << PAR(parent));
    }
  }

  void JsCache::downloadFromUrl(const std::string &url, const std::string &fileName, bool absolute) {
    TRC_FUNCTION_ENTER(PAR(url) << PAR(fileName));

    createFile(fileName);

    std::string urlLoading = absolute ? url : getAbsoluteUrl(url);

    TRC_DEBUG("Getting: " << PAR(urlLoading));

    try {
      std::filesystem::path getFile(fileName);
      std::filesystem::path downloadFile(fileName + ".download");
      if (std::filesystem::exists(downloadFile)) {
        std::filesystem::remove(downloadFile);
      }

      m_iRestApiService->getFile(urlLoading, downloadFile.string());

      std::filesystem::copy_file(downloadFile, getFile, std::filesystem::copy_options::overwrite_existing);
      std::filesystem::remove(downloadFile);
    } catch (const std::filesystem::filesystem_error &e) {
      CATCH_EXC_TRC_WAR(std::filesystem::filesystem_error, e, "cannot get " << PAR(urlLoading));
      throw e;
    }

    TRC_FUNCTION_LEAVE("")
  }

  bool JsCache::cacheExists() {
    std::string filename = getCacheDataFilePath(SERVER_DIR);
    return std::filesystem::exists(filename);
  }

  void JsCache::checkCache() {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION("=============================================================" << std::endl <<
      "Checking Iqrf Repo for updates");

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    downloadFromUrl(SERVER_URL, m_serverStateFilePath);
    ServerState remoteServerState = getCacheServer(m_serverStateFilePath);

    TRC_INFORMATION(
      "Comparing db checksums: " <<
      NAME_PAR(localChecksum, m_serverState.m_databaseChecksum) <<
      NAME_PAR(remoteChecksum, remoteServerState.m_databaseChecksum)
    );
    m_upToDate = m_serverState.m_databaseChecksum == remoteServerState.m_databaseChecksum;

    if (m_upToDate) {
      TRC_INFORMATION("Iqrf Repo is up to date");
      m_cacheStatus = CacheStatus::UP_TO_DATE;
    } else {
      TRC_INFORMATION("Iqrf Repo has been changed => reload");
      m_cacheStatus = CacheStatus::UPDATE_NEEDED;
    }

    TRC_FUNCTION_LEAVE(PAR(m_upToDate));
  }

  void JsCache::downloadCache() {
    TRC_FUNCTION_ENTER("");

    // std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    TRC_INFORMATION("[IQRF Repository cache] Downloading cache ...");
    std::cout << "[IQRF Repository cache] Downloading cache ..." << std::endl;

    std::string archivePath = getTmpPath("cache.zip");
    downloadFromUrl(ZIP_URL, archivePath);
    downloadFromUrl(SERVER_URL, m_serverStateFilePath);

    if (!std::filesystem::exists(archivePath)) {
      THROW_EXC_TRC_WAR(std::logic_error, "file not exist " << PAR(archivePath));
    }

    zip_t *archive = nullptr;
    zip_file_t *zipFile = nullptr;
    int err;
    const zip_uint64_t BUF_SIZE = 8196;
    char buf[BUF_SIZE];

    if ((archive = zip_open(archivePath.c_str(), 0, &err)) == NULL) {
      zip_error_to_str(buf, sizeof(buf), err, errno);
      THROW_EXC_TRC_WAR(std::logic_error, "Can't open zip archive: " << buf);
    }

    zip_int64_t num_entries = zip_get_num_entries(archive, 0);

    for (zip_uint64_t i = 0; i < (zip_uint64_t)num_entries; i++) {
      std::string name = zip_get_name(archive, i, 0);

      std::string extractedPath = getTmpCachePath(name);
      createFile(extractedPath);

      zip_stat_t zipStat;
      if (zip_stat_index(archive, i, 0, &zipStat) == 0) {
        zipFile = zip_fopen_index(archive, i, 0);
        if (!zipFile) {
          THROW_EXC_TRC_WAR(std::logic_error, "Can't open file from zip: " << extractedPath);
        }

        std::ofstream outfile(extractedPath, std::ofstream::binary);
        if (!outfile.is_open()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Can't open output file to inflate from zip: " << extractedPath);
        }

        zip_uint64_t sum = 0;
        zip_int64_t len = 0;
        while (sum != zipStat.size) {
          len = zip_fread(zipFile, buf, BUF_SIZE);
          if (len < 0) {
            THROW_EXC_TRC_WAR(std::logic_error, "Can't write file from zip: " << extractedPath);
          }

          outfile.write(buf, len);
          sum += len;
        }

        outfile.close();
        zip_fclose(zipFile);
      }
    }

    if (zip_close(archive) == -1) {
      TRC_WARNING("Can't close zip: " << archivePath);
    }

    if (!validateCache()) {
      std::filesystem::remove_all(getTmpPath("cache"));
      std::filesystem::remove(archivePath);
      throw std::logic_error("Remote cache content not valid against cache schema constraints.");
    }

    // rename old cache dir to cache.bkp
    std::string cacheName = getCachePath("cache");
    std::string cacheNameBkp = getCachePath("cache.old");
    if (std::filesystem::exists(cacheName)) {
      std::filesystem::remove_all(cacheNameBkp);
      std::filesystem::rename(cacheName, cacheNameBkp);
    }
    // move cache from tmp
    std::string tmpCache = getTmpPath("cache");
    std::filesystem::copy(tmpCache, cacheName, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(tmpCache);
    std::filesystem::remove(archivePath);

    TRC_INFORMATION("[IQRF Repository cache] Cache successfully downloaded.");
    std::cout << "[IQRF Repository cache] Cache successfully downloaded." << std::endl;

    TRC_FUNCTION_LEAVE("")
  }

  bool JsCache::validateCache() {
    TRC_FUNCTION_ENTER("");

    TRC_INFORMATION("[IQRF Repository cache] Validating cache contents...");
    std::cout << "[IQRF Repository cache] Validating cache contents..." << std::endl;

    // companies data
    try {
      validateCompaniesData();
      validateManufacturersData();
      validateProductsData();
      validateOsDpaData();
      validateStandardsListData();
      validateStandardsVersionsData();
      validatePackagesData();
      validateQuantitiesData();
    } catch (const std::exception &e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "[IQRF Repository cache] " << e.what() << std::endl);
      std::cerr << "[IQRF Repository cache] " << e.what() << std::endl;
      return false;
    }

    TRC_INFORMATION("[IQRF Repository cache] Cache contents successfully validated.");
    std::cout << "[IQRF Repository cache] Cache contents successfully validated." << std::endl;

    TRC_FUNCTION_LEAVE("");
    return true;
  }

  void JsCache::logSchemaViolations(const std::vector<std::string> &errors) {
    for (auto itr = errors.begin(); itr != errors.end(); ++itr) {
      TRC_WARNING("[IQRF Repository cache] " << *itr << std::endl);
    }
  }

  void JsCache::validateCompaniesData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("companies.json");
    auto filePath = getTmpCacheDataFilePath("companies");
    TRC_DEBUG("Loading schema file: " + schemaPath);
    JsonValidationUtils::populateSchema(schema, schemaPath);
    valijson::ValidationResults errors;
    TRC_DEBUG("Validating data file: " + filePath);
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateManufacturersData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("manufacturers.json");
    auto filePath = getTmpCacheDataFilePath("manufacturers");
    JsonValidationUtils::populateSchema(schema, schemaPath);
    valijson::ValidationResults errors;
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateProductsData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("products.json");
    auto filePath = getTmpCacheDataFilePath("products");
    JsonValidationUtils::populateSchema(
      schema,
      schemaPath,
      [&schemaDir = m_schemaDir](const std::string &uri) -> const json* {
        json *doc = new json();
        std::string path = schemaDir + '/' + uri;
        if (!valijson::utils::loadDocument(path, *doc)) {
          return nullptr;
        }
        return doc;
      }
    );
    valijson::ValidationResults errors;
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateOsDpaData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("osdpa.json");
    auto filePath = getTmpCacheDataFilePath("osdpa");
    JsonValidationUtils::populateSchema(schema, schemaPath);
    valijson::ValidationResults errors;
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateStandardsListData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("standardList.json");
    auto filePath = getTmpCacheDataFilePath("standards");
    JsonValidationUtils::populateSchema(schema, schemaPath);
    valijson::ValidationResults errors;
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateStandardsVersionsData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema standardVersionsSchema, standardDataSchema;
    auto standardVersionsSchemaPath = getSchemaFilePath("standardVersions.json");
    auto standardDataSchemaPath = getSchemaFilePath("standardData.json");
    JsonValidationUtils::populateSchema(standardVersionsSchema, standardVersionsSchemaPath);
    JsonValidationUtils::populateSchema(standardDataSchema, standardDataSchemaPath);

    std::vector<std::filesystem::path> dirs;
    for (const auto &item : std::filesystem::directory_iterator(getTmpCachePath("standards"))) {
      if (item.is_directory()) {
        dirs.emplace_back(item.path());
      }
    }

    valijson::ValidationResults errors;
    for (const auto &dir : dirs) {
      auto standardVersionFilePath = dir.string() + "/data.json";
      valijson::ValidationResults errors;
      if (!JsonValidationUtils::validateFile(standardVersionsSchema, standardVersionFilePath, errors)) {
        logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
        throw JsonValidationUtils::SchemaViolationException("Data file " + standardVersionFilePath + " violates constraints of schema " + standardVersionsSchemaPath);
      }

      std::ifstream standardVersionFile(standardVersionFilePath);

      json version_doc = json::parse(standardVersionFile);
      std::vector<double> versions = version_doc["versions"];

      for (const auto &version : versions) {
        std::ostringstream oss;
        oss << dir.string() << '/' << std::setprecision(2) << std::fixed << version << "/data.json";
        auto standardDataFilePath = oss.str();
        if (!JsonValidationUtils::validateFile(standardDataSchema, standardDataFilePath, errors)) {
          logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
          throw JsonValidationUtils::SchemaViolationException("Data file " + standardDataFilePath + " violates constraints of schema " + standardDataSchemaPath);
        }
      }
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validatePackagesData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("package.json");
    JsonValidationUtils::populateSchema(
      schema,
      schemaPath,
      [&schemaDir = m_schemaDir](const std::string &uri) -> const json* {
        json *doc = new json();
        std::string path = schemaDir + '/' + uri;
        if (!valijson::utils::loadDocument(path, *doc)) {
          return nullptr;
        }
        return doc;
      }
    );

    std::vector<std::filesystem::path> dirs;
    for (const auto &item : std::filesystem::directory_iterator(getTmpCachePath("packages/id"))) {
      if (item.is_directory()) {
        dirs.emplace_back(item.path());
      }
    }

    valijson::ValidationResults errors;
    for (const auto &dir : dirs) {
      auto filePath = dir.string() + "/data.json";
      if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
        logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
        throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
      }
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::validateQuantitiesData() {
    TRC_FUNCTION_ENTER("");

    valijson::Schema schema;
    auto schemaPath = getSchemaFilePath("quantities.json");
    auto filePath = getTmpCacheDataFilePath("quantities");
    JsonValidationUtils::populateSchema(schema, schemaPath);
    valijson::ValidationResults errors;
    if (!JsonValidationUtils::validateFile(schema, filePath, errors)) {
      logSchemaViolations(JsonValidationUtils::getSchemaViolations(errors));
      throw JsonValidationUtils::SchemaViolationException("Data file " + filePath + " violates constraints of schema " + schemaPath);
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::deleteCache() {
    TRC_FUNCTION_ENTER("");

    try {
      std::filesystem::remove_all(m_cacheDir);
    } catch (const std::filesystem::filesystem_error &e) {
      CATCH_EXC_TRC_WAR(std::filesystem::filesystem_error, e, "[IQRF Repository cache] Failed to delete cache: " << e.what());
      std::cerr << "[IQRF Repository cache] Failed to delete cache: " << e.what() << std::endl;
    }

    TRC_FUNCTION_LEAVE("");
  }

  void JsCache::loadCache(bool firstLoad) {
    TRC_FUNCTION_ENTER("");

    std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

    auto serverState = m_serverState;
    auto companyMap = m_companyMap;
    auto manufacturerMap = m_manufacturerMap;
    auto productMap = m_productMap;
    auto osDpaMap = m_osDpaMap;
    auto standardMap = m_standardMap;
    auto packageMap = m_packageMap;
    auto quantityMap = m_quantityMap;
    try {
      TRC_INFORMATION("[IQRF Repository cache] Loading cache ... ");
      std::cout << "[IQRF Repository cache] Loading cache ... " << std::endl;

      updateCacheServer();
      updateCacheCompanies();
      updateCacheManufacturers();
      updateCacheProducts();
      updateCacheOsDpa();
      updateCacheStandards();
      updateCachePackages();
      updateCacheQuantities();

      m_upToDate = true;
      m_cacheStatus = CacheStatus::UPDATED;
      TRC_INFORMATION("[IQRF Repository cache] Cache successfully loaded.");
      std::cout << "[IQRF Repository cache] Cache successfully loaded." << std::endl;

      // invoke call back
      {
        std::lock_guard<std::recursive_mutex> lck(m_updateMtx);
        for (auto &hndlIt : m_cacheReloadedHndlMap) {
          if (hndlIt.second) {
            hndlIt.second();
          }
        }
      }
    } catch (std::exception &e) {
      CATCH_EXC_TRC_WAR(std::logic_error, e, "[IQRF Repository cache] Loading cache failed: " << e.what());
      std::cerr << "[IQRF Repository cache] Loading IqrfRepo cache failed: " << e.what() << std::endl;
      m_cacheStatus = CacheStatus::UPDATE_FAILED;
      m_cacheUpdateError = e.what();
      m_serverState = serverState;
      m_companyMap = companyMap;
      m_manufacturerMap = manufacturerMap;
      m_productMap = productMap;
      m_osDpaMap = osDpaMap;
      m_standardMap = standardMap;
      m_packageMap = packageMap;
      m_quantityMap = quantityMap;
      if (firstLoad) {
        THROW_EXC_TRC_WAR(std::logic_error, "[IQRF Repository cache] Failed to initialize cache.");
      }
    }

    TRC_FUNCTION_LEAVE("")
  }

  void JsCache::worker() {
    TRC_FUNCTION_ENTER("");

    while (m_cacheUpdateFlag) {
      std::unique_lock<std::mutex> lock(m_cacheUpdateMtx);
      if (m_checkPeriodInMinutes > 0) {
        TRC_INFORMATION("Periodic cache update: " << PAR(m_checkPeriodInMinutes));
        m_cacheUpdateCv.wait_for(lock, std::chrono::minutes(static_cast<uint8_t>(m_checkPeriodInMinutes)));
      } else {
        TRC_DEBUG("Periodic cache update not scheduled.");
        m_cacheUpdateCv.wait(lock);
      }

      if (!m_cacheUpdateFlag) {
        continue;
      }

      bool invoked = m_invoked;
      m_cacheStatus = CacheStatus::PENDING;
      m_cacheUpdateError = "ok";

      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
        try {
          checkCache();
          if (invoked) {
            // wake up invoking thread
            m_invokeWorkerCv.notify_all();
            // wait for invoking thread to collect cache status
            m_cacheUpdateCv.wait(lock);
          }
          if (!m_upToDate) {
            downloadCache();
            loadCache();
            if (invoked) {
              // wake up invoking thread
              m_invokeWorkerCv.notify_all();
              // wait for invoking thread to collect cache status after update
              m_cacheUpdateCv.wait(lock);
            }
          }
        } catch (std::exception &e) {
          CATCH_EXC_TRC_WAR(std::logic_error, e, std::endl << "Iqrf Repo download failure ... next attempt in " << m_checkPeriodInMinutes << " minutes");
          std::cerr << e.what() << std::endl << "Iqrf Repo download failure ... next attempt in " << m_checkPeriodInMinutes << " minutes" << std::endl;
          m_cacheStatus = CacheStatus::UPDATE_FAILED;
          m_cacheUpdateError = e.what();
          if (invoked) {
            m_invokeWorkerCv.notify_all();
            m_cacheUpdateCv.wait(lock);
          }
        }
        m_exclusiveAccess.reset();
      } catch (const std::exception &e) {
        std::string errorStr("Exclusive access unavailable, cache update cancelled.");
        CATCH_EXC_TRC_WAR(std::exception, e, errorStr);
        m_cacheStatus = CacheStatus::UPDATE_FAILED;
        m_cacheUpdateError = errorStr;
        if (invoked) {
          m_invokeWorkerCv.notify_all();
          m_cacheUpdateCv.wait(lock);
        }
      }
    }

    TRC_FUNCTION_LEAVE("");
  }
}
