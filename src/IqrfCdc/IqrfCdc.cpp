/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define IIqrfChannelService_EXPORTS

#include "IqrfCdc.h"
#include "CDCImpl.h"
#include "AccessControl.h"
#include <thread>
#include <mutex>
#include <memory>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfCdc.hxx"

TRC_INIT_MODULE(iqrf::IqrfCdc)

namespace iqrf {

  class IqrfCdc::Imp
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
      DSResponse dsResponse = DSResponse::BUSY;
      int attempt = 0;
      counter++;

      TRC_INFORMATION("Sending to IQRF CDC: " << std::endl << MEM_HEX(message.data(), static_cast<uint8_t>(message.size())));

      if (m_cdc) {
        while (attempt++ < 10) {
          TRC_INFORMATION("Trying to sent: " << counter << "." << attempt);
          dsResponse = m_cdc->sendData(message);
          if (dsResponse == DSResponse::OK)
            break;
          TRC_DEBUG("Send failed: " << PAR(dsResponse) << "Sleep for a while and try next attempt ... ");
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (dsResponse == DSResponse::OK) {
          m_accessControl.sniff(message);
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC send failed: " << PAR(dsResponse));
        }
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "CDC not active: " << PAR(dsResponse));
      }
    }

    bool enterProgrammingState()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Entering programming mode.");

      PTEResponse response;
      try {
        if (m_cdc) {
          response = m_cdc->enterProgrammingMode();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC not active");
        }
      }
      catch (std::exception& ex) {
        CATCH_EXC_TRC_WAR(std::exception, ex, "Entering programming mode failed.");
        TRC_FUNCTION_LEAVE("");
        return false;
      }

      if (response != PTEResponse::OK) {
        TRC_WARNING("Entering programming mode failed: " << PAR((int)response));
        TRC_FUNCTION_LEAVE("");
        return false;
      }

      TRC_FUNCTION_LEAVE("");
      return true;
    }

    IIqrfChannelService::UploadErrorCode
      upload(
        const UploadTarget target,
        const std::basic_string<uint8_t>& data,
        const uint16_t address
    )
    {
      // write data to TR module
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Uploading");

      bool useAddress = false;

      unsigned char targetInt = 0;
      switch (target) {
        case UploadTarget::UPLOAD_TARGET_CFG:
          targetInt = 0x80;
          break;
        case UploadTarget::UPLOAD_TARGET_RFPMG:
          targetInt = 0x81;
          break;
        case UploadTarget::UPLOAD_TARGET_RFBAND:
          targetInt = 0x82;
          break;
        case UploadTarget::UPLOAD_TARGET_ACCESS_PWD:
          targetInt = 0x83;
          break;
        case UploadTarget::UPLOAD_TARGET_USER_KEY:
          targetInt = 0x84;
          break;
        case UploadTarget::UPLOAD_TARGET_FLASH:
          targetInt = 0x85;
          useAddress = true;
          break;
        case UploadTarget::UPLOAD_TARGET_INTERNAL_EEPROM:
          targetInt = 0x86;
          useAddress = true;
          break;
        case UploadTarget::UPLOAD_TARGET_EXTERNAL_EEPROM:
          targetInt = 0x87;
          useAddress = true;
          break;
        case UploadTarget::UPLOAD_TARGET_SPECIAL:
          targetInt = 0x88;
          break;
        default:
          targetInt = 0;
      }

      // unsupported target
      if (targetInt == 0) {
        TRC_WARNING("Unsupported target: " << PAR((int)target));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
      }

      PMResponse response;
      try {
        if (!m_cdc) {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC not active");
        }

        // add address into beginning of data to upload
        if (useAddress) {
          std::basic_string<uint8_t> addressAndData;

          addressAndData += address & 0xFF;
          addressAndData += (address >> 8) & 0xFF;
          addressAndData += data;

          response = m_cdc->upload(targetInt, addressAndData);
        }
        else {
          response = m_cdc->upload(targetInt, data);
        }
      }
      catch (std::exception& ex) {
        TRC_WARNING("Uploading failed: " << ex.what());
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_COMMUNICATION;
      }

      if (response == PMResponse::OK) {
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::UploadErrorCode::UPLOAD_NO_ERROR;
      }

      IIqrfChannelService::UploadErrorCode errorCode;
      switch (response)
      {
        case PMResponse::ERR2:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_TARGET_MEMORY;
          break;
        case PMResponse::ERR3:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_DATA_LEN;
          break;
        case PMResponse::ERR4:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_ADDRESS;
          break;
        case PMResponse::ERR5:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_WRITE_ONLY;
          break;
        case PMResponse::ERR6:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_COMMUNICATION;
          break;
        case PMResponse::ERR7:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
          break;
        case PMResponse::BUSY:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_BUSY;
          break;
        default:
          errorCode = IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_GENERAL;
      }

      TRC_FUNCTION_LEAVE("");
      return errorCode;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Terminating programming mode.");

      PTEResponse response;
      try {
        if (m_cdc) {
          response = m_cdc->terminateProgrammingMode();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC not active");
        }
      }
      catch (std::exception& ex) {
        CATCH_EXC_TRC_WAR(std::exception, ex, "Terminating programming mode failed.");
        TRC_FUNCTION_LEAVE("");
        return false;
      }

      if (response != PTEResponse::OK) {
        TRC_WARNING("Programming mode termination failed: " << PAR((int)response));
        TRC_FUNCTION_LEAVE("");
        return false;
      }

      TRC_FUNCTION_LEAVE("");
      return true;
    }

    void startListen()
    {
      try {
        m_cdc = shape_new CDCImpl(m_interfaceName.c_str());

        if (!m_cdc->test()) {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC Test failed");
        }
        m_cdcValid = true;

        m_cdc->resetTRModule();

      }
      catch (CDCImplException & e) {
        CATCH_EXC_TRC_WAR(CDCImplException, e, "CDC Test failed: " << e.getDescr());
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "CDC failed: ");
      }

      if (m_cdc) {
        m_cdc->registerAsyncMsgListener([&](unsigned char* data, unsigned int length) {
          std::basic_string<unsigned char> message(data, length);
          TRC_INFORMATION("Received from IQRF CDC: " << std::endl << MEM_HEX(message.data(), message.size()));
          m_accessControl.messageHandler(message);
        });
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
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const
    {
      return m_accessControl.hasExclusiveAccess();
    }

    IIqrfChannelService::osInfo getTrModuleInfo() {

      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Reading TR module identification.");

      ModuleInfo *myModuleInfoData;
      IIqrfChannelService::osInfo myOsInfo;
      memset(&myOsInfo, 0, sizeof(myOsInfo));

      try {
        if (m_cdc) {
          myModuleInfoData = m_cdc->getTRModuleInfo();
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC not active");
        }
      }
      catch (std::exception& ex) {
        CATCH_EXC_TRC_WAR(std::exception, ex, "Reading TR module identification failed.");
        TRC_FUNCTION_LEAVE("");
        return myOsInfo;
      }

      myOsInfo.osVersionMajor = myModuleInfoData->osVersion / 16;
      myOsInfo.osVersionMinor = myModuleInfoData->osVersion % 16;
      myOsInfo.osBuild = (uint16_t)myModuleInfoData->osBuild[1] << 8 | myModuleInfoData->osBuild[0];

      TRC_FUNCTION_LEAVE("");
      return myOsInfo;
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
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "CDC failed: ");
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
    std::string m_interfaceName;
    AccessControl<IqrfCdc::Imp> m_accessControl;
  };

  //////////////////////////////////////////////////
  IqrfCdc::IqrfCdc()
  {
    m_imp = shape_new Imp();
  }

  IqrfCdc::~IqrfCdc()
  {
    delete m_imp;
  }

  void IqrfCdc::startListen()
  {
    m_imp->startListen();
  }

  IIqrfChannelService::State IqrfCdc::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfCdc::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfCdc::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
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
