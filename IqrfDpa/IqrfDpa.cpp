#define IIqrfDpaService_EXPORTS

#include "PrfOs.h"
#include "DpaTransactionTask.h"
#include "IqrfDpa.h"
#include "Trace.h"

//TODO workaround old tracing 
#include "IqrfLogging.h"
TRC_INIT();

#include "iqrfgw__IqrfDpa.hxx"

TRC_INIT_MODULE(iqrfgw::IqrfDpa);

namespace iqrfgw {
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

  void IqrfDpa::executeDpaTransaction(DpaTransaction& dpaTransaction)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->ExecuteDpaTransaction(dpaTransaction);
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::killDpaTransaction()
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandler->KillDpaTransaction();
    TRC_FUNCTION_LEAVE("")
  }

  int IqrfDpa::getTimeout() const
  {
    return m_dpaHandlerTimeout;
  }

  void IqrfDpa::setTimeout(int timeout)
  {
    TRC_FUNCTION_ENTER("");
    m_dpaHandlerTimeout = timeout;
    TRC_FUNCTION_LEAVE("")
  }

  IIqrfDpaService::RfMode IqrfDpa::getRfCommunicationMode() const
  {
    return m_rfMode;
  }

  void IqrfDpa::setRfCommunicationMode(IIqrfDpaService::RfMode rfMode)
  {
    TRC_FUNCTION_ENTER("");
    m_rfMode = rfMode;
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

  void IqrfDpa::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance activate" << std::endl <<
      "******************************"
    );

    //TODO workaround old tracing 
    TRC_START("TraceOld.txt", iqrf::Level::dbg, TRC_DEFAULT_FILE_MAXSIZE);

    //try {
      m_dpaHandler = shape_new DpaHandler(m_iqrfDpaChannel);
      if (m_dpaHandlerTimeout > 0) {
        m_dpaHandler->Timeout(m_dpaHandlerTimeout);
      }
      else {
        // 400ms by default
        m_dpaHandler->Timeout(DpaHandler::DEFAULT_TIMING);
      }

      IqrfRfCommunicationMode dpaMode = IqrfRfCommunicationMode::kStd;
      switch (m_rfMode) {
      case RfMode::Lp:
        dpaMode = IqrfRfCommunicationMode::kLp;
        break;
      case RfMode::Std:
        dpaMode = IqrfRfCommunicationMode::kStd;
        break;
      default:;
      }
      m_dpaHandler->SetRfCommunicationMode(dpaMode);

      //Async msg handling
      m_dpaHandler->RegisterAsyncMessageHandler([&](const DpaMessage& dpaMessage) {
        asyncDpaMessageHandler(dpaMessage);
      });

#if 0
      //TR module
      PrfOs prfOs;
      prfOs.read();

      DpaTransactionTask trans(prfOs);
      executeDpaTransaction(trans);
      int result = trans.waitFinish();

      if (result != 0) {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot get TR parameters");
      }

      m_moduleId = prfOs.getModuleId();
      m_osVersion = prfOs.getOsVersion();
      m_trType = prfOs.getTrType();
      m_mcuType = prfOs.getMcuType();
      m_osBuild = prfOs.getOsBuild();
#endif
    //}

    //catch (std::exception& ae) {
    //  TRC_ERROR("There was an error during DPA handler creation: " << ae.what());
    //}

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
    m_dpaHandler->UnregisterAsyncMessageHandler();

    delete m_dpaHandler;
    m_dpaHandler = nullptr;

    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::modify(const shape::Properties *props)
  {
  }

  void IqrfDpa::attachInterface(iqrfgw::IIqrfChannelService* iface)
  {
    m_iqrfChannelService = iface;
    m_iqrfDpaChannel = shape_new IqrfDpaChannel(iface);
  }

  void IqrfDpa::detachInterface(iqrfgw::IIqrfChannelService* iface)
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
