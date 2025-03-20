/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
    void reinitializeCoordinator() override;

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
    mutable std::recursive_mutex m_exclusiveAccessMutex;
    IDpaHandler2* m_dpaHandler = nullptr;
    IDpaTransaction2::RfMode m_rfMode = IDpaTransaction2::RfMode::kStd;
    int m_dpaHandlerTimeout = IDpaTransaction2::DEFAULT_TIMEOUT;
    int m_bondedNodes = 10;
    int m_discoveredNodes = 10;
    IDpaTransaction2::FrcResponseTime m_responseTime = IDpaTransaction2::FrcResponseTime::k40Ms;

    void initializeCoordinator();

    std::mutex m_asyncMessageHandlersMutex;
    std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;
    void asyncDpaMessageHandler(const DpaMessage& dpaMessage);

    std::mutex m_asyncRestartMtx;
    std::condition_variable m_asyncRestartCv;

    void asyncRestartHandler(const DpaMessage& dpaMessage);
    void getIqrfNetworkParams();

    /// Coordinator parameters
    IIqrfDpaService::CoordinatorParameters m_cPar;
    IIqrfDpaService::DpaState state = IIqrfDpaService::DpaState::Ready;
  };
}
