#define IIqrfChannelService_EXPORTS

#include "IqrfSpi.h"
#include "spi_iqrf.h"
#include "sysfs_gpio.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfSpi.hxx"

TRC_INIT_MODULE(iqrf::IqrfSpi);

const unsigned SPI_REC_BUFFER_SIZE = 1024;

namespace iqrf {
  class IqrfSpiAccessor : public IIqrfChannelService::Accessor
  {
  public:
    IqrfSpiAccessor(IqrfSpi::Imp * const IqrfSpi, IIqrfChannelService::AccesType accesType);
    void send(const std::basic_string<unsigned char>& message) override;
    IIqrfChannelService::AccesType getAccessType() override;
    virtual ~IqrfSpiAccessor();
  private:
    IqrfSpi::Imp * m_IqrfSpi = nullptr;
    IIqrfChannelService::AccesType m_type = IIqrfChannelService::AccesType::Normal;
  };

  class IqrfSpi::Imp
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
      int attempt = 0;
      counter++;

      TRC_INFORMATION("Sending to IQRF SPI: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      while (attempt++ < 4) {
        TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);

        // lock scope
        {
          std::lock_guard<std::mutex> lck(m_commMutex);

          // get status
          spi_iqrf_SPIStatus status;
          int retval = spi_iqrf_getSPIStatus(&status);
          if (BASE_TYPES_OPER_OK != retval) {
            THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_getSPIStatus() failed: " << PAR(retval));
          }

          if (status.dataNotReadyStatus == SPI_IQRF_SPI_READY_COMM) {
            int retval = spi_iqrf_write((void*)message.data(), message.size());
            if (BASE_TYPES_OPER_OK != retval) {
              THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_write()() failed: " << PAR(retval));
            }
            break;
          }
          else {
            TRC_INFORMATION(PAR_HEX(status.isDataReady) << PAR_HEX(status.dataNotReadyStatus));
          }
        }
        //wait for next attempt
        TRC_DEBUG("Sleep for a while ... ");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    IIqrfChannelService::State getState() const
    {
      IIqrfChannelService::State state = State::NotReady;
      spi_iqrf_SPIStatus spiStatus1, spiStatus2;
      int ret = 1;

      {
        std::lock_guard<std::mutex> lck(m_commMutex);

        ret = spi_iqrf_getSPIStatus(&spiStatus1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ret = spi_iqrf_getSPIStatus(&spiStatus2);
      }

      switch (ret) {
      case BASE_TYPES_OPER_OK:
        if (spiStatus1.dataNotReadyStatus == SPI_IQRF_SPI_READY_COMM && spiStatus2.dataNotReadyStatus == SPI_IQRF_SPI_READY_COMM) {
          state = State::Ready;
        }
        else {
          TRC_INFORMATION("SPI status1: " << PAR(spiStatus1.dataNotReadyStatus));
          TRC_INFORMATION("SPI status2: " << PAR(spiStatus2.dataNotReadyStatus));
          state = State::NotReady;
        }
        break;

      default:
        state = State::NotReady;
        break;
      }

      return state;
    }

    std::unique_ptr<IIqrfChannelService::Accessor>  getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
    {
      TRC_FUNCTION_ENTER("");
      //TODO if not exclusive
      std::unique_ptr<IIqrfChannelService::Accessor> retval(shape_new IqrfSpiAccessor(this, access));
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
        "IqrfSpi instance activate" << std::endl <<
        "******************************"
      );

      modify(props);
      
      m_rx = shape_new unsigned char[m_bufsize];
      memset(m_rx, 0, m_bufsize);

      int retval = spi_iqrf_init(m_interfaceName.c_str());
      if (BASE_TYPES_OPER_OK != retval) {
        delete[] m_rx;
        m_rx = nullptr;
        THROW_EXC_TRC_WAR(std::logic_error, "Communication interface has not been open.");
      }

      m_runListenThread = true;
      m_listenThread = std::thread(&IqrfSpi::Imp::listen, this);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_runListenThread = false;

      TRC_DEBUG("joining udp listening thread");
      if (m_listenThread.joinable())
        m_listenThread.join();
      TRC_DEBUG("listening thread joined");

      spi_iqrf_destroy();

      delete[] m_rx;

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfSpi instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      props->getMemberAsString("IqrfInterface", m_interfaceName);
    }

    void listen()
    {
      TRC_FUNCTION_ENTER("thread starts");

      try {
        TRC_DEBUG("SPI is ready");

        while (m_runListenThread)
        {
          int recData = 0;

          // lock scope
          {
            std::lock_guard<std::mutex> lck(m_commMutex);

            // get status
            spi_iqrf_SPIStatus status;
            int retval = spi_iqrf_getSPIStatus(&status);
            if (BASE_TYPES_OPER_OK != retval) {
              THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_getSPIStatus() failed: " << PAR(retval));
            }

            if (status.isDataReady) {

              if (status.dataReady > m_bufsize) {
                THROW_EXC_TRC_WAR(std::logic_error, "Received data too long: " << NAME_PAR(len, status.dataReady) << PAR(m_bufsize));
              }

              // reading
              int retval = spi_iqrf_read(m_rx, status.dataReady);
              if (BASE_TYPES_OPER_OK != retval) {
                THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_read() failed: " << PAR(retval));
              }
              recData = status.dataReady;
            }
          }

          // unlocked - possible to write in receiveFromFunc
          if (recData) {
            std::basic_string<unsigned char> message(m_rx, recData);
            messageHandler(message);
          }

          // checking every 10ms
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      catch (std::logic_error& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "listening thread error");
        m_runListenThread = false;
      }
      TRC_WARNING("thread stopped");
    }

  private:
    IIqrfChannelService::ReceiveFromFunc m_receiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_exclusiveReceiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_snifferFromFunc;
    std::string m_interfaceName;

    std::atomic_bool m_runListenThread;
    std::thread m_listenThread;

    std::string m_port;

    unsigned char* m_rx = nullptr;
    unsigned m_bufsize = SPI_REC_BUFFER_SIZE;

    mutable std::mutex m_commMutex;

  };

  //////////////////////////////////////////////////
  IqrfSpiAccessor::IqrfSpiAccessor(IqrfSpi::Imp * IqrfSpi, IIqrfChannelService::AccesType accesType)
    :m_IqrfSpi(IqrfSpi)
    , m_type(accesType)
  {
  }

  void IqrfSpiAccessor::send(const std::basic_string<unsigned char>& message)
  {
    m_IqrfSpi->sendTo(message, m_type);
  }

  IIqrfChannelService::AccesType IqrfSpiAccessor::getAccessType()
  {
    return m_type;
  }

  IqrfSpiAccessor::~IqrfSpiAccessor()
  {
    m_IqrfSpi->resetAccess(m_type);
  }

  //////////////////////////////////////////////////
  IqrfSpi::IqrfSpi()
  {
    m_imp = shape_new Imp();
  }

  IqrfSpi::~IqrfSpi()
  {
    delete m_imp;
  }

  IIqrfChannelService::State IqrfSpi::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfSpi::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  void IqrfSpi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfSpi::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfSpi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void IqrfSpi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfSpi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
