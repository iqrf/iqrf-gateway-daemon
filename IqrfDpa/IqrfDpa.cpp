#define IIqrfDpaService_EXPORTS

//#include "DpaTransactionTask.h"
#include "IqrfDpa.h"
#include "Trace.h"
#include "rapidjson/pointer.h"

//TODO workaround old tracing 
#include "IqrfLogging.h"
TRC_INIT();

#include "iqrf__IqrfDpa.hxx"

TRC_INIT_MODULE(iqrf::IqrfDpa);

namespace iqrf {
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

  std::shared_ptr<IDpaTransaction2> IqrfDpa::executeDpaTransaction(const DpaMessage& request, int32_t timeout)
  {
    TRC_FUNCTION_ENTER("");
    auto result = m_dpaHandler->executeDpaTransaction(request, timeout);
    TRC_FUNCTION_LEAVE("");
    return result;
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

  //TODO for testing async - remove
  //"request": "00.00.0a.00.ff.ff"
  //"response" : "00.00.0a.80.00.00.00.00.1b.be.01"
  //void IqrfDpa::testAsync()
  //{
  //  while (true) {
  //    //std::vector<uint8_t> v = { 0x00, 0x00, 0x0a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1b, 0xbe, 0x01 };
  //    std::vector<uint8_t> v = { 0x00, 0x00, 0x0a, 0x00, 0xff, 0xff };
  //    DpaMessage msg;
  //    msg.DataToBuffer(v.data(), v.size());
  //    asyncDpaMessageHandler(msg);
  //    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  //  }
  //}

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

    m_dpaHandler = shape_new DpaHandler2(m_iqrfDpaChannel);

    const rapidjson::Document& doc = props->getAsJson();
    const rapidjson::Value* valT = rapidjson::Pointer("/DpaHandlerTimeout").Get(doc);
    if (valT && valT->IsInt()) {
      m_dpaHandlerTimeout = valT->GetInt();
      m_dpaHandler->setTimeout(m_dpaHandlerTimeout);
    }
    m_dpaHandler->setTimeout(m_dpaHandlerTimeout);

    const rapidjson::Value* valC = rapidjson::Pointer("/CommunicationMode").Get(doc);
    if (valC && valC->IsString()) {
      std::string communicationMode = valC->GetString();
      if (communicationMode == "LP")
        m_rfMode = IDpaTransaction2::kLp;
      else if (communicationMode == "STD")
        m_rfMode = IDpaTransaction2::kStd;
      else
        m_rfMode = IDpaTransaction2::kStd;
    }
    m_dpaHandler->setRfCommunicationMode(m_rfMode);

    //Async msg handling
    m_dpaHandler->registerAsyncMessageHandler("", [&](const DpaMessage& dpaMessage) {
      asyncDpaMessageHandler(dpaMessage);
    });

    //TODO for testing async - remove
    //m_thd = std::thread(&IqrfDpa::testAsync, this);
    //m_thd.detach();

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
