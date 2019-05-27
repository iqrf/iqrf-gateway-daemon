#pragma once

#include "IIqrfDpaService.h"
#include "IqrfDpaChannel.h"
#include "IDpaHandler2.h"
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

    std::unique_ptr<ExclusiveAccess> getExclusiveAccess() override;
    std::shared_ptr<IDpaTransaction2> executeExclusiveDpaTransaction(const DpaMessage& request, int32_t timeout);
    std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout) override;
    void executeDpaTransactionRepeat( DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout ) override;
    IIqrfDpaService::CoordinatorParameters getCoordinatorParameters() const override;
    int getTimeout() const override;
    void setTimeout(int timeout) override;
    IDpaTransaction2::RfMode getRfCommunicationMode() const override;
    void setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) override;
    IDpaTransaction2::TimingParams getTimingParams() const override;
    void setTimingParams( IDpaTransaction2::TimingParams params ) override;
    IDpaTransaction2::FrcResponseTime getFrcResponseTime() const override;
    void setFrcResponseTime( IDpaTransaction2::FrcResponseTime frcResponseTime ) override;
    void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) override;
    void unregisterAsyncMessageHandler(const std::string& serviceId) override;
    
    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfChannelService* iface);
    void detachInterface(IIqrfChannelService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void setExclusiveAccess();
    void resetExclusiveAccess();
  private:
    IIqrfChannelService* m_iqrfChannelService = nullptr;
    IqrfDpaChannel *m_iqrfDpaChannel = nullptr;  //temporary workaround, see comment in IqrfDpaChannel.h
    std::recursive_mutex m_exclusiveAccessMutex;
    IDpaHandler2* m_dpaHandler = nullptr;
    IDpaTransaction2::RfMode m_rfMode = IDpaTransaction2::RfMode::kStd;
    int m_dpaHandlerTimeout = IDpaTransaction2::DEFAULT_TIMEOUT;
    int m_bondedNodes = 10;
    int m_discoveredNodes = 10;
    IDpaTransaction2::FrcResponseTime m_responseTime = IDpaTransaction2::FrcResponseTime::k40Ms;

    std::mutex m_asyncMessageHandlersMutex;
    std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;
    void asyncDpaMessageHandler(const DpaMessage& dpaMessage);

    std::mutex m_asyncRestartMtx;
    std::condition_variable m_asyncRestartCv;

    void asyncRestartHandler(const DpaMessage& dpaMessage);
    void getIqrfNetworkParams();

    /// Coordinator parameters
    IIqrfDpaService::CoordinatorParameters m_cPar;
  };
}
