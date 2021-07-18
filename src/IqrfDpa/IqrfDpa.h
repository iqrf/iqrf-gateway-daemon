/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "IIqrfDpaService.h"
#include "IqrfDpaChannel.h"
#include "IDpaHandler2.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

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
    void reloadCoordinator() override;
    IIqrfDpaService::DpaState getDpaChannelState() override;
    void registerAnyMessageHandler(const std::string& serviceId, AnyMessageHandlerFunc fun) override;
    void unregisterAnyMessageHandler(const std::string& serviceId) override;

    /**
     * Registers js driver reload callback
     * @param serviceId ID of service registering callback
     * @param handler Callback
     */
    void registerDriverReloadHandler(const std::string &serviceId, DriverReloadHandler handler) override;

    /**
     * Unregisters js driver reload callback
     */
    void unregisterDriverReloadHandler(const std::string &serviceId) override;

    /**
     * Activates component instance
     * @param props Instance properties
     */
    void activate(const shape::Properties *props = 0);
    
    /**
     * Modifies component instance properties
     * @param props Instance properties
     */
    void modify(const shape::Properties *props);

    /**
     * Deactivates component instance
     */
    void deactivate();

    /**
     * Attaches IQRF channel service interface
     * @param iface Interface to attach
     */
    void attachInterface(IIqrfChannelService* iface);

    /**
     * Detaches IQRF channel service interface
     * @param iface Interface to detach
     */
    void detachInterface(IIqrfChannelService* iface);

    /**
     * Attaches Shape tracing service interface
     * @param iface Interface to attach
     */
    void attachInterface(shape::ITraceService* iface);

    /**
     * Detaches Shape tracing service interface
     */
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
     * Sets default TR and network parameters to initialize Daemon API
     */
    void setDefaults();

    /**
     * Sets up interface initialization thread
     */
    void startInterface();

    /**
     * Sets up channel checking thread
     */
    void startChannelCheck();

    /**
     * Interface initialization thread routine
     */
    void interfaceInitializationThread();

    /**
     * Channel checking thread routine
     */
    void channelCheckThread();

    /**
     * Attempts to identify coordinator to retrieve TR and network parameters
     */
    void identifyCoordinator();

    /**
     * Logs transciever parameters
     * @param updated Indicates whether parameters are default or updated
     */
    void logTranscieverParams(bool updated);

    /**
     * Logs network parameters
     * @param updated Indicates whether parameters are default or updated
     */
    void logNetworkParams(bool updated);
    

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
    /// IQRF channel state checking thread
    std::thread m_channelStateThread;
    /// Coordinator parameters
    IIqrfDpaService::CoordinatorParameters m_coordinatorParams;
    /// DPA channel state
    IIqrfDpaService::DpaState m_state = IIqrfDpaService::DpaState::NotReady;
    /// Driver reload handler
    IIqrfDpaService::DriverReloadHandler m_driverReloadHandler = nullptr;
    /// Interface initialization mutex
    std::mutex m_initMutex;
    /// Interface initialization condition variable
    std::condition_variable m_initCv;
    /// IQRF channel state mutex
    std::mutex m_channelStateMutex;
    /// Termination condition
    std::atomic_bool m_runInterface;
  };
}
