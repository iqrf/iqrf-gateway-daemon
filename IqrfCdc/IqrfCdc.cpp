#define IIqrfChannelService_EXPORTS

#include "IqrfCdc.h"

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrfgw__IqrfCdc.hxx"

TRC_INIT_MODULE(iqrf::IqrfCdc);

namespace iqrf {
  IqrfCdc::IqrfCdc()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  IqrfCdc::~IqrfCdc()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfCdc::sendTo(const std::basic_string<unsigned char>& message)
  {
    static int counter = 0;
    DSResponse dsResponse = DSResponse::BUSY;
    int attempt = 0;
    counter++;

    TRC_INFORMATION("Sending to IQRF CDC: " << std::endl << MEM_HEX(message.data(), message.size()));

    while (attempt++ < 4) {
      TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);
      dsResponse = m_cdc->sendData(message);
      if (dsResponse != DSResponse::BUSY)
        break;
      //wait for next attempt
      TRC_DEBUG("Sleep for a while ... ");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (dsResponse != DSResponse::OK) {
      THROW_EXC_TRC_WAR(std::logic_error, "CDC send failed" << PAR(dsResponse));
    }
  }

  void IqrfCdc::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
  {
    TRC_FUNCTION_ENTER("");
    m_receiveFromFunc = receiveFromFunc;
    m_cdc->registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
      m_receiveFromFunc(std::basic_string<unsigned char>(data, length)); });
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfCdc::unregisterReceiveFromHandler()
  {
    TRC_FUNCTION_ENTER("");
    m_receiveFromFunc = ReceiveFromFunc();
    m_cdc->unregisterAsyncMsgListener();
    TRC_FUNCTION_LEAVE("")
  }

  IIqrfChannelService::State IqrfCdc::getState() const
  {
    return IIqrfChannelService::State::Ready;
  }

  void IqrfCdc::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfCdc instance activate" << std::endl <<
      "******************************"
    );

    modify(props);

    m_cdc = shape_new CDCImpl(m_interfaceName.c_str());

    try {
      if (!m_cdc->test()) {
        THROW_EXC_TRC_WAR(std::logic_error, "CDC Test failed");
      }
    }
    catch (CDCImplException & e) {
      THROW_EXC_TRC_WAR(std::logic_error, "CDC Test failed: " << e.getDescr());
    }


    TRC_FUNCTION_LEAVE("")
  }

  void IqrfCdc::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    
    unregisterReceiveFromHandler();
    
    delete m_cdc;
    m_cdc = nullptr;

    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfCdc instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfCdc::modify(const shape::Properties *props)
  {
    props->getMemberAsString("IqrfInterface", m_interfaceName);
  }

  void IqrfCdc::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfCdc::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
