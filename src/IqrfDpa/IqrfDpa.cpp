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

#define IIqrfDpaService_EXPORTS

#include "EnumStringConvertor.h"
#include "DpaHandler2.h"
#include "IqrfDpa.h"
#include "RawDpaEmbedOS.h"
#include "RawDpaEmbedExplore.h"
#include "Trace.h"
#include "rapidjson/pointer.h"
#include "iqrf__IqrfDpa.hxx"
#include <thread>
#include <iostream>

TRC_INIT_MODULE(iqrf::IqrfDpa);

namespace iqrf
{

	class FrcResponseTimeConvertTable
	{
	public:
		static const std::vector<std::pair<IDpaTransaction2::FrcResponseTime, std::string>> &table()
		{
			static std::vector<std::pair<IDpaTransaction2::FrcResponseTime, std::string>> table = {
				{IDpaTransaction2::FrcResponseTime::k40Ms, "k40Ms"},
				{IDpaTransaction2::FrcResponseTime::k360Ms, "k360Ms"},
				{IDpaTransaction2::FrcResponseTime::k680Ms, "k680Ms"},
				{IDpaTransaction2::FrcResponseTime::k1320Ms, "k1320Ms"},
				{IDpaTransaction2::FrcResponseTime::k2600Ms, "k2600Ms"},
				{IDpaTransaction2::FrcResponseTime::k5160Ms, "k5160Ms"},
				{IDpaTransaction2::FrcResponseTime::k10280Ms, "k10280Ms"},
				{IDpaTransaction2::FrcResponseTime::k20620Ms, "k20620Ms"}};
			return table;
		}
		static IDpaTransaction2::FrcResponseTime defaultEnum()
		{
			return IDpaTransaction2::FrcResponseTime::k40Ms;
		}
		static const std::string &defaultStr()
		{
			static std::string u("unknown");
			return u;
		}
	};

	typedef shape::EnumStringConvertor<IDpaTransaction2::FrcResponseTime, FrcResponseTimeConvertTable> FrcResponseTimeStringConvertor;

	class ExclusiveAccessImpl : public IIqrfDpaService::ExclusiveAccess
	{
	public:
		ExclusiveAccessImpl() = delete;
		ExclusiveAccessImpl(IqrfDpa *iqrfDpa)
			: m_iqrfDpa(iqrfDpa)
		{
			m_iqrfDpa->setExclusiveAccess();
		}

		std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage &request, int32_t timeout = -1) override
		{
			TRC_FUNCTION_ENTER("");
			auto result = m_iqrfDpa->executeExclusiveDpaTransaction(request, timeout);
			TRC_FUNCTION_LEAVE("");
			return result;
		}

		void executeDpaTransactionRepeat(const DpaMessage &request, std::unique_ptr<IDpaTransactionResult2> &result, int repeat, int32_t timeout = -1) override
		{
			TRC_FUNCTION_ENTER("");
			m_iqrfDpa->executeDpaTransactionRepeat(request, result, repeat, timeout);
			TRC_FUNCTION_LEAVE("");
		}

		virtual ~ExclusiveAccessImpl()
		{
			m_iqrfDpa->resetExclusiveAccess();
		}

