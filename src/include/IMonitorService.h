#pragma once

#include "IDpaTransactionResult2.h"
#include "IIqrfChannelService.h"
#include "IIqrfDpaService.h"

namespace iqrf {

  class IMonitorService
  {
  public:
    virtual int getDpaQueueLen() const = 0;
    virtual IIqrfChannelService::State getIqrfChannelState() = 0;
    virtual IIqrfDpaService::DpaState getDpaChannelState() = 0;

    virtual ~IMonitorService() {}
  };
}
