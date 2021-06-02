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

  std::unique_ptr<IIqrfDpaService::ExclusiveAccess> IqrfDpa::getExclusiveAccess()
  {
    std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
    return std::unique_ptr<IIqrfDpaService::ExclusiveAccess>(shape_new ExclusiveAccessImpl(this));
  }

  bool IqrfDpa::hasExclusiveAccess() const
  {
    return m_iqrfDpaChannel->hasExclusiveAccess();
  }

  void IqrfDpa::setExclusiveAccess()
  {
    std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
    m_iqrfDpaChannel->setExclusiveAccess();
  }

  void IqrfDpa::resetExclusiveAccess()
  {
    std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
    m_iqrfDpaChannel->resetExclusiveAccess();
  }

  IqrfDpa::IqrfDpa()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  IqrfDpa::~IqrfDpa()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  std::shared_ptr<IDpaTransaction2> IqrfDpa::executeExclusiveDpaTransaction(const DpaMessage& request, int32_t timeout)
  {
    TRC_FUNCTION_ENTER("");
    auto result = m_dpaHandler->executeDpaTransaction(request, timeout);
    TRC_FUNCTION_LEAVE("");
    return result;
  }

  std::shared_ptr<IDpaTransaction2> IqrfDpa::executeDpaTransaction(const DpaMessage& request, int32_t timeout)
  {
    TRC_FUNCTION_ENTER("");
    IDpaTransactionResult2::ErrorCode defaultError = IDpaTransactionResult2::TRN_OK;
    if (m_iqrfDpaChannel->hasExclusiveAccess()) {
      defaultError = IDpaTransactionResult2::TRN_ERROR_IFACE_EXCLUSIVE_ACCESS;
    }
    auto result = m_dpaHandler->executeDpaTransaction(request, timeout, defaultError);
    TRC_FUNCTION_LEAVE("");
    return result;
  }

  void IqrfDpa::executeDpaTransactionRepeat(const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout = -1)
  {
    TRC_FUNCTION_ENTER("");

    for (int rep = 0; rep <= repeat; rep++)
    {
      try
      {
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
      }
      catch (std::exception& e) {
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

  IIqrfDpaService::CoordinatorParameters IqrfDpa::getCoordinatorParameters() const
  {
    return m_cPar;
  }

  int IqrfDpa::getTimeout() const
  {
    return m_dpaHandler->getTimeout();
  }

  void IqrfDpa::setTimeout(int timeout)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->setTimeout(timeout);
    TRC_FUNCTION_LEAVE("")
  }

  IDpaTransaction2::RfMode IqrfDpa::getRfCommunicationMode() const
  {
    return m_dpaHandler->getRfCommunicationMode();
  }

  void IqrfDpa::setRfCommunicationMode(IDpaTransaction2::RfMode rfMode)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->setRfCommunicationMode(rfMode);
    TRC_FUNCTION_LEAVE("")
  }

  IDpaTransaction2::TimingParams IqrfDpa::getTimingParams() const
  {
    return m_dpaHandler->getTimingParams();
  }

  void IqrfDpa::setTimingParams(IDpaTransaction2::TimingParams params)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->setTimingParams(params);
    TRC_FUNCTION_LEAVE("")
  }

  IDpaTransaction2::FrcResponseTime IqrfDpa::getFrcResponseTime() const
  {
    return m_dpaHandler->getFrcResponseTime();
  }

  void IqrfDpa::setFrcResponseTime(IDpaTransaction2::FrcResponseTime frcResponseTime)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->setFrcResponseTime(frcResponseTime);
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun)
  {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
    //TODO check success
    m_asyncMessageHandlers.insert(make_pair(serviceId, fun));

  }

  void IqrfDpa::unregisterAsyncMessageHandler(const std::string& serviceId)
  {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);
    m_asyncMessageHandlers.erase(serviceId);
  }

  void IqrfDpa::asyncDpaMessageHandler(const DpaMessage& dpaMessage)
  {
    std::lock_guard<std::mutex> lck(m_asyncMessageHandlersMutex);

    for (auto & hndl : m_asyncMessageHandlers)
      hndl.second(dpaMessage);
  }

  void IqrfDpa::asyncRestartHandler(const DpaMessage& dpaMessage)
  {
    TRC_FUNCTION_ENTER("");

    // shall be TR reset result async msg
    try {
      iqrf::embed::explore::RawDpaEnumerate iqrfEmbedExploreEnumerate(0);
      iqrfEmbedExploreEnumerate.processAsyncResponse(dpaMessage);
      TRC_DEBUG("Parsed TR reset result async msg");
      if (!iqrfEmbedExploreEnumerate.isAsyncRcode()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid async response code:"
          << NAME_PAR(expected, (int)STATUS_ASYNC_RESPONSE) << NAME_PAR(delivered, (int)iqrfEmbedExploreEnumerate.getRcode()));
      }

      // Get coordinator parameters
      m_cPar.dpaVerWord = (uint16_t)iqrfEmbedExploreEnumerate.getDpaVer();
      m_cPar.dpaVerWordAsStr = iqrfEmbedExploreEnumerate.getDpaVerAsHexaString();
      m_cPar.dpaVer = iqrfEmbedExploreEnumerate.getDpaVerAsString();
      m_cPar.dpaVerMajor = iqrfEmbedExploreEnumerate.getDpaVerMajor();
      m_cPar.dpaVerMinor = iqrfEmbedExploreEnumerate.getDpaVerMinor();
      m_cPar.demoFlag = iqrfEmbedExploreEnumerate.getDemoFlag();
      m_cPar.stdModeSupportFlag = iqrfEmbedExploreEnumerate.getModeStd();
      m_cPar.lpModeSupportFlag = !iqrfEmbedExploreEnumerate.getModeStd();
      m_cPar.lpModeRunningFlag = iqrfEmbedExploreEnumerate.getStdAndLpSupport();
      TRC_INFORMATION("DPA params: " << std::endl <<
        NAME_PAR(dpaVerWord, m_cPar.dpaVerWord) <<
        NAME_PAR(dpaVerWordAsStr, m_cPar.dpaVerWordAsStr) <<
        NAME_PAR(dpaVer, m_cPar.dpaVer) <<
        NAME_PAR(dpaVerMajor, m_cPar.dpaVerMajor) <<
        NAME_PAR(dpaVerMinor, m_cPar.dpaVerMinor) <<
        NAME_PAR(demoFlag, m_cPar.demoFlag) <<
        NAME_PAR(stdModeSupportFlag, m_cPar.stdModeSupportFlag) <<
        NAME_PAR(lpModeSupportFlag, m_cPar.lpModeSupportFlag) <<
        NAME_PAR(lpModeRunningFlag, m_cPar.lpModeRunningFlag) <<
        std::endl
      );

      if (m_cPar.stdModeSupportFlag)
      {
        //dual support from DPA 4.00
        m_rfMode = m_cPar.lpModeRunningFlag ? IDpaTransaction2::kLp : IDpaTransaction2::kStd;
      }

      if (m_cPar.lpModeSupportFlag)
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

  void IqrfDpa::getIqrfNetworkParams()
  {
    TRC_FUNCTION_ENTER("");

    bool cooordinatorIdentified = false;
    const uint32_t resetTime = 3000;

    while (!cooordinatorIdentified) {
      m_iqrfChannelService->startListen();
      std::unique_lock<std::mutex> lock(m_asyncRestartMtx);
      TRC_INFORMATION("Waiting for possible TR reset: " << std::to_string(resetTime) << " milliseconds.");
      if (m_asyncRestartCv.wait_for(lock, std::chrono::milliseconds(resetTime)) == std::cv_status::timeout) {
        TRC_WARNING("TR async reset message not received. Sleeping for: " << std::to_string(m_interfaceCheckPeriod) << " seconds.");
        std::this_thread::sleep_for(std::chrono::seconds(m_interfaceCheckPeriod));
        TRC_INFORMATION("Waking up from reset sleep.");
        if (m_iqrfChannelService->getState() == IIqrfChannelService::State::Ready) {
          TRC_INFORMATION("IQRF channel service is ready, requesting restart.");
          iqrf::embed::os::RawDpaRestart iqrfEmbedOsRestart(0);
          executeDpaTransaction(iqrfEmbedOsRestart.getRequest(), -1);
        } else {
          TRC_INFORMATION("IQRF channel service not ready, waiting.");
        }
      } else {
        TRC_INFORMATION("TR async reset message received.");
        cooordinatorIdentified = true;
      }
    }

    // Get coordinator parameters
    auto exclusiveAccess = getExclusiveAccess();

    iqrf::embed::os::RawDpaRead iqrfEmbedOsRead(0);
    try {
      std::unique_ptr<IDpaTransactionResult2> transResult;
      exclusiveAccess->executeDpaTransactionRepeat(iqrfEmbedOsRead.getRequest(), transResult, 3);
      iqrfEmbedOsRead.processDpaTransactionResult(std::move(transResult));

      m_cPar.moduleId = iqrfEmbedOsRead.getMidAsString();
      m_cPar.mid = iqrfEmbedOsRead.getMid();
      m_cPar.osVersion = iqrfEmbedOsRead.getOsVersionAsString();
      m_cPar.trType = iqrfEmbedOsRead.getTrTypeAsString();
      m_cPar.mcuType = iqrfEmbedOsRead.getTrMcuTypeAsString();
      m_cPar.osBuildWord = (uint16_t)iqrfEmbedOsRead.getOsBuild();
      m_cPar.osBuild = iqrfEmbedOsRead.getOsBuildAsString();
      TRC_INFORMATION("TR params: " << std::endl <<
        NAME_PAR(moduleId, m_cPar.moduleId) <<
        NAME_PAR(osVersion, m_cPar.osVersion) <<
        NAME_PAR(trType, m_cPar.trType) <<
        NAME_PAR(mcuType, m_cPar.mcuType) <<
        NAME_PAR(osBuildWord, m_cPar.osBuildWord) <<
        NAME_PAR(osBuild, m_cPar.osBuild) <<
        std::endl
      );

    }
    catch (std::exception & e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "Cannot get TR parameters")
      state = IIqrfDpaService::DpaState::NotReady;
      std::cout << std::endl << "Error: Cannot get TR parameters msg => interface to DPA coordinator is not working - verify (CDC or SPI or UART) configuration" << std::endl;
    }

    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance activate" << std::endl <<
      "******************************"
    );

    m_dpaHandler = shape_new DpaHandler2(m_iqrfDpaChannel);

    const rapidjson::Document& doc = props->getAsJson();

    {
      const rapidjson::Value* val = rapidjson::Pointer("/DpaHandlerTimeout").Get(doc);
      if (val && val->IsInt()) {
        m_dpaHandlerTimeout = val->GetInt();
        m_dpaHandler->setTimeout(m_dpaHandlerTimeout);
      }
      m_dpaHandler->setTimeout(m_dpaHandlerTimeout);
    }

    m_interfaceCheckPeriod = (uint8_t)rapidjson::Pointer("/interfaceCheckPeriod").Get(doc)->GetUint();

    // handle asyn reset
    registerAsyncMessageHandler("  IqrfDpa", [&](const DpaMessage& dpaMessage) { //spaces in front of "  IqrfDpa" make it first in handlers map
      asyncRestartHandler(dpaMessage);
    });

    // register to IQRF interface
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

    getIqrfNetworkParams();

    // unregister asyn reset - not needed  after getIqrfNetworkParams()
    unregisterAsyncMessageHandler("  IqrfDpa");

    IDpaTransaction2::TimingParams timingParams;
    timingParams.bondedNodes = m_bondedNodes;
    timingParams.discoveredNodes = m_discoveredNodes;
    timingParams.frcResponseTime = m_responseTime;
    timingParams.dpaVersion = m_cPar.dpaVerWord;
    timingParams.osVersion = m_cPar.osVersion;
    m_dpaHandler->setTimingParams(timingParams);

    IIqrfChannelService::State st = m_iqrfChannelService->getState();
    if (st == IIqrfChannelService::State::NotReady) {
      std::cout << std::endl << "Error: Interface to DPA coordinator is not ready - verify (CDC or SPI or UART) configuration" << std::endl;
    }

    TRC_FUNCTION_LEAVE("")
  }

  int IqrfDpa::getDpaQueueLen() const
  {
    return m_dpaHandler->getDpaQueueLen();
  }

  IIqrfChannelService::State IqrfDpa::getIqrfChannelState()
  {
    return m_iqrfChannelService->getState();
  }

  IIqrfDpaService::DpaState IqrfDpa::getDpaChannelState()
  {
    return state;
  }

  void IqrfDpa::registerAnyMessageHandler(const std::string& serviceId, IDpaHandler2::AsyncMessageHandlerFunc fun)
  {
    m_dpaHandler->registerAnyMessageHandler(serviceId, fun);
  }

  void IqrfDpa::unregisterAnyMessageHandler(const std::string& serviceId)
  {
    m_dpaHandler->unregisterAnyMessageHandler(serviceId);
  }

  void IqrfDpa::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance deactivate" << std::endl <<
      "******************************"
    );

    m_iqrfDpaChannel->unregisterReceiveFromHandler();
    m_dpaHandler->unregisterAsyncMessageHandler("");

    delete m_dpaHandler;
    m_dpaHandler = nullptr;

    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void IqrfDpa::attachInterface(iqrf::IIqrfChannelService* iface)
  {
    m_iqrfChannelService = iface;
    m_iqrfDpaChannel = shape_new IqrfDpaChannel(iface);
  }

  void IqrfDpa::detachInterface(iqrf::IIqrfChannelService* iface)
  {
    if (m_iqrfChannelService == iface) {
      m_iqrfChannelService = nullptr;
      delete m_iqrfDpaChannel;
      m_iqrfDpaChannel = nullptr;
    }
  }

  void IqrfDpa::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfDpa::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
