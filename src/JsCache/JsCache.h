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
#pragma once

#include <nlohmann/json.hpp>
#include "IJsCacheService.h"
#include "IJsRenderService.h"
#include "ISchedulerService.h"
#include "ILaunchService.h"
#include "IRestApiService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include "ShapeProperties.h"
#include <string>

namespace iqrf {

  class JsCache : public IJsCacheService {
  public:
    /**
     * Constructor
     */
    JsCache();

    /**
     * Destructor
     */
    virtual ~JsCache();

    /**
     * Initializes component
     * @param props Component configuration
     */
    void activate(const shape::Properties *props = 0);
    /**
     * Modifies component properties
     * @param props Component configuration
     */
    void modify(const shape::Properties *props);

    /**
     * Terminates component
     */
    void deactivate();

    /**
     * Attaches DPA service interface
     * @param iface DPA service interface
     */
    void attachInterface(iqrf::IIqrfDpaService* iface);

    /**
     * Detaches DPA service interface
     * @param iface DPA service interface
     */
    void detachInterface(iqrf::IIqrfDpaService* iface);

    /**
     * Attaches JS render service interface
     * @param iface JS render service interface
     */
    void attachInterface(iqrf::IJsRenderService* iface);

    /**
     * Detaches JS render service interface
     * @param iface JS render service interface
     */
    void detachInterface(iqrf::IJsRenderService* iface);

    /**
     * Attaches scheduler service interface
     * @param iface Scheduler service interface
     */
    void attachInterface(iqrf::ISchedulerService* iface);

    /**
     * Detaches scheduler service interface
     * @param iface Scheduler service interface
     */
    void detachInterface(iqrf::ISchedulerService* iface);

    /**
     * Attaches launch service interface
     * @param iface Launch service interface
     */
    void attachInterface(shape::ILaunchService* iface);

    /**
     * Detaches launch service interface
     * @param iface Launch service interface
     */
    void detachInterface(shape::ILaunchService* iface);

    /**
     * Attaches REST API service interface
     * @param iface REST API service interface
     */
    void attachInterface(shape::IRestApiService* iface);

    /**
     * Detaches REST API service interface
     * @param iface REST API service interface
     */
    void detachInterface(shape::IRestApiService* iface);

    /**
     * Attaches tracing service interface
     * @param iface Tracing service interface
     */
    void attachInterface(shape::ITraceService* iface);

    /**
     * Detaches tracing service interface
     * @param iface Tracing service interface
     */
    void detachInterface(shape::ITraceService* iface);

    /**
     * Returns driver by ID and driver version
     * @param id Driver (standard) ID
     * @param ver Driver version
     * @return std::shared_ptr<StdDriver> Driver
     */
    std::shared_ptr<StdDriver> getDriver(int id, double ver) const override;

    /**
     * Return latest driver by ID
     * @param id Driver (standard) ID
     * @return std::shared_ptr<StdDriver>
     */
    std::shared_ptr<StdDriver> getLatestDriver(int id) const override;

    /**
     * Returns manufacturer by product HWPID
     * @param hwpid Product HWPID
     * @return std::shared_ptr<Manufacturer> Manufacturer
     */
    std::shared_ptr<Manufacturer> getManufacturer(uint16_t hwpid) const override;

    /**
     * FReturns product by product HWPID
     * @param hwpid Product HWPID
     * @return std::shared_ptr<Product> Product
     */
    std::shared_ptr<Product> getProduct(uint16_t hwpid) const override;

    /**
     * Returns product package by HWPID, string OS and string DPA
     * @param hwpid HWPID
     * @param hwpidVer HWPID version
     * @param os OS string
     * @param dpa DPA string
     * @return std::shared_ptr<Package> Package
     */
    std::shared_ptr<Package> getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const override;

    /**
     * Returns product package by HWPID, OS and DPA
     * @param hwpid HWPID
     * @param hwpidVer HWPID version
     * @param os OS
     * @param dpa DPA
     * @return std::shared_ptr<Package> Package
     */
    std::shared_ptr<Package> getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const override;

