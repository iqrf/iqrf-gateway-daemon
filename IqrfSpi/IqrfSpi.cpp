#define IIqrfChannelService_EXPORTS

#include "IqrfSpi.h"
#include "spi_iqrf.h"
#include "sysfs_gpio.h"
#include "machines_def.h"
#include "rapidjson/pointer.h"
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

    bool enterProgrammingState() override;
    UploadErrorCode upload(
      const UploadTarget target, 
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    ) override;
    bool terminateProgrammingState() override;

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

    bool enterProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Entering programming mode.");

      int progModeEnterRes = spi_iqrf_pe();
      if (progModeEnterRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Entering programming mode failed: " << PAR(progModeEnterRes));
        TRC_FUNCTION_LEAVE("");
        return false;
      }

      TRC_FUNCTION_LEAVE("");
      return true;
    }

    
    // try to wait for communication ready state in specified timeout (in ms).
    // returns	last read IQRF SPI status.
    // copied and slightly modified from: spi_example_pgm_hex.c
    spi_iqrf_SPIStatus tryToWaitForPgmReady(uint32_t timeout)
    {
      spi_iqrf_SPIStatus spiStatus = { 0, SPI_IQRF_SPI_DISABLED };
      int operResult = -1;
      uint32_t elapsedTime = 0;
      uint8_t buffer[64];
      unsigned int dataLen = 0;
      uint16_t memStatus = 0x8000;
      uint16_t repStatCounter = 1;

      do
      {
        if (elapsedTime > timeout) {
          TRC_DEBUG("Status: " << PAR(spiStatus.dataNotReadyStatus));
          TRC_DEBUG("Timeout of waiting on ready state expired");
          return spiStatus;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        elapsedTime += 10;

        // getting slave status
        operResult = spi_iqrf_getSPIStatus(&spiStatus);
        if (operResult < 0) {
          TRC_DEBUG("Failed to get SPI status: " << PAR(operResult));
        }
        else {
          if (memStatus != spiStatus.dataNotReadyStatus) {
            if (memStatus != 0x8000) {
              TRC_DEBUG("Status: " << PAR(memStatus));
            }
            memStatus = spiStatus.dataNotReadyStatus;
            repStatCounter = 1;
          }
          else repStatCounter++;
        }

        if (spiStatus.isDataReady == 1) {
          // reading - only to dispose old data if any
          spi_iqrf_read(buffer, spiStatus.dataReady);
        }
      } while (spiStatus.dataNotReadyStatus != SPI_IQRF_SPI_READY_PROG);

      TRC_DEBUG("Status: " << PAR(spiStatus.dataNotReadyStatus));
      return spiStatus;
    }

    IIqrfChannelService::Accessor::UploadErrorCode upload(
      const Accessor::UploadTarget target, 
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    )
    {
      TRC_FUNCTION_ENTER("");
      
      // wait for TR module is ready
      spi_iqrf_SPIStatus spiStatus = tryToWaitForPgmReady(2000);

      // if SPI not ready in 2000 ms, end
      if (spiStatus.dataNotReadyStatus != SPI_IQRF_SPI_READY_PROG) {
        TRC_WARNING("Waiting for ready state failed." << NAME_PAR_HEX(SPI status, spiStatus.dataNotReadyStatus));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_GENERAL;
      }

      // write data to TR module
      TRC_INFORMATION("Uploading");

      bool useAddress = false;

      int targetInt = 0;
      switch (target) {
        case Accessor::UploadTarget::UPLOAD_TARGET_CFG:
          targetInt = CFG_TARGET;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_RFPMG:
          targetInt = RFPMG_TARGET;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_RFBAND:
          targetInt = RFBAND_TARGET;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_ACCESS_PWD:
          targetInt = ACCESS_PWD_TARGET;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_USER_KEY:
          targetInt = USER_KEY_TARGET;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_FLASH:
          targetInt = FLASH_TARGET;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_INTERNAL_EEPROM:
          targetInt = INTERNAL_EEPROM_TARGET;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_EXTERNAL_EEPROM:
          targetInt = EXTERNAL_EEPROM_TARGET;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_SPECIAL:
          targetInt = SPECIAL_TARGET;
          break;
        default:
          targetInt = -1;
      }

      // unsupported target
      if (targetInt == -1) {
        TRC_WARNING("Unsupported target: " << PAR((int)target));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
      }

      int uploadRes = BASE_TYPES_OPER_ERROR;

      // add address into beginning of data to upload
      if (useAddress) {
        std::basic_string<uint8_t> addressAndData;

        addressAndData += address & 0xFF;
        addressAndData += (address >> 8) & 0xFF;
        addressAndData += data;

        uploadRes = spi_iqrf_upload(targetInt, addressAndData.data(), addressAndData.size());
      }
      else {
        uploadRes = spi_iqrf_upload(targetInt, data.data(), data.size());
      }

      // wait for TR module to finish upload operation for at max. 2s
      tryToWaitForPgmReady(2000);

      // check result of write operation
      if (uploadRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Data programming failed. " << NAME_PAR_HEX(Result, uploadRes));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_GENERAL;
      }
      else {
        TRC_INFORMATION("Upload OK");
      }

      TRC_FUNCTION_LEAVE("");
      return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
    }

    bool terminateProgrammingState() {
      TRC_INFORMATION("Terminating programming mode.");

      int progModeTerminateRes = spi_iqrf_pe();
      if (progModeTerminateRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Programming mode termination failed: " << PAR(progModeTerminateRes));
        return false;
      }

      return true;
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

      using namespace rapidjson;

      try {
        spi_iqrf_config_struct cfg = { {}, ENABLE_GPIO, CE0_GPIO, MISO_GPIO, MOSI_GPIO, SCLK_GPIO };

        Document d;
        d.CopyFrom(props->getAsJson(), d.GetAllocator());

        Value* comName = Pointer("/IqrfInterface").Get(d);
        if (comName != nullptr && comName->IsString()) {
          m_interfaceName = comName->GetString();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /IqrfInterface");
        }

        memset(cfg.spiDev, 0, sizeof(cfg.spiDev));
        auto sz = m_interfaceName.size();
        if (sz > sizeof(cfg.spiDev)) sz = sizeof(cfg.spiDev);
        std::copy(m_interfaceName.c_str(), m_interfaceName.c_str() + sz, cfg.spiDev);

        cfg.enableGpioPin = (uint8_t)Pointer("/enableGpioPin").GetWithDefault(d, (int)cfg.enableGpioPin).GetInt();
        cfg.spiCe0GpioPin = (uint8_t)Pointer("/spiCe0GpioPin").GetWithDefault(d, (int)cfg.spiCe0GpioPin).GetInt();
        cfg.spiMisoGpioPin = (uint8_t)Pointer("/spiMisoGpioPin").GetWithDefault(d, (int)cfg.spiMisoGpioPin).GetInt();
        cfg.spiMosiGpioPin = (uint8_t)Pointer("/spiMosiGpioPin").GetWithDefault(d, (int)cfg.spiMosiGpioPin).GetInt();
        cfg.spiClkGpioPin = (uint8_t)Pointer("/spiClkGpioPin").GetWithDefault(d, (int)cfg.spiClkGpioPin).GetInt();

        // for sake of upload
        cfg.spiPgmSwGpioPin = PGM_SW_GPIO;

        TRC_INFORMATION(PAR(m_interfaceName));

        int attempts = 1;
        int res = BASE_TYPES_OPER_ERROR;
        while (attempts < 3) {
          res = spi_iqrf_initAdvanced(&cfg);
          if (BASE_TYPES_OPER_OK == res) {
            break;
          }

          TRC_WARNING(PAR(m_interfaceName) << PAR(attempts) << " Create IqrfInterface failure");
          ++attempts;
          std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }

        if (BASE_TYPES_OPER_OK == res) {
          TRC_WARNING(PAR(m_interfaceName) << " Created");
          m_rx = shape_new unsigned char[m_bufsize];
          memset(m_rx, 0, m_bufsize);
          m_runListenThread = true;
          m_listenThread = std::thread(&IqrfSpi::Imp::listen, this);
        }
        else {
          TRC_WARNING(PAR(m_interfaceName) << " Cannot create IqrfInterface");
        }

      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, PAR(m_interfaceName) << " Cannot create IqrfInterface");
      }

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

  bool IqrfSpiAccessor::enterProgrammingState()
  {
    return m_IqrfSpi->enterProgrammingState();
  }

  IIqrfChannelService::Accessor::UploadErrorCode IqrfSpiAccessor::upload(
    const UploadTarget target, 
    const std::basic_string<uint8_t>& data,
    const uint16_t address
  )
  {
    return m_IqrfSpi->upload(target, data, address);
  }

  bool IqrfSpiAccessor::terminateProgrammingState() {
    return m_IqrfSpi->terminateProgrammingState();
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
