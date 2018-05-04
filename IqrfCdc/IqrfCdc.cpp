#define IIqrfChannelService_EXPORTS

#include "IqrfCdc.h"
#include "CdcInterface.h"
#include "CDCImpl.h"
#include <thread>

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfCdc.hxx"

TRC_INIT_MODULE(iqrf::IqrfCdc);

namespace iqrf {
  class IqrfCdcAccessor : public IIqrfChannelService::Accessor
  {
  public:
    IqrfCdcAccessor(IqrfCdc::Imp * const iqrfCdc, IIqrfChannelService::AccesType accesType);
    void send(const std::basic_string<unsigned char>& message) override;
    IIqrfChannelService::AccesType getAccessType() override;
    virtual ~IqrfCdcAccessor();
  private:
    IqrfCdc::Imp * m_iqrfCdc = nullptr;
    IIqrfChannelService::AccesType m_type = IIqrfChannelService::AccesType::Normal;
  };

  class IqrfCdc::Imp
  {
  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void sendTo(const std::basic_string<unsigned char>& message, IIqrfChannelService::AccesType access)
    {
      switch (access)
      {
      case AccesType::Normal:
        if (!m_exclusiveReceiveFromFunc) {
          send(message);
        }
        else {
          TRC_WARNING("Cannot send: Exclusive access is active");
        }
        break;
      case AccesType::Exclusive:
        send(message);
        break;
      case AccesType::Sniffer:
        TRC_WARNING("Cannot send via sniffer access");
        break;
      default:;
      }
    }

    void send(const std::basic_string<unsigned char>& message)
    {
      static int counter = 0;
      DSResponse dsResponse = DSResponse::BUSY;
      int attempt = 0;
      counter++;

      TRC_INFORMATION("Sending to IQRF CDC: " << std::endl << MEM_HEX(message.data(), message.size()));
      
      if (m_cdc) {
        while (attempt++ < 4) {
          TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);
          dsResponse = m_cdc->sendData(message);
          if (dsResponse != DSResponse::BUSY)
            break;
          //wait for next attempt
          TRC_DEBUG("Sleep for a while ... ");
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (dsResponse == DSResponse::OK) {
          if (m_snifferFromFunc) {
            m_snifferFromFunc(message);
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC send failed" << PAR(dsResponse));
        }
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "CDC not active" << PAR(dsResponse));
      }
    }

    IIqrfChannelService::State getState() const
    {
      if (m_cdc && m_cdcValid) {
        return IIqrfChannelService::State::Ready;
      }
      else {
        return IIqrfChannelService::State::NotReady;
      }
    }

    std::unique_ptr<IIqrfChannelService::Accessor>  getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
    {
      TRC_FUNCTION_ENTER("");
      //TODO if not exclusive
      std::unique_ptr<IIqrfChannelService::Accessor> retval(shape_new IqrfCdcAccessor(this, access));
      switch (access)
      {
      case AccesType::Normal:
        m_receiveFromFunc = receiveFromFunc;
        break;
      case AccesType::Exclusive:
        m_exclusiveReceiveFromFunc = receiveFromFunc;
        break;
      case AccesType::Sniffer:
        m_snifferFromFunc = receiveFromFunc;
        break;
      default:;
      }
      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    void resetAccess(AccesType access)
    {
      TRC_FUNCTION_ENTER("");
      switch (access)
      {
      case AccesType::Normal:
        m_receiveFromFunc = ReceiveFromFunc();
        break;
      case AccesType::Exclusive:
        m_exclusiveReceiveFromFunc = ReceiveFromFunc();
        break;
      case AccesType::Sniffer:
        m_snifferFromFunc = ReceiveFromFunc();
        break;
      default:;
      }
      TRC_FUNCTION_LEAVE("")
    }

    void messageHandler(const std::basic_string<unsigned char>& message)
    {
      if (!m_exclusiveReceiveFromFunc && m_receiveFromFunc) {
        m_receiveFromFunc(message);
      }
      else if (m_exclusiveReceiveFromFunc) {
        m_exclusiveReceiveFromFunc(message);
      }
      else {
        TRC_WARNING("Cannot receive: no access is active");
      }

      if (m_snifferFromFunc) {
        m_snifferFromFunc(message);
      }
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfCdc instance activate" << std::endl <<
        "******************************"
      );

      try {
        modify(props);
        m_cdc = shape_new CDCImpl(m_interfaceName.c_str());
        if (!m_cdc->test()) {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC Test failed");
        }
        m_cdcValid = true;
      }
      catch (CDCImplException & e) {
        CATCH_EXC_TRC_WAR(CDCImplException, e, "CDC Test failed: " << e.getDescr());
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "CDC failed: ");
      }

      if (m_cdc) {
        m_cdc->registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
          messageHandler(std::basic_string<unsigned char>(data, length)); });
      }

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      if (m_cdc) {
        m_cdc->unregisterAsyncMsgListener();
      }

      delete m_cdc;
      m_cdc = nullptr;

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfCdc instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      props->getMemberAsString("IqrfInterface", m_interfaceName);
      TRC_INFORMATION(PAR(m_interfaceName));
    }


  private:
    CDCImpl* m_cdc = nullptr;
    bool m_cdcValid = false;
    IIqrfChannelService::ReceiveFromFunc m_receiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_exclusiveReceiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_snifferFromFunc;
    std::string m_interfaceName;
  };

  //////////////////////////////////////////////////
  IqrfCdcAccessor::IqrfCdcAccessor(IqrfCdc::Imp * iqrfCdc, IIqrfChannelService::AccesType accesType)
    :m_iqrfCdc(iqrfCdc)
    , m_type(accesType)
  {
  }

  void IqrfCdcAccessor::send(const std::basic_string<unsigned char>& message)
  {
    m_iqrfCdc->sendTo(message, m_type);
  }

  IIqrfChannelService::AccesType IqrfCdcAccessor::getAccessType()
  {
    return m_type;
  }

  IqrfCdcAccessor::~IqrfCdcAccessor()
  {
    m_iqrfCdc->resetAccess(m_type);
  }

  //////////////////////////////////////////////////
  IqrfCdc::IqrfCdc()
  {
    m_imp = shape_new Imp();
  }

  IqrfCdc::~IqrfCdc()
  {
    delete m_imp;
  }

  IIqrfChannelService::State IqrfCdc::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfCdc::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  void IqrfCdc::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfCdc::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfCdc::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
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
