#pragma once

#include "IJsRenderService.h"
#include "IIqrfDpaService.h"
#include "IIqrfInfo.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsonDpaApiIqrfStdExt
  {
  public:
    JsonDpaApiIqrfStdExt();
    virtual ~JsonDpaApiIqrfStdExt();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfInfo* iface);
    void detachInterface(IIqrfInfo* iface);

    void attachInterface(IJsRenderService* iface);
    void detachInterface(IJsRenderService* iface);

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