    /**
     * Returns driver combinations by string OS and DPA
     * @param os OS string
     * @param dpa DPA string
     * @return std::map<int, std::map<double, std::vector<std::pair<int, int>>>> Drivers (id, version, hwpid, hwpid version)
     */
    std::map<int, std::map<double, std::vector<std::pair<int, int>>>> getDrivers(const std::string& os, const std::string& dpa) const override;

    /**
     * Returns product drivers by string OS and DPA
     * @param os OS string
     * @param dpa DPA string
     * @return std::map<int, std::map<int, std::string>> Product drivers (hwpid, hwpid version, driver)
     */
    std::map<int, std::map<int, std::string>> getCustomDrivers(const std::string& os, const std::string& dpa) const override;

    /**
     * Returns OS and DPA combinations
     * @return MapOsListDpa OS and DPA combinations
     */
    MapOsListDpa getOsDpa() const override;

    /**
     * Returns OS DPA by id
     * @param id OS DPA ID
     * @return std::shared_ptr<OsDpa> OS DPA
     */
    std::shared_ptr<OsDpa> getOsDpa(int id) const override;

    /**
     * Returns OS DPA by string OS and DPA
     * @param os OS string
     * @param dpa DPA string
     * @return std::shared_ptr<OsDpa> OS DPA
     */
    std::shared_ptr<OsDpa> getOsDpa(const std::string& os, const std::string& dpa) const override;

    /**
     * Returns quantity by sensor type (ID)
     * @param type Sensor type
     * @return std::shared_ptr<Quantity> Quantity
     */
    std::shared_ptr<Quantity> getQuantity(const uint8_t &type) const override;

    /**
     * Returns server state
     * @return IJsCacheService::ServerState Server state
     */
    IJsCacheService::ServerState getServerState() const override;

    /**
     * Invokes cache update worker and returns cache status
     * @return std::tuple<CacheStatus, std::string> Cache status
     */
    std::tuple<CacheStatus, std::string> invokeWorker() override;

    /**
     * Registers cache reload handler function
     * @param clientId Component ID
     * @param hndl Handler function
     */
    void registerCacheReloadedHandler(const std::string &clientId, CacheReloadedFunc hndl) override;

    /**
     * Unregisters cache reload handler
     * @param clientId Component ID
     */
    void unregisterCacheReloadedHandler(const std::string &clientId) override;

  private:
    /**
     * Returns cache server state from file
     * @param fileName Server state file
     * @return ServerState Server state
     */
    ServerState getCacheServer(const std::string &fileName);

    /**
     * Get standard driver
     * @param standardId Standard ID
     * @param version Driver version
     * @return std::shared_ptr<StdDriver> Standard driver
     */
    std::shared_ptr<StdDriver> getStandard(int standardId, double version);

    /**
     * Parses and stores cache server state
     */
    void updateCacheServer();

    /**
     * Parses and stores cache companies
     */
    void updateCacheCompanies();

    /**
     * Parses and stores cache manufacturers
     */
    void updateCacheManufacturers();

    /**
     * Parses and stores cache products
     */
    void updateCacheProducts();

    /**
     * Parses and stores cache OS DPA
     */
    void updateCacheOsDpa();

    /**
     * Parses and stores cache standards
     */
    void updateCacheStandards();

    /**
     * Parses and stores cache packages
     */
    void updateCachePackages();

    /**
     * Parses and stores cache quantities
     */
    void updateCacheQuantities();

    /**
     * Returns absolute path to cache directory
     * @param path Directory name
     * @return std::string Absolute path to cache directory
     */
    std::string getCachePath(const std::string &path);

    /**
     * Returns absolute path to cache directory data file
     * @param relativeDir Directory name
     * @return std::string Absolute path to cache directory data file
     */
    std::string getCacheDataFilePath(const std::string &relativeDir);

