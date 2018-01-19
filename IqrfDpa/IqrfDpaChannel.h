#pragma once

#include "IChannel.h"
#include "IIqrfChannelService.h"

//This is workaround class just to keep clibdpa library backward compatibility to satisfy IChannel deps
//As soon the library is redesigned to Shape component it seems reasonable to pass directly IqrfChannelService

namespace iqrfgw {
  class IqrfDpaChannel : public IChannel
  {
  public:
    IqrfDpaChannel() = delete;
    IqrfDpaChannel(IIqrfChannelService* iqrfChannelService)
    :m_iqrfChannelService(iqrfChannelService) {}
    virtual ~IqrfDpaChannel() {}

    void sendTo(const std::basic_string<unsigned char>& message) override
    {
      m_iqrfChannelService->sendTo(message);
    }

    void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override
    {
      m_iqrfChannelService->registerReceiveFromHandler(receiveFromFunc);
    }

    void unregisterReceiveFromHandler() override
    {
      m_iqrfChannelService->unregisterReceiveFromHandler();
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

  private:
    IIqrfChannelService* m_iqrfChannelService = nullptr;

  };
}
