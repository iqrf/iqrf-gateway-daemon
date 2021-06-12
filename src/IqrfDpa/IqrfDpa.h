#pragma once

#include "IIqrfDpaService.h"
#include "IqrfDpaChannel.h"
#include "IDpaHandler2.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <iostream>

namespace iqrf {
  class IqrfDpa : public IIqrfDpaService
  {
  public:
    IqrfDpa();
    virtual ~IqrfDpa();

    std::unique_ptr<ExclusiveAccess> getExclusiveAccess() override;
    bool hasExclusiveAccess() const override;
    std::shared_ptr<IDpaTransaction2> executeExclusiveDpaTransaction(const DpaMessage& request, int32_t timeout);
    std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout) override;
    void executeDpaTransactionRepeat( const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout ) override;
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
    int getDpaQueueLen() const override;
    IIqrfChannelService::State getIqrfChannelState() override;
    IIqrfDpaService::DpaState getDpaChannelState() override;
    void registerAnyMessageHandler(const std::string& serviceId, AnyMessageHandlerFunc fun) override;
    void unregisterAnyMessageHandler(const std::string& serviceId) override;

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
    /**
     * Async DPA message handler
     * @param dpaMessage DPA message
     */
    void asyncDpaMessageHandler(const DpaMessage& dpaMessage);

    /**
     * Asynch restart handler
     * @param dpaMessage DPA message
     */
    void asyncRestartHandler(const DpaMessage& dpaMessage);

    /**
     * Initializes IQRF interface
     */
    void initializeInterface();

    /**
     * Runs initialization thread
     */
    void runInitThread();

    void getIqrfNetworkParams();
    IIqrfChannelService* m_iqrfChannelService = nullptr;
    IqrfDpaChannel *m_iqrfDpaChannel = nullptr;  //temporary workaround, see comment in IqrfDpaChannel.h
    mutable std::recursive_mutex m_exclusiveAccessMutex;
    IDpaHandler2* m_dpaHandler = nullptr;
    IDpaTransaction2::RfMode m_rfMode = IDpaTransaction2::RfMode::kStd;
    int m_dpaHandlerTimeout = IDpaTransaction2::DEFAULT_TIMEOUT;
    int m_bondedNodes = 10;
    int m_discoveredNodes = 10;
    IDpaTransaction2::FrcResponseTime m_responseTime = IDpaTransaction2::FrcResponseTime::k40Ms;

    /// Async message handler mutex
    std::mutex m_asyncMessageHandlersMutex;
    /// Map of async message handlers
    std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;
    /// Async restart mutex
    std::mutex m_asyncRestartMtx;
    /// Async restart conditional variable
    std::condition_variable m_asyncRestartCv;
    /// Sleep duration in seconds when checking IQRF channel readiness
    uint8_t m_interfaceCheckPeriod;
    /// Initialization thread
    std::thread m_initThread;
    /// Coordinator parameters
    IIqrfDpaService::CoordinatorParameters m_cPar;
    /// DPA channel state
    IIqrfDpaService::DpaState state = IIqrfDpaService::DpaState::NotReady;
  };
}
