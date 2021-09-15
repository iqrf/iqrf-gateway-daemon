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
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace iqrf {
	class IqrfDpa : public IIqrfDpaService {
	public:
		/**
		 * Constructor
		 */
		IqrfDpa();

		/**
		 * Destructor
		 */
		virtual ~IqrfDpa();

		std::unique_ptr<ExclusiveAccess> getExclusiveAccess() override;
		bool hasExclusiveAccess() const override;
		std::shared_ptr<IDpaTransaction2> executeExclusiveDpaTransaction(const DpaMessage &request, int32_t timeout);
		std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage &request, int32_t timeout) override;
		void executeDpaTransactionRepeat(const DpaMessage &request, std::unique_ptr<IDpaTransactionResult2> &result, int repeat, int32_t timeout) override;
		IIqrfDpaService::CoordinatorParameters getCoordinatorParameters() const override;
		int getTimeout() const override;
		void setTimeout(int timeout) override;
		IDpaTransaction2::RfMode getRfCommunicationMode() const override;
		void setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) override;
		IDpaTransaction2::TimingParams getTimingParams() const override;
		void setTimingParams(IDpaTransaction2::TimingParams params) override;
		IDpaTransaction2::FrcResponseTime getFrcResponseTime() const override;
		void setFrcResponseTime(IDpaTransaction2::FrcResponseTime frcResponseTime) override;
		void registerAsyncMessageHandler(const std::string &serviceId, AsyncMessageHandlerFunc fun) override;
		void unregisterAsyncMessageHandler(const std::string &serviceId) override;
		int getDpaQueueLen() const override;
		IIqrfChannelService::State getIqrfChannelState() override;
		IIqrfDpaService::DpaState getDpaChannelState() override;
		void registerAnyMessageHandler(const std::string &serviceId, AnyMessageHandlerFunc fun) override;
		void unregisterAnyMessageHandler(const std::string &serviceId) override;
		void reloadCoordinator(const IIqrfDpaService::CoordinatorParameters &params) override;

		/**
		 * Registers js driver reload callback
		 * @param serviceId ID of service registering callback
		 * @param handler Callback
		 */
		void registerDriverReloadHandler(const std::string &serviceId, DriverReloadHandler handler) override;

		/**
		 * Unregisters js driver reload callback
		 * @param serviceId ID of service registering callback
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
		 * Handles asynchronous dpa message
		 * @param dpaMessage DPA message
		 */
		void asyncDpaMessageHandler(const DpaMessage &dpaMessage);

		/**
		 * Handles asynchronous restart message
		 * @param dpaMessage DPA message
		 */
		void asyncRestartHandler(const DpaMessage &dpaMessage);

		/**
		 * Sets default TR and network parametersto initialize Daemon API
		 */
		void setDefaults();

		/**
		 * Sets up interface initialization thread
		 */
		void startInterface();

		/**
		 * Interface initialization exection
		 */
		void startInterfaceThread();

		/**
		 * Attempts to identify connected coordinator device
		 */
		void identifyCoordinator();

		/**
		 * Logs information about network
		 * @param updated Indicates whether logging updated or default coordinator parameters
		 */
		void logNetworkParams(bool updated);

		/**
		 * Logs information about transciever
		 * @param updated Indicates whether logging updated or default coordinator parameters
		 */
		void logTranscieverParams(bool updated);

		/// IQRF channel service
		IIqrfChannelService *m_iqrfChannelService = nullptr;
		/// DPA channel
		IqrfDpaChannel *m_iqrfDpaChannel = nullptr; //temporary workaround, see comment in IqrfDpaChannel.h
		/// DPA handler
		IDpaHandler2 *m_dpaHandler = nullptr;
		/// Exclusive access mutex
		mutable std::recursive_mutex m_exclusiveAccessMutex;
		/// Async message mutex
		std::mutex m_asyncMessageHandlersMutex;
		/// Async restart mutex
		std::mutex m_asyncRestartMtx;
		/// Async restart condition varible
		std::condition_variable m_asyncRestartCv;
		/// Map of async handlers
		std::map<std::string, AsyncMessageHandlerFunc> m_asyncMessageHandlers;

		///// Interface initialization and maintenance
		/// Interface type
		IIqrfChannelService::InterfaceType m_ifaceType;
		/// Async reset wait time
		const uint16_t m_resetWaitTime = 3000;
		/// Interface maintaining condition
		std::atomic_bool m_runInterface;
		/// Interface initialization mutex
		std::mutex m_startInterfaceMutex;
		/// Interface initialization condition variable
		std::condition_variable m_startInterfaceCv;
		/// Interface initialization thread
		std::thread m_startInterfaceThread;
		
		///// Network
		/// Coordinator parameters
		IIqrfDpaService::CoordinatorParameters m_coordinatorParams;
		/// DPA service state
		IIqrfDpaService::DpaState m_state = IIqrfDpaService::DpaState::Ready;
		/// Driver reload handler
		IIqrfDpaService::DriverReloadHandler m_driverReloadHandler = nullptr;
		/// DPA transaction timeout
		int m_dpaHandlerTimeout = IDpaTransaction2::DEFAULT_TIMEOUT;
		/// FRC response time
		IDpaTransaction2::FrcResponseTime m_responseTime = IDpaTransaction2::FrcResponseTime::k40Ms;
		/// RF mode
		IDpaTransaction2::RfMode m_rfMode = IDpaTransaction2::RfMode::kStd;
		/// Bonded nodes
		uint8_t m_bondedNodes = 10;
		/// Discovered nodes
		uint8_t m_discoveredNodes = 10;
	};
}
