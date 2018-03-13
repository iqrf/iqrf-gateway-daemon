#pragma once

#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "IMessagingService.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsonDpaApi
  {
  public:
    JsonDpaApi();
    virtual ~JsonDpaApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfDpaService* iface);
    void detachInterface(IIqrfDpaService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
