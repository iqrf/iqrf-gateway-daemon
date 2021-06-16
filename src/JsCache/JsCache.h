/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

  class JsCache : public IJsCacheService
  {
  public:
    JsCache();
    virtual ~JsCache();

    StdDriver getDriver(int id, double ver) const override;
    IJsCacheService::Manufacturer getManufacturer(uint16_t hwpid) const override;
    IJsCacheService::Product getProduct(uint16_t hwpid) const override;
    IJsCacheService::Package getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const override;
    Package getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const override;
    std::map<int, std::map<double, std::vector<std::pair<int, int>>>> getDrivers(const std::string& os, const std::string& dpa) const override;
    std::map<int, std::map<int, std::string>> getCustomDrivers(const std::string& os, const std::string& dpa) const override;
    MapOsListDpa getOsDpa() const override;
    OsDpa getOsDpa(int id) const override;
    OsDpa getOsDpa(const std::string& os, const std::string& dpa) const override;
    IJsCacheService::ServerState getServerState() const override;

    void registerCacheReloadedHandler(const std::string & clientId, CacheReloadedFunc hndl) override;
    void unregisterCacheReloadedHandler(const std::string & clientId) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(iqrf::ISchedulerService* iface);
    void detachInterface(iqrf::ISchedulerService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::IRestApiService* iface);
    void detachInterface(shape::IRestApiService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp = nullptr;
  };
}