    /**
     * Returns absolute repository URL from relative URL
     * @param relativeUrl Relative URL
     * @return std::string Absolute repository URL
     */
    std::string getAbsoluteUrl(const std::string &relativeUrl);

    /**
     * Creates file from path
     * @param path Path to file
     */
    void createFile(const std::string &path);

    /**
     * Downloads file from absolute URL
     * @param url File URL
     * @param urlFname
     */
    void downloadFromAbsoluteUrl(const std::string &url, const std::string &fileName);

    /**
     * Downloads file from relative URL
     * @param url File URL
     * @param urlFname
     */
    void downloadFromRelativeUrl(const std::string &url, const std::string &fileName);

    /**
     * Checks if cache data exists in filesystem
     * @return true
     * @return false
     */
    bool cacheExists();

    /**
     * Checks if local and remote cache match and updates cache status
     */
    void checkCache();

    /**
     * Downloads remote cache
     */
    void downloadCache();

    /**
     * Deletes repository cache
     */
    void deleteCache();

    /**
     * Loads cache contents from cache directory
     */
    void loadCache(bool firstLoad = false);

    /**
     * Cache update worker
     */
    void worker();

    /// Component name
    std::string m_name = "JsCache";
    /// DPA service
    iqrf::IIqrfDpaService *m_iIqrfDpaService = nullptr;
    /// JS render service
    iqrf::IJsRenderService *m_iJsRenderService = nullptr;
    /// Scheduler service
    iqrf::ISchedulerService *m_iSchedulerService = nullptr;
    /// Launch service
    shape::ILaunchService *m_iLaunchService = nullptr;
    /// REST API service
    shape::IRestApiService *m_iRestApiService = nullptr;
    /// Cache update mutex
    mutable std::recursive_mutex m_updateMtx;
    /// Path to cache root directory
    std::string m_cacheDir = "";
    /// Repository URL
    std::string m_urlRepo = "https://repository.iqrfalliance.org/api";
    /// Cache repository directory
    std::string m_iqrfRepoCache = "iqrfRepoCache";
    /// Download cache if cache is empty
    bool m_downloadIfRepoCacheEmpty = false;
    /// Cache update thread
    std::thread m_cacheUpdateThread;
    /// Cache update thread variable
    bool m_cacheUpdateFlag = false;
    /// Cache update check period
    double m_checkPeriodInMinutes = 0;
    /// Minimum cache update check period
    const double m_checkPeriodInMinutesMin = 1;
    /// Cache update mutex
    std::mutex m_cacheUpdateMtx;
    /// Cache update condition variable
    std::condition_variable m_cacheUpdateCv;
    /// Update worker invocation condition variable
    std::condition_variable m_invokeWorkerCv;
    /// Indicates that cache update worker was invoked manually
    bool m_invoked = false;
    /// Cache status (up to date / needs update / update failed)
    CacheStatus m_cacheStatus = CacheStatus::UPDATE_NEEDED;
    /// Cache update error
    std::string m_cacheUpdateError;
    /// Cache up to date local
    bool m_upToDate = false;
    /// Cache reload handlers map
    std::map<std::string, CacheReloadedFunc> m_cacheReloadedHndlMap;
    /// Exclusive access
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    /// Server state file
    std::string m_serverStateFilePath;
    /// Server state (repository)
    ServerState m_serverState;
    /// Companies
    std::map<unsigned int, Company> m_companyMap;
    /// Manufacturers
    std::map<unsigned int, Manufacturer> m_manufacturerMap;
    /// Products
    std::map<uint16_t, Product> m_productMap;
    /// OS DPA combinations
    std::map<unsigned int, OsDpa> m_osDpaMap;
    /// Packages
    std::map<unsigned int, Package> m_packageMap;
    /// Standards
    std::map<int, StdItem> m_standardMap;
    /// Quantities
    std::map<uint8_t, Quantity> m_quantityMap;
  };
}
