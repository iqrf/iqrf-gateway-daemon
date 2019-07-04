#pragma once

#include "JsDriverRequest.h"
#include "IDpaTransactionResult2.h"
#include "IIqrfChannelService.h"

namespace iqrf {

  class IGwMonitorService
  {
  public:
    virtual int getDpaQueueLen() const = 0;
    virtual IIqrfChannelService::State getIqrfChannelState() = 0;

    virtual ~IGwMonitorService() {}
  };
}
