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

TRC_INIT_MODULE(iqrf::IqrfDpa);

namespace iqrf {

  class FrcResponseTimeConvertTable
  {
  public:
    static const std::vector<std::pair<IDpaTransaction2::FrcResponseTime, std::string>>& table()
    {
      static std::vector <std::pair<IDpaTransaction2::FrcResponseTime, std::string>> table = {
        { IDpaTransaction2::FrcResponseTime::k40Ms, "k40Ms" },
        { IDpaTransaction2::FrcResponseTime::k360Ms, "k360Ms" },
        { IDpaTransaction2::FrcResponseTime::k680Ms, "k680Ms" },
        { IDpaTransaction2::FrcResponseTime::k1320Ms, "k1320Ms" },
        { IDpaTransaction2::FrcResponseTime::k2600Ms, "k2600Ms" },
        { IDpaTransaction2::FrcResponseTime::k5160Ms, "k5160Ms" },
        { IDpaTransaction2::FrcResponseTime::k10280Ms, "k10280Ms" },
        { IDpaTransaction2::FrcResponseTime::k20620Ms, "k20620Ms" }
      };
      return table;
    }
    static IDpaTransaction2::FrcResponseTime defaultEnum()
    {
      return IDpaTransaction2::FrcResponseTime::k40Ms;
    }
    static const std::string& defaultStr()
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
    ExclusiveAccessImpl(IqrfDpa* iqrfDpa)
      :m_iqrfDpa(iqrfDpa)
    {
      m_iqrfDpa->setExclusiveAccess();
    }

    std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout = -1) override
    {
      TRC_FUNCTION_ENTER("");
      auto result = m_iqrfDpa->executeExclusiveDpaTransaction(request, timeout);
      TRC_FUNCTION_LEAVE("");
      return result;
    }

