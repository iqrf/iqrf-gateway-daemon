#pragma once

#include "ShapeProperties.h"
#include "IIqrfInfo.h"
#include "IJsRenderService.h"
#include "IJsCacheService.h"
#include "IIqrfDpaService.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {
  class IqrfInfo : public IIqrfInfo
  {
  public:
    IqrfInfo();
    virtual ~IqrfInfo();

    std::map<int, sensor::EnumeratePtr> getSensors() const override;
    std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const override;
    std::map<int, dali::EnumeratePtr> getDalis() const override;
    std::map<int, light::EnumeratePtr> getLights() const override;
    std::map<int, embed::node::BriefInfoPtr> getNodes() const override;
    void insertNodes(const std::map<int, embed::node::BriefInfo> & nodes) override;
    void startEnumeration() override;
    rapidjson::Document getNodeMetaData(int nadr) const override;
    void setNodeMetaData(int nadr, const rapidjson::Value & metaData) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
