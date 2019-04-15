#pragma once

#include "IMetaDataApi.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {
  class JsonMngMetaDataApi : public IMetaDataApi
  {
  public:
    JsonMngMetaDataApi();
    virtual ~JsonMngMetaDataApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    bool iSmetaDataToMessages() const override;
    rapidjson::Document getMetaData(uint16_t nAdr) const override;

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}