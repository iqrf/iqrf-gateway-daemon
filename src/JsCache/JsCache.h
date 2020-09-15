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

    const StdDriver* getDriver(int id, double ver) const override;
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
