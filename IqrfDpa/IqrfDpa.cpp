#define IIqrfDpaService_EXPORTS

#include "EnumStringConvertor.h"
#include "DpaHandler2.h"
#include "IqrfDpa.h"
#include "PrfOs.h"
#include "PrfEnum.h"
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

  void IqrfDpa::setTimingParams( IDpaTransaction2::TimingParams params )
  {
    TRC_FUNCTION_ENTER( "" );
    m_dpaHandler->setTimingParams( params );
    TRC_FUNCTION_LEAVE( "" )
  }

  IDpaTransaction2::FrcResponseTime IqrfDpa::getFrcResponseTime() const
  {
    return m_dpaHandler->getFrcResponseTime();
  }

  void IqrfDpa::setFrcResponseTime( IDpaTransaction2::FrcResponseTime frcResponseTime )
  {
    TRC_FUNCTION_ENTER( "" );
    m_dpaHandler->setFrcResponseTime( frcResponseTime );
    TRC_FUNCTION_LEAVE( "" )
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

  void IqrfDpa::asyncResetHandler(const DpaMessage& dpaMessage)
  {
    TRC_FUNCTION_ENTER("");

    const DpaMessage::DpaPacket_t& p = dpaMessage.DpaPacket();
    
    if (p.DpaResponsePacket_t.NADR == 0 &&
      p.DpaResponsePacket_t.PNUM == PNUM_ENUMERATION &&
      p.DpaResponsePacket_t.PCMD == CMD_GET_PER_INFO &&
      p.DpaResponsePacket_t.ResponseCode == STATUS_ASYNC_RESPONSE
      )
    { // shall be TR reset result async msg

      PrfEnum prfEnum;
      if (prfEnum.parseCoordinatorResetAsyncResponse(dpaMessage)) {
        TRC_DEBUG("Parsed TR reset result async msg");
        // Get coordinator parameters
        m_cPar.dpaVerWord = prfEnum.getDpaVerWord();
        m_cPar.dpaVerWordAsStr = prfEnum.getDpaVerWordAsStr();
        m_cPar.dpaVer = prfEnum.getDpaVer();
        m_cPar.dpaVerMajor = prfEnum.getDpaVerMajor();
        m_cPar.dpaVerMinor = prfEnum.getDpaVerMinor();
        m_cPar.demoFlag = prfEnum.getDemoFlag();
        m_cPar.stdModeSupportFlag = prfEnum.getStdModeSupportFlag();
        m_cPar.lpModeSupportFlag = prfEnum.getLpModeSupportFlag();
        TRC_INFORMATION("DPA params: " << std::endl <<
          NAME_PAR(dpaVer, m_cPar.dpaVer) <<
          NAME_PAR(demoFlag, m_cPar.demoFlag) <<
          NAME_PAR(stdModeSupportFlag, m_cPar.stdModeSupportFlag) <<
          NAME_PAR(lpModeSupportFlag, m_cPar.lpModeSupportFlag) <<
          std::endl
        );
        if (m_cPar.stdModeSupportFlag) {
          m_rfMode = IDpaTransaction2::kStd;
        }
        if (m_cPar.lpModeSupportFlag) {
          m_rfMode = IDpaTransaction2::kLp;
        }
        m_dpaHandler->setRfCommunicationMode(m_rfMode);
      }
      else {
        TRC_WARNING("Wrong format of TR reset result async msg");
      }

      m_asyncResetCv.notify_all();

    }
    else {
      TRC_DEBUG("Not a TR reset async response")
    }

    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::getIqrfNetworkParams()
  {
    TRC_FUNCTION_ENTER("");

    bool sentExplicitReset = false;

    while (true)
    { // wait for reset TR module async msg.
      std::unique_lock<std::mutex> lck(m_asyncResetMtx);

      //wait for async msg after reset
      const int wtime = 3000;
      TRC_INFORMATION("Waiting for TR reset result [ms]: " << PAR(wtime));

      //while (m_asyncResetCv.wait_for(lck, std::chrono::milliseconds(wtime))== std::cv_status::timeout) {
      if (m_asyncResetCv.wait_for(lck, std::chrono::milliseconds(wtime)) == std::cv_status::timeout) {

        if (!sentExplicitReset) {
          sentExplicitReset = true;
          TRC_INFORMATION("No async TR reset response => Send explicit request");
          PrfOs prfOs;
          prfOs.setResetCmd();
          auto trn = executeDpaTransaction(prfOs.getDpaRequest(), -1);
          // don't care about result => interested in async reset response
          //auto res = trn->get();
        }
        else {
          TRC_WARNING("Cannot get TR reset async msg");
          break;
        }
      }
      else {
        TRC_WARNING("TR reset async msg was handled");
        break;
      }
    }

    // Get coordinator parameters
    PrfOs prfOs;
    prfOs.setReadCmd();

    TRC_DEBUG("Send TR OS Read");
    auto trn = executeDpaTransaction(prfOs.getDpaRequest(), -1);
    auto res = trn->get();

    if (res->getErrorCode() == 0) {
      prfOs.parseResponse(res->getResponse());
      m_cPar.moduleId = prfOs.getModuleId();
      m_cPar.osVersion = prfOs.getOsVersion();
      m_cPar.trType = prfOs.getTrType();
      m_cPar.mcuType = prfOs.getMcuType();
      m_cPar.osBuild = prfOs.getOsBuild();
      TRC_INFORMATION("TR params: " << std::endl <<
        NAME_PAR(moduleId, m_cPar.moduleId) <<
        NAME_PAR(osVersion, m_cPar.osVersion) <<
        NAME_PAR(trType, m_cPar.trType) <<
        NAME_PAR(mcuType, m_cPar.mcuType) <<
        NAME_PAR(osBuild, m_cPar.osBuild) <<
        std::endl
      );
    }
    else {
      TRC_WARNING("Cannot get TR parameters");
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

    // handle asyn reset
    registerAsyncMessageHandler("  IqrfDpa", [&](const DpaMessage& dpaMessage) { //spaces in front of "  IqrfDpa" make it first in handlers map
      asyncResetHandler(dpaMessage);
    });

    // register to IQRF interface
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

    m_iqrfChannelService->startListen();

    getIqrfNetworkParams();

    IDpaTransaction2::TimingParams timingParams;
    timingParams.bondedNodes = m_bondedNodes;
    timingParams.discoveredNodes = m_discoveredNodes;
    timingParams.frcResponseTime = m_responseTime;
    timingParams.dpaVersion = m_cPar.dpaVerWord;
    timingParams.osVersion = m_cPar.osVersion;
    m_dpaHandler->setTimingParams( timingParams );

    TRC_FUNCTION_LEAVE("")
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
