#pragma once

#include "ILaunchService.h"
#include "IJsCacheService.h"
#include "ISchedulerService.h"
#include "IRestApiService.h"
#include "ITraceService.h"
#include "ShapeProperties.h"
#include <string>

namespace iqrf {

  class JsCache : public IJsCacheService
  {
  public:
    JsCache();
    virtual ~JsCache();

    const std::string& getDriver(int id, int ver) const override;
    const std::map<int, const IJsCacheService::StdDriver*> getAllLatestDrivers() const override;
    const IJsCacheService::Manufacturer* getManufacturer(uint16_t hwpid) const override;
    const IJsCacheService::Product* getProduct(uint16_t hwpid) const override;
    const IJsCacheService::Package* getPackage(uint16_t hwpid, const std::string& os, const std::string& dpa) const override;
    IJsCacheService::ServerState getServerState() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(iqrf::ISchedulerService* iface);
    void detachInterface(iqrf::ISchedulerService* iface);

    void attachInterface(shape::IRestApiService* iface);
    void detachInterface(shape::IRestApiService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp = nullptr;
  };
}
