#pragma once

#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsonMngMetaDataApi
  {
  public:
    JsonMngMetaDataApi();
    virtual ~JsonMngMetaDataApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

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
