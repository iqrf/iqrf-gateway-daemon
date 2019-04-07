#pragma once

#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "JsonSerializer.h"
#include "IIqrfDpaService.h"
#include "ISchedulerService.h"
#include "ITraceService.h"
#include <string>

class DpaMessage;

namespace iqrf {

  class LegacyApiSupport
  {
  public:
    LegacyApiSupport();
    virtual ~LegacyApiSupport();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::ISchedulerService* iface);
    void detachInterface(iqrf::ISchedulerService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    void handleMsgFromMessaging(const std::string & messagingId, const std::basic_string<uint8_t> & msg);
    void handleAsyncDpaMessage(const DpaMessage& dpaMessage);

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    JsonSerializer m_serializer;
    iqrf::IIqrfDpaService* m_dpa = nullptr;
    iqrf::ISchedulerService* m_scheduler = nullptr;
    std::string m_name;
    bool m_asyncDpaMessage = false;
    std::vector<std::string> m_filters =
    {
      "dpaV1"
    };

  };
}
