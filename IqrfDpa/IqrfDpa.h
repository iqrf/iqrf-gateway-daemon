#pragma once

#include "IIqrfDpaService.h"
#include "IqrfDpaChannel.h"
#include "DpaHandler.h"
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

    void executeDpaTransaction(DpaTransaction& dpaTransaction) override;
    void killDpaTransaction() override;
    int getTimeout() const override;
    void setTimeout(int timeout) override;
    RfMode getRfCommunicationMode() const override;
    void setRfCommunicationMode(RfMode rfMode) override;
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
    DpaHandler* m_dpaHandler = nullptr;
    RfMode m_rfMode = RfMode::Std;
    int m_dpaHandlerTimeout = 200;

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