	private:
		IqrfDpa *m_iqrfDpa = nullptr;
	};

	IqrfDpa::IqrfDpa() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("")
	}

	IqrfDpa::~IqrfDpa() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("")
	}

	///// Component lifecycle

	void IqrfDpa::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IqrfDpa instance activate" << std::endl
			<< "******************************"
		);

		// create DPA handler
		m_dpaHandler = shape_new DpaHandler2(m_iqrfDpaChannel);

		// set component instance properties
		modify(props);

		// set default variables
		setDefaults();
		
		// register async message handler
		m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage &dpaMessage) {
			asyncDpaMessageHandler(dpaMessage);
		});

		// enable maintaining condition
		m_runInterface = true;
		startInterface();

		TRC_FUNCTION_LEAVE("")
	}

	void IqrfDpa::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		rapidjson::Document &doc = const_cast<rapidjson::Document&>(props->getAsJson());
		m_dpaHandlerTimeout = rapidjson::Pointer("/DpaHandlerTimeout").GetWithDefault(doc, IDpaTransaction2::DEFAULT_TIMEOUT).GetUint();
		m_dpaHandler->setTimeout(m_dpaHandlerTimeout);
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IqrfDpa instance deactivate" << std::endl
			<< "******************************"
		);

		m_iqrfDpaChannel->unregisterReceiveFromHandler();
		m_dpaHandler->unregisterAsyncMessageHandler("");

		delete m_dpaHandler;
		m_dpaHandler = nullptr;

		TRC_FUNCTION_LEAVE("")
	}

	///// Interface initialization and maintenance

	void IqrfDpa::startInterface() {
		TRC_FUNCTION_ENTER("");
		m_ifaceType = m_iqrfChannelService->getInterfaceType();
		if (m_ifaceType == IIqrfChannelService::InterfaceType::MQTT) {
			m_iqrfChannelService->startListen();
			TRC_INFORMATION("");
			return;
		}
		if (m_startInterfaceThread.joinable()) {
			m_startInterfaceThread.join();
			TRC_DEBUG("Initialization thread joined.");
		}
		m_startInterfaceThread = std::thread([this] { startInterfaceThread(); });
		pthread_setname_np(m_startInterfaceThread.native_handle(), "dpaInterface");
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::startInterfaceThread() {
		TRC_FUNCTION_ENTER("");
		registerAsyncMessageHandler("  IqrfDpa", [&](const DpaMessage &dpaMessage) {
			asyncRestartHandler(dpaMessage);
		});
		identifyCoordinator();
		unregisterAsyncMessageHandler("  IqrfDpa");
		IDpaTransaction2::TimingParams timingParams;
		timingParams.bondedNodes = m_bondedNodes;
		timingParams.discoveredNodes = m_discoveredNodes;
		timingParams.frcResponseTime = m_responseTime;
		timingParams.dpaVersion = m_coordinatorParams.dpaVerWord;
		timingParams.osVersion = m_coordinatorParams.osVersion;
		m_dpaHandler->setTimingParams(timingParams);
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::identifyCoordinator() {
		TRC_FUNCTION_ENTER("");

		m_iqrfChannelService->startListen();
		bool explicitRestartTimeout = false;
		while (true) {
			std::shared_ptr<IDpaTransaction2> result;
			std::unique_lock<std::mutex> lock(m_asyncRestartMtx);
			TRC_INFORMATION("Waiting for TR reset message for " << m_resetWaitTime << " ms.");
			if (m_asyncRestartCv.wait_for(lock, std::chrono::milliseconds(m_resetWaitTime)) == std::cv_status::timeout) {
				if (result != nullptr) {
					result->abort();
				}
				if (explicitRestartTimeout) {
					m_state = IIqrfDpaService::DpaState::NotReady;
					TRC_WARNING("Failed to initialize DPA interface.");
					std::cout << std::endl << "Failed to initialize DPA interface to coordinator.";
					return;
				}
				TRC_WARNING("TR reset message not received, sending explicit reset.");
				iqrf::embed::os::RawDpaRestart restartRequest(0);
				result = executeDpaTransaction(restartRequest.getRequest(), -1);
				explicitRestartTimeout = true;
			} else {
				TRC_INFORMATION("TR reset message received.");
				break;
			}
		}
		auto exclusiveAccess = getExclusiveAccess();
		iqrf::embed::os::RawDpaRead readOs(0);
		try {
			std::unique_ptr<IDpaTransactionResult2> result;
			exclusiveAccess->executeDpaTransactionRepeat(readOs.getRequest(), result, 3);
			readOs.processDpaTransactionResult(std::move(result));
			// module
			m_coordinatorParams.mid = readOs.getMid();
			m_coordinatorParams.moduleId = readOs.getMidAsString();
			m_coordinatorParams.trMcuType = (uint8_t)readOs.getTrMcuType();
			m_coordinatorParams.trType = readOs.getTrTypeAsString();
			m_coordinatorParams.mcuType = readOs.getTrMcuTypeAsString();
			// product
			m_coordinatorParams.osBuildWord = (uint16_t)readOs.getOsBuild();
			m_coordinatorParams.osBuild = readOs.getOsBuildAsString();
			m_coordinatorParams.osVersionByte = (uint8_t)readOs.getOsVersion();
			m_coordinatorParams.osVersion = readOs.getOsVersionAsString();
			m_coordinatorParams.hwpid = readOs.getHwpid();
			m_coordinatorParams.hwpidVersion = (uint16_t)readOs.getHwpidVer();
			logTranscieverParams(true);
			if (m_driverReloadHandler != nullptr) {
				m_driverReloadHandler();
			}
		} catch (const std::exception &e) {
			m_state = IIqrfDpaService::DpaState::NotReady;
			TRC_WARNING("Failed to retrieve TR parameters.");
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::reloadCoordinator(const IIqrfDpaService::CoordinatorParameters &params) {
		this->m_coordinatorParams = params;
		if (m_driverReloadHandler != nullptr) {
			m_driverReloadHandler();
		}
	}

	///// Transactions

	std::shared_ptr<IDpaTransaction2> IqrfDpa::executeExclusiveDpaTransaction(const DpaMessage &request, int32_t timeout) {
		TRC_FUNCTION_ENTER("");
		auto result = m_dpaHandler->executeDpaTransaction(request, timeout);
		TRC_FUNCTION_LEAVE("");
		return result;
	}

	std::shared_ptr<IDpaTransaction2> IqrfDpa::executeDpaTransaction(const DpaMessage &request, int32_t timeout) {
		TRC_FUNCTION_ENTER("");
		IDpaTransactionResult2::ErrorCode defaultError = IDpaTransactionResult2::TRN_OK;
		if (m_iqrfDpaChannel->hasExclusiveAccess()) {
			defaultError = IDpaTransactionResult2::TRN_ERROR_IFACE_EXCLUSIVE_ACCESS;
		}
		auto result = m_dpaHandler->executeDpaTransaction(request, timeout, defaultError);
		TRC_FUNCTION_LEAVE("");
		return result;
	}

	void IqrfDpa::executeDpaTransactionRepeat(const DpaMessage &request, std::unique_ptr<IDpaTransactionResult2> &result, int repeat, int32_t timeout = -1) {
		TRC_FUNCTION_ENTER("");
		for (int rep = 0; rep <= repeat; rep++) {
			try {
				std::shared_ptr<IDpaTransaction2> transaction = m_dpaHandler->executeDpaTransaction(request, timeout);
				result = std::move(transaction->get());
				TRC_DEBUG("Result from read transaction as string:" << PAR(result->getErrorString()));
				IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)result->getErrorCode();
				if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
					TRC_FUNCTION_LEAVE("");
					return;
				} else {
					std::string errorStr;
					if (errorCode < 0)
						errorStr = "Transaction error: ";
					else
						errorStr = "DPA error: ";
					errorStr += result->getErrorString();
					THROW_EXC_TRC_WAR(std::logic_error, errorStr);
				}
			} catch (std::exception &e) {
				CATCH_EXC_TRC_WAR(std::logic_error, e, e.what());
				if (rep == repeat)
				{
					TRC_FUNCTION_LEAVE("");
					THROW_EXC_TRC_WAR(std::logic_error, e.what())
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
		}
	}

	///// Handler execution

	void IqrfDpa::asyncDpaMessageHandler(const DpaMessage &dpaMessage) {
		TRC_FUNCTION_ENTER("");
		std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
		for (auto &hndl : m_asyncMessageHandlers) {
			hndl.second(dpaMessage);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::asyncRestartHandler(const DpaMessage &dpaMessage) {
		TRC_FUNCTION_ENTER("");
		try {
			iqrf::embed::explore::RawDpaEnumerate iqrfEmbedExploreEnumerate(0);
			iqrfEmbedExploreEnumerate.processAsyncResponse(dpaMessage);
			TRC_DEBUG("Parsed TR reset result async msg");
			if (!iqrfEmbedExploreEnumerate.isAsyncRcode()) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid async response code:"
					<< NAME_PAR(expected, (int)STATUS_ASYNC_RESPONSE) << NAME_PAR(delivered, (int)iqrfEmbedExploreEnumerate.getRcode())
				);
			}

			// update network parameters
			m_coordinatorParams.dpaVerWord = (uint16_t)iqrfEmbedExploreEnumerate.getDpaVer();
			m_coordinatorParams.dpaVerWordAsStr = iqrfEmbedExploreEnumerate.getDpaVerAsHexaString();
			m_coordinatorParams.dpaVer = iqrfEmbedExploreEnumerate.getDpaVerAsString();
			m_coordinatorParams.dpaVerMajor = iqrfEmbedExploreEnumerate.getDpaVerMajor();
			m_coordinatorParams.dpaVerMinor = iqrfEmbedExploreEnumerate.getDpaVerMinor();
			m_coordinatorParams.demoFlag = iqrfEmbedExploreEnumerate.getDemoFlag();
			m_coordinatorParams.stdModeSupportFlag = iqrfEmbedExploreEnumerate.getModeStd();
			m_coordinatorParams.lpModeSupportFlag = !iqrfEmbedExploreEnumerate.getModeStd();
			m_coordinatorParams.lpModeRunningFlag = iqrfEmbedExploreEnumerate.getStdAndLpSupport();
			// log network params
			logNetworkParams(true);

			if (m_coordinatorParams.stdModeSupportFlag) {
				//dual support from DPA 4.00
				m_rfMode = m_coordinatorParams.lpModeRunningFlag ? IDpaTransaction2::kLp : IDpaTransaction2::kStd;
			}

			if (m_coordinatorParams.lpModeSupportFlag) {
				m_rfMode = IDpaTransaction2::kLp;
			}
			m_dpaHandler->setRfCommunicationMode(m_rfMode);
		} catch (std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Wrong format of TR reset result async msg");
		}
		m_asyncRestartCv.notify_all();
		TRC_FUNCTION_LEAVE("")
	}

	///// Getters and setters

	std::unique_ptr<IIqrfDpaService::ExclusiveAccess> IqrfDpa::getExclusiveAccess() {
		std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
		return std::unique_ptr<IIqrfDpaService::ExclusiveAccess>(shape_new ExclusiveAccessImpl(this));
	}

	bool IqrfDpa::hasExclusiveAccess() const {
		return m_iqrfDpaChannel->hasExclusiveAccess();
	}

	void IqrfDpa::setExclusiveAccess() {
		std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
		m_iqrfDpaChannel->setExclusiveAccess();
	}

	void IqrfDpa::resetExclusiveAccess() {
		std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
		m_iqrfDpaChannel->resetExclusiveAccess();
	}

	int IqrfDpa::getDpaQueueLen() const {
		return m_dpaHandler->getDpaQueueLen();
	}

	IIqrfChannelService::State IqrfDpa::getIqrfChannelState(){
		return m_iqrfChannelService->getState();
	}

	IIqrfDpaService::DpaState IqrfDpa::getDpaChannelState() {
		return m_state;
	}

	IIqrfDpaService::CoordinatorParameters IqrfDpa::getCoordinatorParameters() const {
		return m_coordinatorParams;
	}

	int IqrfDpa::getTimeout() const {
		return m_dpaHandler->getTimeout();
	}

	void IqrfDpa::setTimeout(int timeout) {
		TRC_FUNCTION_ENTER("");
		m_dpaHandler->setTimeout(timeout);
		TRC_FUNCTION_LEAVE("")
	}

	IDpaTransaction2::RfMode IqrfDpa::getRfCommunicationMode() const {
		return m_dpaHandler->getRfCommunicationMode();
	}

	void IqrfDpa::setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) {
		TRC_FUNCTION_ENTER("");
		m_dpaHandler->setRfCommunicationMode(rfMode);
		TRC_FUNCTION_LEAVE("")
	}

	IDpaTransaction2::TimingParams IqrfDpa::getTimingParams() const {
		return m_dpaHandler->getTimingParams();
	}

	void IqrfDpa::setTimingParams(IDpaTransaction2::TimingParams params) {
		TRC_FUNCTION_ENTER("");
		m_dpaHandler->setTimingParams(params);
		TRC_FUNCTION_LEAVE("")
	}

	IDpaTransaction2::FrcResponseTime IqrfDpa::getFrcResponseTime() const {
		return m_dpaHandler->getFrcResponseTime();
	}

	void IqrfDpa::setFrcResponseTime(IDpaTransaction2::FrcResponseTime frcResponseTime) {
		TRC_FUNCTION_ENTER("");
		m_dpaHandler->setFrcResponseTime(frcResponseTime);
		TRC_FUNCTION_LEAVE("")
	}

	///// Interface and handlers management

	void IqrfDpa::registerAsyncMessageHandler(const std::string &serviceId, AsyncMessageHandlerFunc fun) {
		std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
		m_asyncMessageHandlers.insert(make_pair(serviceId, fun));
	}

	void IqrfDpa::unregisterAsyncMessageHandler(const std::string &serviceId) {
		std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
		m_asyncMessageHandlers.erase(serviceId);
	}

	void IqrfDpa::registerAnyMessageHandler(const std::string &serviceId, IDpaHandler2::AsyncMessageHandlerFunc fun) {
		m_dpaHandler->registerAnyMessageHandler(serviceId, fun);
	}

	void IqrfDpa::unregisterAnyMessageHandler(const std::string &serviceId) {
		m_dpaHandler->unregisterAnyMessageHandler(serviceId);
	}

	void IqrfDpa::registerDriverReloadHandler(const std::string &serviceId, IIqrfDpaService::DriverReloadHandler handler) {
		(void)serviceId;
		m_driverReloadHandler = handler;
	}

	void IqrfDpa::unregisterDriverReloadHandler(const std::string &serviceId) {
		(void)serviceId;
		m_driverReloadHandler = nullptr;
	}


	void IqrfDpa::attachInterface(iqrf::IIqrfChannelService *iface) {
		m_iqrfChannelService = iface;
		m_iqrfDpaChannel = shape_new IqrfDpaChannel(iface);
	}

	void IqrfDpa::detachInterface(iqrf::IIqrfChannelService *iface) {
		if (m_iqrfChannelService == iface) {
			m_iqrfChannelService = nullptr;
			delete m_iqrfDpaChannel;
			m_iqrfDpaChannel = nullptr;
		}
	}

	void IqrfDpa::attachInterface(shape::ITraceService *iface){
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfDpa::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

	///// Auxiliary methods

	void IqrfDpa::setDefaults() {
		TRC_FUNCTION_ENTER("");
		// module
		m_coordinatorParams.moduleId = "FFFFFFFF";
		m_coordinatorParams.mid = 4294967295;
		m_coordinatorParams.trType = "(DC)TR-72Dx";
		m_coordinatorParams.mcuType = "PIC16LF1938";
		// OS
		m_coordinatorParams.osVersion = "4.03D";
		m_coordinatorParams.osBuild = "08C8";
		m_coordinatorParams.osBuildWord = 2248;
		// DPA
		m_coordinatorParams.dpaVer = "4.14";
		m_coordinatorParams.dpaVerMajor = 4;
		m_coordinatorParams.dpaVerMinor = 20;
		m_coordinatorParams.dpaVerWord = 1044;
		m_coordinatorParams.dpaVerWordAsStr = "0414";
		// flags
		m_coordinatorParams.demoFlag = false;
		m_coordinatorParams.stdModeSupportFlag = true;
		m_coordinatorParams.lpModeSupportFlag = false;
		m_coordinatorParams.lpModeRunningFlag = true;
		// logging
		logTranscieverParams(false);
		logNetworkParams(false);
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDpa::logTranscieverParams(bool updated){
		TRC_INFORMATION((updated ? "Updated " : "Default ") << "TR params: " << std::endl
			<< NAME_PAR(moduleId, m_coordinatorParams.moduleId)
			<< NAME_PAR(osVersion, m_coordinatorParams.osVersion)
			<< NAME_PAR(trType, m_coordinatorParams.trType)
			<< NAME_PAR(mcuType, m_coordinatorParams.mcuType)
			<< NAME_PAR(osBuildWord, m_coordinatorParams.osBuildWord)
			<< NAME_PAR(osBuild, m_coordinatorParams.osBuild)
			<< std::endl
		);
	}

	void IqrfDpa::logNetworkParams(bool updated){
		TRC_INFORMATION((updated ? "Updated " : "Default ") << "DPA params: " << std::endl
			<< NAME_PAR(dpaVerWord, m_coordinatorParams.dpaVerWord) 
			<< NAME_PAR(dpaVerWordAsStr, m_coordinatorParams.dpaVerWordAsStr)
			<< NAME_PAR(dpaVer, m_coordinatorParams.dpaVer)
			<< NAME_PAR(dpaVerMajor, m_coordinatorParams.dpaVerMajor)
			<< NAME_PAR(dpaVerMinor, m_coordinatorParams.dpaVerMinor)
			<< NAME_PAR(demoFlag, m_coordinatorParams.demoFlag)
			<< NAME_PAR(stdModeSupportFlag, m_coordinatorParams.stdModeSupportFlag)
			<< NAME_PAR(lpModeSupportFlag, m_coordinatorParams.lpModeSupportFlag)
			<< NAME_PAR(lpModeRunningFlag, m_coordinatorParams.lpModeRunningFlag)
			<< std::endl
		);
	}
}
