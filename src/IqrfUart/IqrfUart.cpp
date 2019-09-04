#define IIqrfChannelService_EXPORTS

#include "IqrfUart.h"
#include "uart_iqrf.h"
#include "machines_def.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>

#ifndef SHAPE_PLATFORM_WINDOWS
#include <termios.h>
#endif

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfUart.hxx"

TRC_INIT_MODULE(iqrf::IqrfUart);

const unsigned SPI_REC_BUFFER_SIZE = 1024;

//TODO implement programming mode as in IqrfSpi
namespace iqrf {

  class IqrfUart::Imp
  {
  public:
    Imp()
      :m_accessControl(this)
    {
    }

    ~Imp()
    {
    }

    void send(const std::basic_string<unsigned char>& message)
    {
      static int counter = 0;
      int attempt = 0;
      counter++;

      TRC_INFORMATION("Sending to IQRF UART: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      while (attempt++ < 4) {
        TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);

        // lock scope
        {
          //std::lock_guard<std::mutex> lck(m_commMutex);

          int retval = uart_iqrf_write((uint8_t*)message.data(), static_cast<unsigned int>(message.size()));
          if (BASE_TYPES_OPER_OK == retval) {
            m_accessControl.sniff(message); //send to sniffer if set
          }
          else {
            THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_write()() failed: " << PAR(retval));
          }
          break;
        }
        //wait for next attempt
        TRC_DEBUG("Sleep for a while ... ");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    bool enterProgrammingState() {
      TRC_FUNCTION_ENTER("");
      //TRC_INFORMATION("Entering programming mode.");
      TRC_WARNING("Not implemented");
#if 0   

      int progModeEnterRes = spi_iqrf_pe();
      if (progModeEnterRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Entering programming mode failed: " << PAR(progModeEnterRes));
        TRC_FUNCTION_LEAVE("");
        return false;
      }
#endif
      TRC_FUNCTION_LEAVE("");
      //return true;
      return false;
    }

#if 0   
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
#endif

    IIqrfChannelService::UploadErrorCode upload(
      const UploadTarget target,
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    )
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Not implemented");
      //silence -Wunused-parameter
      (void)target; 
      (void)data;
      (void)address;

#if 0      
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
#endif

      TRC_FUNCTION_LEAVE("");
      //return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
      return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
    }

    bool terminateProgrammingState() {
      TRC_INFORMATION("Terminating programming mode.");
      TRC_WARNING("Not implemented");
#if 0
      int progModeTerminateRes = spi_iqrf_pe();
      if (progModeTerminateRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Programming mode termination failed: " << PAR(progModeTerminateRes));
        return false;
      }
#endif
      //return true;
      return false;
    }

    void startListen()
    {
      m_runListenThread = true;
      m_listenThread = std::thread(&IqrfUart::Imp::listen, this);
    }

    IIqrfChannelService::State getState() const
    {
      IIqrfChannelService::State state = State::NotReady;
      if (m_accessControl.hasExclusiveAccess())
        state = State::ExclusiveAccess;
      else if (m_runListenThread)
        state = State::Ready;

      return state;
    }

