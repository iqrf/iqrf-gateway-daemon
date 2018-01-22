#pragma once

#include "IDaemonControllerService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class DaemonController : public IDaemonControllerService
  {
  public:
    DaemonController();
    virtual ~DaemonController();
    std::string doService(const std::string & str) const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
  };
}
