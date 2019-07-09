#pragma once

#include "IDpaTransactionResult2.h"
#include "IIqrfChannelService.h"

namespace iqrf {

  class IMonitorService
  {
  public:
    virtual int getDpaQueueLen() const = 0;
    virtual IIqrfChannelService::State getIqrfChannelState() = 0;

    virtual ~IMonitorService() {}
  };
}
