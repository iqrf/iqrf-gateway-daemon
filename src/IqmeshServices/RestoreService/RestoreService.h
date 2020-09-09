#pragma once

#include "IRestoreService.h"
#include "IIqrfRestore.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {

  class RestoreService: public IRestoreService
  {
  public:
    RestoreService();
    virtual ~RestoreService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfRestore* iface);
    void detachInterface(IIqrfRestore* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
