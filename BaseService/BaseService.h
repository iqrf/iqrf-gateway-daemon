#pragma once

#include "IBaseService.h"
#include "ShapeProperties.h"
#include "IMessagingService.h"
#include "IJsonSerializerService.h"
#include "IIqrfDpaService.h"
#include "ISchedulerService.h"
#include "ITraceService.h"
#include <string>

class DpaMessage;

namespace iqrf {

  class BaseService : public IBaseService
  {
  public:
    BaseService();
    virtual ~BaseService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IMessagingService* iface);
    void detachInterface(iqrf::IMessagingService* iface);

    void attachInterface(iqrf::IJsonSerializerService* iface);
    void detachInterface(iqrf::IJsonSerializerService* iface);

    void attachInterface(iqrfgw::IIqrfDpaService* iface);
    void detachInterface(iqrfgw::IIqrfDpaService* iface);

    void attachInterface(iqrfgw::ISchedulerService* iface);
    void detachInterface(iqrfgw::ISchedulerService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    void handleMsgFromMessaging(const std::basic_string<uint8_t> & msg);
    void handleAsyncDpaMessage(const DpaMessage& dpaMessage);

    IMessagingService* m_messaging = nullptr;
    IJsonSerializerService* m_serializer = nullptr;
    iqrfgw::IIqrfDpaService* m_dpa = nullptr;
    iqrfgw::ISchedulerService* m_scheduler = nullptr;
    std::string m_name;
    //IMessaging* m_messaging;
    //IDaemon* m_daemon;
    //std::vector<ISerializer*> m_serializerVect;
    bool m_asyncDpaMessage = false;

  };
}
