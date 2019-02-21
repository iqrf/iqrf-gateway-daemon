/*
* Copyright 2015 MICRORISC s.r.o.
* Copyright 2018 IQRF Tech s.r.o.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define IIqrfChannelService_EXPORTS

#include "IqrfSpi.h"
#include "spi_iqrf.h"
#include "machines_def.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>
#include <condition_variable>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfSpi.hxx"


TRC_INIT_MODULE(iqrf::IqrfSpi);

const unsigned SPI_REC_BUFFER_SIZE = 1024;

namespace iqrf {

  class IqrfSpi::Imp
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

      TRC_INFORMATION("Sending to IQRF SPI: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      while (attempt++ < 11) {
        TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);

        // lock scope
        {
          std::unique_lock<std::mutex> lck(m_commMutex);

          // get status
          spi_iqrf_SPIStatus status;
          int retval = spi_iqrf_getSPIStatus(&status);
          if (BASE_TYPES_OPER_OK != retval) {
            THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_getSPIStatus() failed: " << PAR(retval));
          }

          if (status.dataNotReadyStatus == SPI_IQRF_SPI_READY_COMM) {
            int retval = spi_iqrf_write((void*)message.data(), static_cast<unsigned int>(message.size()));
            if (BASE_TYPES_OPER_OK == retval) {
              m_accessControl.sniff(message); //send to sniffer if set
            }
            else {
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

    bool enterProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Entering programming mode.");

      int progModeEnterRes = BASE_TYPES_OPER_ERROR;

      {
        std::unique_lock<std::mutex> lck(m_commMutex);
        progModeEnterRes = spi_iqrf_pe();

        if (progModeEnterRes == BASE_TYPES_OPER_OK) {
          m_pgmState = true;
        }
        else {
          TRC_WARNING("Entering programming mode spi_iqrf_pe() failed: " << PAR(progModeEnterRes));
          m_pgmState = false;
        }
      }
      m_condVar.notify_all();

      TRC_FUNCTION_LEAVE(PAR(m_pgmState));
      return m_pgmState;
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
        {
          std::unique_lock<std::mutex> lck(m_commMutex);

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
      spi_iqrf_SPIStatus spiStatus = tryToWaitForPgmReady(1000);

      // if SPI not ready in 1000 ms, end
      if (spiStatus.dataNotReadyStatus != SPI_IQRF_SPI_READY_PROG) {
        TRC_WARNING("Waiting for ready state failed." << NAME_PAR_HEX(SPI status, spiStatus.dataNotReadyStatus));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_GENERAL;
      }

      // write data to TR module
      TRC_INFORMATION("Uploading...");

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
      {
        std::unique_lock<std::mutex> lck(m_commMutex);

        if (useAddress) {
          std::basic_string<uint8_t> addressAndData;

          addressAndData += address & 0xFF;
          addressAndData += (address >> 8) & 0xFF;
          addressAndData += data;

          uploadRes = spi_iqrf_upload(targetInt, addressAndData.data(), static_cast<unsigned int>(addressAndData.size()));
        }
        else {
          uploadRes = spi_iqrf_upload(targetInt, data.data(), static_cast<unsigned int>(data.size()));
        }
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

      int progModeTerminateRes = 0;

      {
        std::unique_lock<std::mutex> lck(m_commMutex);
        progModeTerminateRes = spi_iqrf_pt();
        m_pgmState = false;
      }
      m_condVar.notify_all();

      if (progModeTerminateRes != BASE_TYPES_OPER_OK) {
        TRC_WARNING("Programming mode termination failed: " << PAR(progModeTerminateRes));
        return false;
      }

      return true;
    }

    void startListen()
    {
      m_runListenThread = true;
      m_listenThread = std::thread(&IqrfSpi::Imp::listen, this);
    }

    IIqrfChannelService::State getState() const
    {
      IIqrfChannelService::State state = State::NotReady;
      spi_iqrf_SPIStatus spiStatus1, spiStatus2;
      int ret = 1;

      {
        std::unique_lock<std::mutex> lck(m_commMutex);

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
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const
    {
      return m_accessControl.hasExclusiveAccess();
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
        m_cfg = { {0}, POWER_ENABLE_GPIO, BUS_ENABLE_GPIO, PGM_SWITCH_GPIO };

        Document d;
        d.CopyFrom(props->getAsJson(), d.GetAllocator());

        Value* comName = Pointer("/IqrfInterface").Get(d);
        if (comName != nullptr && comName->IsString()) {
          m_interfaceName = comName->GetString();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /IqrfInterface");
        }

        memset(m_cfg.spiDev, 0, sizeof(m_cfg.spiDev));
        auto sz = m_interfaceName.size();
        if (sz > sizeof(m_cfg.spiDev)) sz = sizeof(m_cfg.spiDev);
        std::copy(m_interfaceName.c_str(), m_interfaceName.c_str() + sz, m_cfg.spiDev);

        m_cfg.powerEnableGpioPin = (uint8_t)Pointer("/powerEnableGpioPin").GetWithDefault(d, (int)m_cfg.powerEnableGpioPin).GetInt();
        m_cfg.busEnableGpioPin = (uint8_t)Pointer("/busEnableGpioPin").GetWithDefault(d, (int)m_cfg.busEnableGpioPin).GetInt();
        m_cfg.pgmSwitchGpioPin = (uint8_t)Pointer("/pgmSwitchGpioPin").GetWithDefault(d, (int)m_cfg.pgmSwitchGpioPin).GetInt();
        m_cfg.trModuleReset = TR_MODULE_RESET_ENABLE;
        Value* v = Pointer("/spiReset").Get(d);
        if (v && v->IsBool())
          m_cfg.trModuleReset = v->GetBool() ? TR_MODULE_RESET_ENABLE : TR_MODULE_RESET_DISABLE;

        TRC_INFORMATION(PAR(m_interfaceName));

        int attempts = 1;
        int res = BASE_TYPES_OPER_ERROR;
        while (attempts < 3) {
          res = spi_iqrf_initAdvanced(&m_cfg);
          if (BASE_TYPES_OPER_OK == res) {
            break;
          }

          TRC_WARNING(PAR(m_interfaceName) << PAR(attempts) << " Create IqrfInterface failure");
          ++attempts;
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_runListenThread = false;

      TRC_DEBUG("joining spi listening thread");
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
      (void)props; //silence -Wunused-parameter
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
            std::unique_lock<std::mutex> lck(m_commMutex);
            m_condVar.wait_for(lck, std::chrono::milliseconds(10)); //block for 10 ms - lck released when blocking and resumed if unblocked
            //meantime pgm can be set so verify
            m_condVar.wait(lck, [&]() { return !m_pgmState; }); //block if pgmState - lck released when blocking and resumed if unblocked

            // get status
            spi_iqrf_SPIStatus status;
            int retval = spi_iqrf_getSPIStatus(&status);
            if (BASE_TYPES_OPER_OK != retval) {
              THROW_EXC_TRC_WAR(std::logic_error, "spi_iqrf_getSPIStatus() failed: " << PAR(retval));
            }

            if (status.isDataReady) {

              if (status.dataReady > 0 && static_cast<unsigned>(status.dataReady) > m_bufsize) {
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
    AccessControl<IqrfSpi::Imp> m_accessControl;
    std::string m_interfaceName;

    std::atomic_bool m_runListenThread;
    std::thread m_listenThread;

    std::string m_port;

    unsigned char* m_rx = nullptr;
    unsigned m_bufsize = SPI_REC_BUFFER_SIZE;

    mutable std::mutex m_commMutex;
    std::condition_variable m_condVar;
    bool m_pgmState = false;
    spi_iqrf_config_struct m_cfg;
  };

  //////////////////////////////////////////////////
  IqrfSpi::IqrfSpi()
  {
    m_imp = shape_new Imp();
  }

  IqrfSpi::~IqrfSpi()
  {
    delete m_imp;
  }

  void IqrfSpi::startListen()
  {
    return m_imp->startListen();
  }

  IIqrfChannelService::State IqrfSpi::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfSpi::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfSpi::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
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
