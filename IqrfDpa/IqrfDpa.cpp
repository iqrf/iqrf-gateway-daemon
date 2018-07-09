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
      m_iqrfDpa->setExclusiveAccess(true);
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
      m_iqrfDpa->setExclusiveAccess(false);
    }

  private:
    IqrfDpa* m_iqrfDpa = nullptr;
  };

  std::unique_ptr<IIqrfDpaService::ExclusiveAccess> IqrfDpa::getExclusiveAccess()
  {
    std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> retval;
    if (!m_exclusiveAccess) {
      retval.reset(shape_new ExclusiveAccessImpl(this));
    }
    return retval;
  }

  void IqrfDpa::setExclusiveAccess(bool val)
  {
    std::unique_lock<std::recursive_mutex> lck(m_exclusiveAccessMutex);
    if (val) {
      m_iqrfDpaChannel->getExclusiveAccess();
      m_exclusiveAccess = true;
    }
    else {
      m_iqrfDpaChannel->ungetExclusiveAccess();
      m_exclusiveAccess = false;
    }
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
    if (m_exclusiveAccess) {
      defaultError = IDpaTransactionResult2::TRN_ERROR_IFACE_BUSY;
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

  void IqrfDpa::getIqrfNetworkParams()
  {
    TRC_FUNCTION_ENTER("");
    
    std::mutex mtx;
    std::condition_variable cv;
    
    //Provisional Async msg handling
    m_dpaHandler->unregisterAsyncMessageHandler("");
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      std::unique_lock<std::mutex> lck(mtx);
      PrfEnum prfEnum;
      if (prfEnum.parseCoordinatorResetAsyncResponse(dpaMessage)) {
        // Get coordinator parameters
        m_cPar.m_dpaVer = prfEnum.getDpaVer();
        m_cPar.dpaVerMajor = prfEnum.getDpaVerMajor();
        m_cPar.dpaVerMinor = prfEnum.getDpaVerMinor();
        m_cPar.demoFlag = prfEnum.getDemoFlag();
        m_cPar.stdModeSupportFlag = prfEnum.getStdModeSupportFlag();
        m_cPar.lpModeSupportFlag = prfEnum.getLpModeSupportFlag();
        m_initCoord = true;
      }
      else {
        TRC_WARNING("Wrong format of TR reset result async msg");
      }
      cv.notify_all();
    });

    // Get coordinator parameters
    PrfOs prfOs;

    { // reset TR module
      std::unique_lock<std::mutex> lck(mtx);
      if (!m_initCoord) {
        prfOs.setResetCmd();
        auto trn = executeDpaTransaction(prfOs.getDpaRequest(), -1);
        auto res = trn->get();
        // don't care about result

        //wait for async msg after reset
        int waitPeriod = 100;
        int waitNumber = 0;
        int waitMax = 10000;
        while (cv.wait_for(lck, std::chrono::milliseconds(waitPeriod)) == std::cv_status::timeout) {
          int wtime = ++waitNumber * waitPeriod;
          TRC_INFORMATION("Waiting for TR reset result: " << PAR(wtime));
          if (wtime > waitMax) {
            TRC_WARNING("Cannot get TR reset result: " << PAR(wtime));
            break;
          }
        }
      }
    }

    prfOs.setReadCmd();
    {
      auto trn = executeDpaTransaction(prfOs.getDpaRequest(), -1);
      auto res = trn->get();

      if (res->getErrorCode() == 0) {
        prfOs.parseResponse(res->getResponse());
        m_cPar.moduleId = prfOs.getModuleId();
        m_cPar.osVersion = prfOs.getOsVersion();
        m_cPar.trType = prfOs.getTrType();
        m_cPar.mcuType = prfOs.getMcuType();
        m_cPar.osBuild = prfOs.getOsBuild();
      }
      else {
        //THROW_EXC_TRC_WAR(std::logic_error, "Cannot get TR parameters");
        TRC_WARNING("Cannot get TR parameters");
      }
    }

    //Async msg handling
    m_dpaHandler->unregisterAsyncMessageHandler("");
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

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

    {
      const rapidjson::Value* val = rapidjson::Pointer("/CommunicationMode").Get(doc);
      if (val && val->IsString()) {
        std::string communicationMode = val->GetString();
        if (communicationMode == "LP")
          m_rfMode = IDpaTransaction2::kLp;
        else if (communicationMode == "STD")
          m_rfMode = IDpaTransaction2::kStd;
        else
          m_rfMode = IDpaTransaction2::kStd;
      }
      m_dpaHandler->setRfCommunicationMode(m_rfMode);
    }

    {
      const rapidjson::Value* val = rapidjson::Pointer("/BondedNodes").Get(doc);
      if (val && val->IsInt()) {
        m_bondedNodes = val->GetInt();
      }
    }

    {
      const rapidjson::Value* val = rapidjson::Pointer("/DiscoveredNodes").Get(doc);
      if (val && val->IsInt()) {
        m_discoveredNodes = val->GetInt();
      }
    }

    {
      const rapidjson::Value* val = rapidjson::Pointer("/ResponseTime").Get(doc);
      if (val && val->IsString()) {
        FrcResponseTimeStringConvertor conv;
        m_responseTime = conv.str2enum(val->GetString());
      }
    }

    IDpaTransaction2::FRC_TimingParams params;
    params.bondedNodes = m_bondedNodes;
    params.discoveredNodes = m_discoveredNodes;
    params.responseTime = m_responseTime;
    m_dpaHandler->setFrcTiming(params);

    getIqrfNetworkParams();

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