    void executeDpaTransactionRepeat(const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout = -1) override
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
    IqrfDpa* m_iqrfDpa = nullptr;
  };

  IqrfDpa::IqrfDpa() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  IqrfDpa::~IqrfDpa() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  // Component lifecycle ==========================

  void IqrfDpa::activate(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance activate" << std::endl <<
      "******************************"
    );

    m_dpaHandler = shape_new DpaHandler2(m_iqrfDpaChannel);

    modify(props);

    // register to IQRF interface
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

    setDefaults();

    m_runInterface = true;

    startChannelCheck();
    startInterface();

    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::modify(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    const rapidjson::Document &doc = props->getAsJson();
    m_dpaHandlerTimeout = rapidjson::Pointer("/DpaHandlerTimeout").Get(doc)->GetInt();
    m_dpaHandler->setTimeout(m_dpaHandlerTimeout);
    m_interfaceCheckPeriod = (uint8_t)rapidjson::Pointer("/interfaceCheckPeriod").Get(doc)->GetUint();
    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::deactivate() {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance deactivate" << std::endl <<
      "******************************"
    );

    m_runInterface = false;

    if (m_initThread.joinable()) {
      m_initThread.join();
    }

    if (m_channelStateThread.joinable()) {
      m_channelStateThread.join();
    }

    m_iqrfDpaChannel->unregisterReceiveFromHandler();
    m_dpaHandler->unregisterAsyncMessageHandler("");

    delete m_dpaHandler;
    m_dpaHandler = nullptr;

    TRC_FUNCTION_LEAVE("")
  }

  // Handler execution ============================

  void IqrfDpa::asyncDpaMessageHandler(const DpaMessage& dpaMessage)
  {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);

    for (auto & hndl : m_asyncMessageHandlers)
      hndl.second(dpaMessage);
  }

  void IqrfDpa::asyncRestartHandler(const DpaMessage& dpaMessage) {
    TRC_FUNCTION_ENTER("");

    try {
      iqrf::embed::explore::RawDpaEnumerate iqrfEmbedExploreEnumerate(0);
      iqrfEmbedExploreEnumerate.processAsyncResponse(dpaMessage);
      TRC_DEBUG("Parsed TR reset result async msg");
      if (!iqrfEmbedExploreEnumerate.isAsyncRcode()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid async response code:"
          << NAME_PAR(expected, (int)STATUS_ASYNC_RESPONSE) << NAME_PAR(delivered, (int)iqrfEmbedExploreEnumerate.getRcode()));
      }

      // Update network params
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

      if (m_coordinatorParams.stdModeSupportFlag)
      {
        //dual support from DPA 4.00
        m_rfMode = m_coordinatorParams.lpModeRunningFlag ? IDpaTransaction2::kLp : IDpaTransaction2::kStd;
      }

      if (m_coordinatorParams.lpModeSupportFlag)
      {
        m_rfMode = IDpaTransaction2::kLp;
      }

      m_dpaHandler->setRfCommunicationMode(m_rfMode);
    }
    catch (std::exception & e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "Wrong format of TR reset result async msg");
    }

    m_asyncRestartCv.notify_all();

    TRC_FUNCTION_LEAVE("")
  }

  // Interface initialization =====================

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
    logTranscieverParams(false);
    logNetworkParams(false);
    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::startInterface() {
    TRC_FUNCTION_ENTER("");

    if (m_initThread.joinable()) {
      TRC_DEBUG("Initialization thread joinable, joining.");
      m_initThread.join();
    }
    m_initThread = std::thread([this] { interfaceInitializationThread(); });
    pthread_setname_np(m_initThread.native_handle(), "_dpaInit");

    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::interfaceInitializationThread() {
    IIqrfChannelService::State state;
    while (m_runInterface) {
      std::unique_lock<std::mutex> lock(m_channelStateMutex);
      state = m_iqrfChannelService->getState();
      lock.unlock();
      if (state == IIqrfChannelService::State::NotReady) {
        TRC_WARNING("IQRF channel not ready, attempting to re-initialize...");
        registerAsyncMessageHandler("  IqrfDpa", [&](const DpaMessage& dpaMessage) {
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
      }
      {
        std::unique_lock<std::mutex> lock(m_initMutex);
        TRC_DEBUG("Sleeping for 30s");
        m_initCv.wait_for(lock, std::chrono::seconds(30));
        TRC_DEBUG("Initialization thread waking up...");
      }
    }
  }

  void IqrfDpa::startChannelCheck() {
    TRC_FUNCTION_ENTER("");

    if (m_channelStateThread.joinable()) {
      TRC_DEBUG("State checking thread joinable, joining.");
      m_channelStateThread.join();
    }
    m_channelStateThread = std::thread([this] { channelCheckThread(); });
    pthread_setname_np(m_channelStateThread.native_handle(), "_dpaChannelCheck");

    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::channelCheckThread() {
    while (m_runInterface) {
      std::unique_lock<std::mutex> lock(m_channelStateMutex);
      m_iqrfChannelService->refreshState();
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }

  void IqrfDpa::identifyCoordinator() {
    TRC_FUNCTION_ENTER("");

    std::shared_ptr<IDpaTransaction2> result;
    bool resetReceived = false;
    const uint16_t resetDelay = 3000;

    while (!resetReceived) {
      // attempt to initialize iqrf channel
      m_iqrfChannelService->startListen();
      // mutex
      std::unique_lock<std::mutex> lock(m_asyncRestartMtx);
      TRC_INFORMATION("Waiting for TR reset (" << std::to_string(resetDelay) << " ms).");
      // wait for reset
      if (m_asyncRestartCv.wait_for(lock, std::chrono::milliseconds(resetDelay)) == std::cv_status::timeout) {
        // check transaction result, if not empty, transaction is stuck
        if (result != nullptr) {
          result->abort();
          result = nullptr;
        }
        TRC_WARNING("TR reset message not received, sleeping for " << std::to_string(m_interfaceCheckPeriod) << " seconds.");
        // no message received, sleep before trying again
        std::this_thread::sleep_for(std::chrono::seconds(m_interfaceCheckPeriod));
        // check if iqrf channel state is ready
        if (m_iqrfChannelService->getState() == IIqrfChannelService::State::Ready) {
          // channel state ready, send explicit restart
          iqrf::embed::os::RawDpaRestart restartRequest(0);
          result = executeDpaTransaction(restartRequest.getRequest(), -1);
        } else {
          // channel not ready, repeat cycle
          TRC_WARNING("IQRF channel service not ready, retrying...");
        }
      } else {
        TRC_INFORMATION("TR reset message received.");
        resetReceived = true;
      }
    }

    auto exclusiveAccess = getExclusiveAccess();
    iqrf::embed::os::RawDpaRead readRequest(0);

    try {
      // execute OS read
      std::unique_ptr<IDpaTransactionResult2> readResult;
      exclusiveAccess->executeDpaTransactionRepeat(readRequest.getRequest(), readResult, 3);
      readRequest.processDpaTransactionResult(std::move(readResult));
      // update module params
      m_coordinatorParams.moduleId = readRequest.getMidAsString();
      m_coordinatorParams.mid = readRequest.getMid();
      m_coordinatorParams.trType = readRequest.getTrTypeAsString();
      m_coordinatorParams.mcuType = readRequest.getTrMcuTypeAsString();
      // update OS params
      m_coordinatorParams.osVersion = readRequest.getOsVersionAsString();
      m_coordinatorParams.osBuild = readRequest.getOsBuildAsString();
      m_coordinatorParams.osBuildWord = (uint16_t)readRequest.getOsBuild();
      // update DPA state
      m_state = IIqrfDpaService::DpaState::Ready;
      // log updated params
      logTranscieverParams(true);
      // reload drivers
      if (m_driverReloadHandler != nullptr) {
        m_driverReloadHandler();
      }
    } catch (const std::exception &e) {
      m_state = IIqrfDpaService::DpaState::NotReady;
      TRC_WARNING("Failed to retrieve TR parameters.");
    }

    TRC_FUNCTION_LEAVE("");
  }

  void IqrfDpa::reloadCoordinator() {
    m_runInterface = false;
    m_initThread.join();
    m_channelStateThread.join();
    m_iqrfChannelService->destroyInterface();
    m_runInterface = true;
    startChannelCheck();
    startInterface();
  }

  // Transactions =================================

  std::shared_ptr<IDpaTransaction2> IqrfDpa::executeExclusiveDpaTransaction(const DpaMessage& request, int32_t timeout) {
    TRC_FUNCTION_ENTER("");
    auto result = m_dpaHandler->executeDpaTransaction(request, timeout);
    TRC_FUNCTION_LEAVE("");
    return result;
  }

  std::shared_ptr<IDpaTransaction2> IqrfDpa::executeDpaTransaction(const DpaMessage& request, int32_t timeout) {
    TRC_FUNCTION_ENTER("");
    IDpaTransactionResult2::ErrorCode defaultError = IDpaTransactionResult2::TRN_OK;
    if (m_iqrfDpaChannel->hasExclusiveAccess()) {
      defaultError = IDpaTransactionResult2::TRN_ERROR_IFACE_EXCLUSIVE_ACCESS;
    }
    auto result = m_dpaHandler->executeDpaTransaction(request, timeout, defaultError);
    TRC_FUNCTION_LEAVE("");
    return result;
  }

  void IqrfDpa::executeDpaTransactionRepeat(const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout = -1) {
    TRC_FUNCTION_ENTER("");

    for (int rep = 0; rep <= repeat; rep++) {
      try {
        std::shared_ptr<IDpaTransaction2> transaction = m_dpaHandler->executeDpaTransaction(request, timeout);
        result = std::move(transaction->get());
        TRC_DEBUG("Result from read transaction as string:" << PAR(result->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)result->getErrorCode();
        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK)
        {
          TRC_FUNCTION_LEAVE("");
          return;
        }
        else
        {
          std::string errorStr;
          if (errorCode < 0)
            errorStr = "Transaction error: ";
          else
            errorStr = "DPA error: ";
          errorStr += result->getErrorString();
          THROW_EXC_TRC_WAR(std::logic_error, errorStr);
        }
      } catch (std::exception& e) {
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

  // Getters and setters ==========================

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

  IIqrfChannelService::State IqrfDpa::getIqrfChannelState() {
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
    m_dpaHandler->setTimeout(timeout);
  }

  IDpaTransaction2::RfMode IqrfDpa::getRfCommunicationMode() const {
    return m_dpaHandler->getRfCommunicationMode();
  }

  void IqrfDpa::setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) {
    m_dpaHandler->setRfCommunicationMode(rfMode);
  }

  IDpaTransaction2::TimingParams IqrfDpa::getTimingParams() const {
    return m_dpaHandler->getTimingParams();
  }

  void IqrfDpa::setTimingParams(IDpaTransaction2::TimingParams params) {
    m_dpaHandler->setTimingParams(params);
  }

  IDpaTransaction2::FrcResponseTime IqrfDpa::getFrcResponseTime() const {
    return m_dpaHandler->getFrcResponseTime();
  }

  void IqrfDpa::setFrcResponseTime(IDpaTransaction2::FrcResponseTime frcResponseTime) {
    m_dpaHandler->setFrcResponseTime(frcResponseTime);
  }

  // Handlers and interfaces ======================

  void IqrfDpa::registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
    m_asyncMessageHandlers.insert(make_pair(serviceId, fun));
  }

  void IqrfDpa::unregisterAsyncMessageHandler(const std::string& serviceId) {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
    m_asyncMessageHandlers.erase(serviceId);
  }

  void IqrfDpa::registerAnyMessageHandler(const std::string& serviceId, IDpaHandler2::AsyncMessageHandlerFunc fun) {
    m_dpaHandler->registerAnyMessageHandler(serviceId, fun);
  }

  void IqrfDpa::unregisterAnyMessageHandler(const std::string& serviceId) {
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

  void IqrfDpa::attachInterface(iqrf::IIqrfChannelService* iface) {
    m_iqrfChannelService = iface;
    m_iqrfDpaChannel = shape_new IqrfDpaChannel(iface);
  }

  void IqrfDpa::detachInterface(iqrf::IIqrfChannelService* iface) {
    if (m_iqrfChannelService == iface) {
      m_iqrfChannelService = nullptr;
      delete m_iqrfDpaChannel;
      m_iqrfDpaChannel = nullptr;
    }
  }

  void IqrfDpa::attachInterface(shape::ITraceService* iface) {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfDpa::detachInterface(shape::ITraceService* iface) {
    shape::Tracer::get().removeTracerService(iface);
  }

  // Auxiliary methods ============================

  void IqrfDpa::logTranscieverParams(bool updated) {
    TRC_INFORMATION((updated ? "Updated " : "Default ") << "TR params: " << std::endl <<
      NAME_PAR(moduleId, m_coordinatorParams.moduleId) <<
      NAME_PAR(osVersion, m_coordinatorParams.osVersion) <<
      NAME_PAR(trType, m_coordinatorParams.trType) <<
      NAME_PAR(mcuType, m_coordinatorParams.mcuType) <<
      NAME_PAR(osBuildWord, m_coordinatorParams.osBuildWord) <<
      NAME_PAR(osBuild, m_coordinatorParams.osBuild) <<
      std::endl
    );
  }

  void IqrfDpa::logNetworkParams(bool updated) {
    TRC_INFORMATION((updated ? "Updated " : "Default ") << "DPA params: " << std::endl <<
      NAME_PAR(dpaVerWord, m_coordinatorParams.dpaVerWord) <<
      NAME_PAR(dpaVerWordAsStr, m_coordinatorParams.dpaVerWordAsStr) <<
      NAME_PAR(dpaVer, m_coordinatorParams.dpaVer) <<
      NAME_PAR(dpaVerMajor, m_coordinatorParams.dpaVerMajor) <<
      NAME_PAR(dpaVerMinor, m_coordinatorParams.dpaVerMinor) <<
      NAME_PAR(demoFlag, m_coordinatorParams.demoFlag) <<
      NAME_PAR(stdModeSupportFlag, m_coordinatorParams.stdModeSupportFlag) <<
      NAME_PAR(lpModeSupportFlag, m_coordinatorParams.lpModeSupportFlag) <<
      NAME_PAR(lpModeRunningFlag, m_coordinatorParams.lpModeRunningFlag) <<
      std::endl
    );
  }
}
