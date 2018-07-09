#pragma once

#include "IChannel.h"
#include "IIqrfChannelService.h"
#include "Trace.h"

//This is workaround class just to keep clibdpa library backward compatibility to satisfy IChannel deps
//As soon the library is redesigned to Shape component it seems reasonable to pass directly IqrfChannelService

namespace iqrf {
  class IqrfDpaChannel : public IChannel
  {
  public:
    IqrfDpaChannel() = delete;
    IqrfDpaChannel(IIqrfChannelService* iqrfChannelService)
    :m_iqrfChannelService(iqrfChannelService) {}
    virtual ~IqrfDpaChannel() {}

    void sendTo(const std::basic_string<unsigned char>& message) override
    {
      if (!m_accessorExclusive) {
        m_accessor->send(message);
      }
      else {
        m_accessorExclusive->send(message);
      }
    }

    void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override
    {
      m_receiveFromFunc = receiveFromFunc;
      m_accessor = m_iqrfChannelService->getAccess(m_receiveFromFunc, IIqrfChannelService::AccesType::Normal);
    }

    void unregisterReceiveFromHandler() override
    {
      m_accessor.reset();
      m_accessorExclusive.reset();
      m_receiveFromFunc = ReceiveFromFunc();
    }

    State getState() override
    {
      switch (m_iqrfChannelService->getState()) {
      case IIqrfChannelService::State::NotReady:
        return State::NotReady;
      case IIqrfChannelService::State::Ready:
        return State::Ready;
      default:
        return State::NotReady;
      }
      return State::NotReady;
    }

    void getExclusiveAccess()
    {
      TRC_FUNCTION_ENTER("");
      m_accessorExclusive = m_iqrfChannelService->getAccess(m_receiveFromFunc, IIqrfChannelService::AccesType::Exclusive);
      TRC_FUNCTION_LEAVE("");
    }

    void ungetExclusiveAccess()
    {
      TRC_FUNCTION_ENTER("");
      m_accessorExclusive.reset();
      TRC_FUNCTION_LEAVE("");
    }

  private:
    IIqrfChannelService* m_iqrfChannelService = nullptr;
    ReceiveFromFunc m_receiveFromFunc;
    std::unique_ptr<IIqrfChannelService::Accessor> m_accessor;
    std::unique_ptr<IIqrfChannelService::Accessor> m_accessorExclusive;
  };
}