    std::unique_ptr<IIqrfChannelService::Accessor>  getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
    {
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const
    {
      return m_accessControl.hasExclusiveAccess();
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Reading TR module identification - not implemented.");

      IIqrfChannelService::osInfo myOsInfo;
      memset(&myOsInfo, 0, sizeof(myOsInfo));

      TRC_FUNCTION_LEAVE("");
      return myOsInfo;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfUart instance activate" << std::endl <<
        "******************************"
      );

      using namespace rapidjson;

      try {

        Document d;
        d.CopyFrom(props->getAsJson(), d.GetAllocator());

        Value* comName = Pointer("/IqrfInterface").Get(d);
        if (comName != nullptr && comName->IsString()) {
          m_interfaceName = comName->GetString();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /IqrfInterface");
        }

        Value* baudRate = Pointer("/baudRate").Get(d);
        if (baudRate != nullptr && baudRate->IsInt()) {
          m_baudRate = baudRate->GetInt();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /IqrfInterface");
        }

        memset(m_cfg.uartDev, 0, sizeof(m_cfg.uartDev));
        auto sz = m_interfaceName.size();
        if (sz > sizeof(m_cfg.uartDev)) sz = sizeof(m_cfg.uartDev);
        std::copy(m_interfaceName.c_str(), m_interfaceName.c_str() + sz, m_cfg.uartDev);

        m_cfg.baudRate = get_baud(m_baudRate);

        m_cfg.powerEnableGpioPin = (uint8_t)Pointer("/powerEnableGpioPin").GetWithDefault(d, (int)m_cfg.powerEnableGpioPin).GetInt();
        m_cfg.busEnableGpioPin = (uint8_t)Pointer("/busEnableGpioPin").GetWithDefault(d, (int)m_cfg.busEnableGpioPin).GetInt();
        m_cfg.pgmSwitchGpioPin = -1;

        TRC_INFORMATION(PAR(m_interfaceName) << PAR(m_baudRate));

        int attempts = 1;
        int res = BASE_TYPES_OPER_ERROR;
        while (attempts < 3) {

          res = uart_iqrf_init(&m_cfg);

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

#ifdef SHAPE_PLATFORM_WINDOWS
    int get_baud(int baud) { return baud; }
#else
    // converts integer baud to Linux define
    int get_baud(int baud)
    {
      switch (baud) {
      case 9600:
        return B9600;
      case 19200:
        return B19200;
      case 38400:
        return B38400;
      case 57600:
        return B57600;
      case 115200:
        return B115200;
      default:
        return -1;
      }
    }
#endif

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_runListenThread = false;

      TRC_DEBUG("joining udp listening thread");
      if (m_listenThread.joinable())
        m_listenThread.join();
      TRC_DEBUG("listening thread joined");

      uart_iqrf_destroy();

      delete[] m_rx;

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfUart instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void listen()
    {
      TRC_FUNCTION_ENTER("thread starts");

      try {
        while (m_runListenThread)
        {
          int recData = 0;

          // reading
          uint8_t reclen = 0;
          int retval = uart_iqrf_read(m_rx, &reclen, 100); //waits for 100 ms
            
          switch (retval) {
          case BASE_TYPES_OPER_OK:
          case UART_IQRF_ERROR_TIMEOUT:
            recData = reclen;
            break;
          case BASE_TYPES_OPER_ERROR:
          case UART_IQRF_ERROR_CRC:
            TRC_WARNING("uart_iqrf_read() failed: " << PAR(retval));
            break;
          case BASE_TYPES_LIB_NOT_INITIALIZED:
          default:
            THROW_EXC_TRC_WAR(std::logic_error, "uart_iqrf_read() failed: " << PAR(retval));
          }

          if (recData > 0) {
            TRC_DEBUG(PAR(recData));
            std::basic_string<unsigned char> message(m_rx, recData);
            m_accessControl.messageHandler(message);
          }

        }
      }
      catch (std::logic_error& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "listening thread error");
        m_runListenThread = false;
      }
      TRC_WARNING("thread stopped");
    }

  private:
    AccessControl<IqrfUart::Imp> m_accessControl;
    std::string m_interfaceName;
    int m_baudRate = 0;

    std::atomic_bool m_runListenThread;
    std::thread m_listenThread;

    std::string m_port;

    unsigned char* m_rx = nullptr;
    unsigned m_bufsize = SPI_REC_BUFFER_SIZE;

    //mutable std::mutex m_commMutex;

    T_UART_IQRF_CONFIG_STRUCT m_cfg;

  };

  //////////////////////////////////////////////////
  IqrfUart::IqrfUart()
  {
    m_imp = shape_new Imp();
  }

  IqrfUart::~IqrfUart()
  {
    delete m_imp;
  }

  void IqrfUart::startListen()
  {
    return m_imp->startListen();
  }

  IIqrfChannelService::State IqrfUart::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfUart::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfUart::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
  }

  void IqrfUart::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfUart::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfUart::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void IqrfUart::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfUart::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
