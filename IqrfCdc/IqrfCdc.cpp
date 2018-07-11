#define IIqrfChannelService_EXPORTS

#include "IqrfCdc.h"
#include "CdcInterface.h"
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

TRC_INIT_MODULE(iqrf::IqrfCdc);

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
          m_accessControl.sniff(message);
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "CDC send failed" << PAR(dsResponse));
        }
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "CDC not active" << PAR(dsResponse));
      }
    }

    bool enterProgrammingState() 
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Entering programming mode.");

      PTEResponse response;
      try {
        response = m_cdc->enterProgrammingMode();
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

    IIqrfChannelService::Accessor::UploadErrorCode 
      upload(
        const Accessor::UploadTarget target, 
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
        case Accessor::UploadTarget::UPLOAD_TARGET_CFG:
          targetInt = 0x80;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_RFPMG:
          targetInt = 0x81;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_RFBAND:
          targetInt = 0x82;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_ACCESS_PWD:
          targetInt = 0x83;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_USER_KEY:
          targetInt = 0x84;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_FLASH:
          targetInt = 0x85;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_INTERNAL_EEPROM:
          targetInt = 0x86;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_EXTERNAL_EEPROM:
          targetInt = 0x87;
          useAddress = true;
          break;
        case Accessor::UploadTarget::UPLOAD_TARGET_SPECIAL:
          targetInt = 0x88;
          break;
        default:
          targetInt = 0;
      }

      // unsupported target
      if (targetInt == 0) {
        TRC_WARNING("Unsupported target: " << PAR((int)target));
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
      }
      
      PMResponse response;
      try {
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
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_COMMUNICATION;
      }

      if (response == PMResponse::OK) {
        TRC_FUNCTION_LEAVE("");
        return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
      }

      IIqrfChannelService::Accessor::UploadErrorCode errorCode;
      switch (response)
      {
        case PMResponse::ERR2:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_TARGET_MEMORY;
          break;
        case PMResponse::ERR3:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_DATA_LEN;
          break;
        case PMResponse::ERR4:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_ADDRESS;
          break;
        case PMResponse::ERR5:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_WRITE_ONLY;
          break;
        case PMResponse::ERR6:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_COMMUNICATION;
          break;
        case PMResponse::ERR7:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
          break;
        case PMResponse::BUSY:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_BUSY;
          break;
        default:
          errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_ERROR_GENERAL;
      }

      TRC_FUNCTION_LEAVE("");
      return errorCode;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Terminating programming mode.");

      PTEResponse response;
      try {
        response = m_cdc->terminateProgrammingMode();
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
          m_accessControl.messageHandler(std::basic_string<unsigned char>(data, length)); });
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
