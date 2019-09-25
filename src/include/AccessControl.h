
#include <mutex>
#include <memory>
#include "Trace.h"

namespace iqrf {

  template<class IqrfChannel> class AccessorImpl;

  template<class IqrfChannel>
  class AccessControl
  {
  public:
    AccessControl(IqrfChannel * iqrfChannel)
      :m_iqrfChannel(iqrfChannel)
    {
    }

    void sendTo(const std::basic_string<unsigned char>& message, IIqrfChannelService::AccesType access)
    {
      std::unique_lock<std::mutex> lck(m_mtx);
      switch (access)
      {
      case IIqrfChannelService::AccesType::Normal:

        if (!m_exclusiveReceiveFromFunc) {
          m_iqrfChannel->send(message);
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot send: Exclusive access is active");
        }
        break;
      case IIqrfChannelService::AccesType::Exclusive:
        m_iqrfChannel->send(message);
        break;
      case IIqrfChannelService::AccesType::Sniffer:
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot send via sniffer access");
        break;
      default:;
      }
    }

    void sniff(const std::basic_string<unsigned char>& message) {
      if (m_snifferReceiveFromFunc) {
        m_snifferReceiveFromFunc(message);
      }
    }

    bool hasExclusiveAccess() const
    {
      std::unique_lock<std::mutex> lck(m_mtx);
      return (bool)m_exclusiveReceiveFromFunc;
    }

    std::unique_ptr<IIqrfChannelService::Accessor> getAccess(IIqrfChannelService::ReceiveFromFunc receiveFromFunc, IIqrfChannelService::AccesType access)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_mtx);
      std::unique_ptr<IIqrfChannelService::Accessor> retval;
      switch (access)
      {
      case IIqrfChannelService::AccesType::Normal:
        retval.reset(shape_new AccessorImpl<IqrfChannel>(this, access));
        m_normalReceiveFromFunc = receiveFromFunc;
        break;
      case IIqrfChannelService::AccesType::Exclusive:
        if (!m_exclusiveReceiveFromFunc) {
          retval.reset(shape_new AccessorImpl<IqrfChannel>(this, access));
          m_exclusiveReceiveFromFunc = receiveFromFunc;
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Exclusive access already assigned");
        }
        break;
      case IIqrfChannelService::AccesType::Sniffer:
        retval.reset(shape_new AccessorImpl<IqrfChannel>(this, access));
        m_snifferReceiveFromFunc = receiveFromFunc;
        break;
      default:;
      }
      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    void resetAccess(IIqrfChannelService::AccesType access)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_mtx);
      switch (access)
      {
      case IIqrfChannelService::AccesType::Normal:
        m_normalReceiveFromFunc = IIqrfChannelService::ReceiveFromFunc();
        break;
      case IIqrfChannelService::AccesType::Exclusive:
        m_exclusiveReceiveFromFunc = IIqrfChannelService::ReceiveFromFunc();
        break;
      case IIqrfChannelService::AccesType::Sniffer:
        m_snifferReceiveFromFunc = IIqrfChannelService::ReceiveFromFunc();
        break;
      default:;
      }
      TRC_FUNCTION_LEAVE("")
    }

    void messageHandler(const std::basic_string<unsigned char>& message)
    {
      std::unique_lock<std::mutex> lck(m_mtx);
      if (!m_exclusiveReceiveFromFunc && m_normalReceiveFromFunc) {
        m_normalReceiveFromFunc(message);
      }
      else if (m_exclusiveReceiveFromFunc) {
        m_exclusiveReceiveFromFunc(message);
      }
      else {
        TRC_WARNING("Cannot receive: no access is active");
      }

      if (m_snifferReceiveFromFunc) {
        m_snifferReceiveFromFunc(message);
      }
    }

    bool enterProgrammingState()
    {
      return m_iqrfChannel->enterProgrammingState();
    }

    // 'address' parameter is NOT used, if upload target has already defined its own address, 
    // which to upload code into, e.g. RF band or RFPGM
    IIqrfChannelService::UploadErrorCode upload(
      const IIqrfChannelService::UploadTarget target,
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    )
    {
      return m_iqrfChannel->upload(target, data, address);
    }

    bool terminateProgrammingState()
    {
      return m_iqrfChannel->terminateProgrammingState();
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      return m_iqrfChannel->getTrModuleInfo();
    }

  private:
    IIqrfChannelService::ReceiveFromFunc m_normalReceiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_exclusiveReceiveFromFunc;
    IIqrfChannelService::ReceiveFromFunc m_snifferReceiveFromFunc;
    IqrfChannel * m_iqrfChannel = nullptr;
    mutable std::mutex m_mtx;
  };

  ///////////////////////////
  template<class IqrfChannel>
  class AccessorImpl : public IIqrfChannelService::Accessor
  {
  public:
    AccessorImpl() = delete;

    AccessorImpl(AccessControl<IqrfChannel> * accessControl, IIqrfChannelService::AccesType accesType)
      :m_accessControl(accessControl)
      , m_type(accesType)
    {
    }

    virtual ~AccessorImpl()
    {
      m_accessControl->resetAccess(m_type);
    }

    void send(const std::basic_string<unsigned char>& message) override
    {
      m_accessControl->sendTo(message, m_type);
    }

    IIqrfChannelService::AccesType getAccessType() override
    {
      return m_type;
    }

    bool enterProgrammingState() override
    {
      return m_accessControl->enterProgrammingState();
    }

    IIqrfChannelService::UploadErrorCode upload(
      const IIqrfChannelService::UploadTarget target, const std::basic_string<uint8_t>& data, const uint16_t address
    ) override
    {
      return m_accessControl->upload(target, data, address);
    }

    bool terminateProgrammingState() override
    {
      return m_accessControl->terminateProgrammingState();
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      return m_accessControl->getTrModuleInfo();
    }

  private:
    AccessControl<IqrfChannel> * m_accessControl = nullptr;
    IIqrfChannelService::AccesType m_type = IIqrfChannelService::AccesType::Normal;
    IIqrfChannelService::ReceiveFromFunc m_receiveFromFunc;
  };
}
