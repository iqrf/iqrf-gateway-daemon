#pragma once

#include "IIqrfDpaService.h"
#include "IqrfDpaChannel.h"
#include "DpaHandler2.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

#include <string>
#include <mutex>
#include <map>

namespace iqrf {
  class IqrfDpa : public IIqrfDpaService
  {
  public:
    IqrfDpa();
    virtual ~IqrfDpa();

    std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout) override;
    int getTimeout() const override;
    void setTimeout(int timeout) override;
    DpaHandler2::RfMode getRfCommunicationMode() const override;
    void setRfCommunicationMode(DpaHandler2::RfMode rfMode) override;
    //void registerAsyncMessageHandler(std::function<void(const DpaMessage&)> messageHandler) override;
    //void unregisterAsyncMessageHandler(const std::string& serviceId) override;
    void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) override;
    void unregisterAsyncMessageHandler(const std::string& serviceId) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfChannelService* iface);
    void detachInterface(IIqrfChannelService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    IIqrfChannelService* m_iqrfChannelService = nullptr;
    IqrfDpaChannel *m_iqrfDpaChannel = nullptr;  //temporary workaround, see comment in IqrfDpaChannel.h
    IDpaHandler2* m_dpaHandler = nullptr;
    //DpaHandler2::RfMode m_rfMode = DpaHandler2::RfMode::kStd;
    //int m_dpaHandlerTimeout = 200;

    std::mutex m_asyncMessageHandlersMutex;
    std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;
    void asyncDpaMessageHandler(const DpaMessage& dpaMessage);

    /// TR module
    std::string m_moduleId;
    std::string m_osVersion;
    std::string m_trType;
    bool m_fcc = false;
    std::string m_mcuType;
    std::string m_osBuild;
  };
}
